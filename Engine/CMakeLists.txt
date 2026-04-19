cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE ENGINE_SOURCES
    "src/Core/*.cpp"
    "src/Renderer/*.cpp"
    "src/Scene/*.cpp"
    "src/Scripting/*.cpp"
    "src/Audio/*.cpp"
    "src/Physics/*.cpp"
    "src/VPK/*.cpp"
)

add_library(PillarEngine STATIC ${ENGINE_SOURCES})

target_include_directories(PillarEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VPK
)

target_link_libraries(PillarEngine PUBLIC
    glad
    glfw
    glm::glm
    imgui
    stb
    miniaudio
    tinygltf
    assimp
    nlohmann_json::nlohmann_json
    EnTT::EnTT
    OpenGL::GL
)

target_compile_definitions(PillarEngine PUBLIC
    PILLAR_ENGINE
    GLM_ENABLE_EXPERIMENTAL
    $<$<CONFIG:Debug>:PILLAR_DEBUG>
    $<$<CONFIG:Release>:PILLAR_RELEASE>
)

if(MSVC)
    target_compile_options(PillarEngine PRIVATE /W4 /MP)
else()
    target_compile_options(PillarEngine PRIVATE -Wall -Wextra)
endif()
