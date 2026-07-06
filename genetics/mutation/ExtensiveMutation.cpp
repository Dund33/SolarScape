#include "ExtensiveMutation.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/mutation/ManeuverMutationUtils.h"
#include "genetics/utils/Refinement.h"

namespace
{
    Maneuver withInitDelay(
        const Maneuver& maneuver,
        Real initDelay)
    {
        return Maneuver(
            maneuver.getThrustDirection(),
            maneuver.getThrottleValue(),
            initDelay,
            maneuver.getDuration());
    }

    Maneuver randomManeuver(
        Real minInitDelay,
        Real maxInitDelay,
        Real minDuration,
        Real maxDuration)
    {
        static thread_local std::mt19937 rng(std::random_device{}());

        std::uniform_real_distribution<Real> initDelayDist(
            minInitDelay,
            maxInitDelay);
        std::uniform_real_distribution<Real> durationDist(
            minDuration,
            maxDuration);
        std::uniform_real_distribution<Real> directionDist(
            -1.0,
            1.0);
        std::uniform_real_distribution<Real> throttleDist(
            MIN_MANEUVER_THROTTLE,
            1.0);

        Vector3 direction;

        do
        {
            direction = Vector3(
                directionDist(rng),
                directionDist(rng),
                directionDist(rng));
        }
        while (direction.norm() <= 0.0);

        direction = direction / direction.norm();

        return Maneuver(
            direction,
            throttleDist(rng),
            initDelayDist(rng),
            durationDist(rng));
    }

    void addRandomManeuver(
        std::vector<Maneuver>& maneuvers,
        Real minInitDelay,
        Real maxInitDelay,
        Real minDuration,
        Real maxDuration)
    {
        static thread_local std::mt19937 rng(std::random_device{}());

        const Maneuver maneuver =
            randomManeuver(
                minInitDelay,
                maxInitDelay,
                minDuration,
                maxDuration);
        std::uniform_int_distribution<std::size_t> insertIndexDist(
            0,
            maneuvers.size());
        const std::size_t insertIndex = insertIndexDist(rng);

        maneuvers.insert(
            maneuvers.begin() + static_cast<std::ptrdiff_t>(insertIndex),
            maneuver);

        const std::size_t nextIndex = insertIndex + 1;

        if (nextIndex >= maneuvers.size())
        {
            return;
        }

        const Real nextInitDelay = maneuvers[nextIndex].getInitDelay();
        const Real insertedManeuverEnd =
            maneuver.getInitDelay() + maneuver.getDuration();
        const Real adjustedInitDelay =
            std::max(0.0, nextInitDelay - insertedManeuverEnd);

        maneuvers[nextIndex] =
            withInitDelay(
                maneuvers[nextIndex],
                adjustedInitDelay);
    }

    void addRandomManeuversUntil(
        std::vector<Maneuver>& maneuvers,
        std::size_t targetSize,
        std::size_t maxManeuvers,
        Real minInitDelay,
        Real maxInitDelay,
        Real minDuration,
        Real maxDuration)
    {
        while (maneuvers.size() < targetSize &&
               maneuvers.size() < maxManeuvers)
        {
            addRandomManeuver(
                maneuvers,
                minInitDelay,
                maxInitDelay,
                minDuration,
                maxDuration);
        }
    }

    void removeRandomManeuver(
        std::vector<Maneuver>& maneuvers)
    {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<std::size_t> removeIndexDist(
            0,
            maneuvers.size() - 1);
        const std::size_t removeIndex = removeIndexDist(rng);
        const Maneuver removedManeuver = maneuvers[removeIndex];

        maneuvers.erase(
            maneuvers.begin() + static_cast<std::ptrdiff_t>(removeIndex));

        if (removeIndex >= maneuvers.size())
        {
            return;
        }

        maneuvers[removeIndex] =
            withInitDelay(
                maneuvers[removeIndex],
                maneuvers[removeIndex].getInitDelay() +
                    removedManeuver.getInitDelay() +
                    removedManeuver.getDuration());
    }

    void mutateManeuversUniformly(
        std::vector<Maneuver>& maneuvers,
        double mutationProbability,
        Real maxTimeOffset,
        Real maxDurationOffset,
        Real maxDirectionOffset,
        Real maxThrottleOffset)
    {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::bernoulli_distribution shouldMutate(mutationProbability);
        std::uniform_real_distribution<Real> timeDelta(
            -maxTimeOffset,
             maxTimeOffset);
        std::uniform_real_distribution<Real> durationDelta(
            -maxDurationOffset,
             maxDurationOffset);
        std::uniform_real_distribution<Real> directionDelta(
            -maxDirectionOffset,
             maxDirectionOffset);
        std::uniform_real_distribution<Real> throttleDelta(
            -maxThrottleOffset,
             maxThrottleOffset);

        for (Maneuver& maneuver : maneuvers)
        {
            maneuver =
                ManeuverMutationUtils::mutateUniformly(
                    maneuver,
                    shouldMutate,
                    timeDelta,
                    durationDelta,
                    directionDelta,
                    throttleDelta,
                    rng);
        }
    }
}

ExtensiveMutation::ExtensiveMutation(
    double mutationProbabilityValue,
    double addProbabilityValue,
    double removeProbabilityValue,
    std::size_t minManeuverCount,
    std::size_t maxManeuverCount,
    Real minInitDelayValue,
    Real maxInitDelayValue,
    Real minDurationValue,
    Real maxDurationValue,
    Real maxTimeOffsetValue,
    Real maxDurationOffsetValue,
    Real maxDirectionOffsetValue,
    Real maxThrottleOffsetValue)
    : mutationProbability(mutationProbabilityValue),
      addProbability(addProbabilityValue),
      removeProbability(removeProbabilityValue),
      minManeuvers(minManeuverCount),
      maxManeuvers(maxManeuverCount),
      minInitDelay(minInitDelayValue),
      maxInitDelay(maxInitDelayValue),
      minDuration(minDurationValue),
      maxDuration(maxDurationValue),
      maxTimeOffset(maxTimeOffsetValue),
      maxDurationOffset(maxDurationOffsetValue),
      maxDirectionOffset(maxDirectionOffsetValue),
      maxThrottleOffset(maxThrottleOffsetValue)
{
    if (mutationProbabilityValue < 0.0 || mutationProbabilityValue > 1.0)
    {
        throw std::invalid_argument(
            "Mutation probability must be in range [0, 1].");
    }

    if (addProbabilityValue < 0.0 || addProbabilityValue > 1.0)
    {
        throw std::invalid_argument(
            "Add probability must be in range [0, 1].");
    }

    if (removeProbabilityValue < 0.0 || removeProbabilityValue > 1.0)
    {
        throw std::invalid_argument(
            "Remove probability must be in range [0, 1].");
    }

    if (minManeuverCount > maxManeuverCount)
    {
        throw std::invalid_argument(
            "minManeuvers cannot be greater than maxManeuvers.");
    }

    if (minInitDelayValue > maxInitDelayValue)
    {
        throw std::invalid_argument(
            "minInitDelay cannot be greater than maxInitDelay.");
    }

    if (minDurationValue > maxDurationValue)
    {
        throw std::invalid_argument(
            "minDuration cannot be greater than maxDuration.");
    }
}

void ExtensiveMutation::mutate(
    Specimen& specimen,
    bool closeToTarget) const
{
    static thread_local std::mt19937 rng(std::random_device{}());

    std::vector<Maneuver> maneuvers = specimen.getManeuvers();
    const Real mutationScale =
        Refinement::mutationScale(
            closeToTarget);

    addRandomManeuversUntil(
        maneuvers,
        minManeuvers,
        maxManeuvers,
        minInitDelay,
        maxInitDelay,
        minDuration,
        maxDuration);

    const bool canAdd = maneuvers.size() < maxManeuvers;
    const bool canRemove =
        maneuvers.size() >
        std::max<std::size_t>(minManeuvers, 1);
    std::bernoulli_distribution shouldAdd(addProbability);
    std::bernoulli_distribution shouldRemove(removeProbability);
    bool add = shouldAdd(rng) && canAdd;
    bool remove = shouldRemove(rng) && canRemove;

    if (!add && !remove)
    {
        mutateManeuversUniformly(
            maneuvers,
            mutationProbability,
            maxTimeOffset * mutationScale,
            maxDurationOffset * mutationScale,
            maxDirectionOffset * mutationScale,
            maxThrottleOffset * mutationScale);

        specimen = Specimen(std::move(maneuvers));
        return;
    }

    if (add && remove)
    {
        std::bernoulli_distribution chooseAdd(0.5);
        add = chooseAdd(rng);
        remove = !add;
    }

    if (add)
    {
        addRandomManeuver(
            maneuvers,
            minInitDelay,
            maxInitDelay,
            minDuration,
            maxDuration);
    }
    else if (remove)
    {
        removeRandomManeuver(maneuvers);
    }

    mutateManeuversUniformly(
        maneuvers,
        mutationProbability,
        maxTimeOffset * mutationScale,
        maxDurationOffset * mutationScale,
        maxDirectionOffset * mutationScale,
        maxThrottleOffset * mutationScale);

    specimen = Specimen(std::move(maneuvers));
}
