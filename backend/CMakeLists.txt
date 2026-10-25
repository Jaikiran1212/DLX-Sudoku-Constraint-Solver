cmake_minimum_required(VERSION 3.22)
project(dlx_backend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(DLX_ENABLE_SANITIZERS "Build with AddressSanitizer and UBSan" OFF)
option(DLX_BUILD_TESTS "Build unit tests" ON)

find_package(Drogon CONFIG REQUIRED)

# Solver, validation, config and JSON mapping: no HTTP handling in here.
add_library(dlx_core STATIC
  src/solver/DlxSolver.cpp
  src/service/GridValidation.cpp
  src/service/SudokuService.cpp
  src/service/ResultMapper.cpp
  src/Config.cpp)
target_include_directories(dlx_core PUBLIC src)
target_link_libraries(dlx_core PUBLIC Drogon::Drogon)  # brings in jsoncpp
target_compile_definitions(dlx_core PUBLIC DLX_VERSION="${PROJECT_VERSION}")
target_compile_options(dlx_core PUBLIC -Wall -Wextra -Wpedantic)

add_executable(dlx_server
  src/main.cpp
  src/http/Middleware.cpp
  src/http/Responses.cpp
  src/controllers/SolverController.cpp)
target_link_libraries(dlx_server PRIVATE dlx_core)

if(DLX_ENABLE_SANITIZERS)
  foreach(t dlx_core dlx_server)
    target_compile_options(${t} PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${t} PUBLIC -fsanitize=address,undefined)
  endforeach()
endif()

if(DLX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
