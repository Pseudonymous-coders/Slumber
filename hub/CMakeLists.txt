cmake_minimum_required(VERSION 2.8.11)
project(slumber)

enable_language(C)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "csrc/*.c")

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
		${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
	)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")

message(FATAL_ERROR "None linux distributions are currently not supported for slumber!")

endif()


if(NOT SLUMBER)
set(HOSTBASE "x86_64-linux-gnu")
	set(HOSTFLAG "HOSTMACHINE")
	set(TARGETBIN slumberhost)
	set(TYPELOCATION "/usr/local")
	set(HOSTBASELOC "/usr/lib/${HOSTBASE}/lib")
	set(EXTRALIBS "${HOSTBASELOC}crypto.so" "${HOSTBASELOC}ssl.so" "${HOSTBASELOC}bluetooth.so"
"${HOSTBASELOC}glib-2.0.so" "${HOSTBASELOC}pthread.so" "${HOSTBASELOC}glib-2.0.so")
	set(EXTRAINCLUDES "/usr/include/glib-2.0" "/usr/lib/${HOSTBASE}/glib-2.0/include" "${TYPELOCATION}/include/pstreams")
else()
	set(BASE "arm-linux-gnueabihf")
	set(CMAKE_AR "${BASE}-ar")
	set(CMAKE_C_COMPILER "${BASE}-gcc")
	set(CMAKE_CXX_COMPILER "${BASE}-g++")
	set(CMAKE_RANLIB "${BASE}-ranlib")
	set(CMAKE_AS "${BASE}-as")
	set(HOSTFLAG "SLUMBERMACHINE")
	set(TARGETBIN slumberhub)
	set(TYPELOCATION "/usr/${BASE}")
	set(EXTRALIBS "${TYPELOCATION}/lib/libcrypto.so" "${TYPELOCATION}/lib/libssl.so" "${TYPELOCATION}/lib/libpthread.so"
"${TYPELOCATION}/lib/libglib-2.0.so")
	set(EXTRAINCLUDES "")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D ${HOSTFLAG} -static-libgcc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D ${HOSTFLAG} -std=c++11 -ggdb -fpermissive -static-libstdc++")

# message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

# Linking options and locations
set(LIBRARY_PATH "${TYPELOCATION}/lib")
set(INCLUDE_LOCATION "${TYPELOCATION}/include")

set(BLOC "${TYPELOCATION}/lib/lib")
set(LLIBS "${BLOC}boost_atomic.so" "${BLOC}boost_chrono.so" "${BLOC}boost_thread.so" "${BLOC}boost_system.so" "${BLOC}boost_filesystem.so"
${EXTRALIBS} "${BLOC}cpprest.so" "${BLOC}gatt.so")

add_executable(${TARGETBIN} ${SOURCE_FILES})

add_subdirectory(MACE)

target_include_directories(${TARGETBIN} PUBLIC "${PROJECT_SOURCE_DIR}/include/" ${MACE_INCLUDES} ${EXTRAINCLUDES})
target_link_libraries(${TARGETBIN} LINK_PUBLIC MACE ${LLIBS})
