// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatCallbackStop.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
	
	auto ThreadMessageLambda = [this, bThreadMessageReceived, ThreadMessage](UPubnubChatMessage* Message)
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
		(*ThreadChannel)->OnMessageReceivedNative.AddLambda(ThreadMessageLambda);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.HasThread.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.HasThread.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasThreadStateChangesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Threads.HasThread.4Advanced.StateChanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
 * Verifies that HasThread still returns true (thread exists but deleted) and GetThread().ThreadChannel->IsDeleted() returns true.
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
		Chat->DeleteChannel(TestChannelID, false);
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
		
		// Verify thread channel is not deleted
		if(*ThreadChannel)
		{
			FPubnubChatIsDeletedResult ThreadIsDeletedResult = (*ThreadChannel)->IsDeleted();
			TestFalse("ThreadIsDeleted check should succeed", ThreadIsDeletedResult.Result.Error);
			TestFalse("Thread channel should not be deleted before message deletion", ThreadIsDeletedResult.IsDeleted);
		}
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
		
		// Verify thread channel is also soft-deleted
		if(*ThreadChannel)
		{
			FPubnubChatIsDeletedResult ThreadIsDeletedResult = (*ThreadChannel)->IsDeleted();
			TestFalse("ThreadIsDeleted check should succeed", ThreadIsDeletedResult.Result.Error);
			TestTrue("Thread channel should be soft-deleted", ThreadIsDeletedResult.IsDeleted);
		}
		
		// Verify GetThread returns the thread and it's deleted
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
		if(GetThreadResult.ThreadChannel)
		{
			FPubnubChatIsDeletedResult GetThreadIsDeletedResult = GetThreadResult.ThreadChannel->IsDeleted();
			TestFalse("GetThreadIsDeleted check should succeed", GetThreadIsDeletedResult.Result.Error);
			TestTrue("GetThread should return deleted thread channel", GetThreadIsDeletedResult.IsDeleted);
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
			// Hard delete the message and thread for cleanup
			(*ReceivedMessage)->Delete(false);
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		
		// Verify thread channel is also soft-deleted
		if(*ThreadChannel)
		{
			FPubnubChatIsDeletedResult ThreadIsDeletedResult = (*ThreadChannel)->IsDeleted();
			TestFalse("ThreadIsDeleted check should succeed", ThreadIsDeletedResult.Result.Error);
			TestTrue("Thread channel should be soft-deleted", ThreadIsDeletedResult.IsDeleted);
		}
		
		// Verify GetThread returns the thread and it's deleted
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
		if(GetThreadResult.ThreadChannel)
		{
			FPubnubChatIsDeletedResult GetThreadIsDeletedResult = GetThreadResult.ThreadChannel->IsDeleted();
			TestFalse("GetThreadIsDeleted check should succeed", GetThreadIsDeletedResult.Result.Error);
			TestTrue("GetThread should return deleted thread channel", GetThreadIsDeletedResult.IsDeleted);
		}
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
		
		// Verify thread channel is also restored
		if(*ThreadChannel)
		{
			FPubnubChatIsDeletedResult ThreadIsDeletedResult = (*ThreadChannel)->IsDeleted();
			TestFalse("ThreadIsDeleted check should succeed", ThreadIsDeletedResult.Result.Error);
			TestFalse("Thread channel should not be deleted after restore", ThreadIsDeletedResult.IsDeleted);
		}
		
		// Verify GetThread returns the thread and it's restored (not deleted)
		FPubnubChatThreadChannelResult GetThreadResult = Chat->GetThreadChannel(*ReceivedMessage);
		TestFalse("GetThreadChannel should succeed", GetThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetThreadResult.ThreadChannel);
		if(GetThreadResult.ThreadChannel)
		{
			FPubnubChatIsDeletedResult GetThreadIsDeletedResult = GetThreadResult.ThreadChannel->IsDeleted();
			TestFalse("GetThreadIsDeleted check should succeed", GetThreadIsDeletedResult.Result.Error);
			TestFalse("GetThread should return restored (not deleted) thread channel", GetThreadIsDeletedResult.IsDeleted);
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
			// Remove thread before cleanup
			FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
			if(HasThreadResult.HasThread)
			{
				Chat->RemoveThreadChannel(*ReceivedMessage);
			}
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
