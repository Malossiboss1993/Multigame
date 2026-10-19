cmake_minimum_required(VERSION 3.16)

# Specify compilers
# If no compiler was specified through CC/CXX environment variables or the
# CMake cache, use the default compiler of the system.
if ("${CMAKE_C_COMPILER}" STREQUAL "")
    message(STATUS "No C compiler was specified. Using the default C compiler.")
endif()
if ("${CMAKE_CXX_COMPILER}" STREQUAL "")
    message(STATUS "No C++ compiler was specified. Using the default C++ compiler.")
endif()

# Name of the project
project(Multigame)

# Specify linker flags - set CMAKE_CURRENT_BINARY_DIR to append the current build directory
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'$ORIGIN'" )
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'$ORIGIN'" )

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# Check if the build directory starts with build* prefix
if(NOT ${CMAKE_CURRENT_BINARY_DIR} MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/build*")
    message(FATAL_ERROR "\n CMake needs to be run in a directory with prefix /build*")
endif()

# Output compiler Information and build type
message(STATUS "====== Current compiler is: ${CMAKE_CXX_COMPILER} ======")
message(STATUS "====================================")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "No selected build type")
    message(STATUS "Setting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "Selected build type is ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "====================================")

# Get all source files and headers
file(GLOB SOURCES
    main_app/*.h
    main_app/*.cpp
    main_app/anim_player/*.h
    main_app/anim_player/*.cpp
    main_app/app_select/*.h
    main_app/app_select/*.cpp
    main_app/audio_player/*.h
    main_app/audio_player/*.cpp
    main_app/applications/*.h
    main_app/applications/*.cpp
    main_app/applications/roulette/*.h
    main_app/applications/roulette/*.cpp
    main_app/applications/roulette/math_logic/*.h
    main_app/applications/roulette/math_logic/*.cpp
    main_app/applications/roulette/status_line/*.h
    main_app/applications/roulette/status_line/*.cpp
    main_app/applications/roulette/table_area/*.h
    main_app/applications/roulette/table_area/*.cpp
    main_app/applications/roulette/wheel_area/*.h
    main_app/applications/roulette/wheel_area/*.cpp
    main_app/applications/kids_fantasy/*.h
    main_app/applications/kids_fantasy/*.cpp
    main_app/applications/kids_fantasy/lines/*.h
    main_app/applications/kids_fantasy/lines/*.cpp
    main_app/applications/kids_fantasy/math_logic/*.h
    main_app/applications/kids_fantasy/math_logic/*.cpp
    main_app/applications/kids_fantasy/reels_area/*.h
    main_app/applications/kids_fantasy/reels_area/*.cpp
    main_app/applications/kids_fantasy/reels_area/reels/*.h
    main_app/applications/kids_fantasy/reels_area/reels/*.cpp
    main_app/applications/kids_fantasy/status_line/*.h
    main_app/applications/kids_fantasy/status_line/*.cpp
    main_app/applications/particle_builder/*.h
    main_app/applications/particle_builder/*.cpp
    main_app/button/*.h
    main_app/button/*.cpp
    main_app/font/*.h
    main_app/font/*.cpp
    main_app/interpolator/*.h
    main_app/interpolator/*.cpp
    main_app/renderer/*.h
    main_app/renderer/*.cpp
    main_app/texture/*.h
    main_app/texture/*.cpp
    main_app/timer/*.h
    main_app/timer/*.cpp
    main_app/particle_system/*.h
    main_app/particle_system/*.cpp
    main_app/panel/*.h
    main_app/panel/*.cpp
    debug/*.h
    debug/*.cpp
    main.cpp)

# Create executable PROJECT_NAME
add_executable(${PROJECT_NAME} ${SOURCES})

# Specify C++17 as the standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Add include directories
set(MULTIGAME_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBS ${MULTIGAME_ROOT_DIR}/libs)
set(INTERNAL_DIRS
    ${MULTIGAME_ROOT_DIR}
    ${MULTIGAME_ROOT_DIR}/debug
    ${MULTIGAME_ROOT_DIR}/main_app
    ${MULTIGAME_ROOT_DIR}/main_app/anim_player
    ${MULTIGAME_ROOT_DIR}/main_app/app_select
    ${MULTIGAME_ROOT_DIR}/main_app/audio_player
    ${MULTIGAME_ROOT_DIR}/main_app/applications
    ${MULTIGAME_ROOT_DIR}/main_app/applications/kids_fantasy
    ${MULTIGAME_ROOT_DIR}/main_app/applications/roulette
    ${MULTIGAME_ROOT_DIR}/main_app/button
    ${MULTIGAME_ROOT_DIR}/main_app/font
    ${MULTIGAME_ROOT_DIR}/main_app/interpolator
    ${MULTIGAME_ROOT_DIR}/main_app/panel
    ${MULTIGAME_ROOT_DIR}/main_app/particle_system
    ${MULTIGAME_ROOT_DIR}/main_app/renderer
    ${MULTIGAME_ROOT_DIR}/main_app/texture
    ${MULTIGAME_ROOT_DIR}/main_app/timer)
set(LIBS_INCLUDE_DIRS
    ${LIBS}
    ${LIBS}/freetype
    ${LIBS}/glad
    ${LIBS}/glm
    ${LIBS}/imgui
    ${LIBS}/KHR
    ${LIBS}/rapidjson
    ${LIBS}/shader
    ${LIBS}/spdlog
    ${LIBS}/stb_image)
include_directories(${INTERNAL_DIRS})
include_directories(${LIBS_INCLUDE_DIRS})

#find required libraries
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

# Link against the prebuilt irrKlang library shipped in the repository.
set(IrrKlang_LIBRARY
    ${MULTIGAME_ROOT_DIR}/libs/irrKlang/bin/linux-gcc-64/libIrrKlang.so)
target_include_directories(${PROJECT_NAME} PRIVATE ${MULTIGAME_ROOT_DIR}/libs/irrKlang/include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${IrrKlang_LIBRARY})

#add library glad
add_library(Glad ${LIBS}/glad/glad.h ${LIBS}/glad.c)

#add library imgui
file(GLOB IMGUI_SOURCES ${LIBS}/imgui/*.h ${LIBS}/imgui/*.cpp)
add_library(ImGui ${IMGUI_SOURCES})

#link all libraries to executable
target_link_libraries(${PROJECT_NAME} PRIVATE glfw GL ${IrrKlang_LIBRARY}
                        Glad freetype ImGui X11 pthread Xrandr
                        Xi ${CMAKE_DL_LIBS} stdc++fs)
