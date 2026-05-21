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
        math/ProbeFactory.cpp
        math/ProbeFactory.h
        math/ProbeProperties.cpp
        math/ProbeProperties.h
)

add_library(SolarScape::Math ALIAS SolarScapeMath)

target_include_directories(SolarScapeMath
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(SolarScapeSimulation STATIC
        simulation/Maneuver.h
        simulation/ManeuverSchedule.h
        simulation/Simulation.cpp
        simulation/Simulation.h
        simulation/SimulationFactory.h
        simulation/Verlet.cpp
        simulation/Verlet.h
        simulation/VerletFactory.cpp
        simulation/VerletFactory.h
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

add_library(SolarScapeGeneticCore STATIC
        genetics/Specimen.cpp
        genetics/Specimen.h
        genetics/fitness/FitnessValue.h
)

add_library(SolarScape::GeneticCore ALIAS SolarScapeGeneticCore)

target_include_directories(SolarScapeGeneticCore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticCore
        PUBLIC
        SolarScape::Math
        SolarScape::Simulation
)

add_library(SolarScapeGeneticOperators STATIC
        genetics/selection/SelectionFactory.h
        genetics/selection/Selection.h
        genetics/selection/TournamentSelection.cpp
        genetics/selection/TournamentSelection.h
        genetics/selection/TournamentSelectionFactory.cpp
        genetics/selection/TournamentSelectionFactory.h
        genetics/mutation/MutationFactory.h
        genetics/mutation/Mutation.h
        genetics/mutation/RandomUniformMutation.cpp
        genetics/mutation/RandomUniformMutation.h
        genetics/mutation/RandomUniformMutationFactory.cpp
        genetics/mutation/RandomUniformMutationFactory.h
        genetics/init/InitializerFactory.h
        genetics/init/Initializer.h
        genetics/init/RandomInitializer.cpp
        genetics/init/RandomInitializer.h
        genetics/init/RandomInitializerFactory.cpp
        genetics/init/RandomInitializerFactory.h
        genetics/crossing/CrossoverFactory.h
        genetics/crossing/Crossover.h
        genetics/crossing/RandomCutCrossover.cpp
        genetics/crossing/RandomCutCrossover.h
        genetics/crossing/RandomCutCrossoverFactory.cpp
        genetics/crossing/RandomCutCrossoverFactory.h
)

add_library(SolarScape::GeneticOperators ALIAS SolarScapeGeneticOperators)

target_include_directories(SolarScapeGeneticOperators
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticOperators
        PUBLIC
        SolarScape::GeneticCore
)

add_library(SolarScapeGeneticFitness STATIC
        genetics/fitness/FitnessEvaluatorFactory.h
        genetics/fitness/FitnessEvaluator.h
        genetics/fitness/SimulationFitnessEvaluator.cpp
        genetics/fitness/SimulationFitnessEvaluator.h
        genetics/fitness/SimulationFitnessEvaluatorFactory.cpp
        genetics/fitness/SimulationFitnessEvaluatorFactory.h
)

add_library(SolarScape::GeneticFitness ALIAS SolarScapeGeneticFitness)

target_include_directories(SolarScapeGeneticFitness
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticFitness
        PUBLIC
        SolarScape::GeneticCore
        SolarScape::Simulation
)

add_library(SolarScapeGeneticSearch STATIC
        genetics/search/LocalImprovement.h
        genetics/search/LocalImprovementFactory.h
        genetics/search/NormalRandomSearch.cpp
        genetics/search/NormalRandomSearch.h
        genetics/search/NormalRandomSearchFactory.cpp
        genetics/search/NormalRandomSearchFactory.h
)

add_library(SolarScape::GeneticSearch ALIAS SolarScapeGeneticSearch)

target_include_directories(SolarScapeGeneticSearch
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticSearch
        PUBLIC
        SolarScape::GeneticCore
        SolarScape::GeneticFitness
)

add_library(SolarScapeGeneticAlgorithm STATIC
        genetics/GeneticAlgorithm.cpp
        genetics/GeneticAlgorithm.h
)

add_library(SolarScape::GeneticAlgorithm ALIAS SolarScapeGeneticAlgorithm)

target_include_directories(SolarScapeGeneticAlgorithm
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticAlgorithm
        PUBLIC
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticSearch
)

add_library(SolarScapeGenetic INTERFACE)

add_library(SolarScape::Genetic ALIAS SolarScapeGenetic)

target_link_libraries(SolarScapeGenetic
        INTERFACE
        SolarScape::GeneticCore
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticSearch
        SolarScape::GeneticAlgorithm
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
