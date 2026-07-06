#ifndef SOLARSCAPE_CONSTS_H
#define SOLARSCAPE_CONSTS_H

#include <cstddef>

#include "math/Real.h"

inline constexpr double STANDARD_GRAVITY = 9.80665;

inline constexpr std::size_t POPULATION_SIZE = 200;
inline constexpr std::size_t GENERATIONS = 600;
inline constexpr std::size_t ELITE_COUNT = 2;

inline constexpr std::size_t ALGO_TARGET_ISLAND_COUNT = 4;
inline constexpr std::size_t ALGO_MIGRATION_INTERVAL = 15;
inline constexpr std::size_t ALGO_MIN_MIGRANT_COUNT = 1;
inline constexpr std::size_t ALGO_ARCHIVE_REINTRODUCTION_INTERVAL = 10;
inline constexpr std::size_t ALGO_ARCHIVE_REINTRODUCTION_COUNT =
    POPULATION_SIZE / 40;

inline constexpr Real TARGET_WINDOW_DISTANCE = 1000000.0;

inline constexpr std::size_t MIN_MANEUVERS = 3;
inline constexpr std::size_t MAX_MANEUVERS = 25;
inline constexpr std::size_t RANDOM_INITIALIZER_MIN_MANEUVERS_RETRY_COUNT = 16;

inline constexpr Real MIN_MANEUVER_TIME = 0.0;
inline constexpr Real MIN_MANEUVER_DURATION = 1.0;
inline constexpr Real MAX_MANEUVER_DURATION = 15000.0;

inline constexpr double MUTATION_PROBABILITY = 0.20;
inline constexpr double EXTENSIVE_MUTATION_ADD_PROBABILITY = 0.50;
inline constexpr double EXTENSIVE_MUTATION_REMOVE_PROBABILITY = 0.25;
inline constexpr Real MUTATION_TIME_RANGE = 50000.0;
inline constexpr Real MUTATION_DURATION_RANGE = 8000.0;
inline constexpr Real MUTATION_DIRECTION_RANGE = 0.55;
inline constexpr Real MUTATION_THROTTLE_RANGE = 0.35;

inline constexpr Real ALIGNED_SIMILARITY_CROSSOVER_MIN_REGION_SIMILARITY = 0.3;

inline constexpr std::size_t TOURNAMENT_SIZE = 3;

#endif
