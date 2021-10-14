# Licensed to the LF AI & Data foundation under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required( VERSION 3.18 )
project(wrapper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include( ExternalProject )
set( ROCKSDB_VERSION "6.15.2" )
set( ROCKSDB_SOURCE_URL
    "https://github.com/facebook/rocksdb/archive/v${ROCKSDB_VERSION}.tar.gz")

if( CUSTOM_THIRDPARTY_DOWNLOAD_PATH )
    set( THIRDPARTY_DOWNLOAD_PATH ${CUSTOM_THIRDPARTY_DOWNLOAD_PATH} )
else()
    set( THIRDPARTY_DOWNLOAD_PATH ${CMAKE_BINARY_DIR}/3rdparty_download/download )
endif()
message( STATUS "Thirdparty downloaded file path: ${THIRDPARTY_DOWNLOAD_PATH}" )

#-----------------------Using ccache if possible------------
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    message(STATUS "Using ccache: ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
    set(ENV{CCACHE_COMMENTS} "1")
endif (CCACHE_FOUND)

macro( build_rocksdb )
    message( STATUS "Building ROCKSDB-${ROCKSDB_VERSION} from source" )

    set( ROCKSDB_CMAKE_ARGS
         "-DWITH_GFLAGS=OFF"
         "-DWITH_TESTS=OFF"
         "-DWITH_TOOLS=OFF"
         "-DROCKSDB_INSTALL_ON_WINDOWS=ON"
         "-DROCKSDB_BUILD_SHARED=OFF"
         "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
         #This is used to solve 'illegal instruction' problem in some machine
         "-DPORTABLE=ON"
        )
    if (MSYS)
        # https://github.com/msys2/MINGW-packages/tree/8ba3ad86277253608d5afec724251419cdb4002a/mingw-w64-rocksdb
        set( ROCKSDB_PATCH_COMMAND
             patch -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/build_rocksdb_for_msys.patch
            )
    endif (MSYS)

    ExternalProject_Add(
            rocksdb-ep
            PREFIX              ${CMAKE_BINARY_DIR}/3rdparty_download/rocksdb-subbuild
            BINARY_DIR          rocksdb-bin
            DOWNLOAD_DIR        ${THIRDPARTY_DOWNLOAD_PATH}
            INSTALL_DIR         ${CMAKE_INSTALL_PREFIX}
            URL                 ${ROCKSDB_SOURCE_URL}
            URL_MD5             "67f9e04fda62af551dd039c37b1808ac"
            CMAKE_ARGS          ${ROCKSDB_CMAKE_ARGS}
            INSTALL_COMMAND     make install
            PATCH_COMMAND       "${ROCKSDB_PATCH_COMMAND}"
            )

    ExternalProject_Get_Property( rocksdb-ep INSTALL_DIR )
    ExternalProject_Get_Property( rocksdb-ep BINARY_DIR )

    if( NOT IS_DIRECTORY ${INSTALL_DIR}/include )
        file( MAKE_DIRECTORY "${INSTALL_DIR}/include" )
    endif()

endmacro()

build_rocksdb()
