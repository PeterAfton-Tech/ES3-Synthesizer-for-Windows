cmake_minimum_required(VERSION 3.22)
project(ES3 VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime do Visual C++ embutido: o usuário não precisa instalar nada extra
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6)   # confirme/registre a versão e a licença (AGPLv3 ou comercial)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ES3
    COMPANY_NAME "ES3 Project"
    PLUGIN_MANUFACTURER_CODE Es3P
    PLUGIN_CODE Es3x
    FORMATS VST3 Standalone
    PRODUCT_NAME "ES3"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(ES3 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ES3 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ES3 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Testes do núcleo de DSP (sem JUCE)
enable_testing()
add_executable(es3_engine_tests Tests/engine_tests.cpp)
target_include_directories(es3_engine_tests PRIVATE Source)
add_test(NAME engine COMMAND es3_engine_tests)
