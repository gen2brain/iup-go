# ProGuard / R8 rules shipped with the IUP library (consumer-side).
#
# The C driver calls into these classes and methods by name via JNI, so R8
# must not rename, remove, or inline them. Everything under io.github.gen2brain.iupgo
# is load-bearing surface, keep all of it.

-keep class io.github.gen2brain.iupgo.** { *; }
-keep interface io.github.gen2brain.iupgo.** { *; }

# JNI entry points registered via RegisterNatives are discovered at link time
# by symbol name (Java_io_github_gen2brain_iupgo_*). R8's -keepclasseswithmembernames
# already preserves native method signatures, but be explicit:
-keepclasseswithmembers class * {
    native <methods>;
}

# Keep line numbers in shrunk stack traces, helps logcat triage.
-keepattributes SourceFile,LineNumberTable

# Strip verbose/debug logs in consumer release builds.
-assumenosideeffects class android.util.Log {
    public static *** v(...);
    public static *** d(...);
}
