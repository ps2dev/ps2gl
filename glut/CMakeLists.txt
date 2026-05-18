cmake_minimum_required(VERSION 3.13)

# GLUT library for ps2gl
project(ps2glut VERSION 1.0.0 LANGUAGES CXX C)

# Set output library name
set(EE_LIB "libps2glut.a")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PS2SDK}/ports/include
)

# Link directories
link_directories(
    ${PS2SDK}/ports/lib
)

# Compiler flags
if(DEBUG)
    add_compile_definitions(_DEBUG)
endif()

# Warning flags (matching Makefile)
set(WARNING_FLAGS
    -Wno-strict-aliasing
    -Wno-conversion-null
)

# VU0 code is broken so disable for now
add_compile_definitions(
    NO_VU0_VECTORS
    NO_ASM
)

add_compile_options(${WARNING_FLAGS})

# ============================================================================
# Source files
# ============================================================================
set(PS2GLUT_SOURCES
    src/glut_font_image.cpp
    src/pads.cpp
    src/ps2glut.cpp
)

# ============================================================================
# Build the library
# ============================================================================
add_library(ps2glut STATIC ${PS2GLUT_SOURCES})

set_target_properties(ps2glut PROPERTIES
    OUTPUT_NAME "ps2glut"
    ARCHIVE_OUTPUT_NAME "ps2glut"
)

target_include_directories(ps2glut PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Install targets
# ============================================================================
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${EE_LIB}
    DESTINATION "${PS2SDK}/ports/lib"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/GL
    DESTINATION "${PS2SDK}/ports/include"
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# Print configuration summary
# ============================================================================
message(STATUS "")
message(STATUS "ps2glut configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Debug build: ${DEBUG}")
message(STATUS "  Output library: ${EE_LIB}")
message(STATUS "  Install prefix: ${PS2SDK}/ports")
message(STATUS "")
