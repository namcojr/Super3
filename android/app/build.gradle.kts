import java.util.Properties

plugins {
  id("com.android.application")
  id("org.jetbrains.kotlin.android")
}

val keystorePropertiesFile = rootProject.file("key.properties")
val keystoreProperties = Properties()
val hasKeystoreProperties = keystorePropertiesFile.exists()

if (hasKeystoreProperties) {
  keystorePropertiesFile.inputStream().use { keystoreProperties.load(it) }
}

val hasReleaseKeystore = hasKeystoreProperties &&
  listOf("storeFile", "storePassword", "keyAlias", "keyPassword").all {
    !keystoreProperties.getProperty(it).isNullOrBlank()
  }

android {
  namespace = "com.izzy2lost.super3"
  compileSdk = 36

  buildFeatures {
    buildConfig = true
  }
  buildToolsVersion = "36.1.0"
  ndkVersion = "29.0.14206865"

  defaultConfig {
    // Own app ID: installs alongside SUPER3 from the Play Store (code namespace stays com.izzy2lost.super3)
    applicationId = "com.namco.super3"
    minSdk = 26
    targetSdk = 36

    versionCode = 24
    versionName = "1.3.0"

    // Off: nothing is written to the Android log (Super3App, proguard-rules.pro, CMakeLists.txt)
    buildConfigField("boolean", "LOGGING", "false")

    ndk {
      abiFilters += listOf("arm64-v8a")
    }

    externalNativeBuild {
      cmake {
        cppFlags += listOf("-fexceptions", "-frtti")
      }
    }
  }

  signingConfigs {
    if (hasReleaseKeystore) {
      create("release") {
        storeFile = file(keystoreProperties.getProperty("storeFile"))
        storePassword = keystoreProperties.getProperty("storePassword")
        keyAlias = keystoreProperties.getProperty("keyAlias")
        keyPassword = keystoreProperties.getProperty("keyPassword")
      }
    }
  }

  buildTypes {
    release {
      // R8 strips every android.util.Log call (proguard-rules.pro); native logging is off by default (CMakeLists.txt)
      isMinifyEnabled = true
      proguardFiles(
        getDefaultProguardFile("proguard-android-optimize.txt"),
        "proguard-rules.pro"
      )
      if (hasReleaseKeystore) {
        signingConfig = signingConfigs.getByName("release")
      }
    }
    // Personal build: optimized like release, debug-key signed. Name comes from src/personal/res.
    // -Pmigrate makes it debuggable, so settings can be copied with "adb shell run-as".
    create("personal") {
      initWith(getByName("release"))
      versionNameSuffix = "-personal"
      isDebuggable = project.hasProperty("migrate")
      // Keeps logging: fps/frame-time lines and shader errors in logcat (see UPDATING.md)
      buildConfigField("boolean", "LOGGING", "true")
      isMinifyEnabled = false
      externalNativeBuild {
        cmake {
          arguments += "-DSUPER3_LOGGING=ON"
        }
      }
      signingConfig = signingConfigs.getByName("debug")
      matchingFallbacks += listOf("release")
    }
  }

  externalNativeBuild {
    cmake {
      path = file("src/main/cpp/CMakeLists.txt")
      version = "3.30.3"
    }
  }

  packaging {
    resources.excludes += setOf(
      "**/*.md",
      "META-INF/LICENSE*",
      "META-INF/NOTICE*"
    )
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
  }
}

kotlin {
  compilerOptions {
    jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_21)
  }
}

dependencies {
  implementation("androidx.core:core-ktx:1.15.0")
  implementation("androidx.appcompat:appcompat:1.7.0")
  implementation("androidx.constraintlayout:constraintlayout:2.1.4")
  implementation("androidx.documentfile:documentfile:1.0.1")
  implementation("androidx.recyclerview:recyclerview:1.4.0")
  implementation("com.google.android.material:material:1.14.0-alpha07")
}
