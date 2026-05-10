//
// Created by Luke on 5/9/2026.
//

#include "Specimen.h"

Specimen::Specimen()
    : fitness(std::numeric_limits<double>::max())
{
}

Specimen::Specimen(const std::vector<Maneuver>& maneuvers)
    : maneuvers(maneuvers),
      fitness(std::numeric_limits<double>::max())
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

const Maneuver& Specimen::operator[](std::size_t index) const
{
    return maneuvers[index];
}

Maneuver& Specimen::operator[](std::size_t index)
{
    return maneuvers[index];
}

double Specimen::getFitness() const
{
    return fitness;
}

void Specimen::setFitness(double fitness)
{
    this->fitness = fitness;
}