project(Game)

add_executable(Game
    src/Utils/Log.cpp
    src/Game.cpp
    src/Main.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Game PUBLIC
        DEBUG
        LOGGING_ENABLED
        ASSERTS_ENABLED
    )
else()
    target_compile_definitions(Game PUBLIC
        NDEBUG
    )
endif()

if (PROJECT_PLATFORM STREQUAL "LINUX")
    target_compile_definitions(Game PUBLIC
        PLATFORM_LINUX=1
    )
endif()

target_include_directories(Game PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(Game PRIVATE cxx_std_20)

target_link_libraries(Game PRIVATE
    SDL3::SDL3
    spdlog::spdlog_header_only
)
