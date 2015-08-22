project( StreamingPlayer )
cmake_minimum_required(VERSION  2.8.11)


find_package( Qt5Gui REQUIRED )
find_package( Qt5Widgets REQUIRED )
find_package( Qt5Network REQUIRED )
find_package( Qt5GStreamer REQUIRED )


find_path( GLIB_INCLUDE_PATH glib.h )
find_path( GLIB_CONFIG_PATH glibconfig.h )
find_library( GLIB_LIBRARY glib-2.0 )


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory( include )
add_subdirectory( src )
add_subdirectory( ui )
add_subdirectory( res )

qt5_wrap_ui( STREAMIMG_PLAYER_UI_HEADERS ${STREAMING_PLAYER_UI} )

include_directories(
    include
    ui
    ${Qt5Gui_INCLUDE_DIRS}
    ${QTGSTREAMER_INCLUDES}
    ${GLIB_INCLUDE_PATH}
    ${GLIB_CONFIG_PATH}
)
add_executable(
    streamingPlayer WIN32
    main.cpp
    ${STREAMING_PLAYER_SOURCES}
    ${STREAMING_PLAYER_HEADERS}
    ${STREAMIMG_PLAYER_UI_HEADERS}
    ${STREAMING_PLAYER_RES}
)

target_link_libraries(
    streamingPlayer
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
	Qt5::WinMain
    ${QTGSTREAMER_LIBRARY}
    ${QTGSTREAMER_UI_LIBRARY}
    ${GLIB_LIBRARY}
)
