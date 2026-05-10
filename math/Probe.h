//
// Created by Luke on 5/10/2026.
//

#ifndef SOLARSCAPE_PROBE_H
#define SOLARSCAPE_PROBE_H
#include "Body.h"


class Probe : public Body
{
public:
    Probe();
    virtual ~Probe();
    double getFuel() const;
    double getEmptyMass() const;
    double getMass() const;
};


#endif //SOLARSCAPE_PROBE_H
