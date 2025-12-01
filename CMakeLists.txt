cmake_minimum_required(VERSION 3.13)

# Set the toolchain file before project()
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{PS2DEV})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{PS2DEV}/share/ps2dev.cmake" CACHE FILEPATH "Toolchain file")
    else()
        message(FATAL_ERROR "PS2DEV environment variable is not set. Please set it to your PS2DEV installation path.")
    endif()
endif()

project(ps2gl VERSION 1.0.0 LANGUAGES CXX C)

# Options
option(BUILD_TESTS "Build test projects" OFF)
option(BUILD_GLUT "Build GLUT library" ON)
option(BUILD_EXAMPLES "Build example programs" OFF)
option(DEBUG "Enable debug build" OFF)

# Check if PS2SDK is set (should be done by toolchain file)
if(NOT DEFINED PS2SDK)
    message(FATAL_ERROR "PS2SDK is not defined. Make sure the toolchain file is loaded correctly.")
endif()

# Set output library name
set(EE_LIB "libps2gl.a")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vu1
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
set(PS2GL_SOURCES
    src/base_renderer.cpp
    src/clear.cpp
    src/displaycontext.cpp
    src/dlgmanager.cpp
    src/dlist.cpp
    src/drawcontext.cpp
    src/gblock.cpp
    src/glcontext.cpp
    src/gmanager.cpp
    src/gsmemory.cpp
    src/immgmanager.cpp
    src/indexed_renderer.cpp
    src/inverse.cpp
    src/lighting.cpp
    src/linear_renderer.cpp
    src/material.cpp
    src/matrix.cpp
    src/metrics.cpp
    src/renderermanager.cpp
    src/texture.cpp
)

# ============================================================================
# VU1 Renderers
# ============================================================================
set(RENDERERS
    fast_nolights
    fast
    general_nospec_quad
    general_nospec_tri
    general_nospec
    general_pv_diff_quad
    general_pv_diff_tri
    general_pv_diff
    general_quad
    general_tri
    general
    indexed
    scei
)

# Check for VU1 tools availability
# Prefer open-source tools (openvcl + masp) over proprietary ones (vcl + gasp)
find_program(OPENVCL_FOUND openvcl)
find_program(MASP_FOUND masp)
find_program(VCL_FOUND vcl)
find_program(GASP_FOUND gasp)
find_program(DVP_AS_FOUND dvp-as)

# Determine which tools to use
if(OPENVCL_FOUND AND MASP_FOUND AND DVP_AS_FOUND)
    set(VU1_TOOLS_AVAILABLE TRUE)
    set(VCL_TOOL ${OPENVCL_FOUND})
    set(GASP_TOOL ${MASP_FOUND})
    message(STATUS "VU1 tools found - using open-source tools (openvcl + masp)")
elseif(VCL_FOUND AND GASP_FOUND AND DVP_AS_FOUND)
    set(VU1_TOOLS_AVAILABLE TRUE)
    set(VCL_TOOL ${VCL_FOUND})
    set(GASP_TOOL ${GASP_FOUND})
    message(STATUS "VU1 tools found - using proprietary tools (vcl + gasp)")
else()
    set(VU1_TOOLS_AVAILABLE FALSE)
    message(STATUS "VU1 tools not found - will use pre-built VU1 objects from source tree")
endif()

# VU1 preprocessing rules
# These implement the complex preprocessing pipeline from the Makefile:
# .vcl -> _pp1.vcl -> _pp2.vcl -> _pp3.vcl -> _pp4.vcl -> _vcl.vsm -> .vo

set(VU1_OBJECTS)
foreach(RENDERER ${RENDERERS})
    set(VCL_SOURCE "${CMAKE_SOURCE_DIR}/vu1/${RENDERER}.vcl")
    set(VO_SOURCE "${CMAKE_SOURCE_DIR}/vu1/${RENDERER}.vo")
    set(PP1_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}_pp1.vcl")
    set(PP2_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}_pp2.vcl")
    set(PP3_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}_pp3.vcl")
    set(PP4_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}_pp4.vcl")
    set(VSM_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}_vcl.vsm")
    set(VO_FILE "${CMAKE_BINARY_DIR}/vu1/${RENDERER}.vo")

    if(NOT VU1_TOOLS_AVAILABLE)
        # Use pre-built .vo file from source tree
        if(EXISTS ${VO_SOURCE})
            message(STATUS "Using pre-built VU1 object: ${RENDERER}.vo")
            add_custom_command(
                OUTPUT ${VO_FILE}
                COMMAND ${CMAKE_COMMAND} -E copy ${VO_SOURCE} ${VO_FILE}
                DEPENDS ${VO_SOURCE}
                COMMENT "Copying pre-built VU1 object: ${RENDERER}.vo"
            )
            list(APPEND VU1_OBJECTS ${VO_FILE})
        else()
            message(WARNING "Pre-built VU1 object not found: ${VO_SOURCE}")
        endif()
        continue()
    endif()

    # Check if this renderer is known to be incompatible with openvcl
    # indexed renderer uses variable names that openvcl doesn't recognize
    # Temporarily disabled to try compiling with openvcl
    # if(OPENVCL_FOUND AND (RENDERER STREQUAL "indexed" OR RENDERER STREQUAL "scei"))
    #     if(EXISTS ${VO_SOURCE})
    #         message(STATUS "Using pre-built VU1 object for ${RENDERER}.vo (openvcl incompatible)")
    #         add_custom_command(
    #             OUTPUT ${VO_FILE}
    #             COMMAND ${CMAKE_COMMAND} -E copy ${VO_SOURCE} ${VO_FILE}
    #             DEPENDS ${VO_SOURCE}
    #             COMMENT "Copying pre-built VU1 object: ${RENDERER}.vo"
    #         )
    #         list(APPEND VU1_OBJECTS ${VO_FILE})
    #     else()
    #         message(WARNING "Pre-built VU1 object not found: ${VO_SOURCE}")
    #     endif()
    #     continue()
    # endif()

    # Step 1: .vcl -> _pp1.vcl (remove #include, #define, fix .include paths)
    add_custom_command(
        OUTPUT ${PP1_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/vu1
        COMMAND ${CMAKE_COMMAND} -D INPUT=${VCL_SOURCE} -D OUTPUT=${PP1_FILE} -D STEP=pp1 -D SOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        DEPENDS ${VCL_SOURCE} ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        COMMENT "Preprocessing step 1: ${RENDERER}.vcl -> ${RENDERER}_pp1.vcl"
    )

    # Step 2: _pp1.vcl -> _pp2.vcl (gasp/masp preprocessor)
    add_custom_command(
        OUTPUT ${PP2_FILE}
        COMMAND ${CMAKE_COMMAND} -D INPUT=${PP1_FILE} -D OUTPUT=${PP2_FILE} -D STEP=pp2 -D SOURCE_DIR=${CMAKE_SOURCE_DIR} -D GASP_TOOL=${GASP_TOOL} -P ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        DEPENDS ${PP1_FILE} ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        COMMENT "Preprocessing step 2: ${RENDERER}_pp1.vcl -> ${RENDERER}_pp2.vcl (${GASP_TOOL})"
    )

    # Step 3: _pp2.vcl -> _pp3.vcl (array notation conversion)
    add_custom_command(
        OUTPUT ${PP3_FILE}
        COMMAND ${CMAKE_COMMAND} -D INPUT=${PP2_FILE} -D OUTPUT=${PP3_FILE} -D STEP=pp3 -P ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        DEPENDS ${PP2_FILE} ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
        COMMENT "Preprocessing step 3: ${RENDERER}_pp2.vcl -> ${RENDERER}_pp3.vcl (array notation)"
    )

    # Step 4: _pp3.vcl -> _pp4.vcl (C preprocessor with memory layout)
    # Different preprocessing for indexed renderer
    if(RENDERER STREQUAL "indexed")
        add_custom_command(
            OUTPUT ${PP4_FILE}
            COMMAND ${CMAKE_COMMAND} -D INPUT=${PP3_FILE} -D OUTPUT=${PP4_FILE} -D STEP=pp4 -D SOURCE_DIR=${CMAKE_SOURCE_DIR} -D COMPILER=${CMAKE_C_COMPILER} -D MEM_HEADER=${CMAKE_SOURCE_DIR}/vu1/vu1_mem_indexed.h -P ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
            DEPENDS ${PP3_FILE} ${CMAKE_SOURCE_DIR}/vu1/vu1_mem_indexed.h ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
            COMMENT "Preprocessing step 4: ${RENDERER}_pp3.vcl -> ${RENDERER}_pp4.vcl (indexed memory layout)"
        )
    else()
        add_custom_command(
            OUTPUT ${PP4_FILE}
            COMMAND ${CMAKE_COMMAND} -D INPUT=${PP3_FILE} -D OUTPUT=${PP4_FILE} -D STEP=pp4 -D SOURCE_DIR=${CMAKE_SOURCE_DIR} -D COMPILER=${CMAKE_C_COMPILER} -D MEM_HEADER=${CMAKE_SOURCE_DIR}/vu1/vu1_mem_linear.h -P ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
            DEPENDS ${PP3_FILE} ${CMAKE_SOURCE_DIR}/vu1/vu1_mem_linear.h ${CMAKE_SOURCE_DIR}/cmake/preprocess_vu1.cmake
            COMMENT "Preprocessing step 4: ${RENDERER}_pp3.vcl -> ${RENDERER}_pp4.vcl (linear memory layout)"
        )
    endif()

    # Step 5: _pp4.vcl -> _vcl.vsm (vcl/openvcl compiler)
    add_custom_command(
        OUTPUT ${VSM_FILE}
        COMMAND ${VCL_TOOL} -o${VSM_FILE} ${PP4_FILE}
        DEPENDS ${PP4_FILE}
        COMMENT "VCL compilation: ${RENDERER}_pp4.vcl -> ${RENDERER}_vcl.vsm (${VCL_TOOL})"
    )

    # Step 6: _vcl.vsm -> .vo (dvp assembler)
    add_custom_command(
        OUTPUT ${VO_FILE}
        COMMAND dvp-as -o ${VO_FILE} ${VSM_FILE}
        DEPENDS ${VSM_FILE}
        COMMENT "DVP assembly: ${RENDERER}_vcl.vsm -> ${RENDERER}.vo"
    )

    # Mark the .vo file as an external object file so CMake includes it in the library
    set_source_files_properties(${VO_FILE} PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED TRUE
    )

    list(APPEND VU1_OBJECTS ${VO_FILE})
endforeach()

# Create a custom target that depends on all VU1 objects
# This ensures all .vo files are built before they're added to the library
if(VU1_OBJECTS)
    add_custom_target(vu1_objects ALL DEPENDS ${VU1_OBJECTS})
endif()

# ============================================================================
# Build the library
# ============================================================================
add_library(ps2gl STATIC ${PS2GL_SOURCES})

set_target_properties(ps2gl PROPERTIES
    OUTPUT_NAME "ps2gl"
    ARCHIVE_OUTPUT_NAME "ps2gl"
)

target_include_directories(ps2gl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Add VU1 objects to the library after it's created
# CMake doesn't handle .vo files well, so we manually add them with ar
if(VU1_OBJECTS)
    # Make ps2gl depend on the vu1_objects target
    add_dependencies(ps2gl vu1_objects)

    add_custom_command(TARGET ps2gl POST_BUILD
        COMMAND ${CMAKE_AR} qc $<TARGET_FILE:ps2gl> ${VU1_OBJECTS}
        COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:ps2gl>
        COMMENT "Adding VU1 objects to library"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )
endif()

# ============================================================================
# Install targets
# ============================================================================
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${EE_LIB}
    DESTINATION "${PS2SDK}/ports/lib"
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/GL
    DESTINATION "${PS2SDK}/ports/include"
    FILES_MATCHING PATTERN "*.h"
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/ps2gl
    DESTINATION "${PS2SDK}/ports/include"
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# Include GLUT if enabled
# ============================================================================
if(BUILD_GLUT)
    add_subdirectory(glut)
endif()

# ============================================================================
# Include examples if enabled
# ============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Include tests if enabled
# ============================================================================
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ============================================================================
# Print configuration summary
# ============================================================================
message(STATUS "")
message(STATUS "ps2gl configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Debug build: ${DEBUG}")
message(STATUS "  Build GLUT: ${BUILD_GLUT}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Output library: ${EE_LIB}")
message(STATUS "  Install prefix: ${PS2SDK}/ports")
message(STATUS "  VU0 vectors: DISABLED")
message(STATUS "  ASM optimizations: DISABLED")
message(STATUS "  VU1 renderers: ${RENDERERS}")
message(STATUS "")
