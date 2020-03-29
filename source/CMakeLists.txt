project(landau-poisson-solver)
cmake_minimum_required(VERSION 2.8)

# Check for MPI toolchain.
find_package(MPI)
if (!MPI_FOUND)
    message(FATAL_ERROR "Could not find MPI library and compiler.")
endif ()
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

# Check compiler supports the C++11 standard (required for GRVY).
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY CXX_STANDARD 11)

# OpenMP check
find_package(OpenMP)
if (!OpenMP_FOUND)
    message(FATAL_ERROR "Could not detect OpenMP linkage.")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# Math library check.
# TODO: MKL support
find_package(BLAS)
if (!BLAS_FOUND)
    message(FATAL_ERROR "BLAS library not found.")
endif ()
set(BLAS_LINK "-lopenblas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_OPENBLAS")
set(CMAKE_LINK_LIBRARY_FLAG ${CMAKE_LINK_LIBRARY_FLAG} ${BLAS_LINKER_FLAGS}) # TODO: investigate

# GRVY Check
# TODO: simple version, should improve or call m4 macros
find_library(GRVY_LIB_FILE NAMES grvy HINTS "${GRVY_DIR}/lib")
get_filename_component(GRVY_LIB ${GRVY_LIB_FILE} PATH)
link_directories(${GRVY_LIB})
include_directories("${GRVY_LIB}/../include")
set(GRVY_LINK "-lgrvy -Wl,-rpath,${GRVY_LIB}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GRVY_CXX_FLAG}")

# FFTW library check
find_package(FFTW)
if (!FFTW_FOUND)
    message(FATAL_ERROR "FFTW library not found.")
endif ()
list(GET FFTW_LIBRARIES 0 FFTW_L0)
get_filename_component(FFTW_LIB ${FFTW_L0} PATH)
link_directories(${FFTW_LIB})
include_directories(${FFTW_INCLUDE_DIRS})
set(FFTW_LINK "-lfftw3_threads -lfftw3")

# Summary
message(Summary:)
message("CC ${CMAKE_C_COMPILER}")
message("CXX ${CMAKE_CXX_COMPILER}")
message("CXX_FLAGS ${CMAKE_CXX_FLAGS}")
message("LINK_FLAG ${CMAKE_LINK_LIBRARY_FLAG}")

# Define target
file(GLOB solver_SRC "*.h" "*.cpp")
add_executable(solver ${solver_SRC} ../config.h)
target_link_libraries(solver "${BLAS_LINK} ${GRVY_LINK} ${FFTW_LINK}")
set_property(TARGET solver PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET solver PROPERTY CXX_STANDARD 11)