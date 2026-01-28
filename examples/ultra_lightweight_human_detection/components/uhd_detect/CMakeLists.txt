set(srcs uhd_detect.cpp)

set(include_dirs .)

set(requires esp-dl)

set(UHD_MODEL_FAMILY "uhd" CACHE STRING "Model family under models/")
set(UHD_MODEL_DIR "ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost"
    CACHE STRING "Model directory under models/${UHD_MODEL_FAMILY}")

set(model_root ${CMAKE_CURRENT_LIST_DIR}/../../../..)
if(UHD_MODEL_DIR MATCHES "/")
    string(REGEX REPLACE "^models/" "" model_rel_dir ${UHD_MODEL_DIR})
    set(model_candidate ${model_root}/models/${model_rel_dir})
else()
    set(model_candidate ${model_root}/models/${UHD_MODEL_FAMILY}/${UHD_MODEL_DIR})
endif()

set(model_dir ${model_candidate})
if(NOT EXISTS ${model_dir})
    set(model_dir "${model_candidate}_espdl")
endif()

get_filename_component(model_name ${model_dir} NAME)
if(model_name MATCHES "_espdl$")
    string(REGEX REPLACE "_espdl$" "" model_name ${model_name})
endif()

set(model_file "${model_name}.espdl")
if(NOT EXISTS ${model_dir}/${model_file})
    if(EXISTS ${model_dir}/${model_name}_nocat.espdl)
        set(model_file "${model_name}_nocat.espdl")
        set(model_name "${model_name}_nocat")
    endif()
endif()

set(model_path ${model_dir}/${model_file})

if(NOT EXISTS ${model_path})
    message(FATAL_ERROR "Model .espdl not found: ${model_path}")
endif()

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires} EMBED_FILES ${model_path})

set(model_symbol ${model_name}_espdl)
target_compile_definitions(${COMPONENT_LIB} PUBLIC UHD_MODEL_SYMBOL=${model_symbol} UHD_MODEL_NAME=\"${model_name}\")
