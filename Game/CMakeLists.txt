project(Game)

add_executable(Game
    src/Main.cpp
)

if (PROJECT_PLATFORM STREQUAL "LINUX")
    target_compile_definitions(Game PUBLIC
        PLATFORM_LINUX=1
    )
endif()

target_compile_features(Game PRIVATE cxx_std_20)

target_link_libraries(Game PRIVATE
    SDL3::SDL3
)
