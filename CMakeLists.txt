# GPROJ CMakeLists.txt
# Creation Date: 08-26-19

# CMake version
# Minimum required is 3.15 for MSVC_RUNTIME_LIBRARY
cmake_minimum_required(VERSION 3.15)

# Project
set(PROJ_NAME "gproj")
project(${PROJ_NAME})
						
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${PROJ_NAME})

# Set global variables
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Create directory variables and glob files
# This section isn't fully utilized (e.g. OBJECT_DIR isn't used)
# TODO
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(OBJECT_DIR "${PROJECT_SOURCE_DIR}/obj")
set(BINARY_DIR "${PROJECT_SOURCE_DIR}/bin")
set(DEP_BASE_DIR "${PROJECT_SOURCE_DIR}/dep")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/inc" "${DEP_BASE_DIR}/inc")

if(WIN32)
	set(DEPENDANCY_DIR "${DEP_BASE_DIR}/win")
else()
	set(DEPENDANCY_DIR "${DEP_BASE_DIR}/lnx")
endif(WIN32)

set(INCLUDE_DIRS ${INCLUDE_DIR})

# Gather files
# If we didn't want to recursively go into each subdirectory 
# then we use GLOB instead of GLOB_RECURSE
message("|| Gathering files")

set(GLM_DIRECTORY ${DEP_BASE_DIR}/inc/glm)
file(GLOB SRC_CPP src/*.cpp)
file(GLOB HEADERS inc/*.h)
file(GLOB INLINE inc/*.inl)
file(GLOB_RECURSE GLM_HEADERS ${GLM_DIRECTORY}/*.hpp)
file(GLOB DEPENDENCY_FILES_H ${DEP_BASE_DIR}/inc/*.h)
file(GLOB DEPENDENCY_FILES_CPP ${DEP_BASE_DIR}/src/*.c*)

message("|| Gathering GLM")
add_subdirectory(${GLM_DIRECTORY})

# Find required packages
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

if(WIN32)
	# Windows does not expect glfw to be installed in Program Files 
	# for this project. Instead, the library is given.
	message("|| Setting WIN32 libraries")
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
	message("|| Setting UNIX libraries")
	find_package(GLFW3 REQUIRED)
	set(LIBS ${GLFW3_LIBRARY} ${CMAKE_DL_LIBS})
endif(WIN32)

# Vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("|| Vulkan found: ${Vulkan_LIBRARIES}")
    list(APPEND LIBS ${Vulkan_LIBRARY})
    list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})
else()
    add_compile_definitions(DW_DISABLE_VULKAN=1)
endif()

# Create targets
message("|| Creating executable")
add_executable(${PROJ_NAME} ${SRC_CPP} ${HEADERS} ${GLM_HEADERS} ${DEPENDENCY_FILES_H} ${DEPENDENCY_FILES_CPP} ${INLINE})

if(MSVC)
  set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif()

# This creates filters for IDEs like Visual Studio
message("|| Creating source groups (filters)")
source_group("inl" FILES ${INLINE})
source_group("inc" FILES ${HEADERS})
source_group("dep" FILES ${DEPENDENCY_FILES_H})
source_group("dep\\glm" FILES ${GLM_HEADERS})
source_group("dep\\src" FILES ${DEPENDENCY_FILES_CPP})
source_group("src" FILES ${SRC_CPP})

# Add include directories and link locations/libraries to the executable
message("|| Adding target specifications")
target_include_directories(${PROJ_NAME} PUBLIC ${INCLUDE_DIRS})
target_link_directories(${PROJ_NAME} PUBLIC ${DEPENDANCY_DIR})
target_link_libraries(${PROJ_NAME} PUBLIC ${LIBS})

# Changes the properties for generation for the code and final executable
set_target_properties(${PROJ_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		OUTPUT_NAME_DEBUG ${PROJ_NAME}_debug
		OUTPUT_NAME_RELEASE ${PROJ_NAME}_release
		OUTPUT_NAME_RELWITHDEBINFO ${PROJ_NAME}_reldeb
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

