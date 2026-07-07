#include "Specimen.h"

#include <utility>

Specimen::Specimen() {}

Specimen::Specimen(const std::vector<Maneuver>& maneuverValues) : maneuvers(maneuverValues) {}

Specimen::Specimen(std::vector<Maneuver>&& maneuverValues) : maneuvers(std::move(maneuverValues)) {}

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

void Specimen::setFitness(const FitnessValue& fitnessValue)
{
    fitness = fitnessValue;
}

void Specimen::clearFitness()
{
    fitness.reset();
}
