cmake_minimum_required(VERSION 3.10)
project(mpc_ot_private_index)

set(CMAKE_CXX_STANDARD 20)

# Include directories
include_directories(
    src/common
    src/io
    src/mpc
    src/net
    src/oblivious_index
    src/ot
)

# Source files for the applications
set(APP_SOURCES
    src/apps/gen_data.cpp
    src/apps/p0.cpp
    src/apps/p1.cpp
    src/apps/p2.cpp
    src/apps/verify.cpp
)

# Source files for the oblivious transfer
set(OT_SOURCES
    src/ot/base_ot.cpp
    src/ot/kkrt.cpp
    src/ot/ot_extension.cpp
)

# Source files for oblivious index selection
set(OBLIVIOUS_INDEX_SOURCES
    src/oblivious_index/private_index_select.cpp
)

# Source files for the MPC protocol
set(MPC_SOURCES
    src/mpc/beaver.hpp
    src/mpc/mpc.hpp
)

# Create the executable for each application
foreach(APP_SOURCE ${APP_SOURCES})
    get_filename_component(APP_NAME ${APP_SOURCE} NAME_WE)
    add_executable(${APP_NAME} ${APP_SOURCE})
endforeach()

# Create the library for oblivious transfer
add_library(ot_lib ${OT_SOURCES})

# Create the library for oblivious index selection
add_library(oblivious_index_lib ${OBLIVIOUS_INDEX_SOURCES})

# Link the libraries to the applications
foreach(APP_SOURCE ${APP_SOURCES})
    get_filename_component(APP_NAME ${APP_SOURCE} NAME_WE)
    target_link_libraries(${APP_NAME} ot_lib oblivious_index_lib)
endforeach()