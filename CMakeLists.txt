cmake_minimum_required(VERSION 3.22)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" BATCHLY_RELEASE_VERSION LIMIT_COUNT 1)
string(REGEX REPLACE "-.*$" "" BATCHLY_BASE_VERSION "${BATCHLY_RELEASE_VERSION}")
project(BatchlyAudio VERSION ${BATCHLY_BASE_VERSION} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Keep the Windows download self-contained, without a separate C++ runtime installer.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(BATCHLY_BUILD_APP "Build the app and VST3 as well as DSP tests" ON)
enable_testing()
add_executable(drift_tests tests/DriftTests.cpp)
target_include_directories(drift_tests PRIVATE src)
add_test(NAME drift_dsp COMMAND drift_tests)
add_executable(patina_tests tests/PatinaTests.cpp)
target_include_directories(patina_tests PRIVATE src)
add_test(NAME patina_dsp COMMAND patina_tests)
add_executable(atrium_tests tests/AtriumTests.cpp)
target_include_directories(atrium_tests PRIVATE src)
add_test(NAME atrium_dsp COMMAND atrium_tests)
add_executable(chime_tests tests/ChimeTests.cpp)
target_include_directories(chime_tests PRIVATE src)
add_test(NAME chime_dsp COMMAND chime_tests)
add_executable(helix_tests tests/HelixTests.cpp)
target_include_directories(helix_tests PRIVATE src)
add_test(NAME helix_dsp COMMAND helix_tests)
add_executable(gleam_tests tests/GleamTests.cpp)
target_include_directories(gleam_tests PRIVATE src)
add_test(NAME gleam_dsp COMMAND gleam_tests)
add_executable(relay_tests tests/RelayTests.cpp)
target_include_directories(relay_tests PRIVATE src)
add_test(NAME relay_dsp COMMAND relay_tests)
add_executable(forge_tests tests/ForgeTests.cpp)
target_include_directories(forge_tests PRIVATE src)
add_test(NAME forge_dsp COMMAND forge_tests)
add_executable(cinder_tests tests/CinderTests.cpp)
target_include_directories(cinder_tests PRIVATE src)
add_test(NAME cinder_dsp COMMAND cinder_tests)
add_executable(ember_tests tests/EmberTests.cpp)
target_include_directories(ember_tests PRIVATE src)
add_test(NAME ember_dsp COMMAND ember_tests)
add_executable(vista_tests tests/VistaTests.cpp)
target_include_directories(vista_tests PRIVATE src)
add_test(NAME vista_dsp COMMAND vista_tests)
add_executable(quartz_tests tests/QuartzTests.cpp)
target_include_directories(quartz_tests PRIVATE src)
add_test(NAME quartz_dsp COMMAND quartz_tests)

if(BATCHLY_BUILD_APP)
    # Pin the framework so a fresh checkout builds against the same audio wrapper.
    include(FetchContent)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/JUCE/CMakeLists.txt")
        add_subdirectory(vendor/JUCE EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 72782788ce18c2d4d760b28e0921d6ffc6431102)
        FetchContent_MakeAvailable(JUCE)
    endif()
    juce_add_plugin(BatchlyAudio
        COMPANY_NAME "Batchly"
        BUNDLE_ID "com.batchly.audio"
        PLUGIN_MANUFACTURER_CODE Btch
        PLUGIN_CODE Badr
        FORMATS VST3 Standalone
        PRODUCT_NAME "Batchly Audio"
        DESCRIPTION "Original modulation, tape color, reverb, resonance, phasing, brightness, echoes, drum shaping, texture, bass saturation, stereo imaging and mastering"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Modulation
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE)
    juce_add_binary_data(BatchlyFonts SOURCES assets/Archivo.ttf assets/JetBrainsMono.ttf tools/apply-update.ps1)
    target_sources(BatchlyAudio PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp src/UpdateService.cpp)
    target_compile_definitions(BatchlyAudio PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        "BATCHLY_RELEASE_VERSION=\"${BATCHLY_RELEASE_VERSION}\"")
    target_link_libraries(BatchlyAudio PRIVATE juce::juce_audio_utils juce::juce_cryptography BatchlyFonts
        PUBLIC juce::juce_recommended_config_flags)
    add_executable(update_tests tests/UpdateTests.cpp src/UpdateService.cpp)
    target_include_directories(update_tests PRIVATE src)
    target_compile_definitions(update_tests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0
        "BATCHLY_RELEASE_VERSION=\"${BATCHLY_RELEASE_VERSION}\"")
    target_link_libraries(update_tests PRIVATE juce::juce_core juce::juce_cryptography BatchlyFonts)
    add_test(NAME update_validation COMMAND update_tests)
    add_executable(editor_tests tests/EditorTests.cpp)
    # Exercise the compiled plugin components using their existing JUCE settings.
    target_include_directories(editor_tests PRIVATE src $<TARGET_PROPERTY:BatchlyAudio,INCLUDE_DIRECTORIES>)
    target_compile_definitions(editor_tests PRIVATE $<TARGET_PROPERTY:BatchlyAudio,COMPILE_DEFINITIONS>)
    target_link_libraries(editor_tests PRIVATE BatchlyAudio)
    add_test(NAME editor_rendering COMMAND editor_tests editor-previews)
endif()
