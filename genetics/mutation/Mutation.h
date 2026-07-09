#ifndef SOLARSCAPE_MUTATION_H
#define SOLARSCAPE_MUTATION_H

class Specimen;

class Mutation
{
public:
    virtual ~Mutation() = default;

    virtual void mutate(Specimen& specimen, bool closeToTarget = false) const = 0;
};

#endif
