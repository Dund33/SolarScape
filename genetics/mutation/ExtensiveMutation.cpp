#include "ExtensiveMutation.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>

#include "genetics/Specimen.h"

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
            -1.0L,
            1.0L);
        std::uniform_real_distribution<Real> throttleDist(
            0.0L,
            1.0L);

        Vector3 direction;

        do
        {
            direction = Vector3(
                directionDist(rng),
                directionDist(rng),
                directionDist(rng));
        }
        while (direction.norm() <= 0.0L);

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

        const Real adjustedInitDelay =
            std::max(
                0.0L,
                maneuvers[nextIndex].getInitDelay() -
                    maneuver.getInitDelay() -
                    maneuver.getDuration());

        maneuvers[nextIndex] =
            withInitDelay(
                maneuvers[nextIndex],
                adjustedInitDelay);
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

    Maneuver mutateManeuverUniformly(
        const Maneuver& maneuver,
        std::bernoulli_distribution& shouldMutate,
        std::uniform_real_distribution<Real>& timeDelta,
        std::uniform_real_distribution<Real>& durationDelta,
        std::uniform_real_distribution<Real>& thrustDelta,
        std::mt19937& rng,
        const ProbeProperties& probeProperties)
    {
        Vector3 throttleVector =
            maneuver.getThrustDirection() *
            maneuver.getThrottleValue();

        Real initDelay = maneuver.getInitDelay();
        Real duration = maneuver.getDuration();

        if (shouldMutate(rng))
        {
            initDelay += timeDelta(rng);
            initDelay = std::max(0.0L, initDelay);
        }

        if (shouldMutate(rng))
        {
            duration += durationDelta(rng);
            duration = std::max(0.0L, duration);
        }

        if (shouldMutate(rng))
        {
            const Real maxPhysicalThrust =
                probeProperties.fuelFlow() *
                probeProperties.specificImpulse();
            const Real thrustScale =
                maxPhysicalThrust > 0.0L
                    ? maxPhysicalThrust
                    : 1.0L;

            throttleVector.x += thrustDelta(rng) / thrustScale;
            throttleVector.y += thrustDelta(rng) / thrustScale;
            throttleVector.z += thrustDelta(rng) / thrustScale;
        }

        const Real throttleNorm =
            std::clamp(
                throttleVector.norm(),
                0.0L,
                1.0L);

        if (throttleNorm <= 0.0L)
        {
            return Maneuver(Vector3{}, 0.0L, initDelay, duration);
        }

        return Maneuver(
            throttleVector / throttleVector.norm(),
            throttleNorm,
            initDelay,
            duration);
    }

    void mutateManeuversUniformly(
        std::vector<Maneuver>& maneuvers,
        double mutationProbability,
        Real maxTimeOffset,
        Real maxDurationOffset,
        Real maxThrustOffset,
        const ProbeProperties& probeProperties)
    {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::bernoulli_distribution shouldMutate(mutationProbability);
        std::uniform_real_distribution<Real> timeDelta(
            -maxTimeOffset,
             maxTimeOffset);
        std::uniform_real_distribution<Real> durationDelta(
            -maxDurationOffset,
             maxDurationOffset);
        std::uniform_real_distribution<Real> thrustDelta(
            -maxThrustOffset,
             maxThrustOffset);

        for (Maneuver& maneuver : maneuvers)
        {
            maneuver =
                mutateManeuverUniformly(
                    maneuver,
                    shouldMutate,
                    timeDelta,
                    durationDelta,
                    thrustDelta,
                    rng,
                    probeProperties);
        }
    }
}

ExtensiveMutation::ExtensiveMutation(
    double mutationProbability,
    double addProbability,
    double removeProbability,
    std::size_t minManeuvers,
    std::size_t maxManeuvers,
    Real minInitDelay,
    Real maxInitDelay,
    Real minDuration,
    Real maxDuration,
    Real maxTimeOffset,
    Real maxDurationOffset,
    Real maxThrustOffset,
    const ProbeProperties& probeProperties)
    : mutationProbability(mutationProbability),
      addProbability(addProbability),
      removeProbability(removeProbability),
      minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitDelay(minInitDelay),
      maxInitDelay(maxInitDelay),
      minDuration(minDuration),
      maxDuration(maxDuration),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxThrustOffset(maxThrustOffset),
      probeProperties(probeProperties)
{
    if (mutationProbability < 0.0 || mutationProbability > 1.0)
    {
        throw std::invalid_argument(
            "Mutation probability must be in range [0, 1].");
    }

    if (addProbability < 0.0 || addProbability > 1.0)
    {
        throw std::invalid_argument(
            "Add probability must be in range [0, 1].");
    }

    if (removeProbability < 0.0 || removeProbability > 1.0)
    {
        throw std::invalid_argument(
            "Remove probability must be in range [0, 1].");
    }

    if (minManeuvers > maxManeuvers)
    {
        throw std::invalid_argument(
            "minManeuvers cannot be greater than maxManeuvers.");
    }

    if (minInitDelay > maxInitDelay)
    {
        throw std::invalid_argument(
            "minInitDelay cannot be greater than maxInitDelay.");
    }

    if (minDuration > maxDuration)
    {
        throw std::invalid_argument(
            "minDuration cannot be greater than maxDuration.");
    }
}

void ExtensiveMutation::mutate(Specimen& specimen) const
{
    static thread_local std::mt19937 rng(std::random_device{}());

    const bool canAdd = specimen.size() < maxManeuvers;
    const bool canRemove = specimen.size() > minManeuvers;
    std::vector<Maneuver> maneuvers = specimen.getManeuvers();
    std::bernoulli_distribution shouldAdd(addProbability);
    std::bernoulli_distribution shouldRemove(removeProbability);
    bool add = shouldAdd(rng) && canAdd;
    bool remove = shouldRemove(rng) && canRemove;

    if (!add && !remove)
    {
        mutateManeuversUniformly(
            maneuvers,
            mutationProbability,
            maxTimeOffset,
            maxDurationOffset,
            maxThrustOffset,
            probeProperties);

        specimen = Specimen(maneuvers);
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
        maxTimeOffset,
        maxDurationOffset,
        maxThrustOffset,
        probeProperties);

    specimen = Specimen(maneuvers);
}
