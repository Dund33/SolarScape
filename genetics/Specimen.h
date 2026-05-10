//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_SPECIMEN_H
#define SOLARSCAPE_SPECIMEN_H

#include <vector>
#include <limits>
#include <optional>

#include "Maneuver.h"

class Specimen
{
public:
    Specimen();
    explicit Specimen(const std::vector<Maneuver>& maneuvers);

    void addManeuver(const Maneuver& maneuver);

    const std::vector<Maneuver>& getManeuvers() const;

    std::size_t size() const;
    bool empty() const;

    long double getTotalImpulse() const;

    const Maneuver& operator[](std::size_t index) const;
    Maneuver& operator[](std::size_t index);

    // Fitness
    std::optional<double> getFitness() const;
    void setFitness(double fitness);

private:
    std::vector<Maneuver> maneuvers;
    std::optional<double> fitness;
};

#endif // SOLARSCAPE_SPECIMEN_H