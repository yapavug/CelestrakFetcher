cmake_minimum_required(VERSION 3.19)
project(CelestrakFetcher LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(CelestrakFetcher
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(CelestrakFetcher PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql)

include(GNUInstallDirs)

install(TARGETS CelestrakFetcher
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET CelestrakFetcher
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
