#include <jni.h>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <dlfcn.h>
#include "firebird/Interface.h"
#include "firebird/Message.h"

namespace fb = Firebird;

using GetMasterPtr = decltype(&fb::fb_get_master_interface);

static constexpr auto LIB_FBCLIENT = "libfbclient.so";
static constexpr auto SYMBOL_GET_MASTER_INTERFACE = "fb_get_master_interface";

static void* handle = nullptr;
static GetMasterPtr masterFunc = nullptr;
static fb::IMaster* master = nullptr;
static fb::IUtil* util = nullptr;
static fb::IProvider* dispatcher = nullptr;
static fb::IStatus* status = nullptr;
static std::unique_ptr<fb::ThrowStatusWrapper> statusWrapper;
static fb::IAttachment* attachment = nullptr;


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
