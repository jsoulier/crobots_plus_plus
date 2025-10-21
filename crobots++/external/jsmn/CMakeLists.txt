project(jsmn)
add_library(jsmn jsmn.c)
target_include_directories(jsmn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})