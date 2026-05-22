#ifndef SOLARSCAPE_SIMPLELINEARREPAIR_H
#define SOLARSCAPE_SIMPLELINEARREPAIR_H

#include "genetics/repair/Repair.h"

class SimpleLinearRepair final : public Repair
{
public:
    void repair(Specimen& specimen) const override;
};

#endif
