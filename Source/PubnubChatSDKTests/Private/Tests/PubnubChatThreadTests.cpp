// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatCallbackStop.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalConverters.h"
#include "PubnubChatEnumLibrary.h"
#include "Private/PubnubChatConst.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// CREATETHREADCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create thread channel without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMessage* Message = NewObject<UPubnubChatMessage>();
			if(Message)
			{
				FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(Message);
				
				TestTrue("CreateThreadChannel should fail when Chat is not initialized", CreateResult.Result.Error);
				TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelInvalidMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.1Validation.InvalidMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelInvalidMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_invalid_msg_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to create thread channel with null Message
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(nullptr);
		
		TestTrue("CreateThreadChannel should fail with null Message", CreateResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelDeletedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.1Validation.DeletedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelDeletedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_deleted_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_deleted_msg";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for deleted thread test");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Delete the message (soft delete)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit for delete to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is deleted
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestTrue("Message should be deleted", IsDeletedResult.IsDeleted);
		
		// Try to create thread channel on deleted message
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		TestTrue("CreateThreadChannel should fail on deleted message", CreateResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelMessageInThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.1Validation.MessageInThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelMessageInThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_in_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_in_thread";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for in thread test");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
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
	
	// Wait for thread message to be received
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ThreadMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	auto ThreadMessageLambda = [this, bThreadMessageReceived, ThreadMessage](UPubnubChatThreadMessage* Message)
	{
		if(Message && !*ThreadMessage)
		{
			*bThreadMessageReceived = true;
			*ThreadMessage = Message;
		}
	};
	
	// Connect to thread channel BEFORE sending message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageLambda]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Connect to thread channel to receive messages
		(*ThreadChannel)->OnThreadMessageReceivedNative.AddLambda(ThreadMessageLambda);
		FPubnubChatOperationResult ConnectResult = (*ThreadChannel)->Connect();
		TestFalse("Connect to thread channel should succeed", ConnectResult.Error);
	}, 0.2f));
	
	// Send text to thread channel to confirm it on server (after connection is established)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() -> bool {
		return *bThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Try to create thread on message that's already in a thread (should fail)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("Thread message was not received");
			return;
		}
		
		// Verify message is in a thread channel
		FPubnubChatMessageData MessageData = (*ThreadMessage)->GetMessageData();
		TestTrue("Message should be in thread channel", MessageData.ChannelID.StartsWith(Pubnub_Chat_Message_Thread_ID_Prefix));
		
		// Try to create thread channel on message that's already in a thread
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ThreadMessage);
		
		TestTrue("CreateThreadChannel should fail when message is already in a thread", CreateResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
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
			// Remove thread
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelThreadAlreadyExistsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.1Validation.ThreadAlreadyExists", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelThreadAlreadyExistsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_exists_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_exists";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for thread exists test");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel first time (local only)
	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		TestFalse("CreateThreadChannel should succeed first time", CreateResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateResult.ThreadChannel);
		
		*ThreadChannel = CreateResult.ThreadChannel;
	}, 0.1f));
	
	// Try to create thread channel again locally (should succeed - local creation is allowed)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		// Note: Creating thread twice locally should succeed (thread not confirmed on server yet)
		// The test will verify failure only after SendText confirms the thread
		TestFalse("CreateThreadChannel should succeed second time (local only)", CreateResult.Result.Error);
	}, 0.1f));
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then try to create thread again (should fail)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Try to create thread channel again after SendText confirmed it (should fail)
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		TestTrue("CreateThreadChannel should fail when thread already exists after SendText", CreateResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}, 1.0f));
	
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
			// Remove thread if it was created and confirmed
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_happy";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for thread happy path");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		TestFalse("CreateThreadChannel should succeed", CreateResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateResult.ThreadChannel);
		
		if(CreateResult.ThreadChannel)
		{
			// Verify thread channel properties
			TestEqual("ParentChannelID should match", CreateResult.ThreadChannel->GetParentChannelID(), (*ReceivedMessage)->GetMessageData().ChannelID);
			TestEqual("ParentMessage should match", CreateResult.ThreadChannel->GetParentMessage(), *ReceivedMessage);
			
			// Verify message doesn't have thread yet (not confirmed)
			FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
			TestFalse("Message should not have thread yet (not confirmed)", HasThreadResult.HasThread);
		}
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread if it was created and confirmed
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// CreateThreadChannel has no optional parameters, so full parameter test is same as happy path
// Skipping redundant test

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that creating thread without SendText makes it local only, so GetThread should fail.
 * Verifies that thread is only created on server after SendText is called.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelLocalOnlyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.4Advanced.LocalOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelLocalOnlyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_local_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_local";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for local thread test");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel (local only, not confirmed)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		TestFalse("CreateThreadChannel should succeed", CreateResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateResult.ThreadChannel);
		
		// Verify GetThreadChannel fails (thread not confirmed on server)
		FPubnubChatThreadChannelResult GetResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestTrue("GetThreadChannel should fail when thread is only local", GetResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread if it was created and confirmed
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

/**
 * Tests that after creating thread and calling SendText, trying to create thread again causes error.
 * Verifies that SendText confirms the thread on server and prevents duplicate creation.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelAfterSendTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannel.4Advanced.AfterSendText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelAfterSendTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_sendtext_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_sendtext";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for sendtext thread test");
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message now has thread (confirmed)
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestTrue("Message should have thread after SendText", HasThreadResult.HasThread);
		
		// Verify message has ThreadRootId MessageAction
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasThreadRootAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_ThreadRootId && !Action.Value.IsEmpty())
			{
				bHasThreadRootAction = true;
				break;
			}
		}
		TestTrue("Message should have ThreadRootId MessageAction", bHasThreadRootAction);
		
		// Try to create thread channel again (should fail)
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(*ReceivedMessage);
		
		TestTrue("CreateThreadChannel should fail when thread already exists after SendText", CreateResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread
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
// GETTHREADCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetThreadChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.GetThreadChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetThreadChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get thread channel without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMessage* Message = NewObject<UPubnubChatMessage>();
			if(Message)
			{
				FPubnubChatThreadChannelResult GetResult = Chat->GetThreadChannel(Message);
				
				TestTrue("GetThreadChannel should fail when Chat is not initialized", GetResult.Result.Error);
				TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetThreadChannelInvalidMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.GetThreadChannel.1Validation.InvalidMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetThreadChannelInvalidMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_invalid_msg_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get thread channel with null Message
		FPubnubChatThreadChannelResult GetResult = Chat->GetThreadChannel(nullptr);
		
		TestTrue("GetThreadChannel should fail with null Message", GetResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetThreadChannelThreadDoesNotExistTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.GetThreadChannel.1Validation.ThreadDoesNotExist", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetThreadChannelThreadDoesNotExistTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_not_exists_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_not_exists";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for get thread not exists");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Try to get thread channel that doesn't exist
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult GetResult = Chat->GetThreadChannel(*ReceivedMessage);
		
		TestTrue("GetThreadChannel should fail when thread doesn't exist", GetResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetThreadChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.GetThreadChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetThreadChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_happy";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for get thread happy");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then get thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult GetResult = Chat->GetThreadChannel(*ReceivedMessage);
		
		TestFalse("GetThreadChannel should succeed", GetResult.Result.Error);
		TestNotNull("ThreadChannel should be retrieved", GetResult.ThreadChannel);
		
		if(GetResult.ThreadChannel)
		{
			// Verify thread channel properties
			TestEqual("ParentChannelID should match", GetResult.ThreadChannel->GetParentChannelID(), (*ReceivedMessage)->GetMessageData().ChannelID);
			TestEqual("ParentMessage should match", GetResult.ThreadChannel->GetParentMessage(), *ReceivedMessage);
		}
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread
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
// REMOVETHREADCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatRemoveThreadChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.RemoveThreadChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRemoveThreadChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to remove thread channel without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMessage* Message = NewObject<UPubnubChatMessage>();
			if(Message)
			{
				FPubnubChatOperationResult RemoveResult = Chat->RemoveThreadChannel(Message);
				
				TestTrue("RemoveThreadChannel should fail when Chat is not initialized", RemoveResult.Error);
				TestFalse("ErrorMessage should not be empty", RemoveResult.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatRemoveThreadChannelInvalidMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.RemoveThreadChannel.1Validation.InvalidMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRemoveThreadChannelInvalidMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_remove_thread_invalid_msg_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to remove thread channel with null Message
		FPubnubChatOperationResult RemoveResult = Chat->RemoveThreadChannel(nullptr);
		
		TestTrue("RemoveThreadChannel should fail with null Message", RemoveResult.Error);
		TestFalse("ErrorMessage should not be empty", RemoveResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatRemoveThreadChannelNoThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.RemoveThreadChannel.1Validation.NoThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRemoveThreadChannelNoThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_remove_thread_no_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_remove_thread_no_thread";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for remove thread no thread");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Try to remove thread channel that doesn't exist
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult RemoveResult = Chat->RemoveThreadChannel(*ReceivedMessage);
		
		TestTrue("RemoveThreadChannel should fail when thread doesn't exist", RemoveResult.Error);
		TestFalse("ErrorMessage should not be empty", RemoveResult.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatRemoveThreadChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.RemoveThreadChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRemoveThreadChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_remove_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_remove_thread_happy";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for remove thread happy");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then verify and remove thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message has thread before removal (wait a bit more to ensure thread is confirmed)
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		if(!HasThreadResult.HasThread)
		{
			AddError("Message should have thread before removal - thread may not be confirmed yet");
			return;
		}
		TestTrue("Message should have thread before removal", HasThreadResult.HasThread);
		
		// Remove thread channel
		FPubnubChatOperationResult RemoveResult = Chat->RemoveThreadChannel(*ReceivedMessage);
		
		if(RemoveResult.Error)
		{
			AddError(FString::Printf(TEXT("RemoveThreadChannel failed: %s"), *RemoveResult.ErrorMessage));
			return;
		}
		TestFalse("RemoveThreadChannel should succeed", RemoveResult.Error);
	}, 1.5f));
	
	// Wait a bit for removal to propagate, then verify message no longer has thread
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message no longer has thread
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("Message should not have thread after removal", HasThreadResult.HasThread);
		
		// Verify ThreadRootId MessageAction was removed
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasThreadRootAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_ThreadRootId && !Action.Value.IsEmpty())
			{
				bHasThreadRootAction = true;
				break;
			}
		}
		TestFalse("Message should not have ThreadRootId MessageAction after removal", bHasThreadRootAction);
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
// HASTHREAD TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasThread.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageHasThreadNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Message without initializing
	UPubnubChatMessage* Message = NewObject<UPubnubChatMessage>();
	
	if(Message)
	{
		FPubnubChatHasThreadResult HasThreadResult = Message->HasThread();
		
		TestTrue("HasThread should fail when Message is not initialized", HasThreadResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HasThreadResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasThread.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageHasThreadHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_has_thread_happy";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for has thread happy");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Check HasThread before creating thread (should be false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestFalse("Message should not have thread initially", HasThreadResult.HasThread);
	}, 0.1f));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then check HasThread (should be true)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread after SendText", HasThreadResult.HasThread);
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread
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
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests HasThread before and after creating/removing thread.
 * Verifies that HasThread correctly reflects thread state changes.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadStateChangesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasThread.4Advanced.StateChanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageHasThreadStateChangesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_thread_state_init";
	const FString TestChannelID = SDK_PREFIX + "test_has_thread_state";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for has thread state");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Check HasThread initially (should be false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestFalse("Message should not have thread initially", HasThreadResult.HasThread);
	}, 0.1f));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then check HasThread (should be true)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread after SendText", HasThreadResult.HasThread);
	}, 1.0f));
	
	// Remove thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult RemoveResult = Chat->RemoveThreadChannel(*ReceivedMessage);
		
		TestFalse("RemoveThreadChannel should succeed", RemoveResult.Error);
	}, 0.1f));
	
	// Wait a bit for removal to propagate, then check HasThread again (should be false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestFalse("Message should not have thread after removal", HasThreadResult.HasThread);
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
// MESSAGE DELETE/RESTORE WITH THREAD TESTS
// ============================================================================

/**
 * Tests that when a message with thread is soft-deleted, the thread is also soft-deleted.
 * Verifies that HasThread still returns true (thread exists but deleted) and GetThreadChannel returns the thread.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageSoftDeleteWithThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.MessageDelete.4Advanced.SoftDeleteWithThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageSoftDeleteWithThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_msg_soft_delete_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_msg_soft_delete_thread";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for soft delete thread");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then verify thread exists
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message has thread before deletion
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread before deletion", HasThreadResult.HasThread);
	}, 1.0f));
	
	// Soft delete the message (should also soft delete the thread)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete message should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit for deletion to propagate, then verify thread is also deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is deleted
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedResult.Result.Error);
		TestTrue("Message should be deleted", IsDeletedResult.IsDeleted);
		
		// Verify message still has thread (thread exists but is soft-deleted)
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should still have thread after soft deletion (thread exists but deleted)", HasThreadResult.HasThread);
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Hard delete the message and thread for cleanup
			(*ReceivedMessage)->Delete(false);
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that when a message with thread is hard-deleted, the thread is also hard-deleted.
 * Verifies that thread channel no longer exists after hard deletion.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHardDeleteWithThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.MessageDelete.4Advanced.HardDeleteWithThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageHardDeleteWithThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_msg_hard_delete_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_msg_hard_delete_thread";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for hard delete thread");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	TSharedPtr<FString> ThreadChannelID = MakeShared<FString>(TEXT(""));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel, ThreadChannelID]()
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
		if(*ThreadChannel)
		{
			*ThreadChannelID = (*ThreadChannel)->GetChannelID();
		}
	}, 0.1f));
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then verify thread exists
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message has thread before deletion
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread before deletion", HasThreadResult.HasThread);
	}, 1.0f));
	
	// Hard delete the message (should also hard delete the thread)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Hard delete the message
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(false);
		TestFalse("Hard delete message should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit for deletion to propagate, then verify thread is also deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ThreadChannelID]()
	{
		if(ThreadChannelID->IsEmpty())
		{
			AddError("ThreadChannelID was not captured");
			return;
		}
		
		// Verify thread channel no longer exists (hard deleted)
		FPubnubChatChannelResult GetThreadResult = Chat->GetChannel(*ThreadChannelID);
		TestTrue("GetChannel should fail for hard-deleted thread", GetThreadResult.Result.Error);
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
 * Tests that when a soft-deleted message with thread is restored, the thread is also restored.
 * Verifies that HasThread returns true after restoration.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreWithThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.MessageDelete.4Advanced.RestoreWithThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageRestoreWithThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_msg_restore_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_msg_restore_thread";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for restore thread");
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Create thread channel and send text to confirm it
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
	
	// Send text to thread channel to confirm it on server
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit for thread confirmation, then soft delete message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message has thread before deletion
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread before deletion", HasThreadResult.HasThread);
		
		// Soft delete the message (should also soft delete the thread)
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete message should succeed", DeleteResult.Error);
	}, 1.0f));
	
	// Wait a bit for deletion to propagate, then verify both are deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is deleted
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedResult.Result.Error);
		TestTrue("Message should be deleted", IsDeletedResult.IsDeleted);
		
		// Verify message still has thread (thread exists but is soft-deleted)
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should still have thread after soft deletion (thread exists but deleted)", HasThreadResult.HasThread);
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
	}, 1.0f));
	
	// Restore the message (should also restore the thread)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Restore the message
		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage)->Restore();
		TestFalse("Restore message should succeed", RestoreResult.Error);
	}, 0.1f));
	
	// Wait a bit for restoration to propagate, then verify both are restored
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is restored
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedResult.Result.Error);
		TestFalse("Message should not be deleted after restore", IsDeletedResult.IsDeleted);
		
		// Verify message has thread again after restoration
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		TestFalse("HasThread should succeed", HasThreadResult.Result.Error);
		TestTrue("Message should have thread after restoration", HasThreadResult.HasThread);
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
	}, 1.0f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			// Remove thread before cleanup
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
// GETTHREADHISTORY TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create ThreadChannel without initializing
	UPubnubChatThreadChannel* ThreadChannel = NewObject<UPubnubChatThreadChannel>();
	
	if(ThreadChannel)
	{
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = ThreadChannel->GetThreadHistory(StartTimetoken, EndTimetoken);
		
		TestTrue("GetThreadHistory should fail when ThreadChannel is not initialized", GetHistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryEmptyStartTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.1Validation.EmptyStartTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryEmptyStartTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_history_empty_start_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_history_empty_start";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for empty start timetoken");
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
	
	// Send text to thread channel to confirm it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Try to get thread history with empty StartTimetoken
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString EmptyStartTimetoken = TEXT("");
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(EmptyStartTimetoken, EndTimetoken);
		
		TestTrue("GetThreadHistory should fail with empty StartTimetoken", GetHistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryEmptyEndTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.1Validation.EmptyEndTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryEmptyEndTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_history_empty_end_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_history_empty_end";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for empty end timetoken");
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
	
	// Send text to thread channel to confirm it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Try to get thread history with empty EndTimetoken
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EmptyEndTimetoken = TEXT("");
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EmptyEndTimetoken);
		
		TestTrue("GetThreadHistory should fail with empty EndTimetoken", GetHistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_history_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_history_happy";
	
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
	const FString TestMessageText = TEXT("Test message for thread history");
	
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
	TSharedPtr<FString> ThreadMessageTimetoken = MakeShared<FString>();
	const FString ThreadMessageText = TEXT("Thread message for history");
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageText, ThreadMessageTimetoken]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
		*ThreadMessageTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	}, 0.5f));
	
	// Wait a bit for message to be stored
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageTimetoken, ThreadMessageText]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Get thread history with only required parameters (StartTimetoken, EndTimetoken) and default Count
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000); // 10 seconds ago
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EndTimetoken);
		
		TestFalse("GetThreadHistory should succeed", GetHistoryResult.Result.Error);
		TestTrue("ThreadMessages array should be valid", GetHistoryResult.ThreadMessages.Num() >= 0);
		
		// Verify that we got at least the message we sent
		if(GetHistoryResult.ThreadMessages.Num() > 0)
		{
			bool FoundThreadMessage = false;
			for(UPubnubChatThreadMessage* ThreadMessage : GetHistoryResult.ThreadMessages)
			{
				if(ThreadMessage)
				{
					FPubnubChatMessageData MessageData = ThreadMessage->GetMessageData();
					if(MessageData.Text.Contains(ThreadMessageText))
					{
						FoundThreadMessage = true;
						TestEqual("Message ParentChannelID should match", ThreadMessage->GetParentChannelID(), (*ThreadChannel)->GetParentChannelID());
						TestFalse("Message Timetoken should not be empty", ThreadMessage->GetMessageTimetoken().IsEmpty());
						break;
					}
				}
			}
			TestTrue("Test thread message should be found in history", FoundThreadMessage);
		}
	}, 1.0f));
	
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_history_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_history_full";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for thread history full");
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
	
	// Send multiple messages to thread channel
	TSharedPtr<TArray<FString>> SentMessages = MakeShared<TArray<FString>>();
	const int NumMessages = 5;
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, SentMessages, NumMessages]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		for(int i = 0; i < NumMessages; i++)
		{
			const FString ThreadMessageText = FString::Printf(TEXT("Thread message %d"), i);
			SentMessages->Add(ThreadMessageText);
			FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
			TestFalse(FString::Printf(TEXT("SendText %d should succeed"), i), SendResult.Error);
		}
	}, 0.5f));
	
	// Wait a bit for messages to be stored, then get thread history with all parameters
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, SentMessages, NumMessages]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Get thread history with all parameters (StartTimetoken, EndTimetoken, Count)
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000); // 10 seconds ago
		const int TestCount = 3; // Request only 3 messages
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EndTimetoken, TestCount);
		
		TestFalse("GetThreadHistory should succeed with all parameters", GetHistoryResult.Result.Error);
		TestTrue("ThreadMessages array should be valid", GetHistoryResult.ThreadMessages.Num() >= 0);
		TestTrue("Should respect Count parameter", GetHistoryResult.ThreadMessages.Num() <= TestCount);
		
		// Verify IsMore flag is set correctly
		if(GetHistoryResult.ThreadMessages.Num() == TestCount)
		{
			TestTrue("IsMore should be true when we got exactly Count messages", GetHistoryResult.IsMore);
		}
		else
		{
			TestFalse("IsMore should be false when we got fewer than Count messages", GetHistoryResult.IsMore);
		}
		
		// Verify all returned messages are ThreadMessages with correct ParentChannelID
		for(UPubnubChatThreadMessage* ThreadMessage : GetHistoryResult.ThreadMessages)
		{
			if(ThreadMessage)
			{
				TestEqual("ThreadMessage ParentChannelID should match", ThreadMessage->GetParentChannelID(), (*ThreadChannel)->GetParentChannelID());
				TestFalse("ThreadMessage Timetoken should not be empty", ThreadMessage->GetMessageTimetoken().IsEmpty());
			}
		}
	}, 1.5f));
	
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
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetThreadHistory with empty thread (no messages).
 * Verifies that GetThreadHistory returns empty array when there are no messages in the time range.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryEmptyThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistory.4Advanced.EmptyThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryEmptyThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_history_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_history_empty";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for empty thread");
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
	
	// Send text to thread channel to confirm it (but don't send any messages to the thread)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread confirmation message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Wait a bit, then get thread history with a time range that doesn't include the confirmation message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Use a time range in the past (before thread was created)
		// StartTimetoken must be greater than EndTimetoken (start from newer, go backwards to older)
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -150000000); // 15 seconds ago (newer)
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -200000000); // 20 seconds ago (older)
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EndTimetoken);
		
		TestFalse("GetThreadHistory should succeed even with empty result", GetHistoryResult.Result.Error);
		TestEqual("ThreadMessages array should be empty", GetHistoryResult.ThreadMessages.Num(), 0);
		TestFalse("IsMore should be false when no messages found", GetHistoryResult.IsMore);
	}, 1.0f));
	
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
// PINMESSAGETOPARENTCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentChannelNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create ThreadChannel without initializing
	UPubnubChatThreadChannel* ThreadChannel = NewObject<UPubnubChatThreadChannel>();
	UPubnubChatThreadMessage* ThreadMessage = NewObject<UPubnubChatThreadMessage>();
	
	if(ThreadChannel && ThreadMessage)
	{
		FPubnubChatOperationResult PinResult = ThreadChannel->PinMessageToParentChannel(ThreadMessage);
		
		TestTrue("PinMessageToParentChannel should fail when ThreadChannel is not initialized", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentChannelNullMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannel.1Validation.NullMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentChannelNullMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_parent_null_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_parent_null_msg";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for pin null");
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
	
	// Send text to thread channel to confirm it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Try to pin null message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadChannel)->PinMessageToParentChannel(nullptr);
		
		TestTrue("PinMessageToParentChannel should fail with null message", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentChannelDifferentThreadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannel.1Validation.DifferentThread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentChannelDifferentThreadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_parent_diff_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_parent_diff_thread";
	
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
	
	// Create two channels
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult1 = Chat->CreatePublicConversation(TestChannelID + TEXT("_1"), ChannelData);
	TestFalse("CreatePublicConversation 1 should succeed", CreateChannelResult1.Result.Error);
	TestNotNull("Channel 1 should be created", CreateChannelResult1.Channel);
	
	FPubnubChatChannelResult CreateChannelResult2 = Chat->CreatePublicConversation(TestChannelID + TEXT("_2"), ChannelData);
	TestFalse("CreatePublicConversation 2 should succeed", CreateChannelResult2.Result.Error);
	TestNotNull("Channel 2 should be created", CreateChannelResult2.Channel);
	
	if(!CreateChannelResult1.Channel || !CreateChannelResult2.Channel)
	{
		if(CreateChannelResult1.Channel)
		{
			Chat->DeleteChannel(TestChannelID + TEXT("_1"));
		}
		if(CreateChannelResult2.Channel)
		{
			Chat->DeleteChannel(TestChannelID + TEXT("_2"));
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessage1Received = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage1 = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<bool> bMessage2Received = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage2 = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect with callbacks to receive messages
	auto MessageLambda1 = [this, bMessage1Received, ReceivedMessage1](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage1)
		{
			*bMessage1Received = true;
			*ReceivedMessage1 = Message;
		}
	};
	auto MessageLambda2 = [this, bMessage2Received, ReceivedMessage2](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage2)
		{
			*bMessage2Received = true;
			*ReceivedMessage2 = Message;
		}
	};
	CreateChannelResult1.Channel->OnMessageReceivedNative.AddLambda(MessageLambda1);
	CreateChannelResult2.Channel->OnMessageReceivedNative.AddLambda(MessageLambda2);
	
	FPubnubChatOperationResult ConnectResult1 = CreateChannelResult1.Channel->Connect();
	TestFalse("Connect 1 should succeed", ConnectResult1.Error);
	FPubnubChatOperationResult ConnectResult2 = CreateChannelResult2.Channel->Connect();
	TestFalse("Connect 2 should succeed", ConnectResult2.Error);
	
	// Wait for subscriptions, then send messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult1, CreateChannelResult2]()
	{
		const FString TestMessageText1 = TEXT("Test message 1");
		FPubnubChatOperationResult SendResult1 = CreateChannelResult1.Channel->SendText(TestMessageText1);
		TestFalse("SendText 1 should succeed", SendResult1.Error);
		
		const FString TestMessageText2 = TEXT("Test message 2");
		FPubnubChatOperationResult SendResult2 = CreateChannelResult2.Channel->SendText(TestMessageText2);
		TestFalse("SendText 2 should succeed", SendResult2.Error);
	}, 0.5f));
	
	// Wait until messages are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessage1Received, bMessage2Received]() -> bool {
		return *bMessage1Received && *bMessage2Received;
	}, MAX_WAIT_TIME));
	
	// Create thread channels
	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel1 = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel2 = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage1, ReceivedMessage2, ThreadChannel1, ThreadChannel2]()
	{
		if(!*ReceivedMessage1 || !*ReceivedMessage2)
		{
			AddError("Messages were not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult1 = Chat->CreateThreadChannel(*ReceivedMessage1);
		TestFalse("CreateThreadChannel 1 should succeed", CreateResult1.Result.Error);
		TestNotNull("ThreadChannel 1 should be created", CreateResult1.ThreadChannel);
		*ThreadChannel1 = CreateResult1.ThreadChannel;
		
		FPubnubChatThreadChannelResult CreateResult2 = Chat->CreateThreadChannel(*ReceivedMessage2);
		TestFalse("CreateThreadChannel 2 should succeed", CreateResult2.Result.Error);
		TestNotNull("ThreadChannel 2 should be created", CreateResult2.ThreadChannel);
		*ThreadChannel2 = CreateResult2.ThreadChannel;
	}, 0.1f));
	
	// Send text to thread channels to confirm them
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel1, ThreadChannel2]()
	{
		if(!*ThreadChannel1 || !*ThreadChannel2)
		{
			AddError("ThreadChannels were not created");
			return;
		}
		
		const FString ThreadMessageText1 = TEXT("Thread message 1");
		FPubnubChatOperationResult SendResult1 = (*ThreadChannel1)->SendText(ThreadMessageText1);
		TestFalse("SendText to thread 1 should succeed", SendResult1.Error);
		
		const FString ThreadMessageText2 = TEXT("Thread message 2");
		FPubnubChatOperationResult SendResult2 = (*ThreadChannel2)->SendText(ThreadMessageText2);
		TestFalse("SendText to thread 2 should succeed", SendResult2.Error);
	}, 0.2f));
	
	// Get thread messages from history
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage2 = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel1, ThreadChannel2, ThreadMessage2]()
	{
		if(!*ThreadChannel1 || !*ThreadChannel2)
		{
			AddError("ThreadChannels were not created");
			return;
		}
		
		// Get thread history from ThreadChannel2 to get a ThreadMessage
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel2)->GetThreadHistory(StartTimetoken, EndTimetoken);
		TestFalse("GetThreadHistory should succeed", GetHistoryResult.Result.Error);
		
		if(GetHistoryResult.ThreadMessages.Num() > 0)
		{
			*ThreadMessage2 = GetHistoryResult.ThreadMessages[0];
		}
	}, 1.0f));
	
	// Try to pin message from ThreadChannel2 to parent channel using ThreadChannel1
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel1, ThreadMessage2]()
	{
		if(!*ThreadChannel1)
		{
			AddError("ThreadChannel1 was not created");
			return;
		}
		
		if(!*ThreadMessage2)
		{
			AddError("ThreadMessage2 was not retrieved");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadChannel1)->PinMessageToParentChannel(*ThreadMessage2);
		
		TestTrue("PinMessageToParentChannel should fail with message from different thread channel", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult1, CreateChannelResult2, Chat, TestChannelID, ReceivedMessage1, ReceivedMessage2, ThreadChannel1, ThreadChannel2]()
	{
		if(*ThreadChannel1)
		{
			(*ThreadChannel1)->Disconnect();
		}
		if(*ThreadChannel2)
		{
			(*ThreadChannel2)->Disconnect();
		}
		if(CreateChannelResult1.Channel)
		{
			CreateChannelResult1.Channel->Disconnect();
		}
		if(CreateChannelResult2.Channel)
		{
			CreateChannelResult2.Channel->Disconnect();
		}
		if(Chat)
		{
			if(*ReceivedMessage1)
			{
				FPubnubChatHasThreadResult HasThreadResult1 = (*ReceivedMessage1)->HasThread();
				if(HasThreadResult1.HasThread)
				{
					Chat->RemoveThreadChannel(*ReceivedMessage1);
				}
			}
			if(*ReceivedMessage2)
			{
				FPubnubChatHasThreadResult HasThreadResult2 = (*ReceivedMessage2)->HasThread();
				if(HasThreadResult2.HasThread)
				{
					Chat->RemoveThreadChannel(*ReceivedMessage2);
				}
			}
			Chat->DeleteChannel(TestChannelID + TEXT("_1"));
			Chat->DeleteChannel(TestChannelID + TEXT("_2"));
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_parent_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_parent_happy";
	
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
	const FString TestMessageText = TEXT("Test message for pin to parent");
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage, CreateChannelResult]()
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
		
		// Verify pinned message is stored in parent channel data
		FPubnubChatChannelData UpdatedChannelData = CreateChannelResult.Channel->GetChannelData();
		TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		
		// Verify pinned message can be retrieved from parent channel
		FPubnubChatMessageResult GetPinnedResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", GetPinnedResult.Result.Error);
		if(GetPinnedResult.Message)
		{
			TestEqual("Pinned message timetoken should match", GetPinnedResult.Message->GetMessageTimetoken(), (*ThreadMessage)->GetMessageTimetoken());
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// PinMessageToParentChannel has no optional parameters, so full parameter test is same as happy path

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests pinning a thread message twice to parent channel.
 * Verifies that pinning a thread message twice overrides the previously pinned message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentChannelTwiceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannel.4Advanced.PinTwiceOverride", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentChannelTwiceTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_parent_twice_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_parent_twice";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for pin twice");
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
	
	// Send two messages to thread channel
	const FString ThreadMessageText1 = TEXT("First thread message");
	const FString ThreadMessageText2 = TEXT("Second thread message");
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage1 = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage2 = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageText1, ThreadMessageText2]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult SendResult1 = (*ThreadChannel)->SendText(ThreadMessageText1);
		TestFalse("SendText 1 should succeed", SendResult1.Error);
		
		FPubnubChatOperationResult SendResult2 = (*ThreadChannel)->SendText(ThreadMessageText2);
		TestFalse("SendText 2 should succeed", SendResult2.Error);
	}, 0.5f));
	
	// Get thread messages from history
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage1, ThreadMessage2, ThreadMessageText1, ThreadMessageText2]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		// Get thread history to get ThreadMessages
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);
		
		FPubnubChatGetThreadHistoryResult GetHistoryResult = (*ThreadChannel)->GetThreadHistory(StartTimetoken, EndTimetoken);
		TestFalse("GetThreadHistory should succeed", GetHistoryResult.Result.Error);
		
		// Find the thread messages we sent
		for(UPubnubChatThreadMessage* TM : GetHistoryResult.ThreadMessages)
		{
			if(TM)
			{
				FPubnubChatMessageData MessageData = TM->GetMessageData();
				if(MessageData.Text.Contains(ThreadMessageText1) && !*ThreadMessage1)
				{
					*ThreadMessage1 = TM;
				}
				else if(MessageData.Text.Contains(ThreadMessageText2) && !*ThreadMessage2)
				{
					*ThreadMessage2 = TM;
				}
			}
		}
	}, 1.0f));
	
	// Pin first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage1, CreateChannelResult]()
	{
		if(!*ThreadChannel || !*ThreadMessage1)
		{
			AddError("ThreadChannel or ThreadMessage1 was not ready");
			return;
		}
		
		FPubnubChatOperationResult PinResult1 = (*ThreadChannel)->PinMessageToParentChannel(*ThreadMessage1);
		TestFalse("First PinMessageToParentChannel should succeed", PinResult1.Error);
		
		// Verify first message is pinned
		FPubnubChatMessageResult GetPinnedResult1 = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", GetPinnedResult1.Result.Error);
		if(GetPinnedResult1.Message)
		{
			TestEqual("First pinned message timetoken should match", GetPinnedResult1.Message->GetMessageTimetoken(), (*ThreadMessage1)->GetMessageTimetoken());
		}
	}, 0.5f));
	
	// Pin second message (should override first)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage2, CreateChannelResult, ThreadMessage1]()
	{
		if(!*ThreadChannel || !*ThreadMessage2)
		{
			AddError("ThreadChannel or ThreadMessage2 was not ready");
			return;
		}
		
		FPubnubChatOperationResult PinResult2 = (*ThreadChannel)->PinMessageToParentChannel(*ThreadMessage2);
		TestFalse("Second PinMessageToParentChannel should succeed", PinResult2.Error);
		
		// Verify second message is now pinned (overriding first)
		FPubnubChatMessageResult GetPinnedResult2 = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", GetPinnedResult2.Result.Error);
		if(GetPinnedResult2.Message)
		{
			TestEqual("Second pinned message timetoken should match", GetPinnedResult2.Message->GetMessageTimetoken(), (*ThreadMessage2)->GetMessageTimetoken());
			TestNotEqual("Second pinned message should be different from first", GetPinnedResult2.Message->GetMessageTimetoken(), (*ThreadMessage1)->GetMessageTimetoken());
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
// UNPINMESSAGEFROMPARENTCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelUnpinMessageFromParentChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.UnpinMessageFromParentChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelUnpinMessageFromParentChannelNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create ThreadChannel without initializing
	UPubnubChatThreadChannel* ThreadChannel = NewObject<UPubnubChatThreadChannel>();
	
	if(ThreadChannel)
	{
		FPubnubChatOperationResult UnpinResult = ThreadChannel->UnpinMessageFromParentChannel();
		
		TestTrue("UnpinMessageFromParentChannel should fail when ThreadChannel is not initialized", UnpinResult.Error);
		TestFalse("ErrorMessage should not be empty", UnpinResult.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelUnpinMessageFromParentChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.UnpinMessageFromParentChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelUnpinMessageFromParentChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_parent_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_from_parent_happy";
	
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
	const FString TestMessageText = TEXT("Test message for unpin from parent");
	
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
	
	// Pin the thread message to parent channel first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessage]()
	{
		if(!*ThreadChannel || !*ThreadMessage)
		{
			AddError("ThreadChannel or ThreadMessage was not ready");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadChannel)->PinMessageToParentChannel(*ThreadMessage);
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.5f));
	
	// Unpin the message from parent channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, CreateChannelResult]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult UnpinResult = (*ThreadChannel)->UnpinMessageFromParentChannel();
		TestFalse("UnpinMessageFromParentChannel should succeed", UnpinResult.Error);
		
		// Verify message is no longer pinned in parent channel
		FPubnubChatChannelData UpdatedChannelData = CreateChannelResult.Channel->GetChannelData();
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		UPubnubJsonUtilities::StringToJsonObject(UpdatedChannelData.Custom, JsonObject);
		
		FString PinnedMessageTimetoken;
		bool HasPinnedMessage = JsonObject->TryGetStringField(UPubnubChatInternalUtilities::GetPinnedMessageTimetokenPropertyKey(), PinnedMessageTimetoken);
		TestFalse("Channel Custom should not contain pinned message data after unpin", HasPinnedMessage && !PinnedMessageTimetoken.IsEmpty());
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// UnpinMessageFromParentChannel has no optional parameters, so full parameter test is same as happy path

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests unpinning when no message is pinned.
 * Verifies that UnpinMessageFromParentChannel doesn't error when no message is pinned.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelUnpinMessageFromParentChannelNoPinnedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.UnpinMessageFromParentChannel.4Advanced.NoPinnedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelUnpinMessageFromParentChannelNoPinnedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_parent_no_pin_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_from_parent_no_pin";
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		const FString TestMessageText = TEXT("Test message for unpin no pin");
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
	
	// Send text to thread channel to confirm it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString ThreadMessageText = TEXT("Thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.2f));
	
	// Try to unpin when no message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		FPubnubChatOperationResult UnpinResult = (*ThreadChannel)->UnpinMessageFromParentChannel();
		
		// UnpinMessage should not error when no message is pinned (based on similar test for regular channels)
		TestFalse("UnpinMessageFromParentChannel should not error when no message is pinned", UnpinResult.Error);
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
// ASYNC FULL PARAMETER TESTS (Threads)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateThreadChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.CreateThreadChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateThreadChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_thread_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_thread_async_full";
	const FString TestMessageText = TEXT("Async thread message");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
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
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatThreadChannelResult> CallbackResult = MakeShared<FPubnubChatThreadChannelResult>();
	FOnPubnubChatThreadChannelResponseNative OnThreadResponse;
	OnThreadResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatThreadChannelResult& ThreadResult)
	{
		*CallbackResult = ThreadResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, OnThreadResponse]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received before creating thread");
			return;
		}
		Chat->CreateThreadChannelAsync(*ReceivedMessage, OnThreadResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("CreateThreadChannelAsync should succeed", CallbackResult->Result.Error);
		TestNotNull("ThreadChannel should be returned", CallbackResult->ThreadChannel);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateChannelResult, ReceivedMessage, TestChannelID]()
	{
		if(*ReceivedMessage)
		{
			Chat->RemoveThreadChannel(*ReceivedMessage);
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetThreadChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.GetThreadChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetThreadChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_thread_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_thread_async_full";
	const FString TestMessageText = TEXT("Async get thread message");
	const FString TestThreadText = TEXT("Thread propagation");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bGetThreadRequestSent = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatThreadChannelResult> CallbackResult = MakeShared<FPubnubChatThreadChannelResult>();
	FOnPubnubChatThreadChannelResponseNative OnThreadResponse;
	OnThreadResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatThreadChannelResult& ThreadResult)
	{
		*CallbackResult = ThreadResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, TestThreadText, bCallbackReceived, bGetThreadRequestSent, OnThreadResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when creating thread for GetThreadChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(ReceivedMessage->Get());
		TestFalse("CreateThreadChannel should succeed", CreateResult.Result.Error);
		if(!CreateResult.ThreadChannel)
		{
			*bCallbackReceived = true;
			return;
		}
		// SendText on thread channel propagates the thread to the server; otherwise GetThreadChannelAsync may fail (no such channel)
		CreateResult.ThreadChannel->Connect();
		FPubnubChatOperationResult SendResult = CreateResult.ThreadChannel->SendText(TestThreadText);
		if(SendResult.Error) { AddError("Thread SendText failed"); *bCallbackReceived = true; return; }
		FPlatformProcess::Sleep(1.0f);
		if(!ReceivedMessage->IsValid()) { AddError("Message invalid before GetThreadChannelAsync"); *bCallbackReceived = true; return; }
		*bGetThreadRequestSent = true;
		Chat->GetThreadChannelAsync(ReceivedMessage->Get(), OnThreadResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, bGetThreadRequestSent]()
	{
		if(*bGetThreadRequestSent)
		{
			TestFalse("GetThreadChannelAsync should succeed", CallbackResult->Result.Error);
			TestNotNull("ThreadChannel should be returned", CallbackResult->ThreadChannel);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateChannelResult, ReceivedMessage, TestChannelID]()
	{
		if(ReceivedMessage->IsValid())
		{
			Chat->RemoveThreadChannel(ReceivedMessage->Get());
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatRemoveThreadChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.RemoveThreadChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRemoveThreadChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_remove_thread_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_remove_thread_async_full";
	const FString TestMessageText = TEXT("Async remove thread message");
	const FString TestThreadText = TEXT("Thread propagation");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bRemoveRequestSent = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, TestThreadText, bCallbackReceived, bRemoveRequestSent, OnOperationResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when creating thread for RemoveThreadChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		FPubnubChatThreadChannelResult CreateResult = Chat->CreateThreadChannel(ReceivedMessage->Get());
		TestFalse("CreateThreadChannel should succeed", CreateResult.Result.Error);
		if(!CreateResult.ThreadChannel)
		{
			*bCallbackReceived = true;
			return;
		}
		// SendText on thread channel propagates the thread to the server; otherwise RemoveThreadChannelAsync may fail
		CreateResult.ThreadChannel->Connect();
		FPubnubChatOperationResult SendResult = CreateResult.ThreadChannel->SendText(TestThreadText);
		if(SendResult.Error) { AddError("Thread SendText failed"); *bCallbackReceived = true; return; }
		FPlatformProcess::Sleep(1.0f);
		if(!ReceivedMessage->IsValid()) { AddError("Message invalid before RemoveThreadChannelAsync"); *bCallbackReceived = true; return; }
		*bRemoveRequestSent = true;
		Chat->RemoveThreadChannelAsync(ReceivedMessage->Get(), OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, bRemoveRequestSent]()
	{
		if(*bRemoveRequestSent)
		{
			TestFalse("RemoveThreadChannelAsync should succeed", CallbackResult->Error);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateChannelResult, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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
// ASYNC FULL PARAMETER TESTS (ThreadChannel)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelGetThreadHistoryAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.GetThreadHistoryAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelGetThreadHistoryAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_history_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_history_async_full";
	const FString TestMessageText = TEXT("Thread history async message");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bHistoryRequestSent = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetThreadHistoryResult> CallbackResult = MakeShared<FPubnubChatGetThreadHistoryResult>();
	TSharedPtr<FPubnubChatThreadChannelResult> ThreadResultShared = MakeShared<FPubnubChatThreadChannelResult>();
	// Server starts from Start (newest) and goes towards the past to End (oldest); both must be valid 17-digit Unix timetokens with Start > End
	const FString StartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(StartTimetoken, -100000000);
	const int Count = 10;
	FOnPubnubChatGetThreadHistoryResponseNative OnHistoryResponse;
	OnHistoryResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetThreadHistoryResult& HistoryResult)
	{
		*CallbackResult = HistoryResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadResultShared, bCallbackReceived, bHistoryRequestSent, StartTimetoken, EndTimetoken, Count, OnHistoryResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when creating thread channel for GetThreadHistoryAsync");
			*bCallbackReceived = true;
			return;
		}
		*ThreadResultShared = Chat->CreateThreadChannel(ReceivedMessage->Get());
		TestFalse("CreateThreadChannel should succeed", ThreadResultShared->Result.Error);
		TestNotNull("ThreadChannel should be created", ThreadResultShared->ThreadChannel);
		if(ThreadResultShared->ThreadChannel)
		{
			*bHistoryRequestSent = true;
			ThreadResultShared->ThreadChannel->GetThreadHistoryAsync(StartTimetoken, EndTimetoken, OnHistoryResponse, Count);
		}
		else
		{
			*bCallbackReceived = true;
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, bHistoryRequestSent]()
	{
		if(*bHistoryRequestSent)
		{
			TestFalse("GetThreadHistoryAsync should succeed", CallbackResult->Result.Error);
			TestTrue("Thread history should be valid", CallbackResult->ThreadMessages.Num() >= 0);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadResultShared, CreateChannelResult, Chat, TestChannelID]()
	{
		if(ThreadResultShared->ThreadChannel)
		{
			ThreadResultShared->ThreadChannel->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelPinMessageToParentAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.PinMessageToParentChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelPinMessageToParentAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_pin_parent_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_pin_parent_async_full";
	const FString TestMessageText = TEXT("Thread pin parent async message");
	const FString TestThreadText = TEXT("Thread reply");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatThreadMessage>> ThreadMessage = MakeShared<TWeakObjectPtr<UPubnubChatThreadMessage>>(nullptr);
	TSharedPtr<FPubnubChatThreadChannelResult> ThreadResultShared = MakeShared<FPubnubChatThreadChannelResult>();
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bPinRequestSent = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadResultShared, bThreadMessageReceived, ThreadMessage, TestThreadText, bCallbackReceived]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when creating thread channel for PinMessageToParentChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		*ThreadResultShared = Chat->CreateThreadChannel(ReceivedMessage->Get());
		TestFalse("CreateThreadChannel should succeed", ThreadResultShared->Result.Error);
		TestNotNull("ThreadChannel should be created", ThreadResultShared->ThreadChannel);
		if(!ThreadResultShared->ThreadChannel)
		{
			*bCallbackReceived = true;
			return;
		}
		ThreadResultShared->ThreadChannel->OnThreadMessageReceivedNative.AddLambda([bThreadMessageReceived, ThreadMessage](UPubnubChatThreadMessage* Message)
		{
			if(Message && !ThreadMessage->IsValid())
			{
				*bThreadMessageReceived = true;
				*ThreadMessage = Message;
			}
		});
		ThreadResultShared->ThreadChannel->Connect();
		FPubnubChatOperationResult SendResult = ThreadResultShared->ThreadChannel->SendText(TestThreadText);
		TestFalse("Thread SendText should succeed", SendResult.Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() { return *bThreadMessageReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadResultShared, ThreadMessage, bCallbackReceived, bPinRequestSent, OnOperationResponse]()
	{
		if(!ThreadMessage->IsValid())
		{
			AddError("Received thread message is invalid when calling PinMessageToParentChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		*bPinRequestSent = true;
		ThreadResultShared->ThreadChannel->PinMessageToParentChannelAsync(ThreadMessage->Get(), OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, bPinRequestSent]()
	{
		if(*bPinRequestSent)
		{
			TestFalse("PinMessageToParentChannelAsync should succeed", CallbackResult->Error);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadResultShared, CreateChannelResult, Chat, TestChannelID]()
	{
		if(ThreadResultShared->ThreadChannel)
		{
			ThreadResultShared->ThreadChannel->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadChannelUnpinMessageFromParentAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadChannel.UnpinMessageFromParentChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadChannelUnpinMessageFromParentAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_unpin_parent_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_unpin_parent_async_full";
	const FString TestMessageText = TEXT("Thread unpin parent async message");
	const FString TestThreadText = TEXT("Thread reply unpin");
	
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
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !ReceivedMessage->IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	TestFalse("Connect should succeed", CreateChannelResult.Channel->Connect().Error);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatThreadMessage>> ThreadMessage = MakeShared<TWeakObjectPtr<UPubnubChatThreadMessage>>(nullptr);
	TSharedPtr<FPubnubChatThreadChannelResult> ThreadResultShared = MakeShared<FPubnubChatThreadChannelResult>();
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bUnpinRequestSent = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadResultShared, bThreadMessageReceived, ThreadMessage, TestThreadText, bCallbackReceived]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when creating thread channel for UnpinMessageFromParentChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		*ThreadResultShared = Chat->CreateThreadChannel(ReceivedMessage->Get());
		TestFalse("CreateThreadChannel should succeed", ThreadResultShared->Result.Error);
		TestNotNull("ThreadChannel should be created", ThreadResultShared->ThreadChannel);
		if(!ThreadResultShared->ThreadChannel)
		{
			*bCallbackReceived = true;
			return;
		}
		ThreadResultShared->ThreadChannel->OnThreadMessageReceivedNative.AddLambda([bThreadMessageReceived, ThreadMessage](UPubnubChatThreadMessage* Message)
		{
			if(Message && !ThreadMessage->IsValid())
			{
				*bThreadMessageReceived = true;
				*ThreadMessage = Message;
			}
		});
		ThreadResultShared->ThreadChannel->Connect();
		FPubnubChatOperationResult SendResult = ThreadResultShared->ThreadChannel->SendText(TestThreadText);
		TestFalse("Thread SendText should succeed", SendResult.Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() { return *bThreadMessageReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadResultShared, ThreadMessage, bCallbackReceived, bUnpinRequestSent, OnOperationResponse]()
	{
		if(!ThreadMessage->IsValid())
		{
			AddError("Received thread message is invalid when calling PinMessageToParentChannel/UnpinMessageFromParentChannelAsync");
			*bCallbackReceived = true;
			return;
		}
		FPubnubChatOperationResult PinResult = ThreadResultShared->ThreadChannel->PinMessageToParentChannel(ThreadMessage->Get());
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
		*bUnpinRequestSent = true;
		ThreadResultShared->ThreadChannel->UnpinMessageFromParentChannelAsync(OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, bUnpinRequestSent]()
	{
		if(*bUnpinRequestSent)
		{
			TestFalse("UnpinMessageFromParentChannelAsync should succeed", CallbackResult->Error);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadResultShared, CreateChannelResult, Chat, TestChannelID]()
	{
		if(ThreadResultShared->ThreadChannel)
		{
			ThreadResultShared->ThreadChannel->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
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

#endif // WITH_DEV_AUTOMATION_TESTS
