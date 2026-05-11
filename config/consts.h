//
// Created by Luke on 5/10/2026.
//

#ifndef SOLARSCAPE_CONSTS_H
#define SOLARSCAPE_CONSTS_H

#include <cstddef>

inline constexpr double MAX_IMPULSE = 3000.0;
inline constexpr double MAX_THRUST = 1000.0;
inline constexpr double STANDARD_GRAVITY = 9.80665;

inline constexpr std::size_t POPULATION_SIZE = 250;
inline constexpr std::size_t GENERATIONS = 250;
inline constexpr std::size_t ELITE_COUNT = 2;

inline constexpr std::size_t MIN_MANEUVERS = 1;
inline constexpr std::size_t MAX_MANEUVERS = 25;

inline constexpr long double MIN_MANEUVER_TIME = 0.0L;
inline constexpr long double MIN_MANEUVER_DURATION = 1.0L;
inline constexpr long double MAX_MANEUVER_DURATION = 10000.0L;

inline constexpr double MUTATION_PROBABILITY = 0.1;
inline constexpr long double MUTATION_TIME_RANGE = 10000.0L;
inline constexpr long double MUTATION_DURATION_RANGE = 5000.0L;
inline constexpr long double MUTATION_THRUST_RANGE = 1000.0L;

inline constexpr std::size_t TOURNAMENT_SIZE = 5;

#endif //SOLARSCAPE_CONSTS_H
