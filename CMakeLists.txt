cmake_minimum_required(VERSION 3.10)

project(SolarScape VERSION 1.0 LANGUAGES CXX)

# =====================================
# C++ Standard
# =====================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =====================================
# Dependencies
# =====================================

find_package(yaml-cpp CONFIG REQUIRED)

# =====================================
# Math Library
# =====================================

add_library(SolarScapeMath STATIC
        math/Vector3.cpp
        math/Vector3.h
        math/Body.cpp
        math/Body.h
        math/Verlet.cpp
        math/Verlet.h
        math/Probe.cpp
        math/Probe.h
)

add_library(SolarScape::Math ALIAS SolarScapeMath)

target_include_directories(SolarScapeMath
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# =====================================
# Simulation Library
# =====================================

add_library(SolarScapeSimulation STATIC
        simulation/DistanceAnalysis.cpp
        simulation/DistanceAnalysis.h
)

add_library(SolarScape::Simulation ALIAS SolarScapeSimulation)

target_include_directories(SolarScapeSimulation
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeSimulation
        PUBLIC
        SolarScape::Math
)

# =====================================
# Config Library
# =====================================

add_library(SolarScapeConfig STATIC
        config/SimulationConfig.cpp
        config/SimulationConfig.h
)

add_library(SolarScape::Config ALIAS SolarScapeConfig)

target_include_directories(SolarScapeConfig
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeConfig
        PUBLIC
        SolarScape::Math
        yaml-cpp::yaml-cpp
)

# =====================================
# Visual Library
# =====================================

add_library(SolarScapeVisual STATIC
        visual/PlotTrajectory.cpp
        visual/PlotTrajectory.h
)

add_library(SolarScape::Visual ALIAS SolarScapeVisual)

target_include_directories(SolarScapeVisual
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# =====================================
# Genetics Library
# =====================================

add_library(SolarScapeGenetic STATIC
        genetics/Specimen.cpp
        genetics/Specimen.h
        genetics/selection/TournamentSelection.cpp
        genetics/selection/TournamentSelection.h
        genetics/mutation/Mutation.cpp
        genetics/mutation/Mutation.h
        genetics/init/RandomInitializer.cpp
        genetics/init/RandomInitializer.h
        genetics/crossing/Crossover.cpp
        genetics/crossing/Crossover.h
        genetics/fitness/FitnessEvaluator.cpp
        genetics/fitness/FitnessEvaluator.h
)

add_library(SolarScape::Genetic ALIAS SolarScapeGenetic)

target_include_directories(SolarScapeGenetic
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGenetic
        PUBLIC
        SolarScape::Math
        SolarScape::Simulation
)

# =====================================
# Copy config.yaml
# =====================================

configure_file(
        config.yaml
        config.yaml
        COPYONLY
)

# =====================================
# Executable
# =====================================

add_executable(SolarScape
        simulation.cpp
)

target_link_libraries(SolarScape
        PRIVATE
        SolarScape::Simulation
        SolarScape::Config
        SolarScape::Visual
        SolarScape::Genetic
)