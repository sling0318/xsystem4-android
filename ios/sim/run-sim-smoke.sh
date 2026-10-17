#!/bin/bash
# Build the iOS smoke app for the simulator, boot a simulator, run it with the
# console attached and take a screenshot.  Fails unless the app prints
# "SMOKE: OK", i.e. an ES 3.0 context came up, the engine's own shaders
# compiled, FreeType read a glyph out of the bundle and the sandbox writable
# path worked.
#
#   bash ios/sim/run-sim-smoke.sh

set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
build=$root/build-ios-sim
stage=$build/stage
app=$build/RanceSmoke.app
bundle=technology.haniwa.xsystem4.smoke
IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-15.0}
target=arm64-apple-ios$IOS_DEPLOYMENT_TARGET-simulator

sdk=$(xcrun --sdk iphonesimulator --show-sdk-path)
clang=$(xcrun --sdk iphonesimulator --find clang)

echo "::group::Cross build for the simulator"
cmake -B "$build" -S "$root/ios" -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$root/ios/toolchain.cmake" \
  -DIOS_PLATFORM=SIMULATOR \
  -DIOS_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET"
cmake --build "$build"
echo "::endgroup::"

echo "::group::Assemble $app"
rm -rf "$app"
mkdir -p "$app/shaders" "$app/fonts" "$app/licenses"

# The engine archives would bring their own main(); the smoke app owns it.
libs=""
for a in $(find "$build" -name '*.a' \
             -not -name libxsystem4.a -not -name libsys4.a | sort); do
  libs="$libs $a"
done

frameworks=""
for f in UIKit Foundation CoreGraphics QuartzCore OpenGLES CoreMotion \
         AudioToolbox AVFoundation AVFAudio CoreAudio Metal GameController \
         CoreVideo IOKit CoreFoundation; do
  frameworks="$frameworks -Wl,-framework,$f"
done

# SDL2 and FreeType are built inside the engine's build tree and are not
# installed there, so locate the header directories instead of guessing them.
find_header() {
  local hit
  hit=$(find "$stage" "$build" -name "$1" -print -quit 2>/dev/null || true)
  if [ -z "$hit" ]; then
    echo "could not locate header $1" >&2
    exit 1
  fi
  dirname "$hit"
}
sdl_inc=$(find_header SDL.h)
sdl_cfg=$(find_header SDL_config.h)
ft_inc=$(find_header ft2build.h)
ft_gen=$(dirname "$(dirname "$(find_header ftoption.h)")")
echo "includes: $sdl_inc $sdl_cfg $ft_inc $ft_gen"

"$clang" -target "$target" -isysroot "$sdk" -O1 -g -Wno-deprecated-declarations \
  -I "$sdl_inc" -I "$sdl_cfg" -I "$ft_inc" -I "$ft_gen" \
  -I "$root/ios/shim" \
  -o "$app/smoke" "$root/ios/sim/smoke.c" \
  $libs $frameworks -liconv

cp "$root/xsystem4/shaders/"*.glsl "$app/shaders/"
cp "$root/xsystem4/fonts/VL-Gothic-Regular.ttf" "$app/fonts/"
cp "$root/ios/sim/Info.plist" "$app/Info.plist"
codesign --force --sign - "$app"
echo "bundle size: $(du -sh "$app" | cut -f1)"
echo "::endgroup::"

echo "::group::Boot simulator"
udid=$(xcrun simctl list devices available -j | node -e "
let d='';process.stdin.on('data',c=>d+=c).on('end',()=>{
  const j=JSON.parse(d).devices;
  let out='';
  for (const rt of Object.keys(j).filter(k=>k.includes('iOS')).sort())
    for (const dev of j[rt])
      if (dev.isAvailable && /^iPhone /.test(dev.name)) out=dev.udid;
  if (!out) throw new Error('no iPhone simulator is available');
  console.log(out);
})")
xcrun simctl boot "$udid" || true
xcrun simctl bootstatus "$udid" -b
xcrun simctl install "$udid" "$app"
echo "udid=$udid"
echo "::endgroup::"

( sleep 10; xcrun simctl io "$udid" screenshot "$build/smoke.png" ) &
screenshot=$!

echo "::group::Run smoke app"
set +e
xcrun simctl launch --console-pty "$udid" "$bundle" 2>&1 | tee "$build/smoke.log"
launch_status=${PIPESTATUS[0]}
set -e
wait $screenshot || echo "screenshot failed"
echo "launch exit=$launch_status"
echo "::endgroup::"

xcrun simctl terminate "$udid" "$bundle" 2>/dev/null || true
xcrun simctl shutdown "$udid" || true

grep -q "SMOKE: OK" "$build/smoke.log"
echo "simulator smoke test OK"
