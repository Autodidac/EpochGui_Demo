cmake_minimum_required(VERSION 3.28)

project(EpochGuiDemo VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(FATAL_ERROR "EpochGui_Demo requires Ninja or Visual Studio for C++23 modules.")
endif()

set(EPOCHGUI_ROOT "" CACHE PATH "Path to the standalone EpochGui repository")

set(_epochgui_candidates)
if(EPOCHGUI_ROOT)
    list(APPEND _epochgui_candidates "${EPOCHGUI_ROOT}")
endif()
list(APPEND _epochgui_candidates
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/EpochGui"
    "${CMAKE_CURRENT_SOURCE_DIR}/EpochGui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../EpochGui"
)

set(_epochgui_resolved_root "")
foreach(_candidate IN LISTS _epochgui_candidates)
    get_filename_component(_candidate_abs "${_candidate}" ABSOLUTE)
    if(EXISTS "${_candidate_abs}/CMakeLists.txt"
        AND EXISTS "${_candidate_abs}/modules/epoch.gui.ixx"
        AND EXISTS "${_candidate_abs}/modules/epoch.gui.rounded_rect.ixx"
        AND EXISTS "${_candidate_abs}/modules/epoch.gui.input.ixx"
        AND EXISTS "${_candidate_abs}/modules/epoch.gui.font.ixx"
        AND EXISTS "${_candidate_abs}/modules/epoch.gui.image.ixx")
        set(_epochgui_resolved_root "${_candidate_abs}")
        break()
    endif()
endforeach()

if(NOT _epochgui_resolved_root)
    message(FATAL_ERROR
        "Standalone EpochGui with rounded rectangles and fallback input was not found. "
        "Pass -DEPOCHGUI_ROOT=<path-to-Autodidac/EpochGui>.")
endif()

set(EPOCHGUI_ROOT "${_epochgui_resolved_root}" CACHE PATH
    "Path to the standalone EpochGui repository" FORCE)
set(EPOCHGUI_SOURCE_ROOT "${EPOCHGUI_ROOT}" CACHE PATH
    "EpochGui source root used by the demo" FORCE)
set(EPOCHGUI_ENABLE_ROUNDED_RECT ON CACHE BOOL
    "Enable optional EpochGui rounded-rectangle geometry for comparison" FORCE)
set(EPOCHGUI_ENABLE_INPUT ON CACHE BOOL
    "Enable optional EpochGui fallback input for this standalone demo" FORCE)

message(STATUS "EpochGui root: ${EPOCHGUI_ROOT}")
message(STATUS "EpochGui fallback input: enabled for demo")

set(EPOCHGUI_DEMO_COMPOSITION "gui" CACHE STRING "gui, media, audio, platform, or full")
set_property(CACHE EPOCHGUI_DEMO_COMPOSITION PROPERTY STRINGS gui media audio platform full)
if(NOT EPOCHGUI_DEMO_COMPOSITION MATCHES "^(gui|media|audio|platform|full)$")
    message(FATAL_ERROR "Unknown EPOCHGUI_DEMO_COMPOSITION")
endif()
set(EPOCHGUI_DEMO_HOST "native" CACHE STRING "native or platform (Win32 currently)")
set_property(CACHE EPOCHGUI_DEMO_HOST PROPERTY STRINGS native platform)
if(NOT EPOCHGUI_DEMO_HOST MATCHES "^(native|platform)$")
    message(FATAL_ERROR "Unknown EPOCHGUI_DEMO_HOST")
endif()
if(EPOCHGUI_DEMO_HOST STREQUAL "platform" AND NOT WIN32)
    message(FATAL_ERROR "Platform-owned Demo GL adapter is currently Win32 only; native host remains available")
endif()
set(_demo_owners)
foreach(_owner IN ITEMS Platform Audio Media)
    string(TOLOWER "${_owner}" _owner_lower)
    string(TOUPPER "${_owner}" _owner_upper)
    set(EPOCH_DEMO_${_owner_upper} 0)
    if(EPOCHGUI_DEMO_COMPOSITION STREQUAL "full" OR EPOCHGUI_DEMO_COMPOSITION STREQUAL "${_owner_lower}" OR (_owner STREQUAL "Platform" AND EPOCHGUI_DEMO_HOST STREQUAL "platform"))
        list(APPEND _demo_owners ${_owner})
        set(EPOCH_DEMO_${_owner_upper} 1)
    endif()
endforeach()

# Dependencies are explicit local source roots; no network fetch or Engine fallback.
set(_demo_parent_build_testing "${BUILD_TESTING}")
set(BUILD_TESTING OFF)
if(NOT TARGET EpochGui)
    add_subdirectory("${EPOCHGUI_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/EpochGui")
endif()
foreach(_owner IN LISTS _demo_owners)
    string(TOUPPER "${_owner}" _owner_upper)
    set(EPOCH_${_owner_upper}_ROOT "" CACHE PATH "Path to standalone Epoch${_owner}Engine")
    if(NOT EPOCH_${_owner_upper}_ROOT)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Epoch${_owner}Engine/CMakeLists.txt")
            set(EPOCH_${_owner_upper}_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Epoch${_owner}Engine")
        else()
            set(EPOCH_${_owner_upper}_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../Epoch${_owner}Engine")
        endif()
    endif()
    if(NOT EXISTS "${EPOCH_${_owner_upper}_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "Set EPOCH_${_owner_upper}_ROOT to a built-source Epoch${_owner}Engine checkout")
    endif()
    if(_owner STREQUAL "Platform")
        set(EPOCH_PLATFORM_BUILD_STATIC ON)
        set(EPOCH_PLATFORM_BUILD_SHARED OFF)
        set(EPOCH_PLATFORM_BUILD_MODULES ON)
        set(EPOCH_PLATFORM_BUILD_EXAMPLES OFF)
        set(EPOCH_PLATFORM_BUILD_TESTS OFF)
    endif()
    if(NOT TARGET Epoch::${_owner}Engine AND NOT (_owner STREQUAL "Platform" AND TARGET EpochPlatformEngine::ModuleStatic))
        add_subdirectory("${EPOCH_${_owner_upper}_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/Epoch${_owner}Engine")
    endif()
endforeach()
set(BUILD_TESTING "${_demo_parent_build_testing}")
add_library(epoch_gui_demo_models STATIC)
target_sources(epoch_gui_demo_models PUBLIC FILE_SET demo_model_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/modules"
    FILES modules/gui.library_models.ixx modules/gui.text_view.ixx)
set_target_properties(epoch_gui_demo_models PROPERTIES CXX_SCAN_FOR_MODULES ON)
target_link_libraries(epoch_gui_demo_models PUBLIC EpochGui)
foreach(_owner IN LISTS _demo_owners)
    if(_owner STREQUAL "Platform")
        target_link_libraries(epoch_gui_demo_models PUBLIC EpochPlatformEngine::ModuleStatic)
    else()
        target_link_libraries(epoch_gui_demo_models PUBLIC Epoch::${_owner}Engine)
    endif()
endforeach()
target_compile_definitions(epoch_gui_demo_models PUBLIC
    EPOCH_DEMO_PLATFORM=${EPOCH_DEMO_PLATFORM} EPOCH_DEMO_AUDIO=${EPOCH_DEMO_AUDIO} EPOCH_DEMO_MEDIA=${EPOCH_DEMO_MEDIA})
include(CTest)
if(BUILD_TESTING)
    add_executable(epoch_gui_demo_libraries_contract tests/gui.library_models_contract.cpp)
    set_target_properties(epoch_gui_demo_libraries_contract PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(epoch_gui_demo_libraries_contract PRIVATE epoch_gui_demo_models)
    add_test(NAME EpochGuiDemo.Libraries COMMAND epoch_gui_demo_libraries_contract)
    add_executable(epoch_gui_demo_text_view_contract tests/gui.text_view_contract.cpp)
    set_target_properties(epoch_gui_demo_text_view_contract PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(epoch_gui_demo_text_view_contract PRIVATE epoch_gui_demo_models)
    add_test(NAME EpochGuiDemo.TextView COMMAND epoch_gui_demo_text_view_contract)
    if(WIN32)
        add_executable(epoch_gui_demo_dpi_contract tests/gui.dpi_geometry_contract.cpp)
        target_include_directories(epoch_gui_demo_dpi_contract PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_compile_features(epoch_gui_demo_dpi_contract PRIVATE cxx_std_23)
        add_test(NAME EpochGuiDemo.DpiGeometry COMMAND epoch_gui_demo_dpi_contract)
        add_executable(epoch_gui_demo_clipboard_contract tests/gui.clipboard_shortcut_contract.cpp)
        target_include_directories(epoch_gui_demo_clipboard_contract PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_compile_features(epoch_gui_demo_clipboard_contract PRIVATE cxx_std_23)
        target_link_libraries(epoch_gui_demo_clipboard_contract PRIVATE user32 opengl32)
        add_test(NAME EpochGuiDemo.ClipboardShortcuts COMMAND epoch_gui_demo_clipboard_contract)
    endif()
endif()

option(EPOCHGUI_DEMO_LIBRARY_PAGE "Open directly on the interlinked libraries page" OFF)
set(EPOCHGUI_DEMO_APP_NAME "EpochGuiDemo" CACHE STRING "Demonstration executable name")

set(_epochgui_demo_renderer_source
    "${CMAKE_CURRENT_SOURCE_DIR}/src/complete_demo_renderer.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_epochgui_demo_renderer_source}")

file(READ "${_epochgui_demo_renderer_source}" _epochgui_demo_renderer_body)
string(REGEX REPLACE
    "^module;[\r\n]+"
    ""
    _epochgui_demo_renderer_body
    "${_epochgui_demo_renderer_body}")

set(_epochgui_demo_renderer_adapter [=[module;

#define splitter_hit_test(splitter_layout, pointer_position, hit_slop) \
    splitter_hit_test_with_capture( \
        (splitter_layout), \
        (pointer_position), \
        (hit_slop), \
        frame.pointer(input::PointerButton::left).pressed)

#define splitter_fraction_from_point(splitter_options, pointer_position) \
    splitter_fraction_from_visible_point( \
        (splitter_options), \
        (pointer_position))

]=])

set(_epochgui_demo_generated_dir
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(_epochgui_demo_generated_renderer
    "${_epochgui_demo_generated_dir}/complete_demo_renderer.cpp")
file(MAKE_DIRECTORY "${_epochgui_demo_generated_dir}")
file(WRITE
    "${_epochgui_demo_generated_renderer}"
    "${_epochgui_demo_renderer_adapter}${_epochgui_demo_renderer_body}")

add_library(epoch_gui_demo_renderer STATIC
    "${_epochgui_demo_generated_renderer}"
    src/app_bridge.cpp
)

set_target_properties(epoch_gui_demo_renderer PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

target_sources(epoch_gui_demo_renderer PUBLIC
    FILE_SET epoch_gui_demo_renderer_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/modules"
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/modules/epoch.gui.demo.opengl.ixx"
)

target_include_directories(epoch_gui_demo_renderer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(epoch_gui_demo_renderer PUBLIC epoch_gui_demo_models)

if(BUILD_TESTING)
    add_executable(epoch_gui_demo_showcase_contract tests/gui.showcase_contract.cpp)
    target_link_libraries(epoch_gui_demo_showcase_contract PRIVATE epoch_gui_demo_renderer)
    add_test(NAME EpochGuiDemo.Showcase COMMAND epoch_gui_demo_showcase_contract)
    set_tests_properties(EpochGuiDemo.Showcase PROPERTIES TIMEOUT 30)
endif()
target_compile_features(epoch_gui_demo_renderer PUBLIC cxx_std_23)
target_compile_definitions(epoch_gui_demo_renderer PRIVATE EPOCHGUI_DEMO_LIBRARY_PAGE=$<BOOL:${EPOCHGUI_DEMO_LIBRARY_PAGE}> EPOCHGUI_DEMO_PLATFORM_HOSTED=$<STREQUAL:${EPOCHGUI_DEMO_HOST},platform>)

if(WIN32)
    enable_language(RC)

    set(_demo_windows_host src/platform/complete_windows_main.cpp)
    if(EPOCHGUI_DEMO_HOST STREQUAL "platform")
        set(_demo_windows_host src/platform/gui.platform_host_win.cpp)
    endif()
    add_executable(epoch_gui_demo_app WIN32
        ${_demo_windows_host}
        src/platform/windows_resources.rc
        src/platform/windows_message_coords.hpp
        src/platform/epochgui_demo.ico
    )

    if(EPOCHGUI_DEMO_HOST STREQUAL "platform")
        target_link_libraries(epoch_gui_demo_app PRIVATE EpochPlatformEngine::EntrypointWindowedStatic)
    endif()
    target_compile_definitions(epoch_gui_demo_app PRIVATE
        UNICODE
        _UNICODE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )

    target_link_libraries(epoch_gui_demo_app PRIVATE
        epoch_gui_demo_renderer
        dwmapi
        opengl32
        uxtheme
    )

    if(MSVC)
        target_compile_options(epoch_gui_demo_app PRIVATE
            "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows_message_coords.hpp"
        )
    endif()
elseif(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 23)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(CMAKE_OBJCXX_EXTENSIONS OFF)

    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)

    add_executable(epoch_gui_demo_app MACOSX_BUNDLE
        src/platform/macos_main.mm
    )

    target_link_libraries(epoch_gui_demo_app PRIVATE
        epoch_gui_demo_renderer
        "${COCOA_FRAMEWORK}"
        "${OPENGL_FRAMEWORK}"
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)

    add_executable(epoch_gui_demo_app
        src/platform/complete_linux_glx_main.cpp
    )

    target_link_libraries(epoch_gui_demo_app PRIVATE
        epoch_gui_demo_renderer
        OpenGL::GL
        X11::X11
    )
else()
    message(FATAL_ERROR "No native OpenGL host is defined for this platform.")
endif()

set_target_properties(epoch_gui_demo_app PROPERTIES
    OUTPUT_NAME "${EPOCHGUI_DEMO_APP_NAME}"
)

target_compile_features(epoch_gui_demo_app PRIVATE cxx_std_23)

add_custom_command(TARGET epoch_gui_demo_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:epoch_gui_demo_app>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/epochgui_demo.ppm"
        "$<TARGET_FILE_DIR:epoch_gui_demo_app>/assets/epochgui_demo.ppm"
    VERBATIM
)

set(_epoch_gui_targets
    epoch_gui_demo_renderer
    epoch_gui_demo_app
)

if(MSVC)
    foreach(_target IN LISTS _epoch_gui_targets)
        target_compile_options(${_target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(_target IN LISTS _epoch_gui_targets)
        target_compile_options(${_target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()

if(WIN32)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT epoch_gui_demo_app)
endif()
