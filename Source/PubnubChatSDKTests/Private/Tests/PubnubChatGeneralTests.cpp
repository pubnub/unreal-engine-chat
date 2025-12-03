// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.General.InitChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);


bool FPubnubChatInitChatTest::RunTest(const FString& Parameters)
{
	//Initial variables - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user";
	
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	//Initialize Chat
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestTrue("InitChat succeeded", !InitResult.Result.Error);
	TestNotNull("Chat object created", InitResult.Chat);
	TestEqual("Chat object matches GetChat", ChatSubsystem->GetChat(), InitResult.Chat);
	
	//Check for any errors on the way (after chat is initialized)
	if(InitResult.Chat)
	{
		InitResult.Chat->OnConnectionStatusChangedNative.AddLambda([this](EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData)
		{
			if(Status == EPubnubChatConnectionStatus::PCCS_CONNECTION_ERROR)
			{
				AddError(FString::Printf(TEXT("Connection error: %s"), *StatusData.Reason));
			}
		});
	}
	
	//Verify current user was created
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("Current user exists", CurrentUser);
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.General.GetChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);


bool FPubnubChatGetChatTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	//GetChat should return nullptr before InitChat
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat();
	TestNull("Chat is null before InitChat", ChatBeforeInit);

	//Initialize Chat - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_get";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		return false;
	}

	//GetChat should return the chat object after InitChat
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat();
	TestNotNull("Chat exists after InitChat", ChatAfterInit);
	TestEqual("GetChat returns same object", ChatAfterInit, InitResult.Chat);

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDestroyChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.General.DestroyChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);


bool FPubnubChatDestroyChatTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	//Initialize Chat - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_destroy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		return false;
	}

	//Verify chat exists
	UPubnubChat* ChatBeforeDestroy = ChatSubsystem->GetChat();
	TestNotNull("Chat exists before destroy", ChatBeforeDestroy);

	//Destroy chat
	ChatSubsystem->DestroyChat();

	//Verify chat is destroyed
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat();
	TestNull("Chat is null after destroy", ChatAfterDestroy);

	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

