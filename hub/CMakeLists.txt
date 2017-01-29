cmake_minimum_required(VERSION 2.8.11)

project(slumber-)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_executable(slumber ${SOURCE_FILES})

add_subdirectory(MACE)

target_include_directories(slumber PUBLIC "${PROJECT_SOURCE_DIR}/include/")

target_include_directories(slumber PUBLIC ${MACE_INCLUDES})
target_link_libraries(slumber LINK_PUBLIC MACE)
