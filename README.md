# iPSX2

A PlayStation 2 emulator for iPhone and iPad, built on PCSX2 / ARMSX2 with a JIT recompiler for arm64.

[![Release](https://img.shields.io/github/v/release/iPSX2/iPSX2?include_prereleases&label=release)](https://github.com/iPSX2/iPSX2/releases)
[![Downloads](https://img.shields.io/github/downloads/iPSX2/iPSX2/total?label=downloads)](https://github.com/iPSX2/iPSX2/releases)
[![Stars](https://img.shields.io/github/stars/iPSX2/iPSX2?style=flat&label=stars)](https://github.com/iPSX2/iPSX2/stargazers)
[![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](COPYING.GPLv3)
![Platform](https://img.shields.io/badge/platform-iOS%2018%2B%20%C2%B7%20arm64-lightgrey)

| Branch | Build | Last commit | What it is |
|---|---|---|---|
| [`main`](https://github.com/iPSX2/iPSX2/tree/main) | [![main](https://github.com/iPSX2/iPSX2/actions/workflows/build-ipa.yml/badge.svg?branch=main)](https://github.com/iPSX2/iPSX2/actions/workflows/build-ipa.yml?query=branch%3Amain) | ![main](https://img.shields.io/github/last-commit/iPSX2/iPSX2/main?label=) | Tested on a device. Releases are tagged from here. |
| [`dev`](https://github.com/iPSX2/iPSX2/tree/dev) | [![dev](https://github.com/iPSX2/iPSX2/actions/workflows/build-ipa.yml/badge.svg?branch=dev)](https://github.com/iPSX2/iPSX2/actions/workflows/build-ipa.yml?query=branch%3Adev) | ![dev](https://img.shields.io/github/last-commit/iPSX2/iPSX2/dev?label=) | Work in progress, waiting for device testing. May break. |

**Website:** [ipsx2.github.io](https://ipsx2.github.io) · **Downloads:** [Releases](../../releases)

This is a standalone fork of [Thiago-Heleno/iPSX2-src](https://github.com/Thiago-Heleno/iPSX2-src). It keeps the JIT core and focuses on performance, accuracy fixes, and a more complete frontend.

> iPSX2 does not include a PS2 BIOS or any games. You need to dump them from hardware you own.

## Requirements

- iPhone or iPad with an arm64 chip, iOS 18 or newer (tested on iOS 18 and iOS 26)
- A way to sideload. A free Apple developer account works; it gives a 7-day certificate with `get-task-allow`, which JIT needs.
- [StikDebug](https://github.com/StephenDev0/StikDebug) to enable JIT. Use the **legacy** script for iPSX2.
- A PS2 BIOS dump

Without JIT the emulator falls back to the interpreter, which is far too slow for most games.

## What's different in this fork

**Emulation fixes**

- **God of War II screen-shake flashes fixed.** The EE FPU recompiler stored the ACC register number in the same field as the destination register, so `MADD.S`/`MSUB.S` read and wrote the wrong register whenever ACC was still cached from an earlier `ADDA.S`/`MULA.S`.
- The fastmem slow path now saves and restores full 128-bit vector registers. Before, it dropped the upper half of cached VU0 and 128-bit GPR values.
- `SQRT.S` now really rounds to nearest, as PCSX2 intends.
- A Metal convert pipeline (`RGB5A1_TO_8I`) is now built with the right attachment format.
- The IOP `printf` emulation no longer writes one byte past its format buffer.
- **microVU:** C calls made from inside a VU block (XGKICK, branch compilation) now preserve the status-flag registers and whole 128-bit vector registers. Before, flags could come back as garbage and the z/w lanes of cached registers and the P register were zeroed on every XGKICK.
- **microVU COP2 mode:** VI registers shared with the EE recompiler now use the same host register on both sides. The VU side used the EE's slot number as a physical register, so integer VU0 instructions run from EE code could read and write the wrong register.
- VU0 reading VU1's registers now reaches them instead of its own memory.
- `SQRT.S`/`RSQRT.S` in full FPU mode round to nearest as well.

**Save states work**

- Saving used to fail every time: the thumbnail could not be encoded on iOS, and that discarded the whole state. Thumbnails are now encoded with ImageIO, and a state is saved even if its thumbnail cannot be made.
- PNG and JPEG images (save-state thumbnails, game covers) can be decoded and encoded; the ImageIO codecs were built but never registered.

**Settings that now actually apply**

- **Aspect Ratio** — the core reads it by name, but the app saved a number, which the core rejected and replaced with 16:9. It now takes effect, globally and per game.
- **Deinterlace** — every choice used to run the mode next to it ("Adaptive" was really Blend BFF). The picker now lists the core's ten modes in the right order and defaults to Automatic. If you picked a mode before, check it once.
- The Shade Boost **Gamma** slider is gone: this core has no gamma control, so it never did anything.

**Frontend** (features ported from the otti83 build)

- Game list with folders, covers, custom titles and play time.
- Per-game Tweaks that follow the global setting until you override them.
- Patches & Cheats per game: the built-in patches from `patches.zip`, your own `.pnach` files in `Documents/patches`, and cheat codes from `Documents/cheats`, each listed separately.
- **Find Cheats Online:** when you open a game's Patches & Cheats, iPSX2 checks [pcsx2_cheats_collection](https://github.com/xs1l3n7x/pcsx2_cheats_collection) for that game and offers to download its codes. Nothing from the collection is bundled, and it can be turned off in Settings.
- Save-state slots, speed control (fast-forward, slow motion, frame advance) and volume control.
- A CPU engine picker (JIT / Interpreter) that writes the settings this core actually reads. Fastmem is on by default.
- Virtual sticks respond to where your thumb is, so touching the rim moves straight away and circular motions register every direction.
- The landscape pad-layout editor matches the in-game layout exactly.

**Performance options**

- **Emulator → Experimental:** MTVU (multi-threaded VU1), off by default.
- **Emulator → Speedhacks:** EE Cycle Skip (0–3), with the EE Cycle Rate description corrected.
- **Graphics → Performance:** Skip Duplicate Frames and Hardware Download Mode.

## Installing

Download the unsigned `.ipa` from the [Releases](../../releases) page, or build one yourself (see below).

1. Sign and install the `.ipa` with your tool of choice (Sideloadly, AltStore, SideStore, plumesign, …).
2. Open iPSX2 once so its Documents folder is created, then add your BIOS and games through the Files app or the in-app importer.
3. Enable JIT with StikDebug using the **legacy** script, then start a game.

## Building

CMake generates an Xcode project; compilation is done by Xcode. The easiest route is GitHub Actions:

1. Fork this repository (or push to your own copy).
2. Open the **Actions** tab, choose **Build Unsigned IPA** and click **Run workflow**. Keep the runner on `macos-latest`; if the job sits in the queue and is cancelled ("not acquired by Runner"), GitHub's Apple Silicon runners are out of capacity, so run it again with `macos-15-intel`.
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

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=iPSX2/iPSX2&type=Date)](https://star-history.com/#iPSX2/iPSX2&Date)

## Credits

- [PCSX2](https://github.com/PCSX2/pcsx2) team, for the emulator this is built on
- [ARMSX2](https://github.com/ARMSX2/ARMSX2), for the arm64 port
- [Thiago-Heleno](https://github.com/Thiago-Heleno/iPSX2-src), for the original iOS port with JIT
- [otti83](https://github.com/otti83), for their [iPSX2](https://github.com/otti83/iPSX2) frontend features ported here
- [xs1l3n7x](https://github.com/xs1l3n7x/pcsx2_cheats_collection), for the cheat collection the app can download from
- Third-party libraries vendored under `cpp/3rdparty/` (SDL3, VIXL, fmt, imgui, rapidyaml, libchdr, libzip, cubeb, rcheevos, freetype, soundtouch, lz4, lzma, zstd), each under its own license

## License

GPL-3.0-or-later, the same as PCSX2. See [COPYING.GPLv3](COPYING.GPLv3).

PlayStation and PS2 are trademarks of Sony Interactive Entertainment. This project is not affiliated with or endorsed by Sony.
