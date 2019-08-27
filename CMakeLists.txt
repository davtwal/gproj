# GPROJ CMakeLists.txt
# Creation Date: 08-26-19

# CMake version
cmake_minimum_required(VERSION 3.10...3.15)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Project
set(PROJ_NAME "gproj")
project(${PROJ_NAME}	VERSION 1.0
						DESCRIPTION "Read the README"
						LANGUAGES C CXX)
						
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${PROJ_NAME})

# Set global variables
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Create directory variables and glob files
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/inc")
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(OBJECT_DIR "${PROJECT_SOURCE_DIR}/obj")
set(BINARY_DIR "${PROJECT_SOURCE_DIR}/bin")
set(DEP_BASE_DIR "${PROJECT_SOURCE_DIR}/dep")

if(WIN32)
	set(DEPENDANCY_DIR "${DEP_BASE_DIR}/win")
else()
	set(DEPENDANCY_DIR "${DEP_BASE_DIR}/lnx")
endif(WIN32)

file(GLOB_RECURSE CPPSources src/*.cpp)
file(GLOB_RECURSE Headers inc/*.h)

# Find required packages
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

if(WIN32)
	# Windows does not expect glfw to be installed in Program Files 
	# for this project. Instead, the library is given.
	message("Setting WIN32 libraries")
	set(GLFW3_LIBRARY glfw)
	add_library(${GLFW3_LIBRARY} STATIC IMPORTED)
	set_target_properties(${GLFW3_LIBRARY} PROPERTIES
			IMPORTED_LOCATION_DEBUG ${DEPENDANCY_DIR}/Debug/glfw3.lib
			IMPORTED_LOCATION_RELEASE ${DEPENDANCY_DIR}/Release/glfw3.lib
			IMPORTED_LOCATION_RELWITHDEBINFO ${DEPENDANCY_DIR}/RelWithDebInfo/glfw3.lib
	)
	
	set(LIBS ${GLFW3_LIBRARY})
	
elseif(UNIX)
	# Linux requires that GLFW3 is installed. This can be done with
	# sudo apt install libglfw3-dev
	message("Setting UNIX libraries")
	find_package(GLFW3 REQUIRED)
	set(LIBS ${GLFW3_LIBRARY} ${CMAKE_DL_LIBS})
endif(WIN32)

# Create targets
add_executable(${PROJ_NAME} ${CPPSources})

target_include_directories(${PROJ_NAME} PUBLIC ${INCLUDE_DIR})
target_link_directories(${PROJ_NAME} PUBLIC ${DEPENDANCY_DIR})
target_link_libraries(${PROJ_NAME} PUBLIC ${LIBS})

set_target_properties(${PROJ_NAME} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		OUTPUT_NAME_DEBUG ${PROJ_NAME}_debug
		OUTPUT_NAME_RELEASE ${PROJ_NAME}_release
		OUTPUT_NAME_RELWITHDEBINFO ${PROJ_NAME}_reldeb
)

