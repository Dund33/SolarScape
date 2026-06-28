#ifndef SOLARSCAPE_PARETOFRONTJSONWRITER_H
#define SOLARSCAPE_PARETOFRONTJSONWRITER_H

#include <string>
#include <vector>

#include "genetics/GeneticAlgorithm.h"
#include "genetics/Specimen.h"

void writeParetoFrontJson(
    const std::string& filePath,
    const std::vector<Specimen>& paretoFront);

void writeParetoFrontJson(
    const std::string& filePath,
    const ParetoFrontHistory& paretoFrontHistory);

#endif
