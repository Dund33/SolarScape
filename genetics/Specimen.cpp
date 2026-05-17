#include "Specimen.h"

#include <array>
#include <numeric>

#include "math/ProbeProperties.h"

namespace
{
    enum class DominanceRelation
    {
        lhsDominates,
        rhsDominates,
        equivalent,
        unordered
    };

    std::array<Real, 3> comparableFitnessValues(
        const FitnessValue& fitness)
    {
        return {
            fitness.minimumDistance,
            fitness.minimumDistanceTime,
            -fitness.minimumDistanceFuelMass};
    }

    DominanceRelation compareByDominance(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        bool lhsStrictlyBetter = false;
        bool rhsStrictlyBetter = false;
        const std::array<Real, 3> lhsValues =
            comparableFitnessValues(lhs);
        const std::array<Real, 3> rhsValues =
            comparableFitnessValues(rhs);

        for (std::size_t i = 0; i < lhsValues.size(); ++i)
        {
            if (lhsValues[i] < rhsValues[i])
            {
                lhsStrictlyBetter = true;
            }

            if (rhsValues[i] < lhsValues[i])
            {
                rhsStrictlyBetter = true;
            }
        }

        if (lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return DominanceRelation::lhsDominates;
        }

        if (rhsStrictlyBetter && !lhsStrictlyBetter)
        {
            return DominanceRelation::rhsDominates;
        }

        if (!lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return DominanceRelation::equivalent;
        }

        return DominanceRelation::unordered;
    }

    bool lexicographicallyBetter(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        const std::array<Real, 3> lhsValues =
            comparableFitnessValues(lhs);
        const std::array<Real, 3> rhsValues =
            comparableFitnessValues(rhs);

        for (std::size_t i = 0; i < lhsValues.size(); ++i)
        {
            if (lhsValues[i] < rhsValues[i])
            {
                return true;
            }

            if (rhsValues[i] < lhsValues[i])
            {
                return false;
            }
        }

        return false;
    }
}

Specimen::Specimen()
{
}

Specimen::Specimen(const std::vector<Maneuver>& maneuvers)
    : maneuvers(maneuvers)
{
}

void Specimen::addManeuver(const Maneuver& maneuver)
{
    maneuvers.push_back(maneuver);
}

const std::vector<Maneuver>& Specimen::getManeuvers() const
{
    return maneuvers;
}

std::size_t Specimen::size() const
{
    return maneuvers.size();
}

bool Specimen::empty() const
{
    return maneuvers.empty();
}

long double Specimen::getTotalFuelUse(
    const ProbeProperties& probeProperties
) const
{
    return std::accumulate(
        maneuvers.begin(),
        maneuvers.end(),
        0.0L,
        [&probeProperties](long double totalFuelUse, const Maneuver& maneuver)
        {
            return totalFuelUse +
            probeProperties.fuelFlow() *
            maneuver.getThrottleValue() *
            maneuver.getDuration();
        });
}

const Maneuver& Specimen::operator[](std::size_t index) const
{
    return maneuvers[index];
}

Maneuver& Specimen::operator[](std::size_t index)
{
    return maneuvers[index];
}

const std::optional<FitnessValue>& Specimen::getFitness() const
{
    return fitness;
}

void Specimen::setFitness(const FitnessValue& fitness)
{
    this->fitness = fitness;
}

void Specimen::clearFitness()
{
    fitness.reset();
}

std::partial_ordering operator<=>(const Specimen& lhs, const Specimen& rhs)
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    switch (compareByDominance(lhsFitness, rhsFitness))
    {
    case DominanceRelation::lhsDominates:
        return std::partial_ordering::less;
    case DominanceRelation::rhsDominates:
        return std::partial_ordering::greater;
    case DominanceRelation::equivalent:
        return std::partial_ordering::equivalent;
    case DominanceRelation::unordered:
        return std::partial_ordering::unordered;
    }

    return std::partial_ordering::unordered;
}

bool operator==(const Specimen& lhs, const Specimen& rhs)
{
    return (lhs <=> rhs) == std::partial_ordering::equivalent;
}

bool operator<(const Specimen& lhs, const Specimen& rhs)
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    switch (compareByDominance(lhsFitness, rhsFitness))
    {
    case DominanceRelation::lhsDominates:
        return true;
    case DominanceRelation::rhsDominates:
        return false;
    case DominanceRelation::equivalent:
    case DominanceRelation::unordered:
        return lexicographicallyBetter(lhsFitness, rhsFitness);
    }

    return false;
}
