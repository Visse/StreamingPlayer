
set( STREAMING_PLAYER_SOURCES
   ${STREAMING_PLAYER_SOURCES}
   ${CMAKE_CURRENT_SOURCE_DIR}/StreamingPlayer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/FlowLayout.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Tile.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeSearchProvider.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeParser.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeManager.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/PlayerTab.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/PlayerDock.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailRetriver.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/GStreamerWidget.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeVideoUrlOpener.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/FeedWidget.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeVideoProvider.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/YoutubeChannelUrlOpener.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/GenericUrlOpener.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/GStreamerPipeline.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/CreateCustomFormatDialog.cpp
   PARENT_SCOPE
)
