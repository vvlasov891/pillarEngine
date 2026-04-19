cmake_minimum_required(VERSION 3.20)

add_executable(PillarPacker main.cpp)

target_link_libraries(PillarPacker PRIVATE PillarEngine)
target_include_directories(PillarPacker PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)

if(MSVC)
    target_compile_definitions(PillarPacker PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
