#include "AlignedSimilarityCrossover.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
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

    struct OffsetRange
    {
        std::size_t parent2Begin{};
        std::size_t parent2End{};
    };

    struct SimilarityRegion
    {
        std::size_t parent2Begin{};
        std::size_t length{};
        Real logSimilaritySum{};
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

    OffsetRange validRangeForOffset(
        std::ptrdiff_t offset,
        std::size_t parent1Size,
        std::size_t parent2Size)
    {
        const std::ptrdiff_t signedParent2Begin =
            std::max(
                static_cast<std::ptrdiff_t>(0),
                -offset);
        const std::ptrdiff_t signedParent2End =
            std::min(
                static_cast<std::ptrdiff_t>(parent2Size),
                static_cast<std::ptrdiff_t>(parent1Size) - offset);

        if (signedParent2Begin >= signedParent2End)
        {
            return {};
        }

        const std::size_t parent2Begin =
            static_cast<std::size_t>(signedParent2Begin);
        const std::size_t parent2End =
            static_cast<std::size_t>(signedParent2End);

        return {
            parent2Begin,
            parent2End};
    }

    Real alignmentCost(
        std::ptrdiff_t offset,
        const std::vector<ManeuverTime>& parent1Times,
        const std::vector<ManeuverTime>& parent2Times)
    {
        const OffsetRange range =
            validRangeForOffset(
                offset,
                parent1Times.size(),
                parent2Times.size());

        Real cost = 0.0L;

        for (std::size_t parent2Index = range.parent2Begin;
             parent2Index < range.parent2End;
             ++parent2Index)
        {
            const std::size_t parent1Index =
                static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(parent2Index) + offset);

            cost +=
                std::abs(
                    parent1Times[parent1Index].initTime -
                    parent2Times[parent2Index].initTime) +
                std::abs(
                    parent1Times[parent1Index].endTime -
                    parent2Times[parent2Index].endTime);
        }

        return cost;
    }

    std::ptrdiff_t bestOffset(
        const std::vector<ManeuverTime>& parent1Times,
        const std::vector<ManeuverTime>& parent2Times)
    {
        const std::ptrdiff_t minOffset =
            -static_cast<std::ptrdiff_t>(parent2Times.size() - 1);
        const std::ptrdiff_t maxOffset =
            static_cast<std::ptrdiff_t>(parent1Times.size() - 1);

        std::ptrdiff_t bestOffset = 0;
        Real bestCost = std::numeric_limits<Real>::infinity();
        std::size_t bestPairCount = 0;

        for (std::ptrdiff_t offset = minOffset;
             offset <= maxOffset;
             ++offset)
        {
            const OffsetRange range =
                validRangeForOffset(
                    offset,
                    parent1Times.size(),
                    parent2Times.size());

            if (range.parent2Begin >= range.parent2End)
            {
                continue;
            }

            const Real cost =
                alignmentCost(
                    offset,
                    parent1Times,
                    parent2Times);
            const std::size_t pairCount =
                range.parent2End - range.parent2Begin;

            if (
                cost < bestCost ||
                (cost == bestCost && pairCount > bestPairCount))
            {
                bestCost = cost;
                bestOffset = offset;
                bestPairCount = pairCount;
            }
        }

        return bestOffset;
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

    SimilarityRegion similarityRegion(
        std::ptrdiff_t offset,
        const Specimen& parent1,
        const Specimen& parent2,
        const std::vector<ManeuverTime>& parent1Times,
        const std::vector<ManeuverTime>& parent2Times,
        Real minPairLogSimilarity,
        Real timeScaleMultiplier,
        Real lengthReward)
    {
        const OffsetRange range =
            validRangeForOffset(
                offset,
                parent1.size(),
                parent2.size());

        SimilarityRegion region{
            range.parent2Begin,
            0,
            0.0L};
        Real logSimilaritySum = 0.0L;
        Real bestScore =
            -std::numeric_limits<Real>::infinity();
        std::size_t currentLength = 0;

        for (std::size_t parent2Index = range.parent2Begin;
             parent2Index < range.parent2End;
             ++parent2Index)
        {
            const std::size_t parent1Index =
                static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(parent2Index) + offset);

            const Real logSimilarity =
                maneuverLogSimilarity(
                    parent1[parent1Index],
                    parent2[parent2Index],
                    parent1Times[parent1Index],
                    parent2Times[parent2Index],
                    timeScaleMultiplier);

            if (logSimilarity < minPairLogSimilarity)
            {
                break;
            }

            logSimilaritySum += logSimilarity;
            ++currentLength;

            const Real score =
                logSimilaritySum +
                lengthReward * static_cast<Real>(currentLength);

            if (score > bestScore)
            {
                bestScore = score;
                region.length = currentLength;
                region.logSimilaritySum = logSimilaritySum;
            }
        }

        return region;
    }

    std::size_t randomizedSwapLength(
        std::size_t maxLength)
    {
        if (maxLength <= 1)
        {
            return maxLength;
        }

        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<std::size_t> dist(1, maxLength);

        return std::max(
            dist(rng),
            dist(rng));
    }
}

AlignedSimilarityCrossover::AlignedSimilarityCrossover(
    Real minPairSimilarity,
    Real timeScaleMultiplier,
    Real lengthReward)
    : minPairLogSimilarity(0.0L),
      timeScaleMultiplier(timeScaleMultiplier),
      lengthReward(lengthReward)
{
    if (minPairSimilarity <= 0.0L || minPairSimilarity > 1.0L)
    {
        throw std::invalid_argument(
            "minPairSimilarity must be in range (0, 1].");
    }

    if (timeScaleMultiplier <= 0.0L)
    {
        throw std::invalid_argument(
            "timeScaleMultiplier must be greater than zero.");
    }

    if (lengthReward < 0.0L)
    {
        throw std::invalid_argument(
            "lengthReward must be non-negative.");
    }

    minPairLogSimilarity =
        std::log(minPairSimilarity);
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
    const std::ptrdiff_t offset =
        bestOffset(
            parent1Times,
            parent2Times);
    const SimilarityRegion region =
        similarityRegion(
            offset,
            parent1,
            parent2,
            parent1Times,
            parent2Times,
            minPairLogSimilarity,
            timeScaleMultiplier,
            lengthReward);

    if (region.length == 0)
    {
        return RandomCutCrossover().cross(
            parent1,
            parent2);
    }

    Specimen child1(parent1.getManeuvers());
    Specimen child2(parent2.getManeuvers());
    const std::size_t swapLength =
        randomizedSwapLength(region.length);

    for (std::size_t i = 0; i < swapLength; ++i)
    {
        const std::size_t parent2Index =
            region.parent2Begin + i;
        const std::size_t parent1Index =
            static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(parent2Index) + offset);

        std::swap(
            child1[parent1Index],
            child2[parent2Index]);
    }

    return {
        std::move(child1),
        std::move(child2)};
}
