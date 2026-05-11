//
// Created by Luke on 5/9/2026.
//

#include "Specimen.h"

#include <numeric>
#include <stdexcept>

#include "math/Probe.h"

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

std::optional<double> Specimen::getFitness() const
{
    return fitness;
}

void Specimen::setFitness(double fitness)
{
    this->fitness = fitness;
}

void Specimen::clearFitness()
{
    fitness.reset();
}
