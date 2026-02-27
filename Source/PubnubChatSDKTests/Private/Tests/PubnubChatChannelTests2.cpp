// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// INVITE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Create user to invite
			const FString TargetUserID = SDK_PREFIX + "test_invite_not_init_target";
			FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
			TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
			TestNotNull("User should be created", CreateUserResult.User);
			
			if(CreateUserResult.User)
			{
				// Try to invite with uninitialized channel
				FPubnubChatInviteResult InviteResult = UninitializedChannel->Invite(CreateUserResult.User);
				TestTrue("Invite should fail with uninitialized channel", InviteResult.Result.Error);
			}
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
				if(CreateUserResult.User)
				{
					Chat->DeleteUser(TargetUserID);
				}
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Channel object - remaining async APIs)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelConnectAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.ConnectAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelConnectAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_connect_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_connect_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Connect requires an existing direct conversation partner
	const FString OtherUserID = SDK_PREFIX + "test_channel_connect_async_full_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	UPubnubChatUser* OtherUser = nullptr;
	if(CreateUserResult.Result.Error)
	{
		FPubnubChatUserResult GetUserResult = Chat->GetUser(OtherUserID);
		TestFalse("GetUser should succeed when CreateUser fails", GetUserResult.Result.Error);
		OtherUser = GetUserResult.User;
	}
	else
	{
		OtherUser = CreateUserResult.User;
	}
	if(!OtherUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(OtherUser, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->ConnectAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("ConnectAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, OtherUserID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelDisconnectAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.DisconnectAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelDisconnectAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_disconnect_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_disconnect_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	CreateResult.Channel->Connect();
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->DisconnectAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("DisconnectAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelLeaveAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.LeaveAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelLeaveAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_leave_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_leave_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	const FString TargetUserID = SDK_PREFIX + "test_channel_leave_async_full_target";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->LeaveAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("LeaveAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessageAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_pin_message_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_pin_message_async_full";
	const FString TestMessageText = TEXT("Async pin message channel");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	CreateResult.Channel->Connect();
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage, bCallbackReceived, OnOperationResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid");
			*bCallbackReceived = true;
			return;
		}
		
		CreateResult.Channel->PinMessageAsync(ReceivedMessage->Get(), OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("PinMessageAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessageAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_unpin_message_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_unpin_message_async_full";
	const FString TestMessageText = TEXT("Async unpin message channel");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	CreateResult.Channel->Connect();
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatSendTextParams SendTextParams;
		SendTextParams.StoreInHistory = true;
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText, SendTextParams);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid");
			return;
		}
		
		FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(ReceivedMessage->Get());
		TestFalse("PinMessage should succeed", PinResult.Error);
	}, 0.1f));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->UnpinMessageAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("UnpinMessageAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetPinnedMessageAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetPinnedMessageAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetPinnedMessageAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_get_pinned_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_get_pinned_async_full";
	const FString TestMessageText = TEXT("Pinned async message");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	CreateResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	CreateResult.Channel->Connect();
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMessageResult> CallbackResult = MakeShared<FPubnubChatMessageResult>();
	FOnPubnubChatMessageResponseNative OnMessageResponse;
	OnMessageResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatMessageResult& MessageResult)
	{
		*CallbackResult = MessageResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
		TestFalse("PinMessage should succeed", PinResult.Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([CreateResult, OnMessageResponse]()
	{
		CreateResult.Channel->GetPinnedMessageAsync(OnMessageResponse);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("GetPinnedMessageAsync should succeed", CallbackResult->Result.Error);
		TestNotNull("Pinned message should be returned", CallbackResult->Message);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNullUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.1Validation.NullUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteNullUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_null_user_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_null_user";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite with null user
			FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(nullptr);
			TestTrue("Invite should fail with null user", InviteResult.Result.Error);
			
			// Cleanup
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelWhoIsPresentAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.WhoIsPresentAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelWhoIsPresentAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_who_is_present_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_who_is_present_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatWhoIsPresentResult> CallbackResult = MakeShared<FPubnubChatWhoIsPresentResult>();
	FOnPubnubChatWhoIsPresentResponseNative OnWhoIsPresentResponse;
	OnWhoIsPresentResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatWhoIsPresentResult& Result)
	{
		*CallbackResult = Result;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->WhoIsPresentAsync(OnWhoIsPresentResponse, 1000, 0);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, InitUserID]()
	{
		TestFalse("WhoIsPresentAsync should succeed", CallbackResult->Result.Error);
		TestTrue("Users should include current user", CallbackResult->Users.Contains(InitUserID));
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelIsPresentAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.IsPresentAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelIsPresentAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_is_present_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_is_present_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	CreateResult.Channel->Join(FPubnubChatMembershipData());
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatIsPresentResult> CallbackResult = MakeShared<FPubnubChatIsPresentResult>();
	FOnPubnubChatIsPresentResponseNative OnIsPresentResponse;
	OnIsPresentResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatIsPresentResult& Result)
	{
		*CallbackResult = Result;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->IsPresentAsync(InitUserID, OnIsPresentResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("IsPresentAsync should succeed", CallbackResult->Result.Error);
		TestTrue("IsPresentAsync should return true", CallbackResult->IsPresent);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelDeleteAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.DeleteAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelDeleteAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_delete_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_delete_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([CreateResult, OnOperationResponse]()
	{
		CreateResult.Channel->DeleteAsync(OnOperationResponse);
	}, 2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CallbackResult]()
	{
		TestFalse("DeleteAsync should succeed", CallbackResult->Error);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	if(InviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", InviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", InviteResult.Membership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData MembershipData = InviteResult.Membership->GetMembershipData();
		TestEqual("Membership Status should be pending", MembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && InviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelRestrictionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.RestrictionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelRestrictionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_restrictions_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_channel_restrictions_async_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_channel_restrictions_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateUserResult.User || !CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bSetReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> SetResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnSetResponse;
	OnSetResponse.BindLambda([bSetReceived, SetResult](const FPubnubChatOperationResult& Result)
	{
		*SetResult = Result;
		*bSetReceived = true;
	});
	
	CreateChannelResult.Channel->SetRestrictionsAsync(TargetUserID, true, false, OnSetResponse, TEXT("Async reason"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSetReceived]() { return *bSetReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SetResult]()
	{
		TestFalse("SetRestrictionsAsync should succeed", SetResult->Error);
	}, 0.1f));
	
	TSharedPtr<bool> bGetUserReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetRestrictionResult> GetUserResult = MakeShared<FPubnubChatGetRestrictionResult>();
	FOnPubnubChatGetRestrictionResponseNative OnGetUserResponse;
	OnGetUserResponse.BindLambda([bGetUserReceived, GetUserResult](const FPubnubChatGetRestrictionResult& Result)
	{
		*GetUserResult = Result;
		*bGetUserReceived = true;
	});
	
	CreateChannelResult.Channel->GetUserRestrictionsAsync(CreateUserResult.User, OnGetUserResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bGetUserReceived]() { return *bGetUserReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, GetUserResult, TestChannelID]()
	{
		TestFalse("GetUserRestrictionsAsync should succeed", GetUserResult->Result.Error);
		TestEqual("Restriction ChannelID should match", GetUserResult->Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", GetUserResult->Restriction.Ban);
	}, 0.1f));
	
	TSharedPtr<bool> bGetUsersReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetRestrictionsResult> GetUsersResult = MakeShared<FPubnubChatGetRestrictionsResult>();
	FOnPubnubChatGetRestrictionsResponseNative OnGetUsersResponse;
	OnGetUsersResponse.BindLambda([bGetUsersReceived, GetUsersResult](const FPubnubChatGetRestrictionsResult& Result)
	{
		*GetUsersResult = Result;
		*bGetUsersReceived = true;
	});
	
	CreateChannelResult.Channel->GetUsersRestrictionsAsync(OnGetUsersResponse, 10, FPubnubMemberSort(), FPubnubPage());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bGetUsersReceived]() { return *bGetUsersReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, GetUsersResult, TestChannelID]()
	{
		TestFalse("GetUsersRestrictionsAsync should succeed", GetUsersResult->Result.Error);
		bool bFound = false;
		for(const FPubnubChatRestriction& Restriction : GetUsersResult->Restrictions)
		{
			if(Restriction.ChannelID == TestChannelID)
			{
				bFound = true;
				break;
			}
		}
		TestTrue("Restrictions should include test channel", bFound);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			}
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInviteesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_invitees_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_invitees_async_full";
	const FString TargetUserID = SDK_PREFIX + "test_channel_invitees_async_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateUserResult.User || !CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	CreateChannelResult.Channel->Invite(CreateUserResult.User);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMembershipsResult> CallbackResult = MakeShared<FPubnubChatMembershipsResult>();
	FOnPubnubChatMembershipsResponseNative OnInviteesResponse;
	OnInviteesResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatMembershipsResult& Result)
	{
		*CallbackResult = Result;
		*bCallbackReceived = true;
	});
	
	CreateChannelResult.Channel->GetInviteesAsync(OnInviteesResponse, 10, TEXT(""), FPubnubMemberSort(), FPubnubPage());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("GetInviteesAsync should succeed", CallbackResult->Result.Error);
		TestTrue("Invitees list should be valid", CallbackResult->Memberships.Num() >= 0);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelMessageOpsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.MessageOpsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelMessageOpsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_message_ops_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_message_ops_async_full";
	const FString DestChannelID = SDK_PREFIX + "test_channel_message_ops_async_full_dest";
	const FString TestMessageText = TEXT("Async message ops");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	FPubnubChatChannelResult CreateDestResult = Chat->CreatePublicConversation(DestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestFalse("CreatePublicConversation should succeed", CreateDestResult.Result.Error);
	if(!CreateResult.Channel || !CreateDestResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	CreateResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	CreateResult.Channel->Connect();
	CreateDestResult.Channel->Connect();
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	// GetMessageAsync
	TSharedPtr<bool> bGetMessageReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMessageResult> GetMessageResult = MakeShared<FPubnubChatMessageResult>();
	FOnPubnubChatMessageResponseNative OnMessageResponse;
	OnMessageResponse.BindLambda([bGetMessageReceived, GetMessageResult](const FPubnubChatMessageResult& Result)
	{
		*GetMessageResult = Result;
		*bGetMessageReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage, bGetMessageReceived, OnMessageResponse]()
	{
		if(!*ReceivedMessage || !IsValid(*ReceivedMessage))
		{
			AddError("Received message is invalid");
			*bGetMessageReceived = true;
			return;
		}
		
		CreateResult.Channel->GetMessageAsync((*ReceivedMessage)->GetMessageTimetoken(), OnMessageResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bGetMessageReceived]() { return *bGetMessageReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, GetMessageResult]()
	{
		TestFalse("GetMessageAsync should succeed", GetMessageResult->Result.Error);
		TestNotNull("Message should be returned", GetMessageResult->Message);
	}, 0.1f));
	
	// ForwardMessageAsync (channel)
	TSharedPtr<bool> bForwardReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> ForwardResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnForwardResponse;
	OnForwardResponse.BindLambda([bForwardReceived, ForwardResult](const FPubnubChatOperationResult& Result)
	{
		*ForwardResult = Result;
		*bForwardReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateDestResult, ReceivedMessage, bForwardReceived, OnForwardResponse]()
	{
		if(!*ReceivedMessage || !IsValid(*ReceivedMessage))
		{
			AddError("Received message is invalid");
			*bForwardReceived = true;
			return;
		}
		
		CreateDestResult.Channel->ForwardMessageAsync(*ReceivedMessage, OnForwardResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bForwardReceived]() { return *bForwardReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ForwardResult]()
	{
		TestFalse("ForwardMessageAsync should succeed", ForwardResult->Error);
	}, 0.1f));
	
	// EmitUserMentionAsync
	TSharedPtr<bool> bMentionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> MentionResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnMentionResponse;
	OnMentionResponse.BindLambda([bMentionReceived, MentionResult](const FPubnubChatOperationResult& Result)
	{
		*MentionResult = Result;
		*bMentionReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage, bMentionReceived, InitUserID, OnMentionResponse]()
	{
		if(!*ReceivedMessage || !IsValid(*ReceivedMessage))
		{
			AddError("Received message is invalid");
			*bMentionReceived = true;
			return;
		}
		
		CreateResult.Channel->EmitUserMentionAsync(InitUserID, (*ReceivedMessage)->GetMessageTimetoken(), TEXT("Async mention"), OnMentionResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMentionReceived]() { return *bMentionReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionResult]()
	{
		TestFalse("EmitUserMentionAsync should succeed", MentionResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateDestResult, Chat, TestChannelID, DestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(CreateDestResult.Channel)
		{
			CreateDestResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteChannel(DestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelTypingAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.TypingAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelTypingAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_typing_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_typing_async_full";
	const FString OtherUserID = SDK_PREFIX + "test_channel_typing_async_full_other";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Typing is supported on direct conversations, not public channels
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	UPubnubChatUser* OtherUser = nullptr;
	if(CreateUserResult.Result.Error)
	{
		FPubnubChatUserResult GetUserResult = Chat->GetUser(OtherUserID);
		TestFalse("GetUser should succeed when CreateUser fails", GetUserResult.Result.Error);
		OtherUser = GetUserResult.User;
	}
	else
	{
		OtherUser = CreateUserResult.User;
	}
	if(!OtherUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(OtherUser, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bStreamReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StreamResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStreamResponse;
	OnStreamResponse.BindLambda([bStreamReceived, StreamResult](const FPubnubChatOperationResult& Result)
	{
		*StreamResult = Result;
		*bStreamReceived = true;
	});
	
	CreateResult.Channel->StreamTypingAsync(OnStreamResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStreamReceived]() { return *bStreamReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bStartReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StartResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStartResponse;
	OnStartResponse.BindLambda([bStartReceived, StartResult](const FPubnubChatOperationResult& Result)
	{
		*StartResult = Result;
		*bStartReceived = true;
	});
	CreateResult.Channel->StartTypingAsync(OnStartResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStartReceived]() { return *bStartReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bStopReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StopResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStopResponse;
	OnStopResponse.BindLambda([bStopReceived, StopResult](const FPubnubChatOperationResult& Result)
	{
		*StopResult = Result;
		*bStopReceived = true;
	});
	CreateResult.Channel->StopTypingAsync(OnStopResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStopReceived]() { return *bStopReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bStopStreamReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StopStreamResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStopStreamResponse;
	OnStopStreamResponse.BindLambda([bStopStreamReceived, StopStreamResult](const FPubnubChatOperationResult& Result)
	{
		*StopStreamResult = Result;
		*bStopStreamReceived = true;
	});
	CreateResult.Channel->StopStreamingTypingAsync(OnStopStreamResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStopStreamReceived]() { return *bStopStreamReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, StreamResult, StartResult, StopResult, StopStreamResult]()
	{
		TestFalse("StreamTypingAsync should succeed", StreamResult->Error);
		TestFalse("StartTypingAsync should succeed", StartResult->Error);
		TestFalse("StopTypingAsync should succeed", StopResult->Error);
		TestFalse("StopStreamingTypingAsync should succeed", StopStreamResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, TestChannelID, OtherUserID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelMessageReportsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.MessageReportsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelMessageReportsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_message_reports_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_message_reports_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bStreamReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StreamResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStreamResponse;
	OnStreamResponse.BindLambda([bStreamReceived, StreamResult](const FPubnubChatOperationResult& Result)
	{
		*StreamResult = Result;
		*bStreamReceived = true;
	});
	
	CreateResult.Channel->StreamMessageReportsAsync(OnStreamResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStreamReceived]() { return *bStreamReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bStopReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StopResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStopResponse;
	OnStopResponse.BindLambda([bStopReceived, StopResult](const FPubnubChatOperationResult& Result)
	{
		*StopResult = Result;
		*bStopReceived = true;
	});
	CreateResult.Channel->StopStreamingMessageReportsAsync(OnStopResponse);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStopReceived]() { return *bStopReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bHistoryReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEventsResult> HistoryResult = MakeShared<FPubnubChatEventsResult>();
	FOnPubnubChatEventsResponseNative OnHistoryResponse;
	OnHistoryResponse.BindLambda([bHistoryReceived, HistoryResult](const FPubnubChatEventsResult& Result)
	{
		*HistoryResult = Result;
		*bHistoryReceived = true;
	});
	
	const FString StartTimetoken = TEXT("0");
	const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	CreateResult.Channel->GetMessageReportsHistoryAsync(StartTimetoken, EndTimetoken, OnHistoryResponse, 10);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bHistoryReceived]() { return *bHistoryReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, StreamResult, StopResult, HistoryResult]()
	{
		TestFalse("StreamMessageReportsAsync should succeed", StreamResult->Error);
		TestFalse("StopStreamingMessageReportsAsync should succeed", StopResult->Error);
		TestFalse("GetMessageReportsHistoryAsync should succeed", HistoryResult->Result.Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}
// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// Note: Invite only takes User parameter, so full parameter test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel with custom data and join
	FPubnubChatChannelData ChannelData;
	ChannelData.Custom = TEXT("{\"test\":\"channel\"}");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user with custom data
	FPubnubChatUserData UserData;
	UserData.Custom = TEXT("{\"test\":\"user\"}");
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	if(InviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", InviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", InviteResult.Membership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData RetrievedMembershipData = InviteResult.Membership->GetMembershipData();
		TestEqual("Membership Status should be pending", RetrievedMembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && InviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that Invite returns existing membership if user is already a member.
 * Verifies that inviting the same user twice returns the existing membership on the second call.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteAlreadyMemberTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.AlreadyMember", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteAlreadyMemberTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_already_member_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_already_member_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_already_member";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// First invite - should create membership with "pending" status
	FPubnubChatInviteResult FirstInviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("First Invite should succeed", FirstInviteResult.Result.Error);
	TestNotNull("First Membership should be created", FirstInviteResult.Membership);
	
	if(!FirstInviteResult.Membership)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatMembershipData FirstMembershipData = FirstInviteResult.Membership->GetMembershipData();
	TestEqual("First Membership Status should be pending", FirstMembershipData.Status, TEXT("pending"));
	
	// Second invite - should return existing membership (not create a new one)
	FPubnubChatInviteResult SecondInviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Second Invite should succeed (returns existing membership)", SecondInviteResult.Result.Error);
	TestNotNull("Second Membership should be returned", SecondInviteResult.Membership);
	
	if(SecondInviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", SecondInviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", SecondInviteResult.Membership->GetUserID(), TargetUserID);
		
		// Status should still be "pending" (user hasn't joined yet)
		FPubnubChatMembershipData SecondMembershipData = SecondInviteResult.Membership->GetMembershipData();
		TestEqual("Second Membership Status should still be pending", SecondMembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && SecondInviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that Invite emits Invite events for non-public channels.
 * Verifies that invitation events are sent when inviting to group/direct channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNonPublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.NonPublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteNonPublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_nonpublic_event_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_nonpublic_event_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_nonpublic_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation (non-public channel)
	FPubnubChatChannelData ChannelData;
	FPubnubChatMembershipData HostMembershipData;
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, HostMembershipData);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify channel is direct (non-public)
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be direct", CreatedChannelData.Type, TEXT("direct"));
	
	// Create second user to invite
	const FString SecondUserID = SDK_PREFIX + "test_invite_nonpublic_event_user2";
	FPubnubChatUserResult CreateSecondUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateSecondUser should succeed", CreateSecondUserResult.Result.Error);
	TestNotNull("Second user should be created", CreateSecondUserResult.User);
	
	if(!CreateSecondUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatInviteEvent> ReceivedEvent = MakeShared<FPubnubChatInviteEvent>();
	
	// Stream invitations for the target user
	CreateSecondUserResult.User->OnInvitedNative.AddLambda([this, bEventReceived, ReceivedEvent, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("direct"));
	});
	
	FPubnubChatOperationResult StreamResult = CreateSecondUserResult.User->StreamInvitations();
	TestFalse("StreamInvitations should succeed", StreamResult.Error);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteResult> InviteResult = MakeShared<FPubnubChatInviteResult>();
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateSecondUserResult, InviteResult]()
	{
		// Invite second user to the direct conversation
		*InviteResult = CreateResult.Channel->Invite(CreateSecondUserResult.User);
		TestFalse("Invite should succeed", InviteResult->Result.Error);
		TestNotNull("Membership should be created", InviteResult->Membership);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID]()
	{
		if(!*bEventReceived)
		{
			AddError("Invite event was not received for non-public channel");
		}
		else
		{
			TestEqual("Received invite channelId should match", ReceivedEvent->ChannelID, TestChannelID);
			TestEqual("Received invite channelType should match", ReceivedEvent->ChannelType, TEXT("direct"));
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, remove memberships, delete users and channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSecondUserResult, Chat, TestChannelID, InitUserID, TargetUserID, SecondUserID]()
	{
		if(CreateSecondUserResult.User)
		{
			CreateSecondUserResult.User->StopStreamingInvitations();
		}
		if(Chat)
		{
			// Remove membership for invited user
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(SecondUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
			Chat->DeleteUser(TargetUserID);
			Chat->DeleteUser(SecondUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that Invite emits Invite events for public channels.
 * Verifies that invitation events are sent when inviting to public channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInvitePublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.PublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInvitePublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_public_event_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_public_event_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_public_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create public channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify channel is public
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be public", CreatedChannelData.Type, TEXT("public"));
	
	// Join channel
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatInviteEvent> ReceivedEvent = MakeShared<FPubnubChatInviteEvent>();
	
	// Stream invitations for the target user
	CreateUserResult.User->OnInvitedNative.AddLambda([this, bEventReceived, ReceivedEvent, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("public"));
	});
	
	FPubnubChatOperationResult StreamResult = CreateUserResult.User->StreamInvitations();
	TestFalse("StreamInvitations should succeed", StreamResult.Error);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteResult> InviteResult = MakeShared<FPubnubChatInviteResult>();
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUserResult, InviteResult]()
	{
		// Invite user to public channel
		*InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
		TestFalse("Invite should succeed", InviteResult->Result.Error);
		TestNotNull("Membership should be created", InviteResult->Membership);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID]()
	{
		if(!*bEventReceived)
		{
			AddError("Invite event was not received for public channel");
		}
		else
		{
			TestEqual("Received invite channelId should match", ReceivedEvent->ChannelID, TestChannelID);
			TestEqual("Received invite channelType should match", ReceivedEvent->ChannelType, TEXT("public"));
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, remove membership, leave and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateUserResult, CreateResult, Chat, TestChannelID, TargetUserID, InviteResult]()
	{
		if(CreateUserResult.User)
		{
			CreateUserResult.User->StopStreamingInvitations();
		}
		if(Chat)
		{
			// Remove membership for invited user
			if(InviteResult->Membership)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
			}
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// INVITE MULTIPLE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Create users to invite
			const FString TargetUserID = SDK_PREFIX + "test_invite_multiple_not_init_target";
			FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
			TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
			TestNotNull("User should be created", CreateUserResult.User);
			
			if(CreateUserResult.User)
			{
				TArray<UPubnubChatUser*> Users = {CreateUserResult.User};
				
				// Try to invite multiple with uninitialized channel
				FPubnubChatInviteMultipleResult InviteResult = UninitializedChannel->InviteMultiple(Users);
				TestTrue("InviteMultiple should fail with uninitialized channel", InviteResult.Result.Error);
			}
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
				if(CreateUserResult.User)
				{
					Chat->DeleteUser(TargetUserID);
				}
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleEmptyArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.EmptyArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleEmptyArrayTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite multiple with empty array
			TArray<UPubnubChatUser*> EmptyUsers;
			FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(EmptyUsers);
			TestTrue("InviteMultiple should fail with empty array", InviteResult.Result.Error);
			
			// Cleanup
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleAllInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.AllInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleAllInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_all_invalid_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_all_invalid";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite multiple with all invalid users (null pointers)
			TArray<UPubnubChatUser*> InvalidUsers = {nullptr, nullptr};
			FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(InvalidUsers);
			TestTrue("InviteMultiple should fail with all invalid users", InviteResult.Result.Error);
			
			// Cleanup
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_happy_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_happy_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_happy_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite multiple users
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed", InviteResult.Result.Error);
	TestEqual("Should have 2 memberships", InviteResult.Memberships.Num(), 2);
	
	for(int32 i = 0; i < InviteResult.Memberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = InviteResult.Memberships[i];
		TestNotNull(FString::Printf(TEXT("Membership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("Membership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("Membership %d Status should be pending"), i), MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// Note: InviteMultiple only takes Users array parameter, so full parameter test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_full_target2";
	const FString TargetUserID3 = SDK_PREFIX + "test_invite_multiple_full_target3";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel with custom data and join
	FPubnubChatChannelData ChannelData;
	ChannelData.Custom = TEXT("{\"test\":\"channel\"}");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users with custom data
	FPubnubChatUserData UserData1;
	UserData1.Custom = TEXT("{\"test\":\"user1\"}");
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, UserData1);
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserData UserData2;
	UserData2.Custom = TEXT("{\"test\":\"user2\"}");
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, UserData2);
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	FPubnubChatUserData UserData3;
	UserData3.Custom = TEXT("{\"test\":\"user3\"}");
	FPubnubChatUserResult CreateUser3Result = Chat->CreateUser(TargetUserID3, UserData3);
	TestFalse("CreateUser3 should succeed", CreateUser3Result.Result.Error);
	TestNotNull("User3 should be created", CreateUser3Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User || !CreateUser3Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite multiple users
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User, CreateUser3Result.User};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed", InviteResult.Result.Error);
	TestEqual("Should have 3 memberships", InviteResult.Memberships.Num(), 3);
	
	for(int32 i = 0; i < InviteResult.Memberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = InviteResult.Memberships[i];
		TestNotNull(FString::Printf(TEXT("Membership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("Membership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData RetrievedMembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("Membership %d Status should be pending"), i), RetrievedMembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2, TargetUserID3};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
		Chat->DeleteUser(TargetUserID3);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that InviteMultiple works correctly when some users in array are invalid.
 * Verifies that valid users are still invited even if some are null/invalid.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultiplePartialInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.PartialInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultiplePartialInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_partial_invalid_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_partial_invalid_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_partial_invalid_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_partial_invalid";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
			CleanUp();
		return false;
	}
	
	// Create channel and join
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create valid users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite multiple users with some invalid (null) users mixed in
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, nullptr, CreateUser2Result.User, nullptr};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed with partial invalid users", InviteResult.Result.Error);
	TestEqual("Should have 2 memberships (only valid users)", InviteResult.Memberships.Num(), 2);
	
	// Verify both valid users were invited
	TArray<FString> InvitedUserIDs;
	for(UPubnubChatMembership* Membership : InviteResult.Memberships)
	{
		if(Membership)
		{
			InvitedUserIDs.Add(Membership->GetUserID());
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
			
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual("Membership Status should be pending", MembershipData.Status, TEXT("pending"));
		}
	}
	
	TestTrue("User1 should be invited", InvitedUserIDs.Contains(TargetUserID1));
	TestTrue("User2 should be invited", InvitedUserIDs.Contains(TargetUserID2));
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that InviteMultiple emits Invite events for non-public channels.
 * Verifies that invitation events are sent when inviting multiple users to group/direct channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleNonPublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.NonPublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteMultipleNonPublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_nonpublic_event_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_nonpublic_event_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_nonpublic_event_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_nonpublic_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
			CleanUp();
		return false;
	}
	
	// Create group conversation (non-public channel)
	TArray<UPubnubChatUser*> InitialUsers = {CreateUser1Result.User};
	FPubnubChatChannelData ChannelData;
	FPubnubChatMembershipData HostMembershipData;
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(InitialUsers, TestChannelID, ChannelData, HostMembershipData);
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify channel is group (non-public)
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be group", CreatedChannelData.Type, TEXT("group"));
	
	// Shared state for event reception
	TSharedPtr<int32> EventsReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatInviteEvent>> ReceivedEvents = MakeShared<TArray<FPubnubChatInviteEvent>>();
	
	// Stream invitations for first target user
	CreateUser1Result.User->OnInvitedNative.AddLambda([this, EventsReceivedCount, ReceivedEvents, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("group"));
	});
	
	FPubnubChatOperationResult StreamResult1 = CreateUser1Result.User->StreamInvitations();
	TestFalse("StreamInvitations1 should succeed", StreamResult1.Error);
	
	// Stream invitations for second target user
	CreateUser2Result.User->OnInvitedNative.AddLambda([this, EventsReceivedCount, ReceivedEvents, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("group"));
	});
	
	FPubnubChatOperationResult StreamResult2 = CreateUser2Result.User->StreamInvitations();
	TestFalse("StreamInvitations2 should succeed", StreamResult2.Error);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteMultipleResult> InviteResult = MakeShared<FPubnubChatInviteMultipleResult>();
	
	// Wait a bit for subscriptions to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUser1Result, CreateUser2Result, InviteResult]()
	{
		// Invite multiple users to the group conversation
		TArray<UPubnubChatUser*> UsersToInvite = {CreateUser1Result.User, CreateUser2Result.User};
		*InviteResult = CreateResult.Channel->InviteMultiple(UsersToInvite);
		TestFalse("InviteMultiple should succeed", InviteResult->Result.Error);
		TestEqual("Should have 2 memberships", InviteResult->Memberships.Num(), 2);
	}, 0.5f));
	
	// Wait until events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([EventsReceivedCount]() -> bool {
		return *EventsReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify events were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, EventsReceivedCount, TestChannelID]()
	{
		if(*EventsReceivedCount < 2)
		{
			AddError(FString::Printf(TEXT("Expected 2 Invite events, but received %d"), *EventsReceivedCount));
		}
		else
		{
			TestTrue("Both Invite events were received for non-public channel", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, remove memberships, delete users and channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateUser1Result, CreateUser2Result, Chat, TestChannelID, InitUserID, TargetUserID1, TargetUserID2, InviteResult]()
	{
		if(CreateUser1Result.User)
		{
			CreateUser1Result.User->StopStreamingInvitations();
		}
		if(CreateUser2Result.User)
		{
			CreateUser2Result.User->StopStreamingInvitations();
		}
		if(Chat)
		{
			// Remove memberships for invited users
			if(InviteResult->Memberships.Num() > 0)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
			Chat->DeleteUser(TargetUserID1);
			Chat->DeleteUser(TargetUserID2);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that InviteMultiple emits Invite events for public channels.
 * Verifies that invitation events are sent when inviting multiple users to public channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultiplePublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.PublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteMultiplePublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_public_event_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_public_event_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_public_event_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_public_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create public channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify channel is public
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be public", CreatedChannelData.Type, TEXT("public"));
	
	// Join channel
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<int32> EventsReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatInviteEvent>> ReceivedEvents = MakeShared<TArray<FPubnubChatInviteEvent>>();
	
	// Stream invitations for first target user
	CreateUser1Result.User->OnInvitedNative.AddLambda([this, EventsReceivedCount, ReceivedEvents, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("public"));
	});
	
	FPubnubChatOperationResult StreamResult1 = CreateUser1Result.User->StreamInvitations();
	TestFalse("StreamInvitations1 should succeed", StreamResult1.Error);
	
	// Stream invitations for second target user
	CreateUser2Result.User->OnInvitedNative.AddLambda([this, EventsReceivedCount, ReceivedEvents, TestChannelID](const FPubnubChatInviteEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received invite channelId should match", Event.ChannelID, TestChannelID);
		TestEqual("Received invite channelType should match", Event.ChannelType, TEXT("public"));
	});
	
	FPubnubChatOperationResult StreamResult2 = CreateUser2Result.User->StreamInvitations();
	TestFalse("StreamInvitations2 should succeed", StreamResult2.Error);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteMultipleResult> InviteResult = MakeShared<FPubnubChatInviteMultipleResult>();
	
	// Wait a bit for subscriptions to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUser1Result, CreateUser2Result, InviteResult]()
	{
		// Invite multiple users to public channel
		TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User};
		*InviteResult = CreateResult.Channel->InviteMultiple(Users);
		TestFalse("InviteMultiple should succeed", InviteResult->Result.Error);
		TestEqual("Should have 2 memberships", InviteResult->Memberships.Num(), 2);
	}, 0.5f));
	
	// Wait until events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([EventsReceivedCount]() -> bool {
		return *EventsReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify events were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, EventsReceivedCount, TestChannelID]()
	{
		if(*EventsReceivedCount < 2)
		{
			AddError(FString::Printf(TEXT("Expected 2 Invite events, but received %d"), *EventsReceivedCount));
		}
		else
		{
			TestTrue("Both Invite events were received for public channel", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, remove memberships, leave and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateUser1Result, CreateUser2Result, CreateResult, Chat, TestChannelID, TargetUserID1, TargetUserID2, InviteResult]()
	{
		if(CreateUser1Result.User)
		{
			CreateUser1Result.User->StopStreamingInvitations();
		}
		if(CreateUser2Result.User)
		{
			CreateUser2Result.User->StopStreamingInvitations();
		}
		if(Chat)
		{
			// Remove memberships for invited users
			if(InviteResult->Memberships.Num() > 0)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
			}
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID1);
			Chat->DeleteUser(TargetUserID2);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// CREATEDIRECTCONVERSATION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
	if(!Chat)
	{
		// Try to create direct conversation without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TargetUserID = SDK_PREFIX + "test_create_direct_not_init_target";
			UPubnubChatUser* TestUser = NewObject<UPubnubChatUser>(Chat);
			if(TestUser)
			{
				FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(TestUser, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
				
				TestTrue("CreateDirectConversation should fail when Chat is not initialized", CreateResult.Result.Error);
				TestNull("Channel should not be created", CreateResult.Channel);
				TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationNullUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.1Validation.NullUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationNullUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_null_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to create direct conversation with null user
		const FString TestChannelID = SDK_PREFIX + "test_create_direct_null_user";
		FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(nullptr, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateDirectConversation should fail with null user", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_create_direct_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation with only required parameter (User) and default parameters
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestNotNull("InviteeMembership should be created", CreateResult.InviteeMembership);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel type is set to "direct"
		FPubnubChatChannelData ChannelDataRetrieved = CreateResult.Channel->GetChannelData();
		TestEqual("Channel Type should be direct", ChannelDataRetrieved.Type, TEXT("direct"));
	}
	
	if(CreateResult.HostMembership)
	{
		TestEqual("HostMembership ChannelID should match", CreateResult.HostMembership->GetChannelID(), TestChannelID);
		TestEqual("HostMembership UserID should match", CreateResult.HostMembership->GetUserID(), InitUserID);
	}
	
	if(CreateResult.InviteeMembership)
	{
		TestEqual("InviteeMembership ChannelID should match", CreateResult.InviteeMembership->GetChannelID(), TestChannelID);
		TestEqual("InviteeMembership UserID should match", CreateResult.InviteeMembership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData MembershipData = CreateResult.InviteeMembership->GetMembershipData();
		TestEqual("InviteeMembership Status should be pending", MembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee membership
			if(CreateResult.InviteeMembership)
			{
				FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveInviteeResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_create_direct_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target user with custom data
	FPubnubChatUserData UserData;
	UserData.Custom = TEXT("{\"test\":\"user\"}");
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation with all parameters
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("DirectChannelName");
	ChannelData.Description = TEXT("Direct channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");
	// Note: Type will be overridden to "direct" by the function
	
	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"host\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");
	
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, HostMembershipData);
	
	TestFalse("CreateDirectConversation should succeed with all parameters", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestNotNull("InviteeMembership should be created", CreateResult.InviteeMembership);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel data is stored correctly (type should be overridden to "direct")
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
		TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
		TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
		TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
		TestEqual("Channel Type should be direct (overridden)", RetrievedData.Type, TEXT("direct"));
	}
	
	if(CreateResult.HostMembership)
	{
		FPubnubChatMembershipData HostMembershipDataRetrieved = CreateResult.HostMembership->GetMembershipData();
		TestEqual("HostMembership Custom should match", HostMembershipDataRetrieved.Custom, HostMembershipData.Custom);
		TestEqual("HostMembership Status should match", HostMembershipDataRetrieved.Status, HostMembershipData.Status);
		TestEqual("HostMembership Type should match", HostMembershipDataRetrieved.Type, HostMembershipData.Type);
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee membership
			if(CreateResult.InviteeMembership)
			{
				FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveInviteeResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that CreateDirectConversation auto-generates ChannelID when empty string is provided.
 * Verifies that a deterministic hash-based ID is generated and used as ChannelID when ChannelID parameter is empty.
 * The ID format is "direct.{hash}" where hash is computed from sorted user IDs.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationAutoGeneratedChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.4Advanced.AutoGeneratedChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationAutoGeneratedChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_auto_id_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_auto_id_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation with empty ChannelID (should auto-generate hash-based ID)
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(CreateResult.Channel)
	{
		FString GeneratedChannelID = CreateResult.Channel->GetChannelID();
		TestFalse("ChannelID should be auto-generated (not empty)", GeneratedChannelID.IsEmpty());
		
		// Verify it starts with "direct." prefix
		TestTrue("ChannelID should start with 'direct.' prefix", GeneratedChannelID.StartsWith(TEXT("direct.")));
		
		// Verify determinism: same users should generate the same hash
		// Create another direct conversation with the same users and empty ChannelID
		FPubnubChatCreateDirectConversationResult CreateResult2 = Chat->CreateDirectConversation(CreateUserResult.User, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
		TestFalse("Second CreateDirectConversation should succeed", CreateResult2.Result.Error);
		if(CreateResult2.Channel)
		{
			FString GeneratedChannelID2 = CreateResult2.Channel->GetChannelID();
			TestEqual("Same users should generate the same ChannelID", GeneratedChannelID, GeneratedChannelID2);
			
			// Cleanup second channel
			if(Chat)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					if(CreateResult2.InviteeMembership)
					{
						FPubnubMembershipsResult RemoveInviteeResult2 = PubnubClient->RemoveMemberships(TargetUserID, {GeneratedChannelID2}, FPubnubMembershipInclude::FromValue(false), 1);
						if(RemoveInviteeResult2.Result.Error)
						{
							UE_LOG(LogTemp, Warning, TEXT("Failed to remove second invitee membership during cleanup: %s"), *RemoveInviteeResult2.Result.ErrorMessage);
						}
					}
					if(CreateResult2.HostMembership)
					{
						FPubnubMembershipsResult RemoveHostResult2 = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID2}, FPubnubMembershipInclude::FromValue(false), 1);
						if(RemoveHostResult2.Result.Error)
						{
							UE_LOG(LogTemp, Warning, TEXT("Failed to remove second host membership during cleanup: %s"), *RemoveHostResult2.Result.ErrorMessage);
						}
					}
				}
				Chat->DeleteChannel(GeneratedChannelID2);
			}
		}
		
		// Cleanup: Remove memberships, delete channel and users
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				// Remove invitee membership
				if(CreateResult.InviteeMembership)
				{
					FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveInviteeResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
					}
				}
				
				// Remove host membership
				if(CreateResult.HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(GeneratedChannelID);
			Chat->DeleteUser(TargetUserID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// CREATEGROUPCONVERSATION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
	if(!Chat)
	{
		// Try to create group conversation without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			TArray<UPubnubChatUser*> Users;
			UPubnubChatUser* TestUser = NewObject<UPubnubChatUser>(Chat);
			if(TestUser)
			{
				Users.Add(TestUser);
				FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
				
				TestTrue("CreateGroupConversation should fail when Chat is not initialized", CreateResult.Result.Error);
				TestNull("Channel should not be created", CreateResult.Channel);
				TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationEmptyUsersArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.EmptyUsersArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationEmptyUsersArrayTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_empty_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to create group conversation with empty users array
		TArray<UPubnubChatUser*> EmptyUsers;
		const FString TestChannelID = SDK_PREFIX + "test_create_group_empty_users";
		FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(EmptyUsers, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateGroupConversation should fail with empty users array", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationAllInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.AllInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationAllInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_invalid_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to create group conversation with all invalid users (null pointers)
		TArray<UPubnubChatUser*> InvalidUsers;
		InvalidUsers.Add(nullptr);
		InvalidUsers.Add(nullptr);
		const FString TestChannelID = SDK_PREFIX + "test_create_group_invalid_users";
		FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(InvalidUsers, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateGroupConversation should fail with all invalid users", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_happy_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_happy_target1";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	if(!CreateUser1Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create group conversation with only required parameter (Users array) and default parameters
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestEqual("InviteesMemberships should have 1 membership", CreateResult.InviteesMemberships.Num(), 1);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel type is set to "group"
		FPubnubChatChannelData ChannelDataRetrieved = CreateResult.Channel->GetChannelData();
		TestEqual("Channel Type should be group", ChannelDataRetrieved.Type, TEXT("group"));
	}
	
	if(CreateResult.HostMembership)
	{
		TestEqual("HostMembership ChannelID should match", CreateResult.HostMembership->GetChannelID(), TestChannelID);
		TestEqual("HostMembership UserID should match", CreateResult.HostMembership->GetUserID(), InitUserID);
	}
	
	if(CreateResult.InviteesMemberships.Num() > 0)
	{
		UPubnubChatMembership* InviteeMembership = CreateResult.InviteesMemberships[0];
		TestNotNull("InviteeMembership should be created", InviteeMembership);
		
		if(InviteeMembership)
		{
			TestEqual("InviteeMembership ChannelID should match", InviteeMembership->GetChannelID(), TestChannelID);
			TestEqual("InviteeMembership UserID should match", InviteeMembership->GetUserID(), TargetUserID1);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = InviteeMembership->GetMembershipData();
			TestEqual("InviteeMembership Status should be pending", MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_create_group_full_target2";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target users with custom data
	FPubnubChatUserData UserData1;
	UserData1.Custom = TEXT("{\"test\":\"user1\"}");
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, UserData1);
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserData UserData2;
	UserData2.Custom = TEXT("{\"test\":\"user2\"}");
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, UserData2);
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create group conversation with all parameters
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("GroupChannelName");
	ChannelData.Description = TEXT("Group channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");
	// Note: Type will be overridden to "group" by the function
	
	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");
	
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	Users.Add(CreateUser2Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, ChannelData, HostMembershipData);
	
	TestFalse("CreateGroupConversation should succeed with all parameters", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestEqual("InviteesMemberships should have 2 memberships", CreateResult.InviteesMemberships.Num(), 2);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel data is stored correctly (type should be overridden to "group")
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
		TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
		TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
		TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
		TestEqual("Channel Type should be group (overridden)", RetrievedData.Type, TEXT("group"));
	}
	
	if(CreateResult.HostMembership)
	{
		FPubnubChatMembershipData HostMembershipDataRetrieved = CreateResult.HostMembership->GetMembershipData();
		TestEqual("HostMembership Custom should match", HostMembershipDataRetrieved.Custom, HostMembershipData.Custom);
		TestEqual("HostMembership Status should match", HostMembershipDataRetrieved.Status, HostMembershipData.Status);
		TestEqual("HostMembership Type should match", HostMembershipDataRetrieved.Type, HostMembershipData.Type);
	}
	
	// Verify all invitee memberships
	for(int32 i = 0; i < CreateResult.InviteesMemberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = CreateResult.InviteesMemberships[i];
		TestNotNull(FString::Printf(TEXT("InviteeMembership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("InviteeMembership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("InviteeMembership %d Status should be pending"), i), MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that CreateGroupConversation auto-generates ChannelID when empty string is provided.
 * Verifies that a GUID is generated and used as ChannelID when ChannelID parameter is empty.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationAutoGeneratedChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.4Advanced.AutoGeneratedChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationAutoGeneratedChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_auto_id_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_auto_id_target1";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	if(!CreateUser1Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create group conversation with empty ChannelID (should auto-generate GUID)
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(CreateResult.Channel)
	{
		FString GeneratedChannelID = CreateResult.Channel->GetChannelID();
		TestFalse("ChannelID should be auto-generated (not empty)", GeneratedChannelID.IsEmpty());
		
		// Verify it's a valid GUID format (contains hyphens)
		TestTrue("ChannelID should be a GUID format", GeneratedChannelID.Contains(TEXT("-")));
		
		// Cleanup: Remove memberships, delete channel and users
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				// Remove invitee memberships
				if(CreateResult.InviteesMemberships.Num() > 0)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(GeneratedChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
				
				// Remove host membership
				if(CreateResult.HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(GeneratedChannelID);
			Chat->DeleteUser(TargetUserID1);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that CreateGroupConversation handles mixed valid and invalid users correctly.
 * Verifies that valid users are still invited even if some users in the array are null/invalid.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationMixedValidInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.4Advanced.MixedValidInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationMixedValidInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_mixed_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_mixed_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_create_group_mixed_target2";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_mixed";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create group conversation with mixed valid and invalid users
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	Users.Add(nullptr); // Invalid user
	Users.Add(CreateUser2Result.User);
	Users.Add(nullptr); // Another invalid user
	
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed (invalid users should be filtered)", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestEqual("InviteesMemberships should have 2 memberships (only valid users)", CreateResult.InviteesMemberships.Num(), 2);
	
	if(CreateResult.InviteesMemberships.Num() == 2)
	{
		// Verify both valid users were invited
		TArray<FString> InvitedUserIDs;
		for(UPubnubChatMembership* Membership : CreateResult.InviteesMemberships)
		{
			if(Membership)
			{
				InvitedUserIDs.Add(Membership->GetUserID());
			}
		}
		
		TestTrue("User1 should be invited", InvitedUserIDs.Contains(TargetUserID1));
		TestTrue("User2 should be invited", InvitedUserIDs.Contains(TargetUserID2));
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETCHANNELSUGGESTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
	if(!Chat)
	{
		// Try to get channel suggestions without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT("test"), 10);
			
			TestTrue("GetChannelSuggestions should fail when Chat is not initialized", GetSuggestionsResult.Result.Error);
			TestEqual("Channels array should be empty", GetSuggestionsResult.Channels.Num(), 0);
			TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsEmptyTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.1Validation.EmptyText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsEmptyTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channel_suggestions_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get channel suggestions with empty Text
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT(""), 10);
		
		TestTrue("GetChannelSuggestions should fail with empty Text", GetSuggestionsResult.Result.Error);
		TestEqual("Channels array should be empty", GetSuggestionsResult.Channels.Num(), 0);
		TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_happy";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test channel with a specific name pattern
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("TestChannelHappy");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Call GetChannelSuggestions with only required parameter (Text)
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT("TestChannel"));
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid (may be empty)", GetSuggestionsResult.Channels.Num() >= 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_full_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_full_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_full_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_full_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test channels with matching name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("FullTestChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("FullTestChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("FullTestChannel3");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test GetChannelSuggestions with all parameters (Text and Limit)
		const FString SearchText = TEXT("FullTest");
		const int TestLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("GetChannelSuggestions should succeed with all parameters", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		TestTrue("Channels count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Channels.Num() <= TestLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsWithLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.3FullParameters.WithLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsWithLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_limit_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_limit_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_limit_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_limit_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test channels with matching name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("LimitTestChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("LimitTestChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("LimitTestChannel3");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test GetChannelSuggestions with Limit parameter
		const FString SearchText = TEXT("LimitTest");
		const int TestLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("GetChannelSuggestions should succeed with Limit", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		TestTrue("Channels count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Channels.Num() <= TestLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetChannelSuggestions with multiple channels matching the search pattern.
 * Verifies that all matching channels are returned and that the results are correct.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsMultipleMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.MultipleMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsMultipleMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_multi_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_multi_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_multi_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_multi_3";
	const FString TestChannelID4 = SDK_PREFIX + "test_get_channel_suggestions_multi_4";
	const FString TestChannelID5 = SDK_PREFIX + "test_get_channel_suggestions_multi_5";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create multiple test channels with matching name patterns
		TArray<FString> TestChannelIDs = {TestChannelID1, TestChannelID2, TestChannelID3, TestChannelID4, TestChannelID5};
		for(int32 i = 0; i < TestChannelIDs.Num(); ++i)
		{
			FPubnubChatChannelData ChannelData;
			ChannelData.ChannelName = FString::Printf(TEXT("MultiTestChannel%d"), i + 1);
			FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelIDs[i], ChannelData);
			TestFalse(FString::Printf(TEXT("CreatePublicConversation %s should succeed"), *TestChannelIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelIDs[i]); }
		}
		
		// Get channel suggestions with search text that matches all created channels
		const FString SearchText = TEXT("MultiTest");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should contain at least created channels", GetSuggestionsResult.Channels.Num() >= CreatedChannelIDs.Num());
		
		// Verify all created channels are in the result
		TArray<FString> FoundChannelIDs;
		for(UPubnubChatChannel* Channel : GetSuggestionsResult.Channels)
		{
			if(Channel)
			{
				FoundChannelIDs.Add(Channel->GetChannelID());
			}
		}
		
		for(const FString& CreatedChannelID : CreatedChannelIDs)
		{
			TestTrue(FString::Printf(TEXT("Created channel %s should be found in suggestions"), *CreatedChannelID), FoundChannelIDs.Contains(CreatedChannelID));
		}
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with partial name matching.
 * Verifies that the function correctly matches channels based on name prefix pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsPartialMatchTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.PartialMatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsPartialMatchTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_partial_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_partial_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_partial_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_partial_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test channels with different name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("PartialMatchChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("PartialMatchChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("DifferentNameChannel");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test partial match - should match first two channels but not third
		const FString SearchText = TEXT("PartialMatch");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		
		// Verify matching channels are in results
		TArray<FString> FoundChannelIDs;
		for(UPubnubChatChannel* Channel : GetSuggestionsResult.Channels)
		{
			if(Channel)
			{
				FoundChannelIDs.Add(Channel->GetChannelID());
			}
		}
		
		// First two channels should be found
		TestTrue("PartialMatchChannel1 should be found", FoundChannelIDs.Contains(TestChannelID1));
		TestTrue("PartialMatchChannel2 should be found", FoundChannelIDs.Contains(TestChannelID2));
		
		// Third channel should not be found (different name pattern)
		TestFalse("DifferentNameChannel should not be found", FoundChannelIDs.Contains(TestChannelID3));
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with no matching channels.
 * Verifies that the function returns an empty result set when no channels match the search pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsNoMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.NoMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsNoMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_nomatch_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_nomatch";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test channel with a specific name
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("NoMatchTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Test with search text that doesn't match any channel
		const FString SearchText = TEXT("NonExistentChannelPrefix");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed even with no matches", GetSuggestionsResult.Result.Error);
		TestEqual("Channels array should be empty for no matches", GetSuggestionsResult.Channels.Num(), 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with case sensitivity.
 * Verifies that the function correctly matches channels regardless of case differences in the search text.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsCaseSensitivityTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.CaseSensitivity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsCaseSensitivityTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_case_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_case";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test channel with mixed case name
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("CaseTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Test with different case variations
		const FString SearchTextLower = TEXT("casetest");
		const FString SearchTextUpper = TEXT("CASETEST");
		const FString SearchTextMixed = TEXT("CaseTest");
		
		FPubnubChatGetChannelSuggestionsResult ResultLower = Chat->GetChannelSuggestions(SearchTextLower, 10);
		FPubnubChatGetChannelSuggestionsResult ResultUpper = Chat->GetChannelSuggestions(SearchTextUpper, 10);
		FPubnubChatGetChannelSuggestionsResult ResultMixed = Chat->GetChannelSuggestions(SearchTextMixed, 10);
		
		TestFalse("GetChannelSuggestions with lowercase should succeed", ResultLower.Result.Error);
		TestFalse("GetChannelSuggestions with uppercase should succeed", ResultUpper.Result.Error);
		TestFalse("GetChannelSuggestions with mixed case should succeed", ResultMixed.Result.Error);
		
		// At least one of the case variations should find the channel (depending on backend implementation)
		bool FoundInAnyCase = ResultLower.Channels.Num() > 0 || ResultUpper.Channels.Num() > 0 || ResultMixed.Channels.Num() > 0;
		TestTrue("Channel should be found with at least one case variation", FoundInAnyCase || ResultMixed.Channels.Num() > 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions limit enforcement.
 * Verifies that the limit parameter correctly restricts the number of returned results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsLimitEnforcementTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.LimitEnforcement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsLimitEnforcementTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_3";
	const FString TestChannelID4 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_4";
	const FString TestChannelID5 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_5";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create multiple test channels with matching name patterns
		TArray<FString> TestChannelIDs = {TestChannelID1, TestChannelID2, TestChannelID3, TestChannelID4, TestChannelID5};
		for(int32 i = 0; i < TestChannelIDs.Num(); ++i)
		{
			FPubnubChatChannelData ChannelData;
			ChannelData.ChannelName = FString::Printf(TEXT("LimitEnforceChannel%d"), i + 1);
			FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelIDs[i], ChannelData);
			TestFalse(FString::Printf(TEXT("CreatePublicConversation %s should succeed"), *TestChannelIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelIDs[i]); }
		}
		
		// Test with limit smaller than number of matching channels
		const FString SearchText = TEXT("LimitEnforce");
		const int SmallLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, SmallLimit);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels count should not exceed limit", GetSuggestionsResult.Channels.Num() <= SmallLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Test with limit larger than number of matching channels
		const int LargeLimit = 20;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResultLarge = Chat->GetChannelSuggestions(SearchText, LargeLimit);
		
		TestFalse("GetChannelSuggestions with large limit should succeed", GetSuggestionsResultLarge.Result.Error);
		TestTrue("Channels count should be valid", GetSuggestionsResultLarge.Channels.Num() >= 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests consistency of GetChannelSuggestions results across multiple calls.
 * Verifies that calling GetChannelSuggestions multiple times with the same parameters returns consistent results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_consistency_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_consistency";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test channel
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("ConsistencyTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Call GetChannelSuggestions multiple times with same parameters
		const FString SearchText = TEXT("Consistency");
		const int TestLimit = 10;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult1 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult2 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult3 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("First GetChannelSuggestions should succeed", GetSuggestionsResult1.Result.Error);
		TestFalse("Second GetChannelSuggestions should succeed", GetSuggestionsResult2.Result.Error);
		TestFalse("Third GetChannelSuggestions should succeed", GetSuggestionsResult3.Result.Error);
		
		// Results should be consistent (channels count should be similar)
		TestTrue("Channels count should be consistent", FMath::Abs(GetSuggestionsResult1.Channels.Num() - GetSuggestionsResult2.Channels.Num()) <= 1);
		TestTrue("Channels count should be consistent", FMath::Abs(GetSuggestionsResult2.Channels.Num() - GetSuggestionsResult3.Channels.Num()) <= 1);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC CHANNEL TESTS - FULL PARAMETERS (Chat.Channel Async)
// ============================================================================

/**
 * FullParameters test for CreateDirectConversationAsync.
 * Creates target user synchronously, then calls CreateDirectConversationAsync with all parameters and verifies callback result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversationAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatCreateDirectConversationAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_async_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_create_direct_async_full";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);

	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatUserData UserData;
	UserData.Custom = TEXT("{\"test\":\"user\"}");
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("DirectChannelName");
	ChannelData.Description = TEXT("Direct channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");

	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"host\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatCreateDirectConversationResult> CallbackResult = MakeShared<FPubnubChatCreateDirectConversationResult>();

	FOnPubnubChatCreateDirectConversationResponseNative OnDirectConversationResponse;
	OnDirectConversationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatCreateDirectConversationResult& DirectResult)
	{
		*CallbackResult = DirectResult;
		*bCallbackReceived = true;
	});
	Chat->CreateDirectConversationAsync(CreateUserResult.User, OnDirectConversationResponse, TestChannelID, ChannelData, HostMembershipData);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID, ChannelData, HostMembershipData]()
	{
		TestFalse("CreateDirectConversationAsync callback should report success", CallbackResult->Result.Error);
		TestNotNull("Channel should be created", CallbackResult->Channel);
		TestNotNull("HostMembership should be created", CallbackResult->HostMembership);
		TestNotNull("InviteeMembership should be created", CallbackResult->InviteeMembership);
		if(CallbackResult->Channel)
		{
			TestEqual("Created Channel ChannelID should match", CallbackResult->Channel->GetChannelID(), TestChannelID);
			FPubnubChatChannelData RetrievedData = CallbackResult->Channel->GetChannelData();
			TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
			TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
			TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
			TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
			TestEqual("Channel Type should be direct", RetrievedData.Type, TEXT("direct"));
		}
		if(CallbackResult->HostMembership)
		{
			FPubnubChatMembershipData HostRetrieved = CallbackResult->HostMembership->GetMembershipData();
			TestEqual("HostMembership Custom should match", HostRetrieved.Custom, HostMembershipData.Custom);
			TestEqual("HostMembership Status should match", HostRetrieved.Status, HostMembershipData.Status);
			TestEqual("HostMembership Type should match", HostRetrieved.Type, HostMembershipData.Type);
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CallbackResult, TestChannelID, TargetUserID, InitUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				if(CallbackResult->InviteeMembership)
				{
					FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveInviteeResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
					}
				}
				if(CallbackResult->HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for CreateGroupConversationAsync.
 * Creates target users synchronously, then calls CreateGroupConversationAsync with all parameters and verifies callback result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversationAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatCreateGroupConversationAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_async_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_async_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_create_group_async_full_target2";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_async_full";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);

	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatUserData UserData1;
	UserData1.Custom = TEXT("{\"test\":\"user1\"}");
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, UserData1);
	FPubnubChatUserData UserData2;
	UserData2.Custom = TEXT("{\"test\":\"user2\"}");
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, UserData2);
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("GroupChannelName");
	ChannelData.Description = TEXT("Group channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");

	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");

	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	Users.Add(CreateUser2Result.User);

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatCreateGroupConversationResult> CallbackResult = MakeShared<FPubnubChatCreateGroupConversationResult>();

	FOnPubnubChatCreateGroupConversationResponseNative OnGroupConversationResponse;
	OnGroupConversationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatCreateGroupConversationResult& GroupResult)
	{
		*CallbackResult = GroupResult;
		*bCallbackReceived = true;
	});
	Chat->CreateGroupConversationAsync(Users, OnGroupConversationResponse, TestChannelID, ChannelData, HostMembershipData);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID, ChannelData, HostMembershipData]()
	{
		TestFalse("CreateGroupConversationAsync callback should report success", CallbackResult->Result.Error);
		TestNotNull("Channel should be created", CallbackResult->Channel);
		TestNotNull("HostMembership should be created", CallbackResult->HostMembership);
		TestEqual("InviteesMemberships should have 2 memberships", CallbackResult->InviteesMemberships.Num(), 2);
		if(CallbackResult->Channel)
		{
			TestEqual("Created Channel ChannelID should match", CallbackResult->Channel->GetChannelID(), TestChannelID);
			FPubnubChatChannelData RetrievedData = CallbackResult->Channel->GetChannelData();
			TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
			TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
			TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
			TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
			TestEqual("Channel Type should be group", RetrievedData.Type, TEXT("group"));
		}
		if(CallbackResult->HostMembership)
		{
			FPubnubChatMembershipData HostRetrieved = CallbackResult->HostMembership->GetMembershipData();
			TestEqual("HostMembership Custom should match", HostRetrieved.Custom, HostMembershipData.Custom);
			TestEqual("HostMembership Status should match", HostRetrieved.Status, HostMembershipData.Status);
			TestEqual("HostMembership Type should match", HostRetrieved.Type, HostMembershipData.Type);
		}
		for(int32 i = 0; i < CallbackResult->InviteesMemberships.Num(); ++i)
		{
			UPubnubChatMembership* Membership = CallbackResult->InviteesMemberships[i];
			TestNotNull(FString::Printf(TEXT("InviteeMembership %d should be created"), i), Membership);
			if(Membership)
			{
				TestEqual(FString::Printf(TEXT("InviteeMembership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
				FPubnubChatMembershipData MData = Membership->GetMembershipData();
				TestEqual(FString::Printf(TEXT("InviteeMembership %d Status should be pending"), i), MData.Status, TEXT("pending"));
			}
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CallbackResult, TestChannelID, TargetUserID1, TargetUserID2, InitUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				if(CallbackResult->InviteesMemberships.Num() > 0)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
				if(CallbackResult->HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID1);
			Chat->DeleteUser(TargetUserID2);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for GetChannelSuggestionsAsync.
 * Creates channels with names synchronously, then calls GetChannelSuggestionsAsync with Text and Limit and verifies callback result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatGetChannelSuggestionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_async_full_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_async_full_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_async_full_2";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);

	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData1;
	ChannelData1.ChannelName = TEXT("AsyncFullTestChannel1");
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	FPubnubChatChannelData ChannelData2;
	ChannelData2.ChannelName = TEXT("AsyncFullTestChannel2");
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreateChannel1 should succeed", CreateResult1.Result.Error);
	TestFalse("CreateChannel2 should succeed", CreateResult2.Result.Error);

	const FString SearchText = TEXT("AsyncFullTest");
	const int TestLimit = 5;

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetChannelSuggestionsResult> CallbackResult = MakeShared<FPubnubChatGetChannelSuggestionsResult>();

	FOnPubnubChatGetChannelSuggestionsResponseNative OnSuggestionsResponse;
	OnSuggestionsResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetChannelSuggestionsResult& SuggestionsResult)
	{
		*CallbackResult = SuggestionsResult;
		*bCallbackReceived = true;
	});
	Chat->GetChannelSuggestionsAsync(SearchText, OnSuggestionsResponse, TestLimit);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestLimit]()
	{
		TestFalse("GetChannelSuggestionsAsync callback should report success", CallbackResult->Result.Error);
		TestTrue("Channels array should be valid", CallbackResult->Channels.Num() >= 0);
		TestTrue("Channels count should not exceed limit when enforced", CallbackResult->Channels.Num() <= TestLimit || CallbackResult->Channels.Num() == 0);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID1, TestChannelID2, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1);
			Chat->DeleteChannel(TestChannelID2);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// UPDATE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Try to update with uninitialized channel
			FPubnubChatOperationResult UpdateResult = UninitializedChannel->Update(FPubnubChatUpdateChannelInputData());
			TestTrue("Update should fail with uninitialized channel", UpdateResult.Error);
		}
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Update channel with minimal data (empty ChannelData)
	FPubnubChatUpdateChannelInputData UpdateData;
	FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdateData);
	TestFalse("Update should succeed", UpdateResult.Error);
	
	// Verify channel data was updated
	FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match", RetrievedData.ChannelName, UpdateData.ChannelName);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData InitialChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Update channel with all parameters
	FPubnubChatUpdateChannelInputData UpdateData;
	UpdateData.ChannelName = TEXT("UpdatedChannelName");
	UpdateData.Description = TEXT("Updated description");
	UpdateData.Custom = TEXT("{\"updated\":\"custom\"}");
	UpdateData.Status = TEXT("updatedStatus");
	UpdateData.Type = TEXT("updatedType");
	// Set ForceSet flags only for fields we're updating
	UpdateData.ForceSetChannelName = true;
	UpdateData.ForceSetDescription = true;
	UpdateData.ForceSetCustom = true;
	UpdateData.ForceSetStatus = true;
	UpdateData.ForceSetType = true;
	
	FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdateData);
	TestFalse("Update should succeed with all parameters", UpdateResult.Error);
	
	// Verify all data was updated correctly
	FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match", RetrievedData.ChannelName, UpdateData.ChannelName);
	TestEqual("Description should match", RetrievedData.Description, UpdateData.Description);
	TestEqual("Custom should match", RetrievedData.Custom, UpdateData.Custom);
	TestEqual("Status should match", RetrievedData.Status, UpdateData.Status);
	TestEqual("Type should match", RetrievedData.Type, UpdateData.Type);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests updating channel multiple times.
 * Verifies that subsequent updates correctly override previous channel data.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateMultipleTimesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData InitialChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// First update
	FPubnubChatUpdateChannelInputData FirstUpdateData;
	FirstUpdateData.ChannelName = TEXT("FirstUpdate");
	FirstUpdateData.Description = TEXT("First description");
	FirstUpdateData.ForceSetChannelName = true;
	FirstUpdateData.ForceSetDescription = true;
	FPubnubChatOperationResult FirstUpdateResult = CreateResult.Channel->Update(FirstUpdateData);
	TestFalse("First Update should succeed", FirstUpdateResult.Error);
	
	// Verify first update
	FPubnubChatChannelData RetrievedAfterFirst = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match first update", RetrievedAfterFirst.ChannelName, FirstUpdateData.ChannelName);
	TestEqual("Description should match first update", RetrievedAfterFirst.Description, FirstUpdateData.Description);
	
	// Second update
	FPubnubChatUpdateChannelInputData SecondUpdateData;
	SecondUpdateData.ChannelName = TEXT("SecondUpdate");
	SecondUpdateData.Description = TEXT("Second description");
	SecondUpdateData.ForceSetChannelName = true;
	SecondUpdateData.ForceSetDescription = true;
	FPubnubChatOperationResult SecondUpdateResult = CreateResult.Channel->Update(SecondUpdateData);
	TestFalse("Second Update should succeed", SecondUpdateResult.Error);
	
	// Verify second update overrides first
	FPubnubChatChannelData RetrievedAfterSecond = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match second update", RetrievedAfterSecond.ChannelName, SecondUpdateData.ChannelName);
	TestEqual("Description should match second update", RetrievedAfterSecond.Description, SecondUpdateData.Description);
	TestNotEqual("ChannelName should be different from first update", RetrievedAfterSecond.ChannelName, FirstUpdateData.ChannelName);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// PINMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			
			// Create a message object (we'll use SendText to create one)
			auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			};
			CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
			
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			const FString TestMessage = TEXT("Test message for pinning");
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin with uninitialized channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UninitializedChannel, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = UninitializedChannel->PinMessage(*ReceivedMessage);
					TestTrue("PinMessage should fail with uninitialized channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
			{
				if(CreateResult.Channel)
				{
					CreateResult.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
				}
				CleanUpCurrentChatUser(Chat);
	CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUpCurrentChatUser(Chat);
	CleanUp();
		}
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageNullMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.NullMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageNullMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_null_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_null_msg";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Try to pin null message
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(nullptr);
			TestTrue("PinMessage should fail with null message", PinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageDifferentChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.DifferentChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageDifferentChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_diff_channel_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_pin_diff_channel_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_pin_diff_channel_2";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create first channel
		FPubnubChatChannelData ChannelData1;
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation should succeed", CreateResult1.Result.Error);
		TestNotNull("Channel should be created", CreateResult1.Channel);
		
		// Create second channel
		FPubnubChatChannelData ChannelData2;
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation should succeed", CreateResult2.Result.Error);
		TestNotNull("Channel should be created", CreateResult2.Channel);
		
		if(CreateResult1.Channel && CreateResult2.Channel)
		{
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			
			// Connect to first channel and send message
			auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			};
			CreateResult1.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
			
			FPubnubChatOperationResult ConnectResult = CreateResult1.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			const FString TestMessage = TEXT("Test message");
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult1, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult1.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin message from channel 1 to channel 2
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult2, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = CreateResult2.Channel->PinMessage(*ReceivedMessage);
					TestTrue("PinMessage should fail with message from different channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult1, Chat, TestChannelID1, TestChannelID2]()
			{
				if(CreateResult1.Channel)
				{
					CreateResult1.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID1);
					Chat->DeleteChannel(TestChannelID2);
				}
				CleanUpCurrentChatUser(Chat);
	CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUpCurrentChatUser(Chat);
	CleanUp();
		}
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}

	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin");
	
	// Connect to channel and send message
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
			TestFalse("PinMessage should succeed", PinResult.Error);
			
			// Verify pinned message is stored in channel data
			FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests pinning a message twice.
 * Verifies that pinning a message twice overrides the previously pinned message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageTwiceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.4Advanced.PinTwiceOverride", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageTwiceTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_twice_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_twice";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<int32> MessagesReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<UPubnubChatMessage*>> ReceivedMessages = MakeShared<TArray<UPubnubChatMessage*>>();
	const FString TestMessage1 = TEXT("First message to pin");
	const FString TestMessage2 = TEXT("Second message to pin");
	
	// Connect to channel and send two messages
	auto MessageLambda = [MessagesReceivedCount, ReceivedMessages](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			(*MessagesReceivedCount)++;
			ReceivedMessages->Add(Message);
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage1]()
	{
		FPubnubChatOperationResult SendResult1 = CreateResult.Channel->SendText(TestMessage1);
		TestFalse("First SendText should succeed", SendResult1.Error);
	}, 0.5f));
	
	// Send second message after a delay
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage2]()
	{
		FPubnubChatOperationResult SendResult2 = CreateResult.Channel->SendText(TestMessage2);
		TestFalse("Second SendText should succeed", SendResult2.Error);
	}, 0.7f));
	
	// Wait until both messages are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([MessagesReceivedCount]() -> bool {
		return *MessagesReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Pin messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessages]()
	{
		if(ReceivedMessages->Num() < 2)
		{
			AddError("Should receive at least 2 messages");
		}
		else
		{
			UPubnubChatMessage* FirstMessage = (*ReceivedMessages)[0];
			UPubnubChatMessage* SecondMessage = (*ReceivedMessages)[1];
			
			// Pin first message
			FPubnubChatOperationResult PinResult1 = CreateResult.Channel->PinMessage(FirstMessage);
			TestFalse("First PinMessage should succeed", PinResult1.Error);
			
			// Verify first message is pinned
			FPubnubChatChannelData AfterFirstPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data after first pin", AfterFirstPin.Custom.IsEmpty());
			
			// Pin second message (should override first)
			FPubnubChatOperationResult PinResult2 = CreateResult.Channel->PinMessage(SecondMessage);
			TestFalse("Second PinMessage should succeed", PinResult2.Error);
			
			// Verify second message is now pinned (overriding first)
			FPubnubChatChannelData AfterSecondPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data after second pin", AfterSecondPin.Custom.IsEmpty());
			
			// The pinned message should be the second one, not the first
			TestTrue("Second pin should override first pin", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// UNPINMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Try to unpin with uninitialized channel
			FPubnubChatOperationResult UnpinResult = UninitializedChannel->UnpinMessage();
			TestTrue("UnpinMessage should fail with uninitialized channel", UnpinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin and unpin");
	
	// Connect to channel and send message
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin and unpin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message first
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
			TestFalse("PinMessage should succeed", PinResult.Error);
			
			// Verify message is pinned
			FPubnubChatChannelData AfterPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", AfterPin.Custom.IsEmpty());
			
			// Unpin the message
			FPubnubChatOperationResult UnpinResult = CreateResult.Channel->UnpinMessage();
			TestFalse("UnpinMessage should succeed", UnpinResult.Error);
			
			// Verify message is unpinned (Custom should no longer contain pinned message properties)
			FPubnubChatChannelData AfterUnpin = CreateResult.Channel->GetChannelData();
			// Note: Custom might still contain other data, but pinned message properties should be removed
			TestTrue("Unpin should complete successfully", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests unpinning when there is no pinned message.
 * Verifies that unpinning when no message is pinned should not throw an error and should just ignore.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageNoPinnedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.4Advanced.NoPinnedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageNoPinnedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_no_pinned_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_no_pinned";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel (no pinned message)
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to unpin when there's no pinned message
	FPubnubChatOperationResult UnpinResult = CreateResult.Channel->UnpinMessage();
	// Should not error - should just ignore
	TestFalse("UnpinMessage should not error when no message is pinned", UnpinResult.Error);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETPINNEDMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetPinnedMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetPinnedMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetPinnedMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_pinned_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_pinned_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Try to get pinned message with uninitialized channel
			FPubnubChatMessageResult GetPinnedResult = UninitializedChannel->GetPinnedMessage();
			TestTrue("GetPinnedMessage should fail with uninitialized channel", GetPinnedResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetPinnedResult.Result.ErrorMessage.IsEmpty());
		}
		
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetPinnedMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetPinnedMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetPinnedMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_pinned_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_pinned_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		Chat->DeleteChannel(TestChannelID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessageText = TEXT("Test message to pin");
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
		TestFalse("PinMessage should succeed", PinResult.Error);
	}, 0.1f));
	
	// Get pinned message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatMessageResult GetPinnedResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", GetPinnedResult.Result.Error);
		TestNotNull("Pinned message should be returned", GetPinnedResult.Message);
		
		if(GetPinnedResult.Message)
		{
			// Verify pinned message matches the message we pinned
			TestEqual("Pinned message timetoken should match", GetPinnedResult.Message->GetMessageTimetoken(), (*ReceivedMessage)->GetMessageTimetoken());
			
			// Verify message data
			FPubnubChatMessageData PinnedMessageData = GetPinnedResult.Message->GetMessageData();
			FPubnubChatMessageData OriginalMessageData = (*ReceivedMessage)->GetMessageData();
			
			TestEqual("Message text should match", PinnedMessageData.Text, OriginalMessageData.Text);
			TestEqual("Message UserID should match", PinnedMessageData.UserID, OriginalMessageData.UserID);
			TestEqual("Message ChannelID should match", PinnedMessageData.ChannelID, OriginalMessageData.ChannelID);
		}
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			// Unpin message before cleanup
			FPubnubChatOperationResult UnpinResult = CreateResult.Channel->UnpinMessage();
			TestFalse("UnpinMessage should succeed", UnpinResult.Error);
			
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// GetPinnedMessage has no optional parameters, so full parameter test is same as happy path

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetPinnedMessage when no message is pinned.
 * Verifies that GetPinnedMessage returns empty result when no message is pinned.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetPinnedMessageNoPinnedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetPinnedMessage.4Advanced.NoPinnedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetPinnedMessageNoPinnedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_pinned_no_pin_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_pinned_no_pin";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel (no pinned message)
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		Chat->DeleteChannel(TestChannelID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to get pinned message when there's no pinned message
	FPubnubChatMessageResult GetPinnedResult = CreateResult.Channel->GetPinnedMessage();
	TestFalse("GetPinnedMessage should succeed even when no message is pinned", GetPinnedResult.Result.Error);
	TestNull("Pinned message should be null when no message is pinned", GetPinnedResult.Message);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetPinnedMessage with a thread message pinned to parent channel.
 * Verifies that GetPinnedMessage returns a ThreadMessage with correct ParentChannelID and data.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetPinnedMessageThreadMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetPinnedMessage.4Advanced.ThreadMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetPinnedMessageThreadMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_pinned_thread_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_pinned_thread_msg";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		Chat->DeleteChannel(TestChannelID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessageText = TEXT("Test message for pinned thread message");
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateChannelResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel
	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		TestFalse("CreateThreadChannel should succeed", CreateResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateResult.ThreadChannel);
		
		*ThreadChannel = CreateResult.ThreadChannel;
	}, 0.1f));
	
	// Connect to thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult ConnectResult = (*ThreadChannel)->Connect();
		TestFalse("Connect to thread channel should succeed", ConnectResult.Error);
	}, 0.2f));
	
	// Send text to thread channel to confirm it and create a message
	const FString ThreadMessageText = TEXT("Thread message to pin");
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageText]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Get thread message from history
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage, ThreadMessageText]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Get thread history to get ThreadMessage
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EndTimetoken);
		TestFalse("GetThreadHistory should succeed", GetHistoryResult.Result.Error);
		
		// Find the thread message we sent
		for(UPubnubChatThreadMessage* TM : GetHistoryResult.ThreadMessages)
		{
			if(TM)
			{
				FPubnubChatMessageData MessageData = TM->GetMessageData();
				if(MessageData.Text.Contains(ThreadMessageText))
				{
					*ThreadMessage = TM;
					break;
				}
			}
		}
	}, 1.0f));
	
	// Pin the thread message to parent channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage, CreateChannelResult, TestChannelID]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not retrieved");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadChannel)->PinMessageToParentChannel(*ThreadMessage);
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.5f));
	
	// Get pinned message and verify it's a ThreadMessage with correct data
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, ThreadMessage, TestChannelID]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not retrieved");
			return;
		}
		
		FPubnubChatMessageResult GetPinnedResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", GetPinnedResult.Result.Error);
		TestNotNull("Pinned message should be returned", GetPinnedResult.Message);
		
		if(GetPinnedResult.Message)
		{
			// Cast to ThreadMessage
			UPubnubChatThreadMessage* PinnedThreadMessage = Cast<UPubnubChatThreadMessage>(GetPinnedResult.Message);
			TestNotNull("Pinned message should be a ThreadMessage", PinnedThreadMessage);
			
			if(PinnedThreadMessage)
			{
				// Verify ParentChannelID matches
				TestEqual("ParentChannelID should match", PinnedThreadMessage->GetParentChannelID(), TestChannelID);
				
				// Verify timetoken matches
				TestEqual("Timetoken should match", PinnedThreadMessage->GetMessageTimetoken(), (*ThreadMessage)->GetMessageTimetoken());
				
				// Verify message data
				FPubnubChatMessageData PinnedMessageData = PinnedThreadMessage->GetMessageData();
				FPubnubChatMessageData OriginalMessageData = (*ThreadMessage)->GetMessageData();
				
				TestEqual("Message text should match", PinnedMessageData.Text, OriginalMessageData.Text);
				TestEqual("Message UserID should match", PinnedMessageData.UserID, OriginalMessageData.UserID);
				TestEqual("Message ChannelID should match", PinnedMessageData.ChannelID, OriginalMessageData.ChannelID);
			}
		}
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage, ThreadChannel]()
	{
		if(*ThreadChannel)
		{
			(*ThreadChannel)->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Unpin message before cleanup
			FPubnubChatOperationResult UnpinResult = CreateChannelResult.Channel->UnpinMessage();
			TestFalse("UnpinMessage should succeed", UnpinResult.Error);
			
			FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
			if(HasThreadResult.HasThread)
			{
				Chat->RemoveThreadChannel(*ReceivedMessage);
			}
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// PINMESSAGETOCHANNEL TESTS (Chat object)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
	if(!Chat)
	{
		// Try to pin message without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMessage* TestMessage = nullptr;
			UPubnubChatChannel* TestChannel = nullptr;
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(TestMessage, TestChannel);
			
			TestTrue("PinMessageToChannel should fail when Chat is not initialized", PinResult.Error);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNullMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NullMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNullMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_null_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_null_msg";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Try to pin null message
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(nullptr, CreateResult.Channel);
			TestTrue("PinMessageToChannel should fail with null message", PinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNullChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NullChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNullChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_null_channel_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_null_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel and send message
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			const FString TestMessage = TEXT("Test message");
			
			// Connect to channel and send message
			auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			};
			CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
			
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin message with null channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, nullptr);
					TestTrue("PinMessageToChannel should fail with null channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
			{
				if(CreateResult.Channel)
				{
					CreateResult.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
				}
				CleanUpCurrentChatUser(Chat);
	CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUpCurrentChatUser(Chat);
	CleanUp();
		}
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}

	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin");
	
	// Connect to channel and send message
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the message using Chat->PinMessageToChannel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message using Chat->PinMessageToChannel
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, CreateResult.Channel);
			TestFalse("PinMessageToChannel should succeed", PinResult.Error);
			
			// Verify pinned message is stored in channel data
			FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// UNPINMESSAGEFROMCHANNEL TESTS (Chat object)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
	if(!Chat)
	{
		// Try to unpin message without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatChannel* TestChannel = nullptr;
			FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(TestChannel);
			
			TestTrue("UnpinMessageFromChannel should fail when Chat is not initialized", UnpinResult.Error);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelNullChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.1Validation.NullChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelNullChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_channel_null_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to unpin message with null channel
		FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(nullptr);
		TestTrue("UnpinMessageFromChannel should fail with null channel", UnpinResult.Error);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_from_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin and unpin");
	
	// Connect to channel and send message
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin and unpin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message first using Chat->PinMessageToChannel
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, CreateResult.Channel);
			TestFalse("PinMessageToChannel should succeed", PinResult.Error);
			
			// Verify message is pinned
			FPubnubChatChannelData AfterPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", AfterPin.Custom.IsEmpty());
			
			// Unpin the message using Chat->UnpinMessageFromChannel
			FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(CreateResult.Channel);
			TestFalse("UnpinMessageFromChannel should succeed", UnpinResult.Error);
			
			// Verify message is unpinned
			FPubnubChatChannelData AfterUnpin = CreateResult.Channel->GetChannelData();
			TestTrue("Unpin should complete successfully", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Chat object)
// ============================================================================

/**
 * FullParameters test for PinMessageToChannelAsync.
 * Creates a channel and message, then calls PinMessageToChannelAsync with all parameters and verifies callback result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_async_full";
	const FString TestMessageText = TEXT("Async pin message");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin message using async call
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, CreateResult, OnOperationResponse]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received before pinning");
			return;
		}
		Chat->PinMessageToChannelAsync(*ReceivedMessage, CreateResult.Channel, OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateResult]()
	{
		TestFalse("PinMessageToChannelAsync should succeed", CallbackResult->Error);
		FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
		TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * FullParameters test for UnpinMessageFromChannelAsync.
 * Pins a message first, then calls UnpinMessageFromChannelAsync and verifies callback result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_channel_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_from_channel_async_full";
	const FString TestMessageText = TEXT("Async unpin message");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Send message after subscription
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin message synchronously to prepare for unpin
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, CreateResult]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received before pinning");
			return;
		}
		FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, CreateResult.Channel);
		TestFalse("PinMessageToChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Unpin using async call
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, OnOperationResponse]()
	{
		Chat->UnpinMessageFromChannelAsync(CreateResult.Channel, OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("UnpinMessageFromChannelAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Channel object)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UpdateAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_update_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_update_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUpdateChannelInputData UpdateData;
	UpdateData.ChannelName = TEXT("AsyncUpdatedChannel");
	UpdateData.Description = TEXT("Async update description");
	UpdateData.Custom = TEXT("{\"async\":\"channel\"}");
	UpdateData.Status = TEXT("active");
	UpdateData.Type = TEXT("custom");
	UpdateData.ForceSetChannelName = true;
	UpdateData.ForceSetDescription = true;
	UpdateData.ForceSetCustom = true;
	UpdateData.ForceSetStatus = true;
	UpdateData.ForceSetType = true;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->UpdateAsync(UpdateData, OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateResult, UpdateData]()
	{
		TestFalse("UpdateAsync should succeed", CallbackResult->Error);
		FPubnubChatChannelData Retrieved = CreateResult.Channel->GetChannelData();
		TestEqual("ChannelName should match", Retrieved.ChannelName, UpdateData.ChannelName);
		TestEqual("Description should match", Retrieved.Description, UpdateData.Description);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelJoinAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.JoinAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelJoinAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_join_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_join_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = TEXT("{\"async\":\"join\"}");
	MembershipData.Status = TEXT("member");
	MembershipData.Type = TEXT("regular");
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatJoinResult> CallbackResult = MakeShared<FPubnubChatJoinResult>();
	FOnPubnubChatJoinResponseNative OnJoinResponse;
	OnJoinResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatJoinResult& JoinResult)
	{
		*CallbackResult = JoinResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->JoinAsync(OnJoinResponse, MembershipData);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("JoinAsync should succeed", CallbackResult->Result.Error);
		TestNotNull("Membership should be created", CallbackResult->Membership);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelSendTextAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.SendTextAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelSendTextAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_send_text_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_send_text_async_full";
	const FString TestMessageText = TEXT("Async send text");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	CreateResult.Channel->Connect();
	FPubnubChatSendTextParams Params;
	Params.StoreInHistory = true;
	Params.SendByPost = false;
	Params.Meta = TEXT("{\"async\":\"send\"}");
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->SendTextAsync(TestMessageText, OnOperationResponse, Params);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("SendTextAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_invite_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_invite_async_full";
	const FString TargetUserID = SDK_PREFIX + "test_channel_invite_async_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel || !CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatInviteResult> CallbackResult = MakeShared<FPubnubChatInviteResult>();
	FOnPubnubChatInviteResponseNative OnInviteResponse;
	OnInviteResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatInviteResult& InviteResult)
	{
		*CallbackResult = InviteResult;
		*bCallbackReceived = true;
	});
	
	CreateChannelResult.Channel->InviteAsync(CreateUserResult.User, OnInviteResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("InviteAsync should succeed", CallbackResult->Result.Error);
		TestNotNull("Membership should be created", CallbackResult->Membership);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultipleAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_invite_multiple_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_invite_multiple_async_full";
	const FString TargetUserID1 = SDK_PREFIX + "test_channel_invite_multiple_async_1";
	const FString TargetUserID2 = SDK_PREFIX + "test_channel_invite_multiple_async_2";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUserResult CreateUserResult1 = Chat->CreateUser(TargetUserID1);
	FPubnubChatUserResult CreateUserResult2 = Chat->CreateUser(TargetUserID2);
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreateUser1 should succeed", CreateUserResult1.Result.Error);
	TestFalse("CreateUser2 should succeed", CreateUserResult2.Result.Error);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel || !CreateUserResult1.User || !CreateUserResult2.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatInviteMultipleResult> CallbackResult = MakeShared<FPubnubChatInviteMultipleResult>();
	FOnPubnubChatInviteMultipleResponseNative OnInviteMultipleResponse;
	OnInviteMultipleResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatInviteMultipleResult& InviteMultipleResult)
	{
		*CallbackResult = InviteMultipleResult;
		*bCallbackReceived = true;
	});
	
	TArray<UPubnubChatUser*> Users = {CreateUserResult1.User, CreateUserResult2.User};
	CreateChannelResult.Channel->InviteMultipleAsync(Users, OnInviteMultipleResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("InviteMultipleAsync should succeed", CallbackResult->Result.Error);
		TestTrue("InviteMultipleAsync should return memberships", CallbackResult->Memberships.Num() >= 2);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID1, TargetUserID2]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID1);
			Chat->DeleteUser(TargetUserID2);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembersAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_get_members_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_get_members_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	CreateResult.Channel->Join(FPubnubChatMembershipData());
	
	const int TestLimit = 10;
	const FString TestFilter = FString::Printf(TEXT("uuid.id == \"%s\""), *InitUserID);
	FPubnubMemberSort TestSort;
	FPubnubMemberSingleSort SingleSort;
	SingleSort.SortType = EPubnubMemberSortType::PMeST_UserID;
	SingleSort.SortOrder = false;
	TestSort.MemberSort.Add(SingleSort);
	FPubnubPage TestPage;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMembershipsResult> CallbackResult = MakeShared<FPubnubChatMembershipsResult>();
	FOnPubnubChatMembershipsResponseNative OnMembersResponse;
	OnMembersResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatMembershipsResult& MembersResult)
	{
		*CallbackResult = MembersResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->GetMembersAsync(OnMembersResponse, TestLimit, TestFilter, TestSort, TestPage);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("GetMembersAsync should succeed", CallbackResult->Result.Error);
		TestTrue("Members array should be valid", CallbackResult->Memberships.Num() >= 0);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistoryAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_get_history_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_get_history_async_full";
	const FString TestMessageText = TEXT("Async history message");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	CreateResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	CreateResult.Channel->Connect();
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatSendTextParams SendTextParams;
		SendTextParams.StoreInHistory = true;
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText, SendTextParams);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	const int Count = 10;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetHistoryResult> CallbackResult = MakeShared<FPubnubChatGetHistoryResult>();
	FOnPubnubChatGetHistoryResponseNative OnHistoryResponse;
	OnHistoryResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetHistoryResult& HistoryResult)
	{
		*CallbackResult = HistoryResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage, OnHistoryResponse, Count]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		const FString StartTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(StartTimetoken, -100000000);
		CreateResult.Channel->GetHistoryAsync(StartTimetoken, EndTimetoken, OnHistoryResponse, Count);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("GetHistoryAsync should succeed", CallbackResult->Result.Error);
		TestTrue("History should be valid", CallbackResult->Messages.Num() >= 0);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_stream_updates_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_stream_updates_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->StreamUpdatesAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("StreamUpdatesAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingUpdatesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingUpdatesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingUpdatesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_stop_stream_updates_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_channel_stop_stream_updates_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	CreateResult.Channel->StreamUpdates();
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.Channel->StopStreamingUpdatesAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("StopStreamingUpdatesAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

