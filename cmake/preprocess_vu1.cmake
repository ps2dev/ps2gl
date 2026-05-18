# VU1 preprocessing script - Simplified version
# Usage: cmake -D INPUT=<input> -D OUTPUT=<output> -D STEP=<step> -D SOURCE_DIR=<dir> -D COMPILER=<cc> -D MEM_HEADER=<header> -D GASP_TOOL=<masp> -P preprocess_vu1.cmake

if(STEP STREQUAL "pp1")
    # Step 1: Clean up C preprocessor directives and fix .include paths
    # - Remove #include and #define (will use gasp-style includes and C preprocessor later)
    # - Fix .include paths to be absolute
    file(READ "${INPUT}" content)
    # Remove #include lines
    string(REGEX REPLACE "#include[^\n]*\n" "" content "${content}")
    # Remove #define lines
    string(REGEX REPLACE "#define[^\n]*\n" "" content "${content}")
    # Fix .include paths to be absolute (only for relative paths)
    # Note: CMake regex doesn't support [[:space:]], use [ \t] instead
    string(REGEX REPLACE "\\.include[ \t]+\"([^/][^\"]*)\"" ".include \"${SOURCE_DIR}/vu1/\\1\"" content "${content}")
    file(WRITE "${OUTPUT}" "${content}")

elseif(STEP STREQUAL "pp2")
    # Step 2: gasp/masp preprocessor for macro expansion
    if(NOT DEFINED GASP_TOOL)
        message(FATAL_ERROR "GASP_TOOL not defined")
    endif()

    # Run masp directly
    execute_process(
        COMMAND "${GASP_TOOL}" -c ";" -I"${SOURCE_DIR}/vu1" -o "${OUTPUT}" "${INPUT}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "masp failed (exit ${result})\nCommand: ${GASP_TOOL} -c \";\" -I${SOURCE_DIR}/vu1 -o ${OUTPUT} ${INPUT}\nOutput: ${output}\nError: ${error}")
    endif()
    if(NOT EXISTS "${OUTPUT}")
        message(FATAL_ERROR "masp did not create output file: ${OUTPUT}")
    endif()

elseif(STEP STREQUAL "pp3")
    # Step 3: Array notation conversion
    # Convert [0] -> _0, [1] -> _1, etc.
    # Convert [x] -> x, [y] -> y, etc. (vector component access)
    file(READ "${INPUT}" content)
    string(REGEX REPLACE "\\[([0-9])\\]" "_\\1" content "${content}")
    string(REGEX REPLACE "\\[([w-zW-Z])\\]" "\\1" content "${content}")
    file(WRITE "${OUTPUT}" "${content}")

elseif(STEP STREQUAL "pp4")
    # Step 4: C preprocessor for memory layout evaluation
    if(NOT DEFINED COMPILER)
        message(FATAL_ERROR "COMPILER not defined")
    endif()
    if(NOT DEFINED MEM_HEADER)
        message(FATAL_ERROR "MEM_HEADER not defined")
    endif()

    # Use -x assembler-with-cpp to force GCC to preprocess .vcl files as assembly
    execute_process(
        COMMAND "${COMPILER}" -E -P -w -x assembler-with-cpp -I"${SOURCE_DIR}/vu1" -imacros "${MEM_HEADER}" "${INPUT}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "C preprocessor failed (exit ${result})\nError: ${error}")
    endif()
    file(WRITE "${OUTPUT}" "${output}")

else()
    message(FATAL_ERROR "Unknown step: ${STEP}")
endif()
