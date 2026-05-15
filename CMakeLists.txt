cmake_minimum_required(VERSION 3.10)

project(SolarScape VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

find_package(yaml-cpp CONFIG REQUIRED)

add_library(SolarScapeMath STATIC
        math/Vector3.cpp
        math/Vector3.h
        math/Body.cpp
        math/Body.h
        math/Probe.cpp
        math/Probe.h
)

add_library(SolarScape::Math ALIAS SolarScapeMath)

target_include_directories(SolarScapeMath
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(SolarScapeSimulation STATIC
        simulation/Maneuver.h
        simulation/Verlet.cpp
        simulation/Verlet.h
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

add_library(SolarScapeVisual STATIC
        visual/PlotTrajectory.cpp
        visual/PlotTrajectory.h
)

add_library(SolarScape::Visual ALIAS SolarScapeVisual)

target_include_directories(SolarScapeVisual
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeVisual
        PUBLIC
        SolarScape::Simulation
)

add_library(SolarScapeGenetic STATIC
        genetics/Specimen.cpp
        genetics/Specimen.h
        genetics/selection/Selection.h
        genetics/selection/TournamentSelection.cpp
        genetics/selection/TournamentSelection.h
        genetics/mutation/Mutation.h
        genetics/mutation/RandomUniformMutation.cpp
        genetics/mutation/RandomUniformMutation.h
        genetics/init/Initializer.h
        genetics/init/RandomInitializer.cpp
        genetics/init/RandomInitializer.h
        genetics/crossing/Crossover.h
        genetics/crossing/RandomCutCrossover.cpp
        genetics/crossing/RandomCutCrossover.h
        genetics/fitness/FitnessResult.cpp
        genetics/fitness/FitnessResult.h
        genetics/fitness/FitnessEvaluator.h
        genetics/fitness/SimulationFitnessEvaluator.cpp
        genetics/fitness/SimulationFitnessEvaluator.h
        genetics/search/NormalRandomSearch.cpp
        genetics/search/NormalRandomSearch.h
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

configure_file(
        config.yaml
        config.yaml
        COPYONLY
)


if(LINUX)
    find_package(TBB REQUIRED)
endif()

add_executable(SolarScape
        simulation.cpp
)

target_link_libraries(SolarScape
        PRIVATE
        SolarScape::Config
        SolarScape::Visual
        SolarScape::Genetic
        SolarScape::Simulation
        $<$<PLATFORM_ID:Linux>:TBB::tbb>
)
