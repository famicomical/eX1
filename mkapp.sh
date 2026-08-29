#!/bin/sh
# mkapp.sh - wrap a built variant binary into a macOS .app bundle.
#
#   ./mkapp.sh x1turbo        # -> build/eX1turbo.app
#
# Needs the binary already built (make <variant>), ImageMagick (`magick`,
# for nearest-neighbor icon upscaling) and iconutil (ships with macOS).
# Bundled apps load ROMs and save config in the shared
# ~/Library/Application Support/eX1/ (created on first run).
set -e

variant=$1
case "$variant" in
	x1)       name=eX1;;
	x1turbo)  name=eX1turbo;;
	x1turboz) name=eX1turboZ;;
	*) echo "usage: $0 x1|x1turbo|x1turboz" >&2; exit 1;;
esac

cd "$(dirname "$0")"
bin=build/e$variant
ico=src/res/$variant.ico
[ -f "$bin" ] || { echo "$bin not built - run: make $variant" >&2; exit 1; }

# --- icon: .ico (32x32) -> .icns, nearest-neighbor upscale to keep pixels crisp
icns=build/icons/$variant.icns
if [ ! -f "$icns" ] || [ "$ico" -nt "$icns" ]; then
	command -v magick >/dev/null || { echo "ImageMagick required: brew install imagemagick" >&2; exit 1; }
	iconset=build/icons/$variant.iconset
	rm -rf "$iconset"
	mkdir -p "$iconset"
	for s in 16 32 128 256 512; do
		d=$((s * 2))
		magick "$ico" -filter point -resize ${s}x${s} "$iconset/icon_${s}x${s}.png"
		magick "$ico" -filter point -resize ${d}x${d} "$iconset/icon_${s}x${s}@2x.png"
	done
	iconutil -c icns -o "$icns" "$iconset"
fi

# --- bundle
app=build/$name.app
rm -rf "$app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources" "$app/Contents/Frameworks"
cp "$bin" "$app/Contents/MacOS/e$variant"
cp "$icns" "$app/Contents/Resources/AppIcon.icns"

# --- bundle SDL2: copy the linked dylib into Frameworks/ and repoint the
# binary at it, so the .app runs on machines without MacPorts/Homebrew SDL2
sdl_dylib=$(otool -L "$app/Contents/MacOS/e$variant" | awk '/libSDL2[^ ]*\.dylib/{print $1; exit}')
if [ -n "$sdl_dylib" ] && [ -f "$sdl_dylib" ]; then
	sdl_base=$(basename "$sdl_dylib")
	cp "$sdl_dylib" "$app/Contents/Frameworks/$sdl_base"
	chmod u+w "$app/Contents/Frameworks/$sdl_base"
	install_name_tool -id "@executable_path/../Frameworks/$sdl_base" \
		"$app/Contents/Frameworks/$sdl_base"
	install_name_tool -change "$sdl_dylib" \
		"@executable_path/../Frameworks/$sdl_base" \
		"$app/Contents/MacOS/e$variant"
	codesign --force -s - "$app/Contents/Frameworks/$sdl_base"
else
	echo "warning: could not locate SDL2 dylib to bundle ($sdl_dylib)" >&2
fi

cat > "$app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>e$variant</string>
	<key>CFBundleName</key>
	<string>$name</string>
	<key>CFBundleDisplayName</key>
	<string>$name</string>
	<key>CFBundleIdentifier</key>
	<string>org.cscp.sdl.$variant</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>2025.11.9</string>
	<key>CFBundleIconFile</key>
	<string>AppIcon</string>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeName</key>
			<string>Floppy Disk Image</string>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>d88</string>
			</array>
		</dict>
		<dict>
			<key>CFBundleTypeName</key>
			<string>Cassette Tape Image</string>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>cmt</string>
				<string>t77</string>
			</array>
		</dict>
	</array>
</dict>
</plist>
EOF

codesign --force -s - "$app"
echo "==> built $app"
