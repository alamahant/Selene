cmake_minimum_required(VERSION 3.16)

project(Selene VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#flathub
option(FLATHUB_BUILD "Build for Flathub/Flatpak" OFF)
option(BUILD_CURL "Build Curl" OFF)

if(FLATHUB_BUILD)
  # Set source directories (these will exist in the Flatpak build sandbox)
  set(LIBEVENT_SRC_DIR "${CMAKE_SOURCE_DIR}/libevent_src")
  set(TOR_SRC_DIR "${CMAKE_SOURCE_DIR}/tor_src")

  # Check if source directories exist
  if(NOT EXISTS ${LIBEVENT_SRC_DIR})
    message(FATAL_ERROR "libevent source directory not found at ${LIBEVENT_SRC_DIR}")
  endif()
  if(NOT EXISTS ${TOR_SRC_DIR})
    message(FATAL_ERROR "tor source directory not found at ${TOR_SRC_DIR}")
  endif()

  # Build libevent
  execute_process(
    COMMAND ./autogen.sh
    WORKING_DIRECTORY ${LIBEVENT_SRC_DIR}
    RESULT_VARIABLE AUTOGEN_RESULT
  )
  if(NOT AUTOGEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run autogen.sh for libevent")
  endif()

  execute_process(
    COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX} --disable-shared #--disable-openssl
    WORKING_DIRECTORY ${LIBEVENT_SRC_DIR}
    RESULT_VARIABLE CONFIGURE_RESULT
  )
  if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure libevent")
  endif()

  execute_process(
    COMMAND make #-j$ENV{NPROC}
    WORKING_DIRECTORY ${LIBEVENT_SRC_DIR}
    RESULT_VARIABLE MAKE_RESULT
  )
  if(NOT MAKE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build libevent")
  endif()

  execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${LIBEVENT_SRC_DIR}
    RESULT_VARIABLE INSTALL_RESULT
  )
  if(NOT INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install libevent")
  endif()

  # Build tor (depends on libevent)
  execute_process(
    COMMAND ./autogen.sh
    WORKING_DIRECTORY ${TOR_SRC_DIR}
    RESULT_VARIABLE AUTOGEN_RESULT_TOR
  )
  if(NOT AUTOGEN_RESULT_TOR EQUAL 0)
    message(FATAL_ERROR "Failed to run autogen.sh for tor")
  endif()

  execute_process(
    COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX}
                        --with-libevent-dir=${CMAKE_INSTALL_PREFIX}
                        --disable-asciidoc
                        --disable-manpage
                        --disable-html-manual
                        --disable-seccomp
                        --disable-lzma
                        --disable-zstd
                        --enable-pic
    WORKING_DIRECTORY ${TOR_SRC_DIR}
    RESULT_VARIABLE CONFIGURE_RESULT_TOR
  )
  if(NOT CONFIGURE_RESULT_TOR EQUAL 0)
    message(FATAL_ERROR "Failed to configure tor")
  endif()

  execute_process(
    COMMAND make #-j$ENV{NPROC}
    WORKING_DIRECTORY ${TOR_SRC_DIR}
    RESULT_VARIABLE MAKE_RESULT_TOR
  )
  if(NOT MAKE_RESULT_TOR EQUAL 0)
    message(FATAL_ERROR "Failed to build tor")
  endif()

  execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${TOR_SRC_DIR}
    RESULT_VARIABLE INSTALL_RESULT_TOR
  )
  if(NOT INSTALL_RESULT_TOR EQUAL 0)
    message(FATAL_ERROR "Failed to install tor")
  endif()

  # Set Tor executable path for your project
  set(TOR_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/bin/tor" CACHE PATH "Path to Tor executable")

#curl
if(BUILD_CURL)

  set(CURL_SRC_DIR "${CMAKE_SOURCE_DIR}/curl_src")

  if(EXISTS ${CURL_SRC_DIR})
    execute_process(
      COMMAND autoreconf -fi
      WORKING_DIRECTORY ${CURL_SRC_DIR}
      RESULT_VARIABLE CURL_AUTORECONF_RESULT
    )

    execute_process(
      COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX}
                          #--disable-shared
                          #--enable-static
                          --with-openssl
                          #--without-zlib
      WORKING_DIRECTORY ${CURL_SRC_DIR}
      RESULT_VARIABLE CURL_CONFIGURE_RESULT
    )


  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${CURL_SRC_DIR}
    RESULT_VARIABLE CURL_MAKE_RESULT
  )
  if(NOT CURL_MAKE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build curl")
  endif()

    execute_process(

      COMMAND make install
      WORKING_DIRECTORY ${CURL_SRC_DIR}
      RESULT_VARIABLE CURL_MAKE_RESULT
    )
  set(CURL_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/bin/curl" CACHE PATH "Path to curl executable")
endif()
endif()
#curl
endif()
#end flathub



find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Qt6Network Qt6Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Multimedia)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(OpenSSL REQUIRED)

    qt_add_executable(Selene
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        torconfig.h torconfig.cpp
        logger.h logger.cpp
        torprocess.h torprocess.cpp
        networkmanager.h networkmanager.cpp
        peerstate.h
        contact.h
        contactmanager.h contactmanager.cpp
        contactcardwidget.h contactcardwidget.cpp
        contactlistwidget.h contactlistwidget.cpp
        chatmanager.h chatmanager.cpp
        messagebubblewidget.h messagebubblewidget.cpp

        constants.h

        emojipickerwidget.h emojipickerwidget.cpp
        resources.qrc
        simplehttpfileserver.h simplehttpfileserver.cpp
        constants.cpp
        chatmessage.h chatmessage.cpp
        chatsession.h chatsession.cpp
        logviewerdialog.h logviewerdialog.cpp
        helpmenudialog.cpp helpmenudialog.h
        securitymanager.cpp securitymanager.h
        Notification.h
        donationdialog.cpp donationdialog.h
        httpclientdialog.h httpclientdialog.cpp
        bridgemanagerdialog.h bridgemanagerdialog.cpp

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Selene APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Selene SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Selene
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Selene PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network Qt6::Multimedia)
target_link_libraries(Selene PRIVATE Qt6::Core)
target_link_libraries(Selene PRIVATE Qt6::Widgets)
target_link_libraries(Selene PRIVATE Qt6::Core)
target_link_libraries(Selene PRIVATE OpenSSL::Crypto)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Selene)
endif()
set_target_properties(Selene PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Selene
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop file and metainfo for Selene

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Selene.desktop"
        DESTINATION "share/applications")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Selene.metainfo.xml"
        DESTINATION "share/metainfo")

# Install icons for different sizes
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicons/192x192.png"
        DESTINATION "share/icons/hicolor/192x192/apps"
        RENAME "io.github.alamahant.Selene.png")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicons/512x512.png"
        DESTINATION "share/icons/hicolor/512x512/apps"
        RENAME "io.github.alamahant.Selene.png")

# Optionally, install 16x16 and 32x32 icons if desired
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicons/16x16.png"
        DESTINATION "share/icons/hicolor/16x16/apps"
        RENAME "io.github.alamahant.Selene.png")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicons/32x32.png"
        DESTINATION "share/icons/hicolor/32x32/apps"
        RENAME "io.github.alamahant.Selene.png")

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Selene)
endif()
