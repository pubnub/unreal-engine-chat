// Copyright 2024 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "FPubnubChatSDKModule"

void FPubnubChatSDKModule::StartupModule()
{
	bool IsEditor = false;
	bool LoadDLL = true;
	FString BaseDir;
	FString LibraryPath;

#if WITH_EDITOR
	
	IsEditor = true;

#endif

	BaseDir = IsEditor? IPluginManager::Get().FindPlugin("PubnubChat")->GetBaseDir() : FPaths::ProjectDir();
	
#if PLATFORM_WINDOWS
	
	LibraryPath = IsEditor? FPaths::Combine(*BaseDir, TEXT("Source/PubnubCoreSDK/sdk/lib/Win64/pubnub-chat.dll")) :
	FPaths::Combine(*BaseDir, TEXT("Binaries/Win64/pubnub-chat.dll"));

#elif PLATFORM_MAC

	LibraryPath = IsEditor? FPaths::Combine(*BaseDir, TEXT("Source/PubnubCoreSDK/sdk/lib/macos/libpubnub-chat.dylib")) :
	FPaths::Combine(*BaseDir, TEXT("Binaries/macos/libpubnub-chat.dylib"));
	
#elif PLATFORM_LINUX

	LoadDLL = false;
	
#endif
	
	ChatSDKLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;


	if (!ChatSDKLibraryHandle && LoadDLL)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ChatSDKModuleHandle", "Failed to load pubnub chat third party library"));
	}
	
}

void FPubnubChatSDKModule::ShutdownModule()
{
	// Free the dll handle
	if(ChatSDKLibraryHandle)
	{
		FPlatformProcess::FreeDllHandle(ChatSDKLibraryHandle);
		ChatSDKLibraryHandle = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPubnubChatSDKModule, PubnubChatSDK)
