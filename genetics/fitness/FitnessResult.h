#ifndef FITNESSRESULT_H
#define FITNESSRESULT_H

#include <array>
#include <cstddef>

class FitnessResult {
public:
    static constexpr std::size_t kSize = 3;

    FitnessResult();
    explicit FitnessResult(const std::array<float, kSize>& values);
    FitnessResult(
        float minimumDistance,
        float minimumDistanceTime,
        float minimumDistanceFuelMass
    );

    float minimumDistance() const;
    float minimumDistanceTime() const;
    float minimumDistanceFuelMass() const;

    const std::array<float, kSize>& values() const;
    std::array<float, kSize>& values();

    float get(std::size_t index) const;
    void set(std::size_t index, float value);

private:
    std::array<float, kSize> values_;
};

#endif // FITNESSRESULT_H
