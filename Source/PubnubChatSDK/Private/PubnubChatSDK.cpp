// Copyright 2024 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "FPubnubChatSDKModule"

void FPubnubChatSDKModule::StartupModule()
{
}

void FPubnubChatSDKModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPubnubChatSDKModule, PubnubChatSDK)
