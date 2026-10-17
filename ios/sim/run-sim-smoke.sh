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
         CoreVideo IOKit CoreFoundation CoreHaptics; do
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

# iOS only hands a full-resolution screen to apps that ship a launch screen.
xcrun ibtool --errors --warnings --notices \
  --compile "$app/LaunchScreen.storyboardc" \
  "$root/ios/sim/LaunchScreen.storyboard"

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
# The app is landscape-only; without this the simulator stays portrait and the
# screenshot comes back rotated 90 degrees.  simctl wants the camelCase name.
xcrun simctl io "$udid" setorientation landscapeLeft || echo "setorientation failed"
xcrun simctl install "$udid" "$app"
echo "udid=$udid"
echo "::endgroup::"

echo "::group::Run smoke app"
# This simctl has no --stdout/--stderr, and --console-pty never returns after
# the app exits, so attach the console in the background, redirect in the
# shell, and kill the launcher once the app has reported its verdict.
xcrun simctl launch --console "$udid" "$bundle" > "$build/smoke.log" 2>&1 &
launcher=$!

wait_for() {
  for _ in $(seq 1 "$2"); do
    grep -q "$1" "$build/smoke.log" 2>/dev/null && return 0
    sleep 1
  done
  return 1
}

status=timeout
if wait_for "SMOKE: audio ok" 60; then
  # The app keeps its window up for several more seconds: good moment to shoot.
  sleep 1
  xcrun simctl io "$udid" screenshot "$build/smoke.png" || echo "screenshot failed"
  if wait_for "SMOKE: OK" 40; then
    status=ok
  elif grep -q "failure(s)" "$build/smoke.log"; then
    status=fail
  fi
elif wait_for "SMOKE: FAIL" 10; then
  status=fail
fi

cat "$build/smoke.log" 2>/dev/null || true
echo "result=$status"
echo "::endgroup::"

kill $launcher 2>/dev/null || true
wait $launcher 2>/dev/null || true
xcrun simctl terminate "$udid" "$bundle" 2>/dev/null || true
xcrun simctl shutdown "$udid" 2>/dev/null || true

[ "$status" = "ok" ]
echo "simulator smoke test OK"
