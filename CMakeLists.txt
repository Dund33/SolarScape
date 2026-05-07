cmake_minimum_required(VERSION 4.2)
project(SolarScape)

set(CMAKE_CXX_STANDARD 14)

include_directories(data)

add_executable(SolarScape
        data/Body.cpp
        data/Body.h
        data/Verlet.h
        simulation.cpp)
