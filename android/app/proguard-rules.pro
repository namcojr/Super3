# Release builds write nothing to the Android log.
-assumenosideeffects class android.util.Log {
    public static boolean isLoggable(java.lang.String, int) return false;
    public static int v(...);
    public static int d(...);
    public static int i(...);
    public static int w(...);
    public static int e(...);
    public static int wtf(...);
    public static int println(...);
}

# Native code calls into these classes through JNI by name: keep them whole.
-keep class org.libsdl.app.** { *; }
-keep class com.izzy2lost.super3.** { *; }

# Readable stack traces in crash reports.
-dontobfuscate

# Stack traces go to System.err, which ends up in the Android log.
-assumenosideeffects class java.lang.Throwable {
    public void printStackTrace();
}
