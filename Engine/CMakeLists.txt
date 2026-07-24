# Engine library
file(GLOB_RECURSE ENGINE_SOURCES src/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS src/*.h)

add_library(Engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(Engine PUBLIC
    SDL2::SDL2
    SDL2_mixer::SDL2_mixer
)

# Main executable
add_executable(OpenRhythmEngine
    ${CMAKE_SOURCE_DIR}/main.cpp
)
target_link_libraries(OpenRhythmEngine PRIVATE Engine)