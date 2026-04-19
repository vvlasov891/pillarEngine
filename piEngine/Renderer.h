cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE EDITOR_SOURCES "src/*.cpp")

add_executable(PillarEditor ${EDITOR_SOURCES})

target_link_libraries(PillarEditor PRIVATE PillarEngine imgui)

target_include_directories(PillarEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Engine/src
)

# Copy shaders to build dir
add_custom_command(TARGET PillarEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Engine/shaders
        $<TARGET_FILE_DIR:PillarEditor>/shaders
    COMMENT "Copying shaders..."
)

set_property(TARGET PillarEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

if(MSVC)
    set_property(TARGET PillarEditor PROPERTY WIN32_EXECUTABLE FALSE)
endif()
