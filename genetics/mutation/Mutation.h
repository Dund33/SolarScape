//
// Created by Luke on 5/10/2026.
//

#ifndef SOLARSCAPE_MUTATION_H
#define SOLARSCAPE_MUTATION_H

class Specimen;

class Mutation
{
public:
    virtual ~Mutation() = default;

    virtual void mutate(Specimen& specimen) const = 0;
};

#endif // SOLARSCAPE_MUTATION_H
