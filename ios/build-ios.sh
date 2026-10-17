#!/bin/sh
# Cross-compile xsystem4 and its dependencies for arm64 iOS devices, then run a
# link-only smoke test (no device required).
#
# Run from the repository root on a macOS host:  sh ios/build-ios.sh
#
# Environment:
#   IOS_DEPLOYMENT_TARGET  minimum iOS version (default 13.0)

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
build=$root/build-ios
IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-13.0}

sdk=$(xcrun --sdk iphoneos --show-sdk-path)
clang=$(xcrun --find --sdk iphoneos clang)

echo "::group::Configure"
cmake -B "$build" -S "$root/ios" -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$root/ios/toolchain.cmake" \
  -DIOS_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET"
echo "::endgroup::"

echo "::group::Build dependencies and engine"
cmake --build "$build" --target install
echo "::endgroup::"

echo "Staged archives:"
find "$build/stage" -name '*.a' | sort

echo "::group::Link smoke test"
engine_a=$(find "$build" -name libxsystem4.a -print -quit)
sys4_a=$(find "$build" -name libsys4.a -print -quit)
if [ -z "$engine_a" ] || [ -z "$sys4_a" ]; then
  echo "engine or libsys4 archive not found" >&2
  exit 1
fi

# Every engine object is linked (force-load), so unresolved symbols anywhere in
# the engine show up here. Dependency archives are passed by reference instead,
# like a real app would, to avoid duplicate-symbol noise between them.
libs=""
for a in $(find "$build/stage" -name '*.a' | sort); do
  libs="$libs $a"
done
libs="$libs $sys4_a"

frameworks=""
for f in UIKit Foundation CoreGraphics QuartzCore OpenGLES CoreMotion \
         AudioToolbox AVFoundation Metal GameController CoreVideo IOKit \
         CoreFoundation; do
  frameworks="$frameworks -Wl,-framework,$f"
done

# main() comes from the force-loaded engine, so the smoke object stays empty.
"$clang" -target "arm64-apple-ios$IOS_DEPLOYMENT_TARGET" -isysroot "$sdk" \
  -Wl,-force_load,"$engine_a" \
  -Wl,-force_load,"$sys4_a" \
  $libs $frameworks -liconv \
  -o "$build/xsystem4-ios-linktest" \
  "$root/ios/smoke/empty.c"

file "$build/xsystem4-ios-linktest"
echo "text symbols: $(nm "$build/xsystem4-ios-linktest" | wc -l)"
echo "::endgroup::"

echo "iOS cross build OK"
