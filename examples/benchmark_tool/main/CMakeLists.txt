set(srcs app_main.cpp)

set(include_dirs .)

set(requires esp-dl)

set(FASTESTDET_MODEL_DIR "fastestdetnext_x3_00_x1_00_64x64_opencv_inter_nearest_cls09_espdl"
    CACHE STRING "Deprecated: use BENCH_MODEL_FAMILY/BENCH_MODEL_DIR; FastestDetNext dir under models/fastestdetnext")
set(BENCH_MODEL_FAMILY "fastestdetnext" CACHE STRING "Model family under models/")
set(BENCH_MODEL_DIR "" CACHE STRING "Model directory under models/<family> (or models/<family>/<dir> when prefixed)")

if(BENCH_MODEL_DIR STREQUAL "")
    set(BENCH_MODEL_DIR ${FASTESTDET_MODEL_DIR})
endif()

if(BENCH_MODEL_DIR MATCHES "/")
    string(REGEX REPLACE "^models/" "" model_rel_dir ${BENCH_MODEL_DIR})
    set(model_candidate ${CMAKE_CURRENT_LIST_DIR}/../../../models/${model_rel_dir})
else()
    set(model_candidate ${CMAKE_CURRENT_LIST_DIR}/../../../models/${BENCH_MODEL_FAMILY}/${BENCH_MODEL_DIR})
endif()

set(model_dir ${model_candidate})
if(NOT EXISTS ${model_dir})
    set(model_dir "${model_candidate}_espdl")
endif()

get_filename_component(model_name ${model_dir} NAME)
if(model_name MATCHES "_espdl$")
    string(REGEX REPLACE "_espdl$" "" model_name ${model_name})
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
