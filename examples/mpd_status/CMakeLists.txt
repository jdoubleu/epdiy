cmake_minimum_required(VERSION 3.16.0)
set(EXTRA_COMPONENT_DIRS "../../src/")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mpd_status)

idf_build_get_property(IDF_VERSION_MAJOR IDF_VERSION_MAJOR)
if(IDF_VERSION_MAJOR LESS "5")
    # disable component manager to prevent ambiguity of some packages
    set(ENV{IDF_COMPONENT_MANAGER} 0)
endif()
