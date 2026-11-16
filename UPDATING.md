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
tables and `generated/UpstreamDefaultConfig.cpp`.

Then run a game and check the log: `adb logcat | grep -E 'SDL/APP|SDL/RENDER'`
(`fps=` every 2 s; shader errors print with numbered source).

## Where things break, and where to fix them

| Symptom | Place |
|---|---|
| Compile error in an upstream interface (IRender2D, IEmulator, OSD) | `native-lib.cpp`, `render2d_android.cpp`, `android_osd.cpp`, `override/` |
| `GLES shader compile failed` in logcat | add a fixup to the list in `gles_compat.cpp` |
| New desktop-only GL call | `override/GL/glew.h` + `gles_compat.cpp` |
| Crash in `UpstreamDefaultConfig` / missing setting | `Scripts/sync-upstream.sh` step 4 |

## Android-specific files

- `override/GL/glew.h`, `gles_compat.cpp`: run upstream's desktop GL 4.1 renderer on OpenGL ES 3.2
- `override/Graphics/Render2D.h`, `render2d_android.cpp`: GLES version of upstream's 2D renderer
- `override/Graphics/SuperAA.h`: no-op (no supersampling)
- `android_osd.cpp`: logcat logger, video callbacks
- `native-lib.cpp`: SUPER3's host loop, set up like upstream `Main.cpp`
- Upstream used as-is: `Src/OSD/{Logger,Outputs,Thread}.cpp`, `Src/OSD/SDL/Audio.cpp`, `Src/OSD/Unix/FileSystemPath.cpp`
