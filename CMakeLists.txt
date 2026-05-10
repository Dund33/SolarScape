cmake_minimum_required(VERSION 3.10)

project(SolarScape VERSION 1.0 LANGUAGES CXX)

# =====================================
# C++17
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
        math/Body.cpp
        math/Verlet.cpp
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
# Gfx Library
# =====================================

add_library(SolarScapeVisual STATIC
        visual/PlotTrajectory.cpp
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
        genetics/selection/TournamentSelection.cpp
)

add_library(SolarScape::Genetic ALIAS SolarScapeGenetic)

target_include_directories(SolarScapeGenetic
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)


# =====================================
# Plyczki :3
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