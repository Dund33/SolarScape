//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_VERLET_H
#define SOLARSCAPE_VERLET_H

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

#include "../data/Body.h"

namespace Verlet
{
    inline Vector3 calculateAccelerationForBody(const std::vector<Body>& bodies, std::size_t bodyIndex, double gravitationalConstant)
    {
        Vector3 acceleration;

        for (std::size_t j = 0; j < bodies.size(); ++j)
        {
            if (bodyIndex == j)
            {
                continue;
            }

            const Vector3 direction = bodies[j].position - bodies[bodyIndex].position;
            const double distanceSquared = direction.lengthSquared();

            if (distanceSquared == 0.0)
            {
                continue;
            }

            const double distance = std::sqrt(distanceSquared);
            const double factor = gravitationalConstant * bodies[j].mass / (distanceSquared * distance);
            acceleration += direction * factor;
        }

        return acceleration;
    }

    inline std::vector<Vector3> calculateAccelerations(const std::vector<Body>& bodies, double gravitationalConstant)
    {
        std::vector<Vector3> accelerations(bodies.size());

        if (bodies.empty())
        {
            return accelerations;
        }

        const unsigned int availableThreads = std::max(1u, std::thread::hardware_concurrency());
        const std::size_t taskCount = std::min<std::size_t>(availableThreads, bodies.size());

        std::vector<std::future<void> > tasks;
        tasks.reserve(taskCount);

        for (std::size_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
        {
            const std::size_t begin = taskIndex * bodies.size() / taskCount;
            const std::size_t end = (taskIndex + 1) * bodies.size() / taskCount;

            tasks.push_back(std::async(std::launch::async, [&bodies, &accelerations, gravitationalConstant, begin, end]() {
                for (std::size_t i = begin; i < end; ++i)
                {
                    accelerations[i] = calculateAccelerationForBody(bodies, i, gravitationalConstant);
                }
            }));
        }

        for (std::size_t i = 0; i < tasks.size(); ++i)
        {
            tasks[i].get();
        }

        return accelerations;
    }

    inline void step(std::vector<Body>& bodies, double timeStep, double gravitationalConstant)
    {
        const std::vector<Vector3> previousAccelerations = calculateAccelerations(bodies, gravitationalConstant);
        const double timeStepSquared = timeStep * timeStep;

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 velocityPart = bodies[i].velocity * timeStep;
            const Vector3 accelerationPart = previousAccelerations[i] * (0.5 * timeStepSquared);
            bodies[i].position += velocityPart + accelerationPart;
        }

        const std::vector<Vector3> nextAccelerations = calculateAccelerations(bodies, gravitationalConstant);

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 averageAcceleration = (previousAccelerations[i] + nextAccelerations[i]) * 0.5;
            bodies[i].velocity += averageAcceleration * timeStep;
        }
    }
}

#endif //SOLARSCAPE_VERLET_H
