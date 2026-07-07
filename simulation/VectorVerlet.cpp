#include "simulation/VectorVerlet.h"

#include <algorithm>
#include <cmath>
#include <immintrin.h>
#include <limits>
#include <stdexcept>
#include <utility>

#include "config/consts.h"

namespace
{
    template <typename Value> double toDouble(Value value)
    {
        return static_cast<double>(value);
    }

    double clampedThrottle(const Maneuver& maneuver)
    {
        return std::clamp(toDouble(maneuver.getThrottleValue()), 0.0, 1.0);
    }

    std::optional<Maneuver> activeManeuverAt(const std::vector<Maneuver>& maneuvers, double time)
    {
        double previousManeuverEndTime = 0.0;

        for (const Maneuver& maneuver : maneuvers)
        {
            const double maneuverStartTime = previousManeuverEndTime + toDouble(maneuver.getInitDelay());
            const double maneuverEndTime = maneuverStartTime + toDouble(maneuver.getDuration());

            if (maneuverStartTime <= time && time < maneuverEndTime)
            {
                return maneuver;
            }

            previousManeuverEndTime = maneuverEndTime;
        }

        return std::nullopt;
    }

    double requestedFuelUseFor(const std::vector<Maneuver>& maneuvers, double fuelFlow)
    {
        double fuelUse = 0.0;

        for (const Maneuver& maneuver : maneuvers)
        {
            fuelUse += fuelFlow * toDouble(maneuver.getThrottleValue()) * toDouble(maneuver.getDuration());
        }

        return fuelUse;
    }

} // namespace

VectorVerlet::VectorVerlet(std::vector<Body> bodies, Body targetBody, Probe probe, std::vector<std::vector<Maneuver>> maneuverBatch,
                           Real gravitationalConstant)
    : batchSize_(maneuverBatch.size()), targetBodyIndex_(bodies.size()), probeBodyIndex_(bodies.size() + 1),
      maneuverBatch_(std::move(maneuverBatch)), bodyStates_(bodies.size() + 2), previousAccelerations_(bodies.size() + 2),
      nextAccelerations_(bodies.size() + 2), probeEmptyMass_(toDouble(probe.emptyMass())), probeFuelFlow_(toDouble(probe.fuelFlow())),
      probeSpecificImpulse_(toDouble(probe.specificImpulse())), gravitationalConstant_(toDouble(gravitationalConstant))
{
    if (batchSize_ == 0 || BatchWidth < batchSize_)
    {
        throw std::invalid_argument("VectorVerlet batch size must be in range [1, 4].");
    }

    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex)
    {
        initializeBodyState(bodyIndex, bodies[bodyIndex]);
    }

    initializeBodyState(targetBodyIndex_, targetBody);
    initializeProbeState(probe);

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        requestedFuelUse_.values[laneIndex] = requestedFuelUseFor(maneuverBatch_[laneIndex], probeFuelFlow_);
    }
}

std::size_t VectorVerlet::batchSize() const
{
    return batchSize_;
}

void VectorVerlet::step(Real timeStep)
{
    const double stepTime = toDouble(timeStep);
    const ActiveManeuvers maneuvers = activeManeuvers();

    calculateAccelerations(previousAccelerations_);
    updatePositions(stepTime);
    calculateAccelerations(nextAccelerations_);

    const AccelerationState maneuverAcceleration = calculateManeuverAccelerations(maneuvers, stepTime);

    updateVelocities(stepTime);
    applyManeuverAcceleration(maneuverAcceleration, stepTime);
    burnFuel(maneuvers, stepTime);

    time_ += stepTime;
}

Real VectorVerlet::requestedFuelUse(std::size_t laneIndex) const
{
    validateLaneIndex(laneIndex);

    return static_cast<Real>(requestedFuelUse_.values[laneIndex]);
}

Real VectorVerlet::initialProbeFuelMass(std::size_t laneIndex) const
{
    validateLaneIndex(laneIndex);

    return static_cast<Real>(initialProbeFuelMass_.values[laneIndex]);
}

Vector3 VectorVerlet::probePosition(std::size_t laneIndex) const
{
    return positionFor(probeBodyIndex_, laneIndex);
}

Vector3 VectorVerlet::targetBodyPosition(std::size_t laneIndex) const
{
    return positionFor(targetBodyIndex_, laneIndex);
}

void VectorVerlet::initializeBodyState(std::size_t bodyIndex, const Body& body)
{
    BodyState& state = bodyStates_[bodyIndex];

    for (std::size_t laneIndex = 0; laneIndex < BatchWidth; ++laneIndex)
    {
        state.positionX.values[laneIndex] = toDouble(body.position().x);
        state.positionY.values[laneIndex] = toDouble(body.position().y);
        state.positionZ.values[laneIndex] = toDouble(body.position().z);
        state.velocityX.values[laneIndex] = toDouble(body.velocity().x);
        state.velocityY.values[laneIndex] = toDouble(body.velocity().y);
        state.velocityZ.values[laneIndex] = toDouble(body.velocity().z);
        state.mass.values[laneIndex] = toDouble(body.mass());
    }
}

void VectorVerlet::initializeProbeState(const Probe& probe)
{
    BodyState& state = bodyStates_[probeBodyIndex_];

    for (std::size_t laneIndex = 0; laneIndex < BatchWidth; ++laneIndex)
    {
        const double fuelMass = toDouble(probe.fuelMass());
        const double mass = probeEmptyMass_ + fuelMass;

        state.positionX.values[laneIndex] = toDouble(probe.position().x);
        state.positionY.values[laneIndex] = toDouble(probe.position().y);
        state.positionZ.values[laneIndex] = toDouble(probe.position().z);
        state.velocityX.values[laneIndex] = toDouble(probe.velocity().x);
        state.velocityY.values[laneIndex] = toDouble(probe.velocity().y);
        state.velocityZ.values[laneIndex] = toDouble(probe.velocity().z);
        state.mass.values[laneIndex] = mass;

        initialProbeFuelMass_.values[laneIndex] = fuelMass;
        probeFuelMass_.values[laneIndex] = fuelMass;
    }
}

void VectorVerlet::validateLaneIndex(std::size_t laneIndex) const
{
    if (batchSize_ <= laneIndex)
    {
        throw std::out_of_range("VectorVerlet lane index is out of range.");
    }
}

Vector3 VectorVerlet::positionFor(std::size_t bodyIndex, std::size_t laneIndex) const
{
    validateLaneIndex(laneIndex);

    const BodyState& state = bodyStates_[bodyIndex];

    return {static_cast<Real>(state.positionX.values[laneIndex]), static_cast<Real>(state.positionY.values[laneIndex]),
            static_cast<Real>(state.positionZ.values[laneIndex])};
}

auto VectorVerlet::activeManeuvers() const -> ActiveManeuvers
{
    ActiveManeuvers result;

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        result[laneIndex] = activeManeuverAt(maneuverBatch_[laneIndex], time_);
    }

    return result;
}

void VectorVerlet::calculateAccelerations(std::vector<AccelerationState>& accelerations) const
{
    accelerations.resize(bodyStates_.size());

    for (std::size_t bodyIndex = 0; bodyIndex < bodyStates_.size(); ++bodyIndex)
    {
        accelerations[bodyIndex] = calculateAccelerationForBody(bodyIndex);
    }
}

auto VectorVerlet::calculateAccelerationForBody(std::size_t bodyIndex) const -> AccelerationState
{
#if defined(__AVX__)
    const BodyState& body = bodyStates_[bodyIndex];
    const __m256d x = loadLaneValues(body.positionX);
    const __m256d y = loadLaneValues(body.positionY);
    const __m256d z = loadLaneValues(body.positionZ);
    const __m256d gravitationalConstant = _mm256_set1_pd(gravitationalConstant_);
    const __m256d zero = _mm256_setzero_pd();

    __m256d accelerationX = zero;
    __m256d accelerationY = zero;
    __m256d accelerationZ = zero;

    for (std::size_t otherBodyIndex = 0; otherBodyIndex < bodyStates_.size(); ++otherBodyIndex)
    {
        if (otherBodyIndex == bodyIndex)
        {
            continue;
        }

        const BodyState& otherBody = bodyStates_[otherBodyIndex];

        const __m256d directionX = _mm256_sub_pd(loadLaneValues(otherBody.positionX), x);
        const __m256d directionY = _mm256_sub_pd(loadLaneValues(otherBody.positionY), y);
        const __m256d directionZ = _mm256_sub_pd(loadLaneValues(otherBody.positionZ), z);

        const __m256d distanceSquared =
            _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(directionX, directionX), _mm256_mul_pd(directionY, directionY)),
                          _mm256_mul_pd(directionZ, directionZ));

        const __m256d distance = _mm256_sqrt_pd(distanceSquared);
        const __m256d denominator = _mm256_mul_pd(distanceSquared, distance);
        const __m256d factor = _mm256_div_pd(_mm256_mul_pd(gravitationalConstant, loadLaneValues(otherBody.mass)), denominator);
        const __m256d validDistance = _mm256_cmp_pd(distanceSquared, zero, _CMP_NEQ_OQ);
        const __m256d maskedFactor = _mm256_and_pd(factor, validDistance);

        accelerationX = _mm256_add_pd(accelerationX, _mm256_mul_pd(directionX, maskedFactor));
        accelerationY = _mm256_add_pd(accelerationY, _mm256_mul_pd(directionY, maskedFactor));
        accelerationZ = _mm256_add_pd(accelerationZ, _mm256_mul_pd(directionZ, maskedFactor));
    }

    AccelerationState result;
    storeLaneValues(result.x, accelerationX);
    storeLaneValues(result.y, accelerationY);
    storeLaneValues(result.z, accelerationZ);

    return result;
#else
    return calculateAccelerationForBodyScalar(bodyIndex);
#endif
}

auto VectorVerlet::calculateAccelerationForBodyScalar(std::size_t bodyIndex) const -> AccelerationState
{
    AccelerationState result;
    const BodyState& body = bodyStates_[bodyIndex];

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        double accelerationX = 0.0;
        double accelerationY = 0.0;
        double accelerationZ = 0.0;

        for (std::size_t otherBodyIndex = 0; otherBodyIndex < bodyStates_.size(); ++otherBodyIndex)
        {
            if (otherBodyIndex == bodyIndex)
            {
                continue;
            }

            const BodyState& otherBody = bodyStates_[otherBodyIndex];
            const double directionX = otherBody.positionX.values[laneIndex] - body.positionX.values[laneIndex];
            const double directionY = otherBody.positionY.values[laneIndex] - body.positionY.values[laneIndex];
            const double directionZ = otherBody.positionZ.values[laneIndex] - body.positionZ.values[laneIndex];
            const double distanceSquared = directionX * directionX + directionY * directionY + directionZ * directionZ;

            if (distanceSquared == 0.0)
            {
                continue;
            }

            const double distance = std::sqrt(distanceSquared);
            const double factor = gravitationalConstant_ * otherBody.mass.values[laneIndex] / (distanceSquared * distance);

            accelerationX += directionX * factor;
            accelerationY += directionY * factor;
            accelerationZ += directionZ * factor;
        }

        result.x.values[laneIndex] = accelerationX;
        result.y.values[laneIndex] = accelerationY;
        result.z.values[laneIndex] = accelerationZ;
    }

    return result;
}

#if defined(__AVX__)
__m256d VectorVerlet::loadLaneValues(const LaneValues& lanes)
{
    return _mm256_loadu_pd(lanes.values.data());
}

void VectorVerlet::storeLaneValues(LaneValues& lanes, __m256d value)
{
    _mm256_storeu_pd(lanes.values.data(), value);
}
#endif

void VectorVerlet::updatePositions(double timeStep)
{
#if defined(__AVX__)
    const __m256d stepTime = _mm256_set1_pd(timeStep);
    const __m256d halfStepTimeSquared = _mm256_set1_pd(0.5 * timeStep * timeStep);

    for (std::size_t bodyIndex = 0; bodyIndex < bodyStates_.size(); ++bodyIndex)
    {
        BodyState& body = bodyStates_[bodyIndex];
        const AccelerationState& acceleration = previousAccelerations_[bodyIndex];

        storeLaneValues(body.positionX, _mm256_add_pd(loadLaneValues(body.positionX),
                                                      _mm256_add_pd(_mm256_mul_pd(loadLaneValues(body.velocityX), stepTime),
                                                                    _mm256_mul_pd(loadLaneValues(acceleration.x), halfStepTimeSquared))));
        storeLaneValues(body.positionY, _mm256_add_pd(loadLaneValues(body.positionY),
                                                      _mm256_add_pd(_mm256_mul_pd(loadLaneValues(body.velocityY), stepTime),
                                                                    _mm256_mul_pd(loadLaneValues(acceleration.y), halfStepTimeSquared))));
        storeLaneValues(body.positionZ, _mm256_add_pd(loadLaneValues(body.positionZ),
                                                      _mm256_add_pd(_mm256_mul_pd(loadLaneValues(body.velocityZ), stepTime),
                                                                    _mm256_mul_pd(loadLaneValues(acceleration.z), halfStepTimeSquared))));
    }
#else
    const double halfStepTimeSquared = 0.5 * timeStep * timeStep;

    for (std::size_t bodyIndex = 0; bodyIndex < bodyStates_.size(); ++bodyIndex)
    {
        BodyState& body = bodyStates_[bodyIndex];
        const AccelerationState& acceleration = previousAccelerations_[bodyIndex];

        for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
        {
            body.positionX.values[laneIndex] +=
                body.velocityX.values[laneIndex] * timeStep + acceleration.x.values[laneIndex] * halfStepTimeSquared;
            body.positionY.values[laneIndex] +=
                body.velocityY.values[laneIndex] * timeStep + acceleration.y.values[laneIndex] * halfStepTimeSquared;
            body.positionZ.values[laneIndex] +=
                body.velocityZ.values[laneIndex] * timeStep + acceleration.z.values[laneIndex] * halfStepTimeSquared;
        }
    }
#endif
}

void VectorVerlet::updateVelocities(double timeStep)
{
#if defined(__AVX__)
    const __m256d halfStepTime = _mm256_set1_pd(0.5 * timeStep);

    for (std::size_t bodyIndex = 0; bodyIndex < bodyStates_.size(); ++bodyIndex)
    {
        BodyState& body = bodyStates_[bodyIndex];
        const AccelerationState& previousAcceleration = previousAccelerations_[bodyIndex];
        const AccelerationState& nextAcceleration = nextAccelerations_[bodyIndex];

        storeLaneValues(body.velocityX,
                        _mm256_add_pd(loadLaneValues(body.velocityX), _mm256_mul_pd(_mm256_add_pd(loadLaneValues(previousAcceleration.x),
                                                                                                  loadLaneValues(nextAcceleration.x)),
                                                                                    halfStepTime)));
        storeLaneValues(body.velocityY,
                        _mm256_add_pd(loadLaneValues(body.velocityY), _mm256_mul_pd(_mm256_add_pd(loadLaneValues(previousAcceleration.y),
                                                                                                  loadLaneValues(nextAcceleration.y)),
                                                                                    halfStepTime)));
        storeLaneValues(body.velocityZ,
                        _mm256_add_pd(loadLaneValues(body.velocityZ), _mm256_mul_pd(_mm256_add_pd(loadLaneValues(previousAcceleration.z),
                                                                                                  loadLaneValues(nextAcceleration.z)),
                                                                                    halfStepTime)));
    }
#else
    const double halfStepTime = 0.5 * timeStep;

    for (std::size_t bodyIndex = 0; bodyIndex < bodyStates_.size(); ++bodyIndex)
    {
        BodyState& body = bodyStates_[bodyIndex];
        const AccelerationState& previousAcceleration = previousAccelerations_[bodyIndex];
        const AccelerationState& nextAcceleration = nextAccelerations_[bodyIndex];

        for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
        {
            body.velocityX.values[laneIndex] +=
                (previousAcceleration.x.values[laneIndex] + nextAcceleration.x.values[laneIndex]) * halfStepTime;
            body.velocityY.values[laneIndex] +=
                (previousAcceleration.y.values[laneIndex] + nextAcceleration.y.values[laneIndex]) * halfStepTime;
            body.velocityZ.values[laneIndex] +=
                (previousAcceleration.z.values[laneIndex] + nextAcceleration.z.values[laneIndex]) * halfStepTime;
        }
    }
#endif
}

auto VectorVerlet::calculateManeuverAccelerations(const ActiveManeuvers& activeManeuvers, double timeStep) const -> AccelerationState
{
    AccelerationState acceleration;

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        if (probeFuelMass_.values[laneIndex] <= 0.0 || !activeManeuvers[laneIndex].has_value())
        {
            continue;
        }

        const Maneuver& maneuver = activeManeuvers[laneIndex].value();
        const double throttleValue = clampedThrottle(maneuver);
        const double fuelNeeded = probeFuelFlow_ * throttleValue * timeStep;
        const double fuelScale = 0.0 < fuelNeeded ? std::min(1.0, probeFuelMass_.values[laneIndex] / fuelNeeded) : 0.0;
        const double effectiveThrottle = throttleValue * fuelScale;
        const double probeMass = bodyStates_[probeBodyIndex_].mass.values[laneIndex];
        const double accelerationScale = effectiveThrottle * probeFuelFlow_ * probeSpecificImpulse_ * STANDARD_GRAVITY / probeMass;
        const Vector3& direction = maneuver.getThrustDirection();

        acceleration.x.values[laneIndex] = toDouble(direction.x) * accelerationScale;
        acceleration.y.values[laneIndex] = toDouble(direction.y) * accelerationScale;
        acceleration.z.values[laneIndex] = toDouble(direction.z) * accelerationScale;
    }

    return acceleration;
}

void VectorVerlet::applyManeuverAcceleration(const AccelerationState& maneuverAcceleration, double timeStep)
{
    BodyState& probe = bodyStates_[probeBodyIndex_];

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        probe.velocityX.values[laneIndex] += maneuverAcceleration.x.values[laneIndex] * timeStep;
        probe.velocityY.values[laneIndex] += maneuverAcceleration.y.values[laneIndex] * timeStep;
        probe.velocityZ.values[laneIndex] += maneuverAcceleration.z.values[laneIndex] * timeStep;
    }
}

void VectorVerlet::burnFuel(const ActiveManeuvers& activeManeuvers, double timeStep)
{
    BodyState& probe = bodyStates_[probeBodyIndex_];

    for (std::size_t laneIndex = 0; laneIndex < batchSize_; ++laneIndex)
    {
        if (!activeManeuvers[laneIndex].has_value())
        {
            continue;
        }

        const double throttleValue = clampedThrottle(activeManeuvers[laneIndex].value());
        probeFuelMass_.values[laneIndex] = std::max(0.0, probeFuelMass_.values[laneIndex] - probeFuelFlow_ * throttleValue * timeStep);
        probe.mass.values[laneIndex] = probeEmptyMass_ + probeFuelMass_.values[laneIndex];
    }
}
