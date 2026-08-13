project(Game)

add_executable(Game
    src/Utils/FileUtils.cpp
    src/Utils/Log.cpp
    src/Utils/VkUtils.cpp
    src/Game.cpp
    src/Main.cpp
    src/Platform.cpp
    src/VmaImpl.cpp
    src/VolkImpl.cpp
    src/VulkanContext.cpp
    src/Window.cpp
)

find_program(SLANGC_PROGRAM slangc
    HINTS $ENV{VULKAN_SDK}/bin
    REQUIRED)

file(GLOB_RECURSE SLANG_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/*.slang
)

set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/res/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SPV_OUTPUTS "")
foreach(SLANG_FILE ${SLANG_FILES})
    get_filename_component(SLANG_NAME ${SLANG_FILE} NAME_WE)
    set(SPV_FILE ${SHADER_OUTPUT_DIR}/${SLANG_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${SLANGC_PROGRAM}
                ${SLANG_FILE}
                -target spirv
                -profile spirv_1_4
                -entry vertMain
                -entry fragMain
                -o ${SPV_FILE}
        DEPENDS ${SLANG_FILE}
        COMMENT "Compiling shader: ${SLANG_NAME}.slang -> ${SLANG_NAME}.spv"
        VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${SPV_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS})
add_dependencies(Game CompileShaders)

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
    SDL3::SDL3-static
    volk
    VulkanMemoryAllocator
    spdlog::spdlog
)
