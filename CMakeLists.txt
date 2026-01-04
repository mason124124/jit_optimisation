cmake_minimum_required(VERSION 4.1)
project(jit_optimisation)

set(CMAKE_CXX_STANDARD 14)

add_executable(jit_optimisation
        src/main/cpp/branch_elimination_benchmark.cpp
        src/main/cpp/branch_prediction_benchmark.cpp
        src/main/cpp/poloymorphism_benchmark.cpp
        src/main/cpp/polymorphism_crtp.cpp
        src/main/cpp/verify_version.cpp)
