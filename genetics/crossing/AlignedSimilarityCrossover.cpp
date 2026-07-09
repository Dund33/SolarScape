#include "AlignedSimilarityCrossover.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/crossing/RandomCutCrossover.h"

namespace
{
    struct ManeuverTime
    {
        Real initTime{};
        Real endTime{};
    };

    struct SimilarPrefix
    {
        std::size_t length{};
    };

    struct ExchangeBlock
    {
        std::size_t begin{};
        std::size_t length{};
    };

    struct OrientedGenomes
    {
        const Specimen& shorter;
        const Specimen& longer;
        const std::vector<ManeuverTime>& shorterTimes;
        const std::vector<ManeuverTime>& longerTimes;
        bool parent1IsShorter{};
    };

    std::vector<ManeuverTime> absoluteManeuverTimes(const Specimen& specimen)
    {
        std::vector<ManeuverTime> times;
        times.reserve(specimen.size());

        Real previousEndTime = 0.0;

        for (const Maneuver& maneuver : specimen.getManeuvers())
        {
            const Real initTime = previousEndTime + maneuver.getInitDelay();
            const Real endTime = initTime + maneuver.getDuration();

            times.push_back({initTime, endTime});

            previousEndTime = endTime;
        }

        return times;
    }

    OrientedGenomes orientGenomes(const Specimen& parent1, const Specimen& parent2, const std::vector<ManeuverTime>& parent1Times,
                                  const std::vector<ManeuverTime>& parent2Times)
    {
        if (parent1.size() <= parent2.size())
        {
            return {parent1, parent2, parent1Times, parent2Times, true};
        }

        return {parent2, parent1, parent2Times, parent1Times, false};
    }

    Real dot(const Vector3& left, const Vector3& right)
    {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    Real directionSimilarity(const Maneuver& lhs, const Maneuver& rhs)
    {
        const Vector3& lhsDirection = lhs.getThrustDirection();
        const Vector3& rhsDirection = rhs.getThrustDirection();

        const Real lhsLength = lhsDirection.length();
        const Real rhsLength = rhsDirection.length();

        if (lhsLength <= 0.0 && rhsLength <= 0.0)
        {
            return 1.0;
        }

        if (lhsLength <= 0.0 || rhsLength <= 0.0)
        {
            return 0.0;
        }

        const Real cosine = std::clamp(dot(lhsDirection, rhsDirection) / (lhsLength * rhsLength), -1.0, 1.0);

        return (cosine + 1.0) * 0.5;
    }

    Real logSimilarity(Real similarity)
    {
        if (similarity <= 0.0)
        {
            return -std::numeric_limits<Real>::infinity();
        }

        return std::log(similarity);
    }

    Real logTimeSimilarity(Real lhs, Real rhs, Real scale)
    {
        return -std::log1p(std::abs(lhs - rhs) / scale);
    }

    Real maneuverLogSimilarity(const Maneuver& lhs, const Maneuver& rhs, const ManeuverTime& lhsTime, const ManeuverTime& rhsTime,
                               Real timeScaleMultiplier)
    {
        const Real throttleSimilarity =
            1.0 - std::abs(std::clamp(lhs.getThrottleValue(), 0.0, 1.0) - std::clamp(rhs.getThrottleValue(), 0.0, 1.0));
        const Real direction = directionSimilarity(lhs, rhs);

        const Real timeScale = std::max({1.0, lhs.getDuration(), rhs.getDuration()}) * timeScaleMultiplier;

        return logSimilarity(throttleSimilarity) + logSimilarity(direction) +
               logTimeSimilarity(lhsTime.initTime, rhsTime.initTime, timeScale) +
               logTimeSimilarity(lhsTime.endTime, rhsTime.endTime, timeScale);
    }

    auto alignedManeuvers(const OrientedGenomes& genomes)
    {
        return std::views::zip(genomes.shorter.getManeuvers(),
                               genomes.longer.getManeuvers() | std::views::take(genomes.shorter.size()), genomes.shorterTimes,
                               genomes.longerTimes | std::views::take(genomes.shorter.size()));
    }

    SimilarPrefix similarPrefix(const OrientedGenomes& genomes, Real minRegionLogSimilarity, Real timeScaleMultiplier)
    {
        SimilarPrefix prefix{};
        Real cumulativeLogSimilarity = 0.0;
        const std::size_t maxPrefixLength = genomes.shorter.size() - 1;

        for (auto&& [shorterManeuver, longerManeuver, shorterTime, longerTime] : alignedManeuvers(genomes))
        {
            if (prefix.length >= maxPrefixLength)
            {
                break;
            }

            const Real nextLogSimilarity = cumulativeLogSimilarity + maneuverLogSimilarity(shorterManeuver, longerManeuver, shorterTime,
                                                                                           longerTime, timeScaleMultiplier);

            if (nextLogSimilarity < minRegionLogSimilarity)
            {
                break;
            }

            cumulativeLogSimilarity = nextLogSimilarity;
            ++prefix.length;
        }

        return prefix;
    }

    ExchangeBlock randomExchangeBlock(const SimilarPrefix& prefix, const OrientedGenomes& genomes, std::mt19937& rng)
    {
        std::uniform_int_distribution<std::size_t> beginDist(0, prefix.length - 1);
        const std::size_t begin = beginDist(rng);
        const std::size_t maxLength = genomes.shorter.size() - begin;
        std::uniform_int_distribution<std::size_t> lengthDist(1, maxLength);

        return {begin, lengthDist(rng)};
    }

    template <std::ranges::input_range ManeuverRange> void appendManeuvers(std::vector<Maneuver>& target, ManeuverRange&& maneuvers)
    {
        for (const Maneuver& maneuver : maneuvers)
        {
            target.push_back(maneuver);
        }
    }

    std::pair<Specimen, Specimen> exchangeBlocks(const OrientedGenomes& genomes, const ExchangeBlock& block)
    {
        const std::size_t blockEnd = block.begin + block.length;

        std::vector<Maneuver> shorterChildManeuvers;
        shorterChildManeuvers.reserve(genomes.shorter.size());
        std::vector<Maneuver> longerChildManeuvers;
        longerChildManeuvers.reserve(genomes.longer.size());

        appendManeuvers(shorterChildManeuvers, genomes.shorter.getManeuvers() | std::views::take(block.begin));
        appendManeuvers(shorterChildManeuvers,
                        genomes.longer.getManeuvers() | std::views::drop(block.begin) | std::views::take(block.length));
        appendManeuvers(shorterChildManeuvers, genomes.shorter.getManeuvers() | std::views::drop(blockEnd));
        appendManeuvers(longerChildManeuvers, genomes.longer.getManeuvers() | std::views::take(block.begin));
        appendManeuvers(longerChildManeuvers,
                        genomes.shorter.getManeuvers() | std::views::drop(block.begin) | std::views::take(block.length));
        appendManeuvers(longerChildManeuvers, genomes.longer.getManeuvers() | std::views::drop(blockEnd));

        if (genomes.parent1IsShorter)
        {
            return {Specimen(std::move(shorterChildManeuvers)), Specimen(std::move(longerChildManeuvers))};
        }

        return {Specimen(std::move(longerChildManeuvers)), Specimen(std::move(shorterChildManeuvers))};
    }
} // namespace

AlignedSimilarityCrossover::AlignedSimilarityCrossover(Real minRegionSimilarity, Real timeScaleMultiplierValue)
    : minRegionLogSimilarity(0.0), timeScaleMultiplier(timeScaleMultiplierValue)
{
    if (minRegionSimilarity <= 0.0 || minRegionSimilarity > 1.0)
    {
        throw std::invalid_argument("minRegionSimilarity must be in range (0, 1].");
    }

    if (timeScaleMultiplierValue <= 0.0)
    {
        throw std::invalid_argument("timeScaleMultiplier must be greater than zero.");
    }

    minRegionLogSimilarity = std::log(minRegionSimilarity);
}

std::pair<Specimen, Specimen> AlignedSimilarityCrossover::cross(const Specimen& parent1, const Specimen& parent2) const
{
    if (parent1.empty() || parent2.empty())
    {
        return {Specimen(parent1.getManeuvers()), Specimen(parent2.getManeuvers())};
    }

    const std::vector<ManeuverTime> parent1Times = absoluteManeuverTimes(parent1);
    const std::vector<ManeuverTime> parent2Times = absoluteManeuverTimes(parent2);
    const OrientedGenomes genomes = orientGenomes(parent1, parent2, parent1Times, parent2Times);
    const SimilarPrefix prefix = similarPrefix(genomes, minRegionLogSimilarity, timeScaleMultiplier);

    if (prefix.length == 0)
    {
        return RandomCutCrossover().cross(parent1, parent2);
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    const ExchangeBlock block = randomExchangeBlock(prefix, genomes, rng);

    return exchangeBlocks(genomes, block);
}
