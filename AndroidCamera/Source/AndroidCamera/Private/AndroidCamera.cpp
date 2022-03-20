// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AndroidCamera.h"
#include "AndroidCameraPermission.h"
#include "AndroidCameraComponent.h"
#include "ImageFormatUtils.h"
#include "ScopedTimer.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Rendering/Texture2DResource.h"
#include <android/log.h>

#define LOG_TAG "AndroidCamera"

void  AndroidThunkCpp_Camera_Start(int DesiredWidth, int DesiredHeight)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Camera_Start", "(II)V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, DesiredWidth, DesiredHeight);
	}
}

void AndroidThunkCpp_Camera_Stop(int CameraId)
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Camera_Stop", "(I)V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, CameraId);
	}
}

extern "C" void Java_com_epicgames_ue4_GameActivity_OnCameraStart(JNIEnv * LocalJNIEnv, jobject LocalThiz, jint CameraId, jint PreviewWidth, jint PreviewHeight, jint CameraRotation)
{
	FAndroidCameraModule::Get().ActivateComponent(CameraId, PreviewWidth, PreviewHeight, CameraRotation);
}

extern "C" bool Java_com_epicgames_ue4_GameActivity_OnImageAvailable(JNIEnv * LocalJNIEnv, jobject LocalThiz,
	jint CameraId,
	jobject YByteBuffer, jint YRowStride, jint YPixelStride,
	jobject UByteBuffer, jint URowStride, jint UPixelStride,
	jobject VByteBuffer, jint VRowStride, jint VPixelStride,
	jint Width, jint Height)
{
	auto Y = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(YByteBuffer));
	auto U = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(UByteBuffer));
	auto V = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(VByteBuffer));

	if (UAndroidCameraComponent* Component = FAndroidCameraModule::Get().GetComponent(CameraId))
	{
		Component->OnImageAvailable(Y, U, V, YRowStride, URowStride, VRowStride, YPixelStride, UPixelStride, VPixelStride);
		return JNI_TRUE;
	}
	else
	{
		return JNI_FALSE;
	}
}

#endif

#define LOCTEXT_NAMESPACE "FAndroidCameraModule"

DEFINE_LOG_CATEGORY(LogCamera);

void FAndroidCameraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if PLATFORM_ANDROID
	check_android_permission("CAMERA");
#endif
}

void FAndroidCameraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	for (auto& IdComponents : IdToComponents)
	{
		UnregisterComponent(IdComponents.first);
	}
}

FAndroidCameraModule& FAndroidCameraModule::Get()
{
	return *reinterpret_cast<FAndroidCameraModule*>(FModuleManager::Get().GetModule("AndroidCamera"));
}

void FAndroidCameraModule::RegisterComponent(UAndroidCameraComponent& Component, int DesiredWidth, int DesiredHeight)
{
	PendingComponents.push(&Component);
	// TODO(dostos): Pass token to find corresponding OnCameraStart for multiple camera support 
#if PLATFORM_ANDROID
	AndroidThunkCpp_Camera_Start(DesiredWidth, DesiredHeight);
#endif

}

void FAndroidCameraModule::ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight, int CameraRotation)
{
	if (PendingComponents.size())
	{
		UAndroidCameraComponent* Component = PendingComponents.front();
		PendingComponents.pop();
		Component->ActivateComponent(CameraId, PreviewWidth, PreviewHeight, CameraRotation);
		IdToComponents.insert({ CameraId, Component });
	}
	else
	{
		UE_LOG(LogCamera, Display, TEXT("Something went wrong. Cannot find pending component for id %d"), CameraId);
	}
}

void FAndroidCameraModule::UnregisterComponent(int CameraId)
{
	if (UAndroidCameraComponent* Component = GetComponent(CameraId))
	{
		IdToComponents.erase(CameraId);
#if PLATFORM_ANDROID
		AndroidThunkCpp_Camera_Stop(CameraId);
#endif
	}
}

UAndroidCameraComponent* FAndroidCameraModule::GetComponent(int CameraId)
{
	auto it = IdToComponents.find(CameraId);
	if (it == IdToComponents.end())
	{
		UE_LOG(LogCamera, Display, TEXT("Cannot find component with id %d"), CameraId);
		return nullptr;
	}
	else
	{
		return it->second;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAndroidCameraModule, AndroidCamera)