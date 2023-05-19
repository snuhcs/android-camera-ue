// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VideoInputModule.h"

#if PLATFORM_ANDROID
#include <android/log.h>

#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Rendering/Texture2DResource.h"

#define LOG_TAG "VideoInput"

bool AndroidThunkCpp_LoadVideo(FString VideoPath) {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method = FJavaWrapper::FindMethod(
        Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_LoadVideo",
        "(Ljava/lang/String;)Z", false);
    jstring path = Env->NewStringUTF(TCHAR_TO_ANSI(*VideoPath));
    return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis,
                                           Method, path);
  }
  return false;
}

bool AndroidThunkCpp_GetNFrames(int N, int W, int H, int* outputBuffer) {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method = FJavaWrapper::FindMethod(
        Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetNFrames",
        "(ILjava/nio/ByteBuffer;)Z", false);
    jobject jArr = Env->NewDirectByteBuffer(outputBuffer, N * W * H * 4);
    bool ret = FJavaWrapper::CallBooleanMethod(
        Env, FJavaWrapper::GameActivityThis, Method, N, jArr);
    Env->DeleteLocalRef(jArr);
    return ret;
  }
  return false;
}

int AndroidThunkCpp_GetFrameCount() {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method = FJavaWrapper::FindMethod(
        Env, FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_GetFrameCount", "()I", false);
    int ret = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
                                          Method);
    return ret;
  }
  return -1;
}

int AndroidThunkCpp_GetVideoWidth() {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method = FJavaWrapper::FindMethod(
        Env, FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_GetVideoWidth", "()I", false);
    int ret = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
                                          Method);
    return ret;
  }
  return -1;
}

int AndroidThunkCpp_GetVideoHeight() {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method = FJavaWrapper::FindMethod(
        Env, FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_GetVideoHeight", "()I", false);
    int ret = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
                                          Method);
    return ret;
  }
  return -1;
}

bool AndroidThunkCpp_CloseVideo() {
  if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
    static jmethodID Method =
        FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
                                 "AndroidThunkJava_CloseVideo", "()Z", false);
    return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis,
                                           Method);
  }
  return false;
}

#endif

#define LOCTEXT_NAMESPACE "FVideoInputModule"

DEFINE_LOG_CATEGORY(LogVideo);

void FVideoInputModule::StartupModule() {
  // This code will execute after your module is loaded into memory; the exact
  // timing is specified in the .uplugin file per-module
  UE_LOG(LogVideo, Display, TEXT("VideoInputModule: StartupModule"));
}

void FVideoInputModule::ShutdownModule() {
  // This function may be called during shutdown to clean up your module.  For
  // modules that support dynamic reloading, we call this function before
  // unloading the module.
  UE_LOG(LogVideo, Display, TEXT("VideoInputModule: ShutdownModule"));
  // UnregisterComponent(IdComponents.first);
}

FVideoInputModule& FVideoInputModule::Get() {
  return *reinterpret_cast<FVideoInputModule*>(
    FModuleManager::Get().GetModule("VideoInput"));
}

bool FVideoInputModule::RegisterComponent(FString VideoPath) {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_LoadVideo(VideoPath);
#endif
  UE_LOG(LogVideo, Display, TEXT("Can't load video: Platform isn't ANDROID."));
  return false;
}

bool FVideoInputModule::UnregisterComponent() {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_CloseVideo();
#endif
  UE_LOG(LogVideo, Display, TEXT("Can't close video: Platform isn't ANDROID."));
  return false;
}

bool FVideoInputModule::CallJava_LoadVideo(FString Path) {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_LoadVideo(Path);
#endif
  UE_LOG(LogVideo, Display, TEXT("Can't load video: Platform isn't ANDROID"));
  return false;
}

int FVideoInputModule::CallJava_GetFrameCount() {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_GetFrameCount();
#endif
  UE_LOG(LogVideo, Display, TEXT("Platform isn't ANDROID"));
  return -1;
}

int FVideoInputModule::CallJava_GetVideoHeight() {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_GetVideoHeight();
#endif
  UE_LOG(LogVideo, Display, TEXT("Platform isn't ANDROID"));
  return -1;
}

int FVideoInputModule::CallJava_GetVideoWidth() {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_GetVideoWidth();
#endif
  UE_LOG(LogVideo, Display, TEXT("Platform isn't ANDROID"));
  return -1;
}

bool FVideoInputModule::CallJava_GetNFrames(int N, int W, int H,
                                            int* OutputBuffer) {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_GetNFrames(N, W, H, OutputBuffer);
#endif
  UE_LOG(LogVideo, Display,
         TEXT("Can't get n frames: Platform isn't ANDROID."));
  return false;
}

bool FVideoInputModule::CallJava_CloseVideo() {
#if PLATFORM_ANDROID
  return AndroidThunkCpp_CloseVideo();
#endif
  UE_LOG(LogVideo, Display, TEXT("Can't close video: Platform isn't ANDROID"));
  return false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVideoInputModule, VideoInput)
