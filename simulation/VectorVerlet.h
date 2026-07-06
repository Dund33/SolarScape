#ifndef SOLARSCAPE_VECTORVERLET_H
#define SOLARSCAPE_VECTORVERLET_H

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#if defined(__AVX__)
#include <immintrin.h>
#endif

#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/Maneuver.h"
#include "simulation/VectorSimulation.h"

class VectorVerlet final : public VectorSimulation
{
public:
    static constexpr std::size_t BatchWidth = 4;

    VectorVerlet(
        std::vector<Body> bodies,
        Body targetBody,
        Probe probe,
        std::vector<std::vector<Maneuver>> maneuverBatch,
        Real gravitationalConstant);

    std::size_t batchSize() const override;
    void step(Real timeStep) override;

    Real requestedFuelUse(std::size_t laneIndex) const override;
    Real initialProbeFuelMass(std::size_t laneIndex) const override;
    Vector3 probePosition(std::size_t laneIndex) const override;
    Vector3 targetBodyPosition(std::size_t laneIndex) const override;

private:
    struct LaneValues
    {
        std::array<double, BatchWidth> values{};
    };

    struct BodyState
    {
        LaneValues positionX;
        LaneValues positionY;
        LaneValues positionZ;
        LaneValues velocityX;
        LaneValues velocityY;
        LaneValues velocityZ;
        LaneValues mass;
    };

    struct AccelerationState
    {
        LaneValues x;
        LaneValues y;
        LaneValues z;
    };

    using ActiveManeuvers =
        std::array<std::optional<Maneuver>, BatchWidth>;

    void initializeBodyState(
        std::size_t bodyIndex,
        const Body& body);

    void initializeProbeState(
        const Probe& probe);

    void validateLaneIndex(
        std::size_t laneIndex) const;

    Vector3 positionFor(
        std::size_t bodyIndex,
        std::size_t laneIndex) const;

    ActiveManeuvers activeManeuvers() const;

    void calculateAccelerations(
        std::vector<AccelerationState>& accelerations) const;

    AccelerationState calculateAccelerationForBody(
        std::size_t bodyIndex) const;

    AccelerationState calculateAccelerationForBodyScalar(
        std::size_t bodyIndex) const;

#if defined(__AVX__)
    static __m256d loadLaneValues(
        const LaneValues& lanes);

    static void storeLaneValues(
        LaneValues& lanes,
        __m256d value);
#endif

    void updatePositions(
        double timeStep);

    void updateVelocities(
        double timeStep);

    AccelerationState calculateManeuverAccelerations(
        const ActiveManeuvers& activeManeuvers,
        double timeStep) const;

    void applyManeuverAcceleration(
        const AccelerationState& maneuverAcceleration,
        double timeStep);

    void burnFuel(
        const ActiveManeuvers& activeManeuvers,
        double timeStep);

    std::size_t batchSize_{};
    std::size_t targetBodyIndex_{};
    std::size_t probeBodyIndex_{};

    std::vector<std::vector<Maneuver>> maneuverBatch_;
    std::vector<BodyState> bodyStates_;
    std::vector<AccelerationState> previousAccelerations_;
    std::vector<AccelerationState> nextAccelerations_;

    LaneValues requestedFuelUse_;
    LaneValues initialProbeFuelMass_;
    LaneValues probeFuelMass_;

    double probeEmptyMass_{};
    double probeFuelFlow_{};
    double probeSpecificImpulse_{};
    double gravitationalConstant_{};
    double time_{};
};

#endif
