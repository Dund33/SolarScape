//
// Created by Luke on 5/9/2026.
//

#include "Specimen.h"

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

long double Specimen::getTotalImpulse() const
{
    long double totalImpulse = 0.0L;

    for (const auto& maneuver : maneuvers)
    {
        totalImpulse +=
            maneuver.getThrust().norm() *
            maneuver.getDuration();
    }

    return totalImpulse;
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