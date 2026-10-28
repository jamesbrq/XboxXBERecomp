# Grabbed by the Ghoulies (original Xbox), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always test the SDK in this checkout.
set(SDK "${CMAKE_CURRENT_SOURCE_DIR}/../..")
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game/")

# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game.toml)
    file(STRINGS config/game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat, xbe = GAME_XBE
option(GHOULIES_TOOLS_ONLY "Build SDK tools before the game's XDK tables are available" OFF)
if(NOT GHOULIES_TOOLS_ONLY)
    add_xbe_image(Ghoulies default.xbe game.toml ${GAME_PARTITIONS} game)
    # CPU conversion avoids the raw input path's geometry corruption in this title.
    target_compile_definitions(Ghoulies PRIVATE XBE_RAW_VERTICES=0)
    file(SHA1 ${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe GHOULIES_IMAGE_SHA1)
    if(GHOULIES_IMAGE_SHA1 STREQUAL "7193b60bc75240bdc0e75e184bbbb22b56e8e397")
        target_compile_definitions(Ghoulies PRIVATE GHOULIES_WALKTEST=1)
    endif()
    # The release runtime otherwise recompiles when an unchanged import library
    # leaves its native DLL older than the executable after a host-only edit.
    add_custom_target(ghoulies_test_ready ALL
        COMMAND ${CMAKE_COMMAND} -E touch $<TARGET_FILE:Ghoulies_code>
        COMMAND ${CMAKE_COMMAND} -E rm -f ${XBE_COMPILER_DIR}/work/Ghoulies/built.txt
        DEPENDS Ghoulies Ghoulies_code VERBATIM)
endif()
