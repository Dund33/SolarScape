#include "AlignedSimilarityCrossover.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>
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

    struct ExchangeRegion
    {
        std::size_t longerBegin{};
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

    std::vector<ManeuverTime> absoluteManeuverTimes(
        const Specimen& specimen)
    {
        std::vector<ManeuverTime> times;
        times.reserve(specimen.size());

        Real previousEndTime = 0.0L;

        for (const Maneuver& maneuver : specimen.getManeuvers())
        {
            const Real initTime =
                previousEndTime + maneuver.getInitDelay();
            const Real endTime =
                initTime + maneuver.getDuration();

            times.push_back({
                initTime,
                endTime});

            previousEndTime = endTime;
        }

        return times;
    }

    OrientedGenomes orientGenomes(
        const Specimen& parent1,
        const Specimen& parent2,
        const std::vector<ManeuverTime>& parent1Times,
        const std::vector<ManeuverTime>& parent2Times)
    {
        if (parent1.size() <= parent2.size())
        {
            return {
                parent1,
                parent2,
                parent1Times,
                parent2Times,
                true};
        }

        return {
            parent2,
            parent1,
            parent2Times,
            parent1Times,
            false};
    }

    Real dot(
        const Vector3& left,
        const Vector3& right)
    {
        return
            left.x * right.x +
            left.y * right.y +
            left.z * right.z;
    }

    Real directionSimilarity(
        const Maneuver& lhs,
        const Maneuver& rhs)
    {
        const Vector3& lhsDirection =
            lhs.getThrustDirection();
        const Vector3& rhsDirection =
            rhs.getThrustDirection();

        const Real lhsLength =
            lhsDirection.length();
        const Real rhsLength =
            rhsDirection.length();

        if (lhsLength <= 0.0L && rhsLength <= 0.0L)
        {
            return 1.0L;
        }

        if (lhsLength <= 0.0L || rhsLength <= 0.0L)
        {
            return 0.0L;
        }

        const Real cosine =
            std::clamp(
                dot(lhsDirection, rhsDirection) /
                    (lhsLength * rhsLength),
                -1.0L,
                1.0L);

        return (cosine + 1.0L) * 0.5L;
    }

    Real logSimilarity(
        Real similarity)
    {
        if (similarity <= 0.0L)
        {
            return -std::numeric_limits<Real>::infinity();
        }

        return std::log(similarity);
    }

    Real logTimeSimilarity(
        Real lhs,
        Real rhs,
        Real scale)
    {
        return -std::log1p(
            std::abs(lhs - rhs) / scale);
    }

    Real maneuverLogSimilarity(
        const Maneuver& lhs,
        const Maneuver& rhs,
        const ManeuverTime& lhsTime,
        const ManeuverTime& rhsTime,
        Real timeScaleMultiplier)
    {
        const Real throttleSimilarity =
            1.0L -
            std::abs(
                std::clamp(
                    lhs.getThrottleValue(),
                    0.0L,
                    1.0L) -
                std::clamp(
                    rhs.getThrottleValue(),
                    0.0L,
                    1.0L));
        const Real direction =
            directionSimilarity(
                lhs,
                rhs);

        const Real timeScale =
            std::max({
                1.0L,
                lhs.getDuration(),
                rhs.getDuration()}) *
            timeScaleMultiplier;

        return
            logSimilarity(throttleSimilarity) +
            logSimilarity(direction) +
            logTimeSimilarity(
                lhsTime.initTime,
                rhsTime.initTime,
                timeScale) +
            logTimeSimilarity(
                lhsTime.endTime,
                rhsTime.endTime,
                timeScale);
    }

    auto alignedManeuvers(
        const OrientedGenomes& genomes,
        std::size_t longerBegin)
    {
        return std::views::zip(
            genomes.shorter.getManeuvers(),
            genomes.longer.getManeuvers() |
                std::views::drop(longerBegin) |
                std::views::take(genomes.shorter.size()),
            genomes.shorterTimes,
            genomes.longerTimes |
                std::views::drop(longerBegin) |
                std::views::take(genomes.shorter.size()));
    }

    Real alignmentLogSimilaritySum(
        const OrientedGenomes& genomes,
        std::size_t longerBegin,
        Real timeScaleMultiplier)
    {
        Real logSimilaritySum = 0.0L;

        for (auto&& [
                 shorterManeuver,
                 longerManeuver,
                 shorterTime,
                 longerTime] :
             alignedManeuvers(
                 genomes,
                 longerBegin))
        {
            logSimilaritySum +=
                maneuverLogSimilarity(
                    shorterManeuver,
                    longerManeuver,
                    shorterTime,
                    longerTime,
                    timeScaleMultiplier);
        }

        return logSimilaritySum;
    }

    std::size_t bestAlignmentBegin(
        const OrientedGenomes& genomes,
        Real timeScaleMultiplier)
    {
        std::size_t bestBegin = 0;
        Real bestScore =
            -std::numeric_limits<Real>::infinity();
        const std::size_t maxLongerBegin =
            genomes.longer.size() - genomes.shorter.size();

        for (std::size_t longerBegin = 0;
             longerBegin <= maxLongerBegin;
             ++longerBegin)
        {
            const Real score =
                alignmentLogSimilaritySum(
                    genomes,
                    longerBegin,
                    timeScaleMultiplier);

            if (score > bestScore)
            {
                bestBegin = longerBegin;
                bestScore = score;
            }
        }

        return bestBegin;
    }

    ExchangeRegion exchangeRegionForAlignment(
        const OrientedGenomes& genomes,
        std::size_t longerBegin,
        Real minRegionLogSimilarity,
        Real timeScaleMultiplier)
    {
        ExchangeRegion region{
            longerBegin,
            0};
        Real cumulativeLogSimilarity = 0.0L;

        for (auto&& [
                 shorterManeuver,
                 longerManeuver,
                 shorterTime,
                 longerTime] :
             alignedManeuvers(
                 genomes,
                 longerBegin))
        {
            const Real nextLogSimilarity =
                cumulativeLogSimilarity +
                maneuverLogSimilarity(
                    shorterManeuver,
                    longerManeuver,
                    shorterTime,
                    longerTime,
                    timeScaleMultiplier);

            if (nextLogSimilarity < minRegionLogSimilarity)
            {
                break;
            }

            cumulativeLogSimilarity = nextLogSimilarity;
            ++region.length;
        }

        return region;
    }

    template <std::ranges::input_range ManeuverRange>
    void appendManeuvers(
        std::vector<Maneuver>& target,
        ManeuverRange&& maneuvers)
    {
        for (const Maneuver& maneuver : maneuvers)
        {
            target.push_back(maneuver);
        }
    }

    std::pair<Specimen, Specimen> exchangeSuffixesAfterRegion(
        const OrientedGenomes& genomes,
        const ExchangeRegion& region)
    {
        const std::size_t shorterCut = region.length;
        const std::size_t longerCut =
            region.longerBegin + region.length;

        std::vector<Maneuver> shorterChildManeuvers;
        shorterChildManeuvers.reserve(
            shorterCut + genomes.longer.size() - longerCut);
        std::vector<Maneuver> longerChildManeuvers;
        longerChildManeuvers.reserve(
            longerCut + genomes.shorter.size() - shorterCut);

        appendManeuvers(
            shorterChildManeuvers,
            genomes.shorter.getManeuvers() |
                std::views::take(shorterCut));
        appendManeuvers(
            shorterChildManeuvers,
            genomes.longer.getManeuvers() |
                std::views::drop(longerCut));
        appendManeuvers(
            longerChildManeuvers,
            genomes.longer.getManeuvers() |
                std::views::take(longerCut));
        appendManeuvers(
            longerChildManeuvers,
            genomes.shorter.getManeuvers() |
                std::views::drop(shorterCut));

        if (genomes.parent1IsShorter)
        {
            return {
                Specimen(std::move(shorterChildManeuvers)),
                Specimen(std::move(longerChildManeuvers))};
        }

        return {
            Specimen(std::move(longerChildManeuvers)),
            Specimen(std::move(shorterChildManeuvers))};
    }
}

AlignedSimilarityCrossover::AlignedSimilarityCrossover(
    Real minRegionSimilarity,
    Real timeScaleMultiplier)
    : minRegionLogSimilarity(0.0L),
      timeScaleMultiplier(timeScaleMultiplier)
{
    if (minRegionSimilarity <= 0.0L || minRegionSimilarity > 1.0L)
    {
        throw std::invalid_argument(
            "minRegionSimilarity must be in range (0, 1].");
    }

    if (timeScaleMultiplier <= 0.0L)
    {
        throw std::invalid_argument(
            "timeScaleMultiplier must be greater than zero.");
    }

    minRegionLogSimilarity =
        std::log(minRegionSimilarity);
}

std::pair<Specimen, Specimen> AlignedSimilarityCrossover::cross(
    const Specimen& parent1,
    const Specimen& parent2
) const
{
    if (parent1.empty() || parent2.empty())
    {
        return {
            Specimen(parent1.getManeuvers()),
            Specimen(parent2.getManeuvers())};
    }

    const std::vector<ManeuverTime> parent1Times =
        absoluteManeuverTimes(parent1);
    const std::vector<ManeuverTime> parent2Times =
        absoluteManeuverTimes(parent2);
    const OrientedGenomes genomes =
        orientGenomes(
            parent1,
            parent2,
            parent1Times,
            parent2Times);
    const std::size_t longerBegin =
        bestAlignmentBegin(
            genomes,
            timeScaleMultiplier);
    const ExchangeRegion region =
        exchangeRegionForAlignment(
            genomes,
            longerBegin,
            minRegionLogSimilarity,
            timeScaleMultiplier);

    if (region.length == 0)
    {
        return RandomCutCrossover().cross(
            parent1,
            parent2);
    }

    return exchangeSuffixesAfterRegion(
        genomes,
        region);
}
