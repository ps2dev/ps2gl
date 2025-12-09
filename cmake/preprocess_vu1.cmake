# VU1 preprocessing script
# Usage: cmake -D INPUT=<input> -D OUTPUT=<output> -D STEP=<step> -D SOURCE_DIR=<dir> -D COMPILER=<cc> -D MEM_HEADER=<header> -P preprocess_vu1.cmake

if(STEP STREQUAL "pp1")
    # Step 1: Remove #include, #define, fix .include paths
    execute_process(
        COMMAND /bin/bash -c "cat ${INPUT} | sed -E 's/#include[[:space:]]+.+// ; s/#define[[:space:]]+.+// ; s|(\\.include[[:space:]]+)\\\"([^/].+)\\\"|\\1\\\"${SOURCE_DIR}/vu1/\\2\\\"|' > ${OUTPUT}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Step 1 preprocessing failed")
    endif()

elseif(STEP STREQUAL "pp2")
    # Step 2: gasp/masp preprocessor
    if(NOT DEFINED GASP_TOOL)
        message(FATAL_ERROR "GASP_TOOL not defined")
    endif()
    # Use wrapper script for better error handling
    execute_process(
        COMMAND ${SOURCE_DIR}/cmake/run_masp.sh ${GASP_TOOL} ${SOURCE_DIR}/vu1 ${OUTPUT} ${INPUT}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Step 2 preprocessing (${GASP_TOOL}) failed\nOutput: ${output}\nError: ${error}\nInput: ${INPUT}\nOutput: ${OUTPUT}")
    endif()

elseif(STEP STREQUAL "pp3")
    # Step 3: Array notation conversion
    execute_process(
        COMMAND /bin/bash -c "cat ${INPUT} | sed -E 's/\\[([0-9])\\]/_\\1/g ; s/\\[([w-zW-Z])\\]/\\1/g' > ${OUTPUT}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Step 3 preprocessing failed")
    endif()

elseif(STEP STREQUAL "pp4")
    # Step 4: C preprocessor with memory layout
    # Use -w to suppress warnings about unmatched quotes in assembly comments
    # Escape backslashes before preprocessing, then restore them after
    # This preserves masp/gasp local labels like \xformed_vert while allowing normal C preprocessing
    execute_process(
        COMMAND /bin/bash -c "sed 's/\\\\/\\\\\\\\/g' ${INPUT} | ${COMPILER} -E -P -w -I${SOURCE_DIR}/vu1 -imacros ${MEM_HEADER} - | sed 's/\\\\\\\\/\\\\/g' > ${OUTPUT}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Step 4 preprocessing failed")
    endif()

else()
    message(FATAL_ERROR "Unknown step: ${STEP}")
endif()
