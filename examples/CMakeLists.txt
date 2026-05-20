# Examples for ps2gl

# Shared code library used by multiple examples
add_library(shared_code STATIC
    shared_code/text_stuff.cpp
)

target_include_directories(shared_code PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/shared_code
    ${CMAKE_SOURCE_DIR}/glut/include
    ${CMAKE_SOURCE_DIR}/include
    ${PS2SDK}/ports/include
)

target_compile_options(shared_code PRIVATE
    -Wno-strict-aliasing
    -Wno-conversion-null
)

target_compile_definitions(shared_code PRIVATE
    NO_VU0_VECTORS
    NO_ASM
)

# Common function to add examples
function(add_ps2gl_example EXAMPLE_NAME SOURCE_FILE)
    add_executable(${EXAMPLE_NAME} ${SOURCE_FILE})

    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/glut/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/shared_code
        ${PS2SDK}/ports/include
    )

    target_compile_options(${EXAMPLE_NAME} PRIVATE
        -Wno-strict-aliasing
        -Wno-conversion-null
    )

    target_compile_definitions(${EXAMPLE_NAME} PRIVATE
        NO_VU0_VECTORS
        NO_ASM
    )

    target_link_directories(${EXAMPLE_NAME} PRIVATE
        ${PS2SDK}/ports/lib
    )

    target_link_libraries(${EXAMPLE_NAME}
        shared_code
        ps2glut
        ps2gl
        ps2stuff
        pad
        dma
    )

    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        OUTPUT_NAME "${EXAMPLE_NAME}.elf"
        SUFFIX ""
    )

    # Strip the executable
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${EXAMPLE_NAME}>
        COMMENT "Stripping ${EXAMPLE_NAME}.elf"
    )

    # Copy data files to build directory
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
        file(GLOB DATA_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.gl"
            "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.rtx"
            "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/*.bin"
        )
        foreach(DATA_FILE ${DATA_FILES})
            get_filename_component(FILENAME ${DATA_FILE} NAME)
            add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DATA_FILE}
                    ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}
                COMMENT "Copying ${FILENAME}"
            )
        endforeach()
    endif()
endfunction()

# Add examples
add_ps2gl_example(box box/box.cpp)
add_ps2gl_example(logo logo/logo.cpp)
add_ps2gl_example(performance performance/performance.cpp)

# tricked_out example requires special handling due to billboard_renderer VU1 code
# First, assemble the billboard VSM to a .vo file
set(BILLBOARD_VSM "${CMAKE_CURRENT_SOURCE_DIR}/tricked_out/billboard_renderer_vcl.vsm")
set(BILLBOARD_VO "${CMAKE_CURRENT_BINARY_DIR}/tricked_out/billboard_renderer.vo")

# Create directory for output
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tricked_out")

# Assemble the VSM file to .vo (DVP assembler should be available)
add_custom_command(
    OUTPUT ${BILLBOARD_VO}
    COMMAND dvp-as -o ${BILLBOARD_VO} ${BILLBOARD_VSM}
    DEPENDS ${BILLBOARD_VSM}
    COMMENT "Assembling billboard_renderer VU1 code"
)

# Create a custom target for the billboard VU1 object
add_custom_target(billboard_vu1_object DEPENDS ${BILLBOARD_VO})

# Add tricked_out example with billboard_renderer sources
add_executable(tricked_out
    tricked_out/tricked_out.cpp
    tricked_out/billboard_renderer.cpp
)

target_include_directories(tricked_out PRIVATE
    ${CMAKE_SOURCE_DIR}/glut/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src       # For ps2gl internal headers
    ${CMAKE_CURRENT_SOURCE_DIR}/shared_code
    ${CMAKE_CURRENT_SOURCE_DIR}/tricked_out
    ${PS2SDK}/ports/include
)

target_compile_options(tricked_out PRIVATE
    -Wno-strict-aliasing
    -Wno-conversion-null
)

target_compile_definitions(tricked_out PRIVATE
    NO_VU0_VECTORS
    NO_ASM
)

target_link_directories(tricked_out PRIVATE
    ${PS2SDK}/ports/lib
)

# Link the billboard VU1 object file
target_link_libraries(tricked_out
    ${BILLBOARD_VO}
    shared_code
    ps2glut
    ps2gl
    ps2stuff
    pad
    dma
)

add_dependencies(tricked_out billboard_vu1_object)

set_target_properties(tricked_out PROPERTIES
    OUTPUT_NAME "tricked_out.elf"
    SUFFIX ""
)

# Strip the executable
add_custom_command(TARGET tricked_out POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:tricked_out>
    COMMENT "Stripping tricked_out.elf"
)

# Copy data files (car.bin texture)
add_custom_command(TARGET tricked_out POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/tricked_out/car.bin
        ${CMAKE_CURRENT_BINARY_DIR}/car.bin
    COMMENT "Copying car.bin texture"
)

# NeHe tutorials
add_ps2gl_example(nehe_lesson01 nehe/lesson01/lesson1.cpp)
add_ps2gl_example(nehe_lesson02 nehe/lesson02/lesson2.cpp)
add_ps2gl_example(nehe_lesson03 nehe/lesson03/lesson3.cpp)
add_ps2gl_example(nehe_lesson04 nehe/lesson04/lesson4.cpp)
add_ps2gl_example(nehe_lesson05 nehe/lesson05/lesson5.cpp)

message(STATUS "")
message(STATUS "ps2gl examples configured:")
message(STATUS "  box, logo, performance, tricked_out")
message(STATUS "  nehe: lesson02, lesson03, lesson04, lesson05")
message(STATUS "")
