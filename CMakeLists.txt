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

add_subdirectory(crobots++/lib/SDL)
add_library(api crobots++/api/src/crobots++.cpp)
target_include_directories(api PUBLIC crobots++/api/include)
file(GLOB ROBOTS robots/*.cpp)
foreach(PATH ${ROBOTS})
    get_filename_component(NAME ${PATH} NAME_WE)
    add_library(${NAME} MODULE ${PATH})
    target_link_libraries(${NAME} PRIVATE api)
endforeach()
add_executable(engine WIN32
    crobots++/engine/main.cpp
    crobots++/engine/renderer.cpp
)
target_link_libraries(engine PRIVATE SDL3::SDL3 api)
set_target_properties(engine PROPERTIES OUTPUT_NAME crobots++)