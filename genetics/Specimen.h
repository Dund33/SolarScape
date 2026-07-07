#ifndef SOLARSCAPE_SPECIMEN_H
#define SOLARSCAPE_SPECIMEN_H

#include <cstddef>
#include <optional>
#include <vector>

#include "genetics/fitness/FitnessValue.h"
#include "simulation/Maneuver.h"

class ProbeProperties;

class Specimen
{
public:
    Specimen();
    explicit Specimen(const std::vector<Maneuver>& maneuverValues);
    explicit Specimen(std::vector<Maneuver>&& maneuverValues);

    const std::vector<Maneuver>& getManeuvers() const;

    std::size_t size() const;
    bool empty() const;

    const Maneuver& operator[](std::size_t index) const;
    Maneuver& operator[](std::size_t index);

    const std::optional<FitnessValue>& getFitness() const;
    void setFitness(const FitnessValue& fitness);
    void clearFitness();

private:
    std::vector<Maneuver> maneuvers;
    std::optional<FitnessValue> fitness;
};

#endif
