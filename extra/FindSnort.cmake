## Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License Version 2 as
## published by the Free Software Foundation.  You may not use, modify or
## distribute this program under any other version of the GNU General
## Public License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#
#
#  Locate DAQ library
#  This module defines
#  DAQ_FOUND, if false, do not try to link to Lua
# 
#  SNORT_FOUND - Found Snort
#  SNORT_EXECUTABLE - The Snort++ executable
#  SNORT_INCLUDE_DIR - Snort include directory
#

set(ERROR_MESSAGE
    "
    Unable to find Snort.  Either
    
    1)  Using ccmake, manually set the cmake variables
        SNORT_INCLUDE_DIR and SNORT_EXECUTABLE.

    2)  run cmake with the following arguments
          -DSNORT_INCLUDE_DIR:PATH=/full/path/to/snort/include/dir
          -DSNORT_EXECUTABLE:PATH=/full/path/to/snort/binary

    3)  set the environment variable SNORT_DIR to the root
        root installation directory.

    4)  Find the file snort.cmake.  Manually set set the
        variable SNORT_IMPORT_FILE using either ccmake or the
        command line (-DSNORT_IMPORT_FILE=/full/install/path/lib/snort/snort.cmake)

    5)  install pkg-config and add snort.pc to the PKG_CONFIG_PATH
            environment variable.

    "
)


# First, lets try to import the Snort
find_file (SNORT_IMPORT_FILE
    NAMES snort.cmake
    HINTS ENV SNORT_DIR
    PATH_SUFFIXES lib lib/snort snort
)

if (SNORT_IMPORT_FILE)
    include (${SNORT_IMPORT_FILE})

    if (NOT SNORT_EXECUTABLE)
        get_target_property(tmp_exe snort LOCATION)
        set(SNORT_EXECUTABLE "${tmp_exe}" CACHE FILEPATH "Snort executable" FORCE)
    endif()

    if (NOT SNORT_INCLUDE_DIR)
        get_target_property(tmp_exe snort  INTERFACE_INCLUDE_DIRECTORIES)
        set(SNORT_INCLUDE_DIR "${tmp_exe}" CACHE FILEPATH "Snort executable" FORCE)
    endif()
endif(SNORT_IMPORT_FILE)



find_package(PkgConfig QUIET)

if (PKG_CONFIG_FOUND)
    pkg_check_modules(SNORT_PKG snort)

    if (SNORT_PKG_FOUND)

        #  add Snort link flags
        string(REPLACE ";" " " tmp_cflags "${SNORT_PKG_CFLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${tmp_cflags}" CACHE STRING
            "Flags used by the compiler during all build types." FORCE)

        #  add Snort link flags
        string(REPLACE ";" " " tmp_lflags "${SNORT_PKG_LDFLAGS}")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${tmp_lflags}"
            CACHE STRING "Flags used by the linker." FORCE)

        if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
            set (CMAKE_INSTALL_PREFIX "${SNORT_PKG_PREFIX}" CACHE PATH
                "Install path prefix, prepended onto install directories." FORCE)
        endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


    endif (SNORT_PKG_FOUND)
endif (PKG_CONFIG_FOUND)


find_path (SNORT_INCLUDE_DIR
    NAMES main/snort_types.h
    HINTS ${SNORT_PKG_INCLUDE_DIRS} ENV SNORT_DIR
    PATH_SUFFIXES snort include/snort
)

find_program (SNORT_EXECUTABLE
    NAMES snort
    HINTS ${SNORT_PKG_PREFIX} ENV SNORT_DIR
    PATH_SUFFIXES bin   # necessary when SNORT_DIR is set
)



include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNORT
    REQUIRED_VARS SNORT_INCLUDE_DIR SNORT_EXECUTABLE
    FAIL_MESSAGE "${ERROR_MESSAGE}"
)


mark_as_advanced(
    SNORT_INCLUDE_DIR
    SNORT_EXECUTABLE
    SNORT_IMPORT_FILE
)


