#ifndef SOLARSCAPE_PARETOFRONTJSONWRITER_H
#define SOLARSCAPE_PARETOFRONTJSONWRITER_H

#include <string>
#include <vector>

#include "genetics/Specimen.h"

void writeParetoFrontJson(
    const std::string& filePath,
    const std::vector<Specimen>& paretoFront);

#endif
