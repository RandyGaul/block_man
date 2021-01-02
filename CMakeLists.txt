cmake_minimum_required(VERSION 3.0)
project(block_man)

# Must have at least C++14.
set(CMAKE_CXX_STANDARD 14)

# ----------------------------------------------------------------------------------------
# TO BE REMOVED BY RANDY LATER - SEE https://github.com/RandyGaul/cute_framework/issues/16
# Platform detection.
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(EMSCRIPTEN TRUE)
elseif(WIN32)
	set(WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
	if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		set(LINUX TRUE)
	else()
		message(FATAL_ERROR, "No supported platform detected.")
	endif()
elseif(APPLE)
	if(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*" OR CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*")
		set(MACOSX TRUE)
	else()
		message(FATAL_ERROR, "No supported platform detected.")
	endif()
else()
	message(FATAL_ERROR, "No supported platform detected.")
endif()

# Defines for cross-platform shader support.
if(WINDOWS)
	add_definitions(-DCUTE_WINDOWS)
elseif(LINUX)
	add_definitions(-DCUTE_LINUX)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(MACOSX)
	add_definitions(-DCUTE_MACOSX)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
elseif(EMSCRIPTEN)
	add_definitions(-DCUTE_EMSCRIPTEN)
endif()
# TO BE REMOVED BY RANDY LATER - SEE https://github.com/RandyGaul/cute_framework/issues/16
# ----------------------------------------------------------------------------------------

# Cute.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/libraries/cute_framework")
find_package(cute REQUIRED)

# Block Man.
set(CUTE_BLOCK_MAN_SRCS
	src/main.cpp
	src/world.h
	src/world.cpp
	src/serialize.h
	src/serialize.cpp
	src/shaders/light_shader.h
)

set(CUTE_BLOCK_MAN_IMGUI_SRCS
	libraries/imgui/imgui.cpp
	libraries/imgui/imgui_demo.cpp
	libraries/imgui/imgui_draw.cpp
	libraries/imgui/imgui_widgets.cpp
)

set(CUTE_BLOCK_MAN_COMPONENT_SRCS
	src/components/animator.h
	src/components/board_piece.h
	src/components/ice_block.h
	src/components/player.h
	src/components/reflection.h
	src/components/transform.h
	src/components/shadow.h
	src/components/mochi.h
	src/components/fire.h
	src/components/light.h
	src/components/oil.h
	src/components/lamp.h
	src/components/ladder.h
)

set(CUTE_BLOCK_MAN_SYSTEM_SRCS
	src/systems/animator_system.h
	src/systems/animator_system.cpp
	src/systems/board_system.h
	src/systems/board_system.cpp
	src/systems/ice_block_system.h
	src/systems/ice_block_system.cpp
	src/systems/player_system.h
	src/systems/player_system.cpp
	src/systems/reflection_system.h
	src/systems/reflection_system.cpp
	src/systems/transform_system.h
	src/systems/transform_system.cpp
	src/systems/shadow_system.h
	src/systems/shadow_system.cpp
	src/systems/mochi_system.h
	src/systems/mochi_system.cpp
	src/systems/light_system.h
	src/systems/light_system.cpp
)

add_executable(block_man
	${CUTE_BLOCK_MAN_SRCS}
	${CUTE_BLOCK_MAN_IMGUI_SRCS}
	${CUTE_BLOCK_MAN_COMPONENT_SRCS}
	${CUTE_BLOCK_MAN_SYSTEM_SRCS}
)
source_group("game" FILES ${CUTE_BLOCK_MAN_SRCS})
source_group("imgui" FILES ${CUTE_BLOCK_MAN_IMGUI_SRCS})
source_group("components" FILES ${CUTE_BLOCK_MAN_COMPONENT_SRCS})
source_group("systems" FILES ${CUTE_BLOCK_MAN_SYSTEM_SRCS})
target_link_libraries(block_man PRIVATE cute)
if(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	target_compile_options(block_man PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_options(block_man PRIVATE -o block_man.html --preload-file ../block_man/data --emrun -O1)
endif()
target_include_directories(block_man PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_include_directories(block_man PUBLIC "${PROJECT_SOURCE_DIR}/libraries")
add_custom_command(TARGET block_man POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/libraries/cute_framework/mingw/libcute.dll" $<TARGET_FILE_DIR:block_man>)
add_custom_command(TARGET block_man POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/src/data" "$<TARGET_FILE_DIR:block_man>/data")

if (MSVC)
	set_property(TARGET block_man PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:block_man>)
endif()
