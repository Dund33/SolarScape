cmake_minimum_required(VERSION 4.2)
project(SolarScape)

set(CMAKE_CXX_STANDARD 14)

include_directories(data)

add_executable(SolarScape
        data/Vector3.cpp
        data/Vector3.h
        data/Body.cpp
        data/Body.h
        computation/DistanceAnalysis.h
        computation/Verlet.h
        simulation.cpp)
