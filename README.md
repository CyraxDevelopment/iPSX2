# iPSX2

A PlayStation 2 emulator for iPhone and iPad, built on PCSX2 / ARMSX2 with a JIT recompiler for arm64.

This is a standalone fork of [Thiago-Heleno/iPSX2-src](https://github.com/Thiago-Heleno/iPSX2-src). It keeps the JIT core and focuses on performance, accuracy fixes, and a more complete frontend.

> iPSX2 does not include a PS2 BIOS or any games. You need to dump them from hardware you own.

## Requirements

- iPhone or iPad with an arm64 chip, iOS 18 or newer (tested on iOS 18 and iOS 26)
- A way to sideload. A free Apple developer account works; it gives a 7-day certificate with `get-task-allow`, which JIT needs.
- [StikDebug](https://github.com/StephenDev0/StikDebug) to enable JIT. Use the **legacy** script for iPSX2.
- A PS2 BIOS dump

Without JIT the emulator falls back to the interpreter, which is far too slow for most games.

## What's different in this fork

**On `main`**

- Performance options exposed in Settings:
  - **Emulator → Experimental:** MTVU (multi-threaded VU1). It was force-disabled on every launch before; now you can opt in.
  - **Emulator → Speedhacks:** EE Cycle Skip (0–3). The EE Cycle Rate description, which had over/underclocking backwards, is corrected.
  - **Graphics → Performance:** Skip Duplicate Frames and Hardware Download Mode (Accurate / No Readbacks / Unsynchronized / Disabled).
- Startup no longer writes the MTVU default to a key the core never reads.
- Several JIT build fixes, and leftover diagnostic code is gated out of normal builds.

**In development (`otti83-features` branch)**

- Frontend features ported from the otti83 build:
  - game list with folders, covers, custom titles and play time
  - per-game Tweaks
  - cheats and patches, including `.pnach` import and widescreen / no-interlacing switches
  - save-state slots
  - speed control (fast-forward, slow motion, frame advance)
  - volume control
- A CPU engine picker (JIT / Interpreter) that writes the settings this core actually reads. Fastmem is on by default.
- Work on an EE FPU JIT register-allocation bug that causes blue-screen flashes during screen shake in God of War II.

These changes move to `main` and into a release once they are verified on a device.

## Installing

Releases will be published on the [Releases](../../releases) page as unsigned `.ipa` files. Until then, build one yourself (see below).

1. Sign and install the `.ipa` with your tool of choice (Sideloadly, AltStore, SideStore, plumesign, …).
2. Open iPSX2 once so its Documents folder is created, then add your BIOS and games through the Files app or the in-app importer.
3. Enable JIT with StikDebug using the **legacy** script, then start a game.

## Building

CMake generates an Xcode project; compilation is done by Xcode. The easiest route is GitHub Actions:

1. Fork this repository (or push to your own copy).
2. Open the **Actions** tab, choose **Build Unsigned IPA** and click **Run workflow**.
3. When it finishes, download the `iPSX2-unsigned-<run>` artifact and extract the `.ipa`.

To build locally on a Mac:

```sh
cmake -S cpp -B build-device -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DiPSX2_REAL_DEVICE=ON
xcodebuild -project build-device/iPSX2.xcodeproj \
  -scheme iPSX2 -sdk iphoneos -configuration Release \
  CODE_SIGNING_ALLOWED=NO archive
```

Code signing is disabled on purpose; sign the resulting app when you install it, and keep the `com.apple.security.cs.allow-jit` entitlement.

## Project layout

```
cpp/            C++ / Objective-C++ core
  pcsx2/        Emulator core (EE, IOP, VU, GS, SPU2 …)
    arm64/      arm64 JIT helpers (VIXL)
    x86/        EE / IOP recompilers, translated to arm64 via VIXL
    GS/Renderers/Metal/   Metal renderer and shaders
  ios_main.mm   App entry point
  iPSX2Bridge.* Objective-C++ bridge used by Swift
swift/          SwiftUI frontend (views, settings, game list)
```

## Credits

- [PCSX2](https://github.com/PCSX2/pcsx2) team, for the emulator this is built on
- [ARMSX2](https://github.com/ARMSX2/ARMSX2), for the arm64 port
- [Thiago-Heleno](https://github.com/Thiago-Heleno/iPSX2-src), for the original iOS port with JIT
- [otti83](https://github.com/otti83), for their [iPSX2](https://github.com/otti83/iPSX2) frontend features ported here
- Third-party libraries vendored under `cpp/3rdparty/` (SDL3, VIXL, fmt, imgui, rapidyaml, libchdr, libzip, cubeb, rcheevos, freetype, soundtouch, lz4, lzma, zstd), each under its own license

## License

GPL-3.0-or-later, the same as PCSX2. See [COPYING.GPLv3](COPYING.GPLv3).

PlayStation and PS2 are trademarks of Sony Interactive Entertainment. This project is not affiliated with or endorsed by Sony.
