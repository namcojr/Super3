# Personal build: keeping up with upstream Supermodel

Branch `core-sync`. `Src/` is an unmodified copy of upstream Supermodel
(commit in `UPSTREAM`). Never edit files in `Src/`; Android code lives in
`android/app/src/main/cpp/`.

## Update to the latest upstream

    git -C ../supermodel-upstream pull
    Scripts/sync-upstream.sh ../supermodel-upstream origin/master
    cd android && JAVA_HOME=/usr/lib/jvm/java-21-openjdk ./gradlew assemblePersonal
    adb install -r app/build/outputs/apk/personal/app-personal.apk

The script lists the upstream commits since the last sync, copies `Src/`,
refreshes Games.xml/Music.xml (Model 3 games only), regenerates the Musashi
tables, `generated/UpstreamDefaultConfig.cpp` and the patched copies below.

Then run a game and check the log: `adb logcat | grep -E 'SDL/APP|SDL/RENDER'`
(`fps=` every 2 s; shader errors print with numbered source).

## Where things break, and where to fix them

| Symptom | Place |
|---|---|
| Compile error in an upstream interface (IRender2D, IEmulator, OSD) | `native-lib.cpp`, `render2d_android.cpp`, `android_osd.cpp`, `override/` |
| `GLES shader compile failed` in logcat | add a fixup to the list in `gles_compat.cpp` |
| New desktop-only GL call | `override/GL/glew.h` + `gles_compat.cpp` |
| Crash in `UpstreamDefaultConfig` / missing setting | `Scripts/sync-upstream.sh` step 4 |

## Patched upstream files

`Src/` is never edited. Where Android needs a change, the sync script builds a
patched copy in `android/app/src/main/cpp/generated/` and the build compiles
that instead. Every change is an exact text match: if upstream rewrites the
line, the sync stops with an error instead of silently dropping the patch.

| Copy | Change | Why |
|---|---|---|
| `Audio_android.cpp` | 100 ms audio queue (upstream 50), under-runs logged | Android audio bursts drained 50 ms; the sound thread then replayed old audio ("to-to" stutter) |
| `New3D_android.cpp` | per-frame geometry in its own buffer; model appends unsynchronised | updating part of the ~144 MB vertex buffer made the Adreno driver copy all of it (8-35 ms per frame) |
| `TextureBank_android.cpp` | one texture upload per rectangle instead of per row | ~29000 calls/s during texture loads |

Measured on the Odin 2 Portal (Daytona 2 attract, 1920x1080): render thread
10-12 ms -> 3-5 ms per frame, CPU-side frame spikes 120 -> 0 in 3 minutes.
Remaining dips are GPU-bound at 1080p (they vanish at 720p).

## Android-specific files

- `override/GL/glew.h`, `gles_compat.cpp`: run upstream's desktop GL 4.1 renderer on OpenGL ES 3.2
- `override/Graphics/Render2D.h`, `render2d_android.cpp`: GLES version of upstream's 2D renderer
- `override/Graphics/SuperAA.h`: no-op (no supersampling)
- `android_osd.cpp`: logcat logger, video callbacks
- `native-lib.cpp`: SUPER3's host loop, set up like upstream `Main.cpp`; vsync frame pacing at 60 Hz; `fps=` and frame-time breakdown in the log every 2 s
- `render2d_android.cpp`: also fills the widescreen side bars with the picture's edge colour when the top 2D layer covers the whole 4:3 area
- `Super3Activity.kt`: asks for the 60 Hz display mode and reports the refresh rate to native code
- Upstream used as-is: `Src/OSD/{Logger,Outputs,Thread}.cpp`, `Src/OSD/SDL/Audio.cpp`, `Src/OSD/Unix/FileSystemPath.cpp`
