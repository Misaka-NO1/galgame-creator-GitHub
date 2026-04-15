cmake_minimum_required(VERSION 3.19)
project(galgame_creator LANGUAGES CXX)

option(CANVAS_TEST "Enable standalone GameCanvas test main" OFF)
option(MODEL_TEST "Enable standalone model test main" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(galgame_creator
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/io/exporter.h
    src/io/exporter.cpp
    src/editors/charactereditor.h
    src/editors/charactereditor.cpp
    src/editors/dialogueeditdialog.h
    src/editors/dialogueeditdialog.cpp
    src/models/character.h
    src/models/character.cpp
    src/models/dialogue.h
    src/models/dialogue.cpp
    src/models/background.h
    src/models/background.cpp
    src/models/project.h
    src/models/project.cpp
    src/models/model_test.cpp
    src/view/gamecanvas.h
    src/view/gamecanvas.cpp
    src/view/typewritereffect.h
    src/view/typewritereffect.cpp
    src/player/player_window.h
    src/player/player_window.cpp
)

target_link_libraries(galgame_creator
    PRIVATE
        Qt::Core
        Qt::Widgets
)

qt_add_executable(galplayer
    WIN32 MACOSX_BUNDLE
    src/player/player_main.cpp
    src/player/player_window.h
    src/player/player_window.cpp
    src/view/typewritereffect.h
    src/view/typewritereffect.cpp
)

target_link_libraries(galplayer
    PRIVATE
        Qt::Core
        Qt::Widgets
)

if(CANVAS_TEST)
    target_compile_definitions(galgame_creator PRIVATE CANVAS_TEST)
endif()

if(MODEL_TEST)
    target_compile_definitions(galgame_creator PRIVATE MODEL_TEST)
endif()

include(GNUInstallDirs)

install(TARGETS galgame_creator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(TARGETS galplayer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET galgame_creator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
