set(srcs app_main.cpp)

set(include_dirs .)

set(requires esp-dl)

set(FASTESTDET_MODEL_DIR "fastestdetnext_x3_00_x1_00_64x64_opencv_inter_nearest_cls09_espdl" CACHE STRING "FastestDetNext model directory under models/fastestdetnext")
if(FASTESTDET_MODEL_DIR MATCHES "_espdl$")
    set(model_dir ${CMAKE_CURRENT_LIST_DIR}/../../../models/fastestdetnext/${FASTESTDET_MODEL_DIR})
    string(REGEX REPLACE "_espdl$" "" model_name ${FASTESTDET_MODEL_DIR})
else()
    set(model_name ${FASTESTDET_MODEL_DIR})
    set(model_dir ${CMAKE_CURRENT_LIST_DIR}/../../../models/fastestdetnext/${FASTESTDET_MODEL_DIR}_espdl)
endif()
set(model_path ${model_dir}/${model_name}.espdl)
set(BENCH_WARMUP 1 CACHE STRING "Benchmark warmup iterations")
set(BENCH_ITERS 10 CACHE STRING "Benchmark measured iterations")

if(NOT EXISTS ${model_path})
    message(FATAL_ERROR "Model .espdl not found: ${model_path}")
endif()

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires} EMBED_FILES ${model_path})

set(model_symbol ${model_name}_espdl)
target_compile_definitions(${COMPONENT_LIB} PUBLIC MODEL_SYMBOL=${model_symbol})
target_compile_definitions(${COMPONENT_LIB} PUBLIC MODEL_INPUT_W=64 MODEL_INPUT_H=64 MODEL_INPUT_C=3 MODEL_INPUT_EXP=-7)
target_compile_definitions(${COMPONENT_LIB} PUBLIC BENCH_WARMUP=${BENCH_WARMUP} BENCH_ITERS=${BENCH_ITERS})
