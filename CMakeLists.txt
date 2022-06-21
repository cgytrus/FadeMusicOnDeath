cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME FadeMusicOnDeath)
project(${PROJECT_NAME})

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
  libraries/fmod
)

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)
target_link_libraries(${PROJECT_NAME} minhook cocos2d)

target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/libraries/fmod/fmod.lib)
