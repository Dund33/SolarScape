cmake_minimum_required(VERSION 3.10)

project(SolarScape VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SOLARSCAPE_ENABLE_NATIVE_OPTIMIZATIONS "Build release binaries with -march=native" ON)

set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if(SOLARSCAPE_ENABLE_NATIVE_OPTIMIZATIONS)
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

find_package(yaml-cpp CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options json)

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
        config/CommandLineOptions.cpp
        config/CommandLineOptions.h
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
        Boost::program_options
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
        genetics/GeneticAlgorithm.cpp
        genetics/GeneticAlgorithm.h
        genetics/ParetoFrontJsonWriter.cpp
        genetics/ParetoFrontJsonWriter.h
        genetics/Specimen.cpp
        genetics/Specimen.h
        genetics/comparison/FitnessObjectiveComparator.cpp
        genetics/comparison/FitnessObjectiveComparator.h
        genetics/comparison/NSGAIIComparator.cpp
        genetics/comparison/NSGAIIComparator.h
        genetics/comparison/NSGAIIRankingComparator.cpp
        genetics/comparison/NSGAIIRankingComparator.h
        genetics/comparison/SpecimenComparator.h
        genetics/comparison/SpecimenRank.h
        genetics/comparison/SimpleSpecimenComparator.cpp
        genetics/comparison/SimpleSpecimenComparator.h
        genetics/comparison/TrajectorySpecimenComparator.cpp
        genetics/comparison/TrajectorySpecimenComparator.h
        genetics/utils/ParetoFrontUtils.cpp
        genetics/utils/ParetoFrontUtils.h
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
        Boost::json
)

if(LINUX)
    find_package(TBB REQUIRED)

    target_link_libraries(SolarScapeGeneticCore
            PUBLIC
            TBB::tbb
    )
endif()

add_library(SolarScapeGeneticOperators STATIC
        genetics/selection/SelectionFactory.h
        genetics/selection/Selection.h
        genetics/selection/TournamentSelection.cpp
        genetics/selection/TournamentSelection.h
        genetics/selection/TournamentSelectionFactory.cpp
        genetics/selection/TournamentSelectionFactory.h
        genetics/mutation/MutationFactory.h
        genetics/mutation/Mutation.h
        genetics/mutation/ExtensiveMutation.cpp
        genetics/mutation/ExtensiveMutation.h
        genetics/mutation/ExtensiveMutationFactory.cpp
        genetics/mutation/ExtensiveMutationFactory.h
        genetics/mutation/ManeuverMutationUtils.h
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
        genetics/crossing/AlignedSimilarityCrossover.cpp
        genetics/crossing/AlignedSimilarityCrossover.h
        genetics/crossing/AlignedSimilarityCrossoverFactory.cpp
        genetics/crossing/AlignedSimilarityCrossoverFactory.h
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

add_library(SolarScapeGeneticAlgo STATIC
        genetics/algo/Algo.cpp
        genetics/algo/Algo.h
)

add_library(SolarScape::GeneticAlgo ALIAS SolarScapeGeneticAlgo)

target_include_directories(SolarScapeGeneticAlgo
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticAlgo
        PUBLIC
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
)

add_library(SolarScapeGeneticNSGAII STATIC
        genetics/nsga/NSGAIIAlgorithm.cpp
        genetics/nsga/NSGAIIAlgorithm.h
)

add_library(SolarScape::GeneticNSGAII ALIAS SolarScapeGeneticNSGAII)

target_include_directories(SolarScapeGeneticNSGAII
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticNSGAII
        PUBLIC
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
)

add_library(SolarScapeGeneticMOEAD STATIC
        genetics/moead/MOEADAlgorithm.cpp
        genetics/moead/MOEADAlgorithm.h
)

add_library(SolarScape::GeneticMOEAD ALIAS SolarScapeGeneticMOEAD)

target_include_directories(SolarScapeGeneticMOEAD
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SolarScapeGeneticMOEAD
        PUBLIC
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
)

add_library(SolarScapeGenetic INTERFACE)

add_library(SolarScape::Genetic ALIAS SolarScapeGenetic)

target_link_libraries(SolarScapeGenetic
        INTERFACE
        SolarScape::GeneticCore
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticAlgo
)

add_custom_target(copy_yaml_files
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/scenario1.yml"
                "${CMAKE_CURRENT_SOURCE_DIR}/scenario2.yml"
                "${CMAKE_CURRENT_SOURCE_DIR}/scenario3.yml"
                "${CMAKE_CURRENT_SOURCE_DIR}/validation-basic.yaml"
                "${CMAKE_CURRENT_SOURCE_DIR}/validation-outer-body.yaml"
                "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Copying YAML configuration files"
        VERBATIM
)


add_executable(SolarScape
        algo_simulation.cpp
)

add_dependencies(SolarScape
        copy_yaml_files
)

target_link_libraries(SolarScape
        PRIVATE
        SolarScape::Config
        SolarScape::Visual
        SolarScape::Genetic
        SolarScape::Simulation
)

add_executable(SolarScapeNSGAII
        nsga_simulation.cpp
)

add_dependencies(SolarScapeNSGAII
        copy_yaml_files
)

target_link_libraries(SolarScapeNSGAII
        PRIVATE
        SolarScape::Config
        SolarScape::GeneticCore
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticNSGAII
        SolarScape::Simulation
)

add_executable(SolarScapeMOEAD
        moead_simulation.cpp
)

add_dependencies(SolarScapeMOEAD
        copy_yaml_files
)

target_link_libraries(SolarScapeMOEAD
        PRIVATE
        SolarScape::Config
        SolarScape::GeneticCore
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticMOEAD
        SolarScape::Simulation
)

add_executable(NSGAIISmokeTest
        tests/nsga_smoke_test.cpp
)

target_link_libraries(NSGAIISmokeTest
        PRIVATE
        SolarScape::GeneticCore
        SolarScape::GeneticOperators
        SolarScape::GeneticFitness
        SolarScape::GeneticNSGAII
)

add_executable(validation
        validation.cpp
        validation/RecordingValidator.cpp
        validation/RecordingValidator.h
)

add_dependencies(validation
        copy_yaml_files
)

target_link_libraries(validation
        PRIVATE
        SolarScape::Config
        SolarScape::Simulation
)
