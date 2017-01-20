cmake_minimum_required(VERSION 2.8.11)

project(slumber-ui)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_executable(slumber-ui ${SOURCE_FILES})

add_subdirectory(MACE)

target_include_directories(slumber-ui PUBLIC "include/")

target_include_directories(slumber-ui PUBLIC ${MACE_INCLUDES})
target_link_libraries(slumber-ui LINK_PUBLIC MACE)
