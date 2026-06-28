#ifndef SOLARSCAPE_POPULATIONPYRAMID_H
#define SOLARSCAPE_POPULATIONPYRAMID_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/init/Initializer.h"

class PopulationPyramid
{
public:
    explicit PopulationPyramid(
        std::vector<std::vector<Specimen>> levels);

    static PopulationPyramid create(
        std::size_t populationSize,
        Initializer& initializer);

    std::vector<std::vector<Specimen>>& levels();
    const std::vector<std::vector<Specimen>>& levels() const;

    void promoteElite(
        std::size_t eliteCount,
        const SpecimenComparator& specimenComparator);

    std::vector<Specimen> flatten() const;

private:
    std::vector<std::vector<Specimen>> levels_;
};

#endif
