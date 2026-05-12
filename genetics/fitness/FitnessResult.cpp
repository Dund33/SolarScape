#include "FitnessResult.h"

FitnessResult::FitnessResult()
    : values_{}
{
}

FitnessResult::FitnessResult(const std::array<float, kSize>& values)
    : values_(values)
{
}

FitnessResult::FitnessResult(
    float minimumDistance,
    float minimumDistanceTime,
    float minimumDistanceFuelMass
)
    : values_{minimumDistance, minimumDistanceTime, minimumDistanceFuelMass}
{
}

float FitnessResult::minimumDistance() const
{
    return values_[0];
}

float FitnessResult::minimumDistanceTime() const
{
    return values_[1];
}

float FitnessResult::minimumDistanceFuelMass() const
{
    return values_[2];
}

const std::array<float, FitnessResult::kSize>& FitnessResult::values() const
{
    return values_;
}

std::array<float, FitnessResult::kSize>& FitnessResult::values()
{
    return values_;
}

float FitnessResult::get(std::size_t index) const
{
    return values_.at(index);
}

void FitnessResult::set(std::size_t index, float value)
{
    values_.at(index) = value;
}
