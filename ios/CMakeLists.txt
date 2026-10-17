# iOS cross build of the xsystem4 engine and its dependencies.
#
# Mirrors the Android super-build in ../CMakeLists.txt, but produces static
# archives only: iOS forbids loading non-system dynamic libraries.
#
#   cmake -B build-ios -S ios -GNinja -DCMAKE_TOOLCHAIN_FILE=ios/toolchain.cmake
#   cmake --build build-ios --target install
#
# The engine itself is built as an ExternalProject so that its Android-only
# CMake logic stays untouched.

cmake_minimum_required(VERSION 3.21)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

project(xsystem4-ios LANGUAGES C)

include(ExternalProject)

set(stagingDir ${CMAKE_CURRENT_BINARY_DIR}/stage)

set(COMMON_CMAKE_ARGS
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake
  -DCMAKE_FIND_ROOT_PATH=${stagingDir}
  -DIOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET}
  -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
  -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
  -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
  -DCMAKE_STAGING_PREFIX:PATH=${stagingDir}
  -DCMAKE_INSTALL_PREFIX:PATH=${stagingDir}
  # Interprocedural optimization only obscures link errors in a first port.
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL=OFF
)

ExternalProject_Add(
  zlib
  URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DZLIB_BUILD_EXAMPLES=OFF
             -DBUILD_SHARED_LIBS=OFF
)

ExternalProject_Add(
  libogg
  URL https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.xz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DCMAKE_POLICY_VERSION_MINIMUM=3.5
             -DBUILD_SHARED_LIBS=OFF
)

ExternalProject_Add(
  libvorbis
  URL https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.xz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DCMAKE_POLICY_VERSION_MINIMUM=3.5
             -DBUILD_SHARED_LIBS=OFF
)

ExternalProject_Add_StepDependencies(libvorbis configure libogg)

ExternalProject_Add(
  libjpeg-turbo
  URL https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.0/libjpeg-turbo-3.1.0.tar.gz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DENABLE_SHARED=OFF
)

ExternalProject_Add(
  libwebp
  URL https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.3.2.tar.gz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DBUILD_SHARED_LIBS=OFF
             -DWEBP_BUILD_ANIM_UTILS=OFF
             -DWEBP_BUILD_CWEBP=OFF
             -DWEBP_BUILD_DWEBP=OFF
             -DWEBP_BUILD_GIF2WEBP=OFF
             -DWEBP_BUILD_IMG2WEBP=OFF
             -DWEBP_BUILD_VWEBP=OFF
             -DWEBP_BUILD_WEBPINFO=OFF
             -DWEBP_BUILD_WEBPMUX=OFF
             -DWEBP_BUILD_EXTRAS=OFF
             -DWEBP_BUILD_WEBP_JS=OFF
)

ExternalProject_Add(
  libpng
  URL https://download.sourceforge.net/libpng/libpng-1.6.47.tar.xz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DPNG_SHARED=OFF
             -DPNG_TESTS=OFF
)

ExternalProject_Add_StepDependencies(libpng configure zlib)

ExternalProject_Add(
  libdeflate
  URL https://github.com/ebiggers/libdeflate/releases/download/v1.25/libdeflate-1.25.tar.gz
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DCMAKE_POLICY_VERSION_MINIMUM=3.5
             -DLIBDEFLATE_BUILD_SHARED_LIB=OFF
             -DLIBDEFLATE_GZIP_SUPPORT=OFF
             -DLIBDEFLATE_BUILD_GZIP=OFF
)

ExternalProject_Add(
  libsndfile
  GIT_REPOSITORY https://github.com/kichikuou/libsndfile.git
  GIT_TAG be82f4be73c77f1e759bda9dfe1625204441465a
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DBUILD_SHARED_LIBS=OFF
             -DBUILD_PROGRAMS=OFF
             -DBUILD_EXAMPLES=OFF
             -DBUILD_TESTING=OFF
             -DENABLE_CPACK=OFF
)

ExternalProject_Add_StepDependencies(libsndfile configure libvorbis libogg)

# libffi is autotools-based. Only ffi_call is used by the engine (no
# closures), so no executable memory is required and iOS's W^X rule is not
# an issue.
ExternalProject_Add(
  libffi
  URL https://github.com/libffi/libffi/releases/download/v3.4.6/libffi-3.4.6.tar.gz
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
    CC="${IOS_TARGET_CC}"
    CXX="${IOS_TARGET_CC}++"
    CPPFLAGS="-I${stagingDir}/include"
    LDFLAGS="${IOS_TARGET_CFLAGS}"
    AR=${CMAKE_AR}
    RANLIB=${CMAKE_RANLIB}
    STRIP=${CMAKE_STRIP}
    <SOURCE_DIR>/configure
    --host=aarch64-apple-ios
    --build=${IOS_HOST_TRIPLE}
    --prefix=${stagingDir}
    --disable-shared
    --disable-dependency-tracking
    --disable-builddir
    --disable-multi-os-directory
    --disable-structs
    --disable-raw-api
    --disable-docs
  BUILD_COMMAND make
  INSTALL_COMMAND make install
)

ExternalProject_Add(
  xsystem4
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../xsystem4
  CMAKE_ARGS ${COMMON_CMAKE_ARGS}
             -DCMAKE_MODULE_PATH=${CMAKE_CURRENT_LIST_DIR}/../cmake
             -DBUILD_SHARED_LIBS=OFF
  BUILD_ALWAYS YES
)

ExternalProject_Add_StepDependencies(xsystem4 configure
  zlib libjpeg-turbo libwebp libpng libffi libsndfile libdeflate)
