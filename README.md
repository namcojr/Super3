# SUPER3 Upstream

SUPER3 Upstream is a Sega Model 3 arcade emulator for Android. It is a fork of
[SUPER3](https://github.com/izzy2lost/Super3) by izzy2lost, which brings the
[Supermodel](https://github.com/trzy/Supermodel) emulator to Android.

This fork's main changes:

- **Up-to-date emulator core.** It runs an unmodified copy of upstream
  Supermodel's emulator code, instead of an older copy with Android changes
  mixed in.
- **Faster and smoother.** Rendering takes a third as long per frame, frame-time
  spikes are gone, and the audio stutter is fixed.
- **Better widescreen.** The 16:9 side bars no longer show stray 3D graphics
  during logos and intermission screens.
- **Frontend integration.** Games launched from ES-DE or another frontend boot
  straight into the game, and exiting returns straight to the frontend.

This app does **not** include any games, ROMs, BIOS files or other copyrighted
game content. You need to supply your own legally obtained ROMs.

## Requirements

| | |
|---|---|
| Android | 8.0 (API 26) or newer |
| CPU | 64-bit ARM (`arm64-v8a`) |
| GPU | OpenGL ES 3.2 |
| Games | Sega Model 3 ROM sets in MAME/Supermodel ZIP format; parent ROM sets are needed for clones |

Model 3 emulation is demanding. Development and testing were done on an AYN
Odin 2 Portal (Snapdragon 8 Gen 2, Adreno 740). Slower devices may need a lower
internal resolution.

## What's new in this fork

### Up-to-date Supermodel core

- `Src/` is a byte-for-byte copy of upstream Supermodel (the commit is
  recorded in [`UPSTREAM`](UPSTREAM)). All Android code lives in
  `android/app/src/main/cpp/`, so upstream fixes can be pulled in with one
  script ([`UPDATING.md`](UPDATING.md) explains the process).
- This brings in upstream's PowerPC accuracy fixes (FPU, MAME PPC fixes) and its
  idle-loop skipping, which saves CPU time while games wait for the next frame.
- 3D is drawn by upstream's own New3D renderer, running on OpenGL ES 3.2
  through a small translation layer. The fork no longer needs a separately
  maintained copy of the renderer.

### Speed and smoothness

Measured on the Odin 2 Portal running the Daytona USA 2 attract mode at
1920×1080:

| | Before | After |
|---|---|---|
| Render-thread time per frame | 10–12 ms | 3–5 ms |
| Frame-time spikes (30–46 ms) in 3 minutes | 120 | 0 |

How:

- **Vertex buffer updates:** per-frame geometry now has its own buffer. The
  Adreno driver used to copy the whole ~144 MB vertex buffer whenever part of it
  changed.
- **Texture uploads:** one upload call per rectangle instead of one per row.
  This used to be about 29,000 calls per second while textures loaded.
- **Display timing:** the game runs in the display's 60 Hz mode with
  vsync-based frame pacing. On 120 Hz screens, frames used to be paced against a
  mismatched timer.
- **Multi-threaded GPU emulation** is on by default, matching upstream.
- **Audio:** the output queue is 100 ms instead of 50 ms, which fixes a
  stutter where old audio was replayed after an under-run.

Tested at about 60 fps on the Odin 2 Portal: Daytona USA 2, Sega Rally 2,
Virtua Fighter 3, Le Mans 24, Virtua Striker 2 and Star Wars Trilogy Arcade.

### 2D improvements in 16:9 widescreen

With **Widescreen** on, Model 3 games show extra 3D picture to the left and right
of the original 4:3 frame. The 2D layers, though, only cover the 4:3 area. When a
full-screen 2D image is shown (logos, title cards, intermission screens), the
side bars used to show leftover 3D graphics next to it.

The side bars are now filled with the edge colours of that 2D image, so these
screens look clean across the whole display. The switch happens only after the
image has been stable for 3 frames, so the bars don't flicker. During normal 3D
gameplay you still get the full widescreen view.

### Frontend integration (ES-DE)

When a frontend launches a ROM (an Android `VIEW` intent with the ZIP's URI):

- SUPER3 boots straight into the game, showing only a short loading screen
  while the ROMs are prepared. The game list is not shown.
- Exiting the game ("Exit game" in the in-game menu) closes the emulator and
  returns to the frontend right away.
- If a game is still running in the background when a new one is launched, the
  old one is shut down first.
- If a ROM is missing or can't be opened, a message is shown and control goes
  back to the frontend.

Opening SUPER3 from the app drawer still shows the game list, as before.

**ES-DE setup:** this fork's package name is `com.namco.super3`, so it can be
installed alongside the original SUPER3 (`com.izzy2lost.super3`). To use it in
ES-DE, add it to `ES-DE/custom_systems/es_find_rules.xml`:

```xml
<?xml version="1.0"?>
<ruleList>
	<emulator name="SUPER3">
		<rule type="androidpackage">
			<entry>com.namco.super3/com.izzy2lost.super3.MainActivity</entry>
			<entry>com.izzy2lost.super3/com.izzy2lost.super3.MainActivity</entry>
		</rule>
	</emulator>
</ruleList>
```

Then pick SUPER3 as the emulator for the Model 3 system in ES-DE.

### Compatibility note: save states

Save states use upstream Supermodel's current format (version 6). Save states
made with the original SUPER3 can't be loaded. In-game progress kept in NVRAM
is not affected.

## Features (from SUPER3)

- On-screen touch controls (Service/Test/Coin/Start and game-specific controls)
- Virtual steering wheel and optional gyro steering for driving games
- Physical controller support and per-game controller mapping
- Save states (10 slots) with screenshot thumbnails
- Game flyers (front/back) in the game list
- Supermodel.ini editor, with a simple settings view and a raw text view

## Setup

1. Install the APK from this repository's Releases page.
2. On first start, choose a **games folder** (where your ROM ZIPs are) and a
   **data folder** (where saves, NVRAM and settings are kept).
3. Tap a game to play it, or launch it from your frontend.

### Video settings

Open the side menu in the game list and look under **Video**:

- **Resolution:** native (496×384) or integer scales from 2× to 8×. Higher is
  sharper but slower.
- **Match device resolution:** renders at your screen's exact resolution and
  turns on Widescreen and Wide background.
- **Widescreen:** widens the horizontal field of view (Supermodel
  `-wide-screen`). At very wide aspect ratios some games may cut off objects at
  the edges.
- **Wide background:** stretches the 2D background layers to the widescreen
  width (Supermodel `-wide-bg`).

### Files and storage

- Save states and thumbnails are stored under the app's data directory in
  `Saves/` (for example `Android/data/com.namco.super3/files/super3/Saves/`)
  and copied to your data folder when a game closes.
- Folder access uses Android's Storage Access Framework, so the app can only
  reach the folders you choose.

## Building from source

Requirements: JDK 21, Android SDK 36, NDK 29.0.14206865. CMake downloads SDL2
2.32.10 and SDL_net 2.2.0 during the build.

```sh
cd android
./gradlew assembleRelease
```

For a signed release build, create `android/key.properties` with
`storeFile`, `storePassword`, `keyAlias` and `keyPassword`. Without that file,
the release APK is unsigned.

To update the emulator core to the latest upstream Supermodel, see
[`UPDATING.md`](UPDATING.md).

## Credits and copyright

This program is free software, licensed under the
[GNU General Public License v3.0](LICENSE). It comes with no warranty.

- **Supermodel**: Copyright © 2003–2026 The Supermodel Team: Bart
  Trzynadlowski, Nik Henson, Ian Curtis and contributors.
  <https://github.com/trzy/Supermodel> · <https://www.supermodel3.com>.
  The full acknowledgements are in [`Docs/README.txt`](Docs/README.txt).
- **SUPER3** (Android port, UI, touch controls, save-state UI, flyers):
  izzy2lost. <https://github.com/izzy2lost/Super3>
- **model3emu-code-sinden, ARM branch**: DirtBagXon, whose ARM work SUPER3
  builds on. <https://github.com/DirtBagXon/model3emu-code-sinden/tree/arm>
- **This fork** (upstream core sync, OpenGL ES 3.2 port of New3D, performance
  work, widescreen side bars, frontend integration): released under the same
  GPL v3.0 licence.

Third-party code included in the app:

| Component | Licence |
|---|---|
| [SDL2](https://github.com/libsdl-org/SDL) and [SDL_net](https://github.com/libsdl-org/SDL_net) | zlib |
| Musashi 68K core, by Karl Stenerud | GPL v3.0, with the author's permission |
| [ymfm](https://github.com/aaronsgiles/ymfm), by Aaron Giles | BSD 3-Clause |
| [TinyXML-2](https://github.com/leethomason/tinyxml2) | zlib |
| zlib / minizip | zlib |
| [minimp3](https://github.com/lieff/minimp3) | CC0 |
| [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) | zlib |
| AndroidX, Material Components for Android | Apache 2.0 |

## Legal

- **No games included:** this app ships with no ROMs or game data and is not
  meant to help with piracy. Only use ROMs you are legally allowed to use.
- **Trademarks:** "SEGA", "Model 3" and all game names are trademarks of their
  respective owners.
- **Not affiliated:** this project is not affiliated with or endorsed by SEGA,
  the Supermodel Team or izzy2lost.
- **Network:** emulation works offline. The flyer view may download artwork
  from GitHub.
