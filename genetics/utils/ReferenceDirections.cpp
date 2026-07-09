#include "ReferenceDirections.h"

#include <vector>

namespace
{
    void appendLatticeDirections(std::vector<ReferenceDirections::Direction>& directions, ReferenceDirections::Direction& current,
                                 std::size_t objective, std::size_t remainingDivisions, std::size_t divisions)
    {
        if (objective + 1 == current.size())
        {
            current[objective] = static_cast<Real>(remainingDivisions) / static_cast<Real>(divisions);
            directions.push_back(current);
            return;
        }

        for (std::size_t value = 0; value <= remainingDivisions; ++value)
        {
            current[objective] = static_cast<Real>(value) / static_cast<Real>(divisions);
            appendLatticeDirections(directions, current, objective + 1, remainingDivisions - value, divisions);
        }
    }

    std::vector<ReferenceDirections::Direction> generateLatticeDirections(std::size_t objectiveCount, std::size_t divisions)
    {
        std::vector<ReferenceDirections::Direction> directions;
        ReferenceDirections::Direction current(objectiveCount, 0.0);

        appendLatticeDirections(directions, current, 0, divisions, divisions);

        return directions;
    }
} // namespace

std::vector<ReferenceDirections::Direction> ReferenceDirections::generate(std::size_t directionCount, std::size_t objectiveCount)
{
    if (objectiveCount == 1)
    {
        return std::vector<Direction>(directionCount, Direction{1.0});
    }

    if (directionCount == 1)
    {
        return {Direction(objectiveCount, 1.0 / static_cast<Real>(objectiveCount))};
    }

    std::size_t divisions = 1;
    std::vector<Direction> lattice;

    do
    {
        lattice = generateLatticeDirections(objectiveCount, divisions);
        ++divisions;
    } while (lattice.size() < directionCount);

    if (lattice.size() == directionCount)
    {
        return lattice;
    }

    std::vector<Direction> directions;
    directions.reserve(directionCount);

    for (std::size_t i = 0; i < directionCount; ++i)
    {
        const std::size_t sourceIndex = i * (lattice.size() - 1) / (directionCount - 1);
        directions.push_back(lattice[sourceIndex]);
    }

    return directions;
}
