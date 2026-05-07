//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_DISTANCE_ANALYSIS_H
#define SOLARSCAPE_DISTANCE_ANALYSIS_H

#include <vector>

#include "../math/Body.h"

namespace DistanceAnalysis
{
    Real distance(
        const Vector3& left,
        const Vector3& right);

    Vector3 absolutePointForBody(
        const Body& targetBody,
        const Vector3& relativePoint);

    Real minimumDistanceFromMovingPoint(
        std::vector<Body> bodies,
        std::size_t observedBodyIndex,
        std::size_t targetBodyIndex,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant);
}

#endif // SOLARSCAPE_DISTANCE_ANALYSIS_H