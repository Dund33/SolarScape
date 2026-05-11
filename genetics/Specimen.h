//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_SPECIMEN_H
#define SOLARSCAPE_SPECIMEN_H

#include <cstddef>
#include <optional>
#include <vector>

#include "genetics/Maneuver.h"

class Probe;

class Specimen
{
public:
    Specimen();
    explicit Specimen(Probe* probe);
    Specimen(const std::vector<Maneuver>& maneuvers, Probe* probe);

    void addManeuver(const Maneuver& maneuver);

    const std::vector<Maneuver>& getManeuvers() const;
    Probe* getProbe() const;
    void setProbe(Probe* probe);

    std::size_t size() const;
    bool empty() const;

    long double getTotalFuelUse() const;

    const Maneuver& operator[](std::size_t index) const;
    Maneuver& operator[](std::size_t index);

    // Fitness
    std::optional<double> getFitness() const;
    void setFitness(double fitness);
    void clearFitness();

private:
    std::vector<Maneuver> maneuvers;
    Probe* probe{};
    std::optional<double> fitness;
};

#endif // SOLARSCAPE_SPECIMEN_H
