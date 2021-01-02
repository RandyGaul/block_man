# Tries to find the appropriate Cute Framework (CF) shared library.
# This script should work with the folder structure found within CF's releases https://github.com/RandyGaul/cute_framework/releases.
# This file should be in the top-level folder of a CF release.
#
# .
# └── cute_framework-X.X/
#     ├── mingw/
#     ├── msvc/
#     ├── include/
#     └── Findcute.cmake
#
# Once found the following CMAKE variables are set.
#
# CUTE_FOUND
# CUTE_INCLUDES
# CUTE_LIBRARIES
# CUTE_LIBRARY_DEBUG
# CUTE_LIBRARY_RELEASE
#
# And on windows.
#
# CUTE_DLL_DEBUG
# CUTE_DLL_RELEASE
#
# Additionally, a "cute" target is created for you to link against.

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

unset(CUTE_LIBRARIES CACHE)
unset(CUTE_LIBRARY_DEBUG CACHE)
unset(CUTE_LIBRARY_RELEASE CACHE)
unset(CUTE_DLL_DEBUG CACHE)
unset(CUTE_DLL_RELEASE CACHE)

# Setup include path for cute.
find_path(
	CUTE_INCLUDES cute.h
	HINTS ${CMAKE_CURRENT_LIST_DIR}
	PATH_SUFFIXES include
)

# Search for correct library depending on the platform.
if(MACOSX)
elseif(LINUX)
elseif(MINGW)
	find_library(
		CUTE_LIBRARY_DEBUG libcute.dll.a
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES mingw
	)
	find_library(
		CUTE_LIBRARY_RELEASE libcute.dll.a
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES mingw
	)

	find_library(
		CUTE_DLL_DEBUG libcute
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES mingw
		NO_DEFAULT_PATH
	)
	find_library(
		CUTE_DLL_RELEASE libcute
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES mingw
		NO_DEFAULT_PATH
	)
else() # MSVC or CL
	set(CF_DEBUG_PATH "msvc/Debug/")
	set(CF_RELEASE_PATH "msvc/Release/")

	if (MSVC_VERSION LESS 1900)
		math(EXPR CF_VS_VERSION "${MSVC_VERSION} / 10 - 60")
	else()
		math(EXPR CF_VS_VERSION "${MSVC_VERSION} / 10 - 50")
	endif()

	string(APPEND CF_DEBUG_PATH "v${MSVC_VERSION}/")
	string(APPEND CF_RELEASE_PATH "v${MSVC_VERSION}/")

	find_library(
		CUTE_LIBRARY_DEBUG cute.lib
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES ${CF_DEBUG_PATH}
	)

	find_library(
		CUTE_LIBRARY_RELEASE cute.lib
		HINTS ${CMAKE_CURRENT_LIST_DIR}
		PATH_SUFFIXES ${CF_RELEASE_PATH}
	)
endif()


# Communicate results.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	cute
	REQUIRED_VARS
		CUTE_INCLUDES
		CUTE_LIBRARY_DEBUG
		CUTE_LIBRARY_RELEASE
)

if(CUTE_FOUND)
	set(CUTE_LIBRARIES optimized ${CUTE_LIBRARY_RELEASE} debug ${CUTE_LIBRARY_DEBUG})
endif()

mark_as_advanced(CUTE_INCLUDES)
mark_as_advanced(CUTE_LIBRARY_DEBUG)
mark_as_advanced(CUTE_LIBRARY_RELEASE)
if (WIN32)
	mark_as_advanced(CUTE_DLL_DEBUG)
	mark_as_advanced(CUTE_DLL_RELEASE)
endif()

# Create the "cute" target.
add_library(cute SHARED IMPORTED)

set_target_properties(
	cute PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CUTE_INCLUDES}"
)

if(UNIX)
	set_target_properties(
		cute PROPERTIES
		IMPORTED_LOCATION "${CUTE_LIBRARY_RELEASE}"
		IMPORTED_LOCATION_DEBUG "${CUTE_LIBRARY_DEBUG}"
	)
elseif(WIN32)
	set_target_properties(
		cute PROPERTIES
		IMPORTED_IMPLIB "${CUTE_LIBRARY_RELEASE}"
		IMPORTED_IMPLIB_DEBUG "${CUTE_LIBRARY_DEBUG}"
	)
	if(NOT CUTE_DLL_DEBUG MATCHES ".*-NOTFOUND")
		set_target_properties(
			cute PROPERTIES
			IMPORTED_LOCATION_DEBUG "${CUTE_DLL_DEBUG}"
		)
	endif()
	if(NOT CUTE_DLL_RELEASE MATCHES ".*-NOTFOUND")
		set_target_properties(
			cute PROPERTIES
			IMPORTED_LOCATION_RELWITHDEBINFO "${CUTE_DLL_RELEASE}"
			IMPORTED_LOCATION_MINSIZEREL "${CUTE_DLL_RELEASE}"
			IMPORTED_LOCATION_RELEASE "${CUTE_DLL_RELEASE}"
		)
	endif()
endif()
