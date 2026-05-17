#include "Specimen.h"

#include <numeric>
#include <stdexcept>

#include "math/Probe.h"

namespace
{
    constexpr std::size_t kMinimumDistanceFuelMassIndex = 2;

    enum class DominanceRelation
    {
        lhsDominates,
        rhsDominates,
        equivalent,
        unordered
    };

    float comparableFitnessValue(
        const FitnessResult& fitness,
        std::size_t index)
    {
        const float value = fitness.get(index);

        if (index == kMinimumDistanceFuelMassIndex)
        {
            return -value;
        }

        return value;
    }

    DominanceRelation compareByDominance(
        const FitnessResult& lhs,
        const FitnessResult& rhs)
    {
        bool lhsStrictlyBetter = false;
        bool rhsStrictlyBetter = false;

        for (std::size_t i = 0; i < FitnessResult::kSize; ++i)
        {
            const float lhsValue = comparableFitnessValue(lhs, i);
            const float rhsValue = comparableFitnessValue(rhs, i);

            if (lhsValue < rhsValue)
            {
                lhsStrictlyBetter = true;
            }

            if (rhsValue < lhsValue)
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
        const FitnessResult& lhs,
        const FitnessResult& rhs)
    {
        for (std::size_t i = 0; i < FitnessResult::kSize; ++i)
        {
            const float lhsValue = comparableFitnessValue(lhs, i);
            const float rhsValue = comparableFitnessValue(rhs, i);

            if (lhsValue < rhsValue)
            {
                return true;
            }

            if (rhsValue < lhsValue)
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

Specimen::Specimen(Probe* probe)
    : probe(probe)
{
}

Specimen::Specimen(const std::vector<Maneuver>& maneuvers, Probe* probe)
    : maneuvers(maneuvers), probe(probe)
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

Probe* Specimen::getProbe() const
{
    return probe;
}

void Specimen::setProbe(Probe* probe)
{
    this->probe = probe;
}

std::size_t Specimen::size() const
{
    return maneuvers.size();
}

bool Specimen::empty() const
{
    return maneuvers.empty();
}

long double Specimen::getTotalFuelUse() const
{
    if (probe == nullptr)
    {
        throw std::invalid_argument("probe must not be null");
    }

    return std::accumulate(
        maneuvers.begin(),
        maneuvers.end(),
        0.0L,
        [this](long double totalFuelUse, const Maneuver& maneuver)
        {
            return totalFuelUse +
            probe->fuelFlow() *
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

const std::optional<FitnessResult>& Specimen::getFitness() const
{
    return fitness;
}

void Specimen::setFitness(const FitnessResult& fitness)
{
    this->fitness = fitness;
}

void Specimen::clearFitness()
{
    fitness.reset();
}

std::partial_ordering operator<=>(const Specimen& lhs, const Specimen& rhs)
{
    const FitnessResult& lhsFitness = lhs.getFitness().value();
    const FitnessResult& rhsFitness = rhs.getFitness().value();

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
    const FitnessResult& lhsFitness = lhs.getFitness().value();
    const FitnessResult& rhsFitness = rhs.getFitness().value();

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
