# Firebird embedded usage in an Android app using C++

This example shows how to use Firebird embedded in an Android app using C++.

The app is initialized using Kotlin and talks with a native C++ module that talks to Firebird.

As a first step, create an Android project using Android Studio.

Application has been created using `targetSdk 32` but we need to change it to `targetSdk 33`:

```diff
commit 7dfecc8af7f9f310c3f4b621c204dde01652aa82
Author: Adriano dos Santos Fernandes <adrianosf@gmail.com>
Date:   Wed Feb 15 21:42:28 2023 -0300

    Set compileSdk to 33.

diff --git a/android-cpp/app/build.gradle b/android-cpp/app/build.gradle
index 5aa45e5..10a1fdd 100644
--- a/android-cpp/app/build.gradle
+++ b/android-cpp/app/build.gradle
@@ -4,7 +4,7 @@ plugins {
 }

 android {
-    compileSdk 32
+    compileSdk 33

     defaultConfig {
         applicationId "com.example.firebirdandroidcpp"
```

Then we need to download a `Firebird embedded AAR` - an Android archive with Firebird libraries compiled to the four Android ABIs (armeabi-v7a, arm64-v8a, x86, x86_64).

Currently only Firebird 5 beta snapshots are bundled as AAR and they can be downloaded from https://github.com/FirebirdSQL/snapshots/releases/tag/snapshot-master.
Don't rely on them for important work.

We will download the file and save as `android-cpp/app/libs/Firebird-5.0.0-android-embedded.aar`.

We need to reference this file in Android app' `build.gradle` file:

```diff
commit 151d490dc86bf74990e0558117b45826e633f410
Author: Adriano dos Santos Fernandes <adrianosf@gmail.com>
Date:   Wed Feb 15 21:42:28 2023 -0300

    Reference libs/Firebird-5.0.0-android-embedded.aar.

diff --git a/android-cpp/app/build.gradle b/android-cpp/app/build.gradle
index 10a1fdd..6991758 100644
--- a/android-cpp/app/build.gradle
+++ b/android-cpp/app/build.gradle
@@ -54,4 +54,6 @@ dependencies {
     testImplementation 'junit:junit:4.13.2'
     androidTestImplementation 'androidx.test.ext:junit:1.1.5'
     androidTestImplementation 'androidx.test.espresso:espresso-core:3.5.1'
-}
+
+    implementation files('libs/Firebird-5.0.0-android-embedded.aar')
+}
```

Then we will grab Firebird's `include` directory and put in `android-cpp/app/src/main/cpp/include`.
These files are not present in the `AAR` file, so it must be get from a non-Android kit.

We are going to configure `cmake` to treat this directory as an `include` directory:

```diff
commit c8a78dbf6cebba57babf47378c8eb69529808aad
Author: Adriano dos Santos Fernandes <adrianosf@gmail.com>
Date:   Wed Feb 15 21:42:28 2023 -0300

    Add app/src/main/cpp/include to CMake include path.

diff --git a/android-cpp/app/src/main/cpp/CMakeLists.txt b/android-cpp/app/src/main/cpp/CMakeLists.txt
index d0ecd00..9233a7b 100644
--- a/android-cpp/app/src/main/cpp/CMakeLists.txt
+++ b/android-cpp/app/src/main/cpp/CMakeLists.txt
@@ -36,6 +36,10 @@ find_library( # Sets the name of the path variable.
         # you want CMake to locate.
         log)

+target_include_directories(
+        firebirdandroidcpp PUBLIC
+        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
+
 # Specifies libraries CMake should link to your target library. You
 # can link multiple libraries, such as libraries you define in this
 # build script, prebuilt third-party libraries, or system libraries.
```

We are then going to change the scaffolded app with the code we need.

In `android-cpp/app/src/main/java/com/example/firebirdandroidcpp/MainActivity.kt` we need to put our code in the `MainActivity.onCreate` method, that's going to be as this:

```kotlin
override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    FirebirdConf.extractAssets(baseContext, false)
    FirebirdConf.setEnv(baseContext)

    connect(File(filesDir, "test.fdb").absolutePath)

    binding = ActivityMainBinding.inflate(layoutInflater)
    setContentView(binding.root)

    try {
        binding.sampleText.text = getCurrentTimestamp()
    }
    catch (e: Exception) {
        binding.sampleText.text = "Error: ${e.message}"
    }
}
```

The `FirebirdConf` class (imported from `org.firebirdsql.android.embedded.FirebirdConf`) has helper functions to setup Firebird usage in Android.

`FirebirdConf.extractAssets` extracts bundled config and data files as Firebird cannot work with them bundled. `FirebirdConf.setEnv` sets necessary environment variables so Firebird know where these files are.

`connect(File(filesDir, "test.fdb").absolutePath)` is the call for the native C++ method we are going to create that connects to the database or create it when it does not exist.

The `binding.sampleText.text = getCurrentTimestamp()` line gets the `CURRENT_TIMESTAMP` using a Firebird query.

It's also good to release resources, so we are going to create an `onDestroy` method that calls our own `disconnect` native C++ method:

```kotlin
override fun onDestroy() {
    disconnect();
    super.onDestroy()
}
```

As a final step in the Kotlin file, we need to declare the external C++ methods:

```kotlin
private external fun connect(databaseName: String)
private external fun disconnect()
private external fun getCurrentTimestamp(): String
```

Now we are going to put the C++ code that is in the middle between the Kotlin app and Firebird.

This file has nothing very special. It's just standard C++ code that interfaces with JNI and also with Firebird.

Since Firebird raw API is not very ease to use, others libraries may be used too.

We start with the C++ headers:

```c++
#include <jni.h>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <dlfcn.h>
#include "firebird/Interface.h"
#include "firebird/Message.h"
```

Then some things to make easier to use the Firebird library:

```c++
namespace fb = Firebird;

using GetMasterPtr = decltype(&fb::fb_get_master_interface);

static constexpr auto LIB_FBCLIENT = "libfbclient.so";
static constexpr auto SYMBOL_GET_MASTER_INTERFACE = "fb_get_master_interface";
```

And the global variables that stores the loaded library and active connection:

```c++
static void* handle = nullptr;
static GetMasterPtr masterFunc = nullptr;
static fb::IMaster* master = nullptr;
static fb::IUtil* util = nullptr;
static fb::IProvider* dispatcher = nullptr;
static fb::IStatus* status = nullptr;
static std::unique_ptr<fb::ThrowStatusWrapper> statusWrapper;
static fb::IAttachment* attachment = nullptr;
```

The more important functions are `loadLibrary`, `unloadLibrary`, `connect`, `disconnect` and `getCurrentTimestamp`, that are the code indirectly called by the Kotlin external methods and actually interface with Firebird. These functions are coded in a way that they do not deal with JNI:

```c++
// Loads Firebird library and get main interfaces.
static void loadLibrary() {
    if (handle)
        return;

    if (!(handle = dlopen(LIB_FBCLIENT, RTLD_NOW)))
        throw std::runtime_error("Error loading Firebird client library.");

    if (!(masterFunc = (GetMasterPtr) dlsym(handle, SYMBOL_GET_MASTER_INTERFACE))) {
        dlclose(handle);
        handle = nullptr;
        throw std::runtime_error("Error getting Firebird master interface.");
    }

    master = masterFunc();
    util = master->getUtilInterface();
    dispatcher = master->getDispatcher();
    status = master->getStatus();
    statusWrapper = std::make_unique<fb::ThrowStatusWrapper>(status);
}

// Unloads Firebird library.
static void unloadLibrary() {
    if (handle) {
        dispatcher->shutdown(statusWrapper.get(), 0, fb_shutrsn_app_stopped);
        status->dispose();
        dispatcher->release();
        dlclose(handle);
        handle = nullptr;
    }
}

// Connects to Firebird database. Creates it if necessary.
static void connect(std::string databaseName) {
    loadLibrary();

    try {
        attachment = dispatcher->attachDatabase(
            statusWrapper.get(),
            databaseName.c_str(),
            0,
            nullptr);
    }
    catch (const fb::FbException&) {
        attachment = dispatcher->createDatabase(
            statusWrapper.get(),
            databaseName.c_str(),
            0,
            nullptr);
    }
}

// Disconnects the database.
static void disconnect() {
    if (attachment) {
        attachment->detach(statusWrapper.get());
        attachment->release();
        attachment = nullptr;
    }

    unloadLibrary();
}

// Query CURRENT_TIMESTAMP using a Firebird query.
static std::string getCurrentTimestamp() {
    const auto transaction = attachment->startTransaction(statusWrapper.get(), 0, nullptr);

    FB_MESSAGE(message, fb::ThrowStatusWrapper,
        (FB_VARCHAR(64), currentTimestamp)
    ) message(statusWrapper.get(), master);

    attachment->execute(
        statusWrapper.get(),
        transaction,
        0,
        "select current_timestamp from rdb$database",
        SQL_DIALECT_CURRENT,
        nullptr,
        nullptr,
        message.getMetadata(),
        message.getData());

    transaction->commit(statusWrapper.get());
    transaction->release();

    return std::string(message->currentTimestamp.str, message->currentTimestamp.length);
}
```

The functions (`Java_com_example_firebirdandroidcpp_MainActivity_*`) are the directly ones called by the Kotlin code and they deal with JNI-specific types and exceptions, passing actual work for the above functions:

```c++
// JNI JMainActivity.connect.
extern "C" JNIEXPORT
void JNICALL Java_com_example_firebirdandroidcpp_MainActivity_connect(
        JNIEnv* env, jobject self, jstring databaseName) {
    try {
        connect(convertJString(env, databaseName));
    }
    catch (...) {
        jniRethrow(env);
    }
}

// JNI JMainActivity.disconnect.
extern "C" JNIEXPORT
void JNICALL Java_com_example_firebirdandroidcpp_MainActivity_disconnect(
        JNIEnv* env, jobject self) {
    try {
        disconnect();
    }
    catch (...) {
        jniRethrow(env);
    }
}

// JNI JMainActivity.getCurrentTimestamp.
extern "C" JNIEXPORT
jstring JNICALL Java_com_example_firebirdandroidcpp_MainActivity_getCurrentTimestamp(
        JNIEnv* env, jobject self) {
    try {
        std::string currentTimestamp = getCurrentTimestamp();
        return env->NewStringUTF(currentTimestamp.c_str());
    }
    catch (...) {
        jniRethrow(env);
        return nullptr;
    }
}
```

The only missing piece of native code is the JNI helper functions.
`convertJString` converts JNI string to `std::string` and `jniRethrow` converts C++ exception to Android exceptions:

```c++
// Converts JNI string to std::string.
static std::string convertJString(JNIEnv* env, jstring str) {
    if (!str)
        return {};

    const auto len = env->GetStringUTFLength(str);
    const auto strChars = env->GetStringUTFChars(str, nullptr);

    std::string result(strChars, len);

    env->ReleaseStringUTFChars(str, strChars);

    return result;
}

// Rethrow C++ as JNI exception.
static void jniRethrow(JNIEnv* env)
{
    std::string message;

    try {
        throw;
        assert(false);
        return;
    }
    catch (const fb::FbException& e) {
        char buffer[1024];
        util->formatStatus(buffer, sizeof(buffer), e.getStatus());
        message = buffer;
    }
    catch (const std::exception& e) {
        message = e.what();
    }
    catch (...) {
        message = "Unrecognized C++ exception";
    }

    const auto exception = env->FindClass("java/lang/Exception");
    env->ThrowNew(exception, message.c_str());
}
```
