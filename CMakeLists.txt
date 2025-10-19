cmake_minimum_required(VERSION 3.24)
project(crobots++)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

set(GLM_BUILD_LIBRARY OFF)
add_subdirectory(crobots++/external/SDL)
add_subdirectory(crobots++/external/box2d)
add_subdirectory(crobots++/external/glm)
add_subdirectory(crobots++/external/imgui)
add_subdirectory(crobots++/external/jsmn)
add_library(api crobots++/api/src/crobots++.cpp)
set_target_properties(api PROPERTIES CXX_STANDARD 23)
target_include_directories(api PUBLIC crobots++/api/include)
file(GLOB ROBOTS robots/*.cpp)
foreach(PATH ${ROBOTS})
    get_filename_component(NAME ${PATH} NAME_WE)
    add_library(${NAME} MODULE ${PATH})
    target_link_libraries(${NAME} PRIVATE api)
endforeach()
add_executable(engine WIN32
    crobots++/engine/api.cpp
    crobots++/engine/camera.cpp
    crobots++/engine/engine.cpp
    crobots++/engine/log.cpp
    crobots++/engine/main.cpp
    crobots++/engine/renderer.cpp
    crobots++/engine/shader.cpp
)
set_target_properties(engine PROPERTIES CXX_STANDARD 23)
set_target_properties(engine PROPERTIES OUTPUT_NAME crobots++)
target_link_libraries(engine PRIVATE SDL3::SDL3 api box2d glm imgui jsmn)
target_precompile_headers(engine PRIVATE
    <cassert>
    <cstdint>
    <cstring>
    <filesystem>
    <format>
    <fstream>
    <iterator>
    <string>
    <string_view>
)

function(add_shader FILE)
    set(DEPENDS ${ARGN})
    set(HLSL ${CMAKE_SOURCE_DIR}/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.spv)
    set(DXIL ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.dxil)
    set(MSL ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.json)
    if(MSVC)
        set(SHADERCROSS crobots++/external/SDL_shadercross/msvc/shadercross.exe)
    endif()
    function(compile OUTPUT)
        add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${SHADERCROSS} ${HLSL} -s hlsl -o ${OUTPUT} -I src
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${HLSL} ${DEPENDS}
            COMMENT ${OUTPUT}
        )
        get_filename_component(NAME ${OUTPUT} NAME)
        string(REPLACE . _ NAME ${NAME})
        set(NAME compile_${NAME})
        add_custom_target(${NAME} DEPENDS ${OUTPUT})
        add_dependencies(crobots++ ${NAME})
    endfunction()
    if (MSVC)
        compile(${SPV})
        compile(${DXIL})
        compile(${MSL})
        compile(${JSON})
    endif()
    function(package OUTPUT)
        get_filename_component(NAME ${OUTPUT} NAME)
        set(BINARY ${BINARY_DIR}/${NAME})
        add_custom_command(
            OUTPUT ${BINARY}
            COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${BINARY}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${OUTPUT}
            COMMENT ${BINARY}
        )
        string(REPLACE . _ NAME ${NAME})
        set(NAME package_${NAME})
        add_custom_target(${NAME} DEPENDS ${BINARY})
        add_dependencies(crobots++ ${NAME})
    endfunction()
    if(APPLE)
        package(${MSL})
    else()
        package(${SPV})
    endif()
    package(${JSON})
endfunction()