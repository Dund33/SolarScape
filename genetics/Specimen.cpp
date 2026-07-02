#include "Specimen.h"

#include <numeric>
#include <utility>

#include "math/ProbeProperties.h"

Specimen::Specimen()
{
}

Specimen::Specimen(const std::vector<Maneuver>& maneuvers)
    : maneuvers(maneuvers)
{
}

Specimen::Specimen(std::vector<Maneuver>&& maneuvers)
    : maneuvers(std::move(maneuvers))
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
