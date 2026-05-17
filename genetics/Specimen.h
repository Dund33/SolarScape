#ifndef SOLARSCAPE_SPECIMEN_H
#define SOLARSCAPE_SPECIMEN_H

#include <compare>
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
    explicit Specimen(const std::vector<Maneuver>& maneuvers);

    void addManeuver(const Maneuver& maneuver);

    const std::vector<Maneuver>& getManeuvers() const;

    std::size_t size() const;
    bool empty() const;

    long double getTotalFuelUse(
        const ProbeProperties& probeProperties
    ) const;

    const Maneuver& operator[](std::size_t index) const;
    Maneuver& operator[](std::size_t index);

    const std::optional<FitnessValue>& getFitness() const;
    void setFitness(const FitnessValue& fitness);
    void clearFitness();

private:
    std::vector<Maneuver> maneuvers;
    std::optional<FitnessValue> fitness;
};

std::partial_ordering operator<=>(const Specimen& lhs, const Specimen& rhs);
bool operator==(const Specimen& lhs, const Specimen& rhs);
bool operator<(const Specimen& lhs, const Specimen& rhs);

#endif
