// Copyright 2025 PubNub Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "Engine/GameInstance.h"


FString PubnubChatTests::GetTestPublishKey()
{
	FString PublishKey = FPlatformMisc::GetEnvironmentVariable(TEXT("PUBLISH_KEY"));
	if(!PublishKey.IsEmpty())
	{
		return PublishKey;
	}
	// Fallback to demo key if environment variable is not set
	return TEXT("demo");
}

FString PubnubChatTests::GetTestSubscribeKey()
{
	FString SubscribeKey = FPlatformMisc::GetEnvironmentVariable(TEXT("SUBSCRIBE_KEY"));
	if(!SubscribeKey.IsEmpty())
	{
		return SubscribeKey;
	}
	// Fallback to demo key if environment variable is not set
	return TEXT("demo");
}


bool FPubnubChatAutomationTestBase::InitTest()
{
	//Initialize GameInstance and PubnubChatSubsystem
	GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	
	if (!TestNotNull("GameInstance exists", GameInstance))
	{return false;}

	ChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();
	if (!TestNotNull("PubnubChat Subsystem exists", ChatSubsystem))
	{return false;}

	//Until we have advanced logger, we need to disable logs, because they would make tests fail
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	return true;
}

void FPubnubChatAutomationTestBase::CleanUp()
{
	//Final clean up
	if(ChatSubsystem)
	{
		ChatSubsystem->DestroyChat();
	}
	
	if(GameInstance)
	{
		GameInstance->Shutdown();
	}

	ChatSubsystem = nullptr;
	GameInstance = nullptr;
}


#endif // WITH_DEV_AUTOMATION_TESTS

