cmake_minimum_required(VERSION 3.10)

project(SolarScape VERSION 1.0 LANGUAGES CXX)

# =====================================
# C++17
# =====================================

set(CMAKE_CXX_STANDARD 17)
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
# Executable
# =====================================

add_executable(SolarScape
        simulation.cpp
)

target_link_libraries(SolarScape
        PRIVATE
        SolarScape::Simulation
        SolarScape::Config
)