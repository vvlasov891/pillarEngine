cmake_minimum_required(VERSION 3.20)

add_executable(PillarGame
    src/GameApp.cpp
)

target_link_libraries(PillarGame PRIVATE PillarEngine)

target_include_directories(PillarGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
    ${CMAKE_SOURCE_DIR}/Engine/src
)

# Copy assets & shaders next to the executable
add_custom_command(TARGET PillarGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Engine/shaders
        $<TARGET_FILE_DIR:PillarGame>/shaders
    COMMENT "Copying shaders to game output..."
)
add_custom_command(TARGET PillarGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Game/assets
        $<TARGET_FILE_DIR:PillarGame>/assets
    COMMENT "Copying game assets..."
)

# Windows: embed icon, make it a proper Win32 exe
if(MSVC)
    set_property(TARGET PillarGame PROPERTY WIN32_EXECUTABLE TRUE)
    target_compile_definitions(PillarGame PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_property(TARGET PillarGame PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
