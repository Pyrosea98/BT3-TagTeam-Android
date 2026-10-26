# The engine bridge and SDL's Java side are called from native code (JNI).
-keep class com.dfdx047.dragonrage.engine.** { *; }
-keep class org.libsdl.app.** { *; }

# Commons Compress (zip reading, data/FileUtils.kt): its optional codecs (Zstandard, XZ, Brotli...) are not shipped
-dontwarn com.github.luben.zstd.**
-dontwarn org.tukaani.xz.**
-dontwarn org.brotli.dec.**
