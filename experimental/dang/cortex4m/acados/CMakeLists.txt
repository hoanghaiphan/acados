cmake_minimum_required(VERSION 2.8.11)
project(acados)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Werror -fPIC -D__SUPPRESSANYOUTPUT__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror -fPIC")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message("\n\n --- The CMake build type is ${CMAKE_BUILD_TYPE} ---\n\n")

if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -funroll-loops -fdiagnostics-show-option")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wno-unused-function -Wno-unused-parameter \
    -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-unused-function -Wno-unused-parameter -Wno-error=unused-variable")
endif()

if(CMAKE_BUILD_TYPE MATCHES Profile)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -funroll-loops -fprofile-use")
endif()

if(CMAKE_BUILD_TYPE MATCHES Test)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    enable_testing(true)
endif()

# set(VIRTUAL_ENV $ENV{VIRTUAL_ENV})
# if(VIRTUAL_ENV)
#     set(CMAKE_INSTALL_PREFIX ${VIRTUAL_ENV})
#     message(STATUS "Detected a python virtualenv in use, setting install prefix to ${VIRTUAL_ENV}")
# else(VIRTUAL_ENV)
#     set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/swig )
#     message(STATUS "Setting install prefix to ${CMAKE_INSTALL_PREFIX}")
# endif(VIRTUAL_ENV)

# Define include folders
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/include/")
include_directories("${PROJECT_SOURCE_DIR}/external/")
include_directories("${PROJECT_SOURCE_DIR}/external/qpOASES/include/")
if(CMAKE_BUILD_TYPE MATCHES Test)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/eigen/Eigen")
        message(FATAL_ERROR "The build type is ${CMAKE_BUILD_TYPE} (Test) but Eigen can not be found")
    endif()
    include_directories("${PROJECT_SOURCE_DIR}/external/eigen/")

    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/casadi-octave-v3.1.1")
        message(FATAL_ERROR "The build type is ${CMAKE_BUILD_TYPE} (Test) but casadi-octave-v3.1.1 can not be found")
    endif()
endif()

# Define source files
include(acados_sources)
add_definitions(-DUSE_ACADOS_TYPES)

## Process external libraries
#include(external/openblas)
#include(external/blasfeo)
#include(external/hpmpc)
#include(external/qpoases)
#include(external/qpdunes)
#include_directories("${PROJECT_SOURCE_DIR}/external/qpDUNES-dev/include/")
#
#if (EXISTS ${PROJECT_SOURCE_DIR}/external/OOQP)
#    add_definitions(-DOOQP)
#    include(external/ma27)
#    include(external/ooqp)
#endif ()

# Define acados library
add_library(acados STATIC ${ACADOS_SRC})
#add_dependencies(acados qpoases hpmpc blasfeo qpdunes)

#if (EXISTS ${PROJECT_SOURCE_DIR}/external/OOQP)
#    add_dependencies(ooqpbase ma27)
#    add_dependencies(acados ooqpbase)
#endif ()

#add_subdirectory(swig)

# Define examples
#add_executable(hpmpc_example ${HPMPC_EXAMPLE_SRC})
#target_link_libraries(hpmpc_example acados hpmpc blasfeo openblas m)

#add_executable(hpmpc_example_partial_tightening ${HPMPC_EXAMPLE_SRC})
#target_link_libraries(hpmpc_example_partial_tightening acados hpmpc blasfeo openblas m)

#add_executable(condensing_qpoases_example ${CONDENSING_QPOASES_EXAMPLE_SRC})
#target_link_libraries(condensing_qpoases_example acados qpoases hpmpc blasfeo m)

#add_executable(nmpc_example ${NMPC_EXAMPLE_SRC} ${CHEN_MODEL_SRC})
#target_link_libraries(nmpc_example acados qpoases blasfeo m)

add_executable(test_nmpc ${NMPC_EXAMPLE_SRC} ${CHEN_MODEL_SRC})
target_link_libraries(test_nmpc acados m)
#target_link_libraries(nmpc_example acados qpoases blasfeo m)

#add_executable(chain_example ${CHAIN_EXAMPLE_SRC})
#target_link_libraries(chain_example acados qpoases blasfeo openblas m)

#add_executable(pendulum_example ${PENDULUM_EXAMPLE_SRC})
#target_link_libraries(pendulum_example acados hpmpc blasfeo openblas m)

#add_executable(pendulum_example_partial_tightening ${PENDULUM_EXAMPLE_PT_SRC})
#target_link_libraries(pendulum_example_partial_tightening acados hpmpc blasfeo openblas m)

#add_executable(aircraft_example ${AIRCRAFT_EXAMPLE_SRC})
#target_link_libraries(aircraft_example acados hpmpc blasfeo openblas m)
