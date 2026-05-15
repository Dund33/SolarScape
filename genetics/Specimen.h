#ifndef SOLARSCAPE_SPECIMEN_H
#define SOLARSCAPE_SPECIMEN_H

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "genetics/fitness/FitnessResult.h"
#include "simulation/Maneuver.h"

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

    const std::optional<FitnessResult>& getFitness() const;
    void setFitness(const FitnessResult& fitness);
    void clearFitness();

private:
    std::vector<Maneuver> maneuvers;
    Probe* probe{};
    std::optional<FitnessResult> fitness;
};

bool operator<(const Specimen& lhs, const Specimen& rhs);

namespace std
{
    template<>
    struct less<Specimen>
    {
        bool operator()(const Specimen& lhs, const Specimen& rhs) const
        {
            return lhs < rhs;
        }
    };
}

#endif
