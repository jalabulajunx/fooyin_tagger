cmake_minimum_required(VERSION 3.18)

project(
    fooyin-tagger
    VERSION 0.1.0
    DESCRIPTION "Audio Tagger Plugin for Fooyin - Wikipedia and MusicBrainz metadata lookup"
    HOMEPAGE_URL "https://github.com/fooyin/fooyin-tagger"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(Fooyin REQUIRED)
find_package(PkgConfig REQUIRED)

# TagLib for tag writing
find_package(TagLib REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/taggerplugin.cpp
    src/taggerplugin.h

    # Core
    src/core/taggingmanager.cpp
    src/core/taggingmanager.h
    src/core/httpclient.cpp
    src/core/httpclient.h
    src/core/matchingengine.cpp
    src/core/matchingengine.h

    # Sources
    src/sources/metadatasource.cpp
    src/sources/metadatasource.h
    src/sources/wikipediasource.cpp
    src/sources/wikipediasource.h
    src/sources/musicbrainzsource.cpp
    src/sources/musicbrainzsource.h

    # Models
    src/models/albummetadata.cpp
    src/models/albummetadata.h
    src/models/matchresult.cpp
    src/models/matchresult.h

    # UI
    src/ui/taggerwidget.cpp
    src/ui/taggerwidget.h
    src/ui/trackmatchdialog.cpp
    src/ui/trackmatchdialog.h

    # Settings
    src/settings/taggersettings.h
    src/settings/taggersettingspage.cpp
    src/settings/taggersettingspage.h
)

# Create plugin using Fooyin's helper function
create_fooyin_plugin(
    fooyin-tagger
    DEPENDS Fooyin::Core Fooyin::Gui Fooyin::Utils Qt6::Widgets Qt6::Network
    SOURCES ${SOURCES}
)

# Link TagLib
target_link_libraries(fooyin-tagger PRIVATE TagLib::tag)
target_compile_definitions(fooyin-tagger PRIVATE HAVE_TAGLIB)

# Copy metadata
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json"
    "${CMAKE_CURRENT_BINARY_DIR}/metadata.json"
    COPYONLY
)

# Install metadata alongside the plugin
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/metadata.json"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/fooyin/plugins"
)
