cmake_minimum_required(VERSION 3.6)
project(obj2difPlus)

add_subdirectory("3rdparty/DifBuilder")
add_subdirectory("3rdparty/tinyobjloader")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "/FS /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "/MT")

set(SOURCE_FILES main.cpp)
add_executable(obj2difPlus ${SOURCE_FILES})

include_directories(3rdparty/tinyobjloader)
include_directories(3rdparty/DifBuilder/include)
include_directories(3rdparty/DifBuilder/3rdparty/)
include_directories(3rdparty/DifBuilder/3rdparty/Dif)
include_directories(3rdparty/DifBuilder/3rdparty/Dif/3rdparty/glm)
include_directories(3rdparty/DifBuilder/3rdparty/Dif/include)
target_compile_options(tinyobjloader PRIVATE "$<$<CONFIG:Debug>:/MTd>" "$<$<CONFIG:Release>:/MT>")
set_target_properties(Dif PROPERTIES COMPILE_FLAGS "/FS")
target_link_libraries(obj2difPlus DifBuilder Dif tinyobjloader)