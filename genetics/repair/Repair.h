#ifndef SOLARSCAPE_REPAIR_H
#define SOLARSCAPE_REPAIR_H

class Specimen;

class Repair
{
public:
    virtual ~Repair() = default;

    virtual void repair(Specimen& specimen) const = 0;
};

#endif
