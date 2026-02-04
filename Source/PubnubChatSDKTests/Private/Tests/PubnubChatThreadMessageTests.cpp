// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatCallbackStop.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "PubnubChatThreadMessage.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalConverters.h"
#include "PubnubChatEnumLibrary.h"
#include "Private/PubnubChatConst.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// PINMESSAGETOPARENTCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessagePinMessageToParentChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.PinMessageToParentChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessagePinMessageToParentChannelNotInitializedTest::RunTest(const FString& Parameters)
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
			UPubnubChatThreadMessage* ThreadMessage = NewObject<UPubnubChatThreadMessage>();
			if(ThreadMessage)
			{
				FPubnubChatOperationResult PinResult = ThreadMessage->PinMessageToParentChannel();
				
				TestTrue("PinMessageToParentChannel should fail when ThreadMessage is not initialized", PinResult.Error);
				TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessagePinMessageToParentChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.PinMessageToParentChannel.2HappyPath.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessagePinMessageToParentChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_pin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_pin_happy";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread pin happy path");
	
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
	
	// Wait for thread message to be received
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
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
		
		const FString ThreadMessageText = TEXT("Thread message for pin test");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() -> bool {
		return *bThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the thread message to parent channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadMessage, CreateChannelResult]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadMessage)->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			return;
		}
		
		// Verify pinned message is stored in parent channel data
		FPubnubChatChannelData UpdatedChannelData = CreateChannelResult.Channel->GetChannelData();
		TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		
		// Verify GetPinnedMessage returns the correct thread message
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match", PinnedMessageResult.Message->GetMessageTimetoken(), (*ThreadMessage)->GetMessageTimetoken());
			
			// Verify it's a ThreadMessage
			UPubnubChatThreadMessage* PinnedThreadMessage = Cast<UPubnubChatThreadMessage>(PinnedMessageResult.Message);
			TestNotNull("Pinned message should be a ThreadMessage", PinnedThreadMessage);
			if(PinnedThreadMessage)
			{
				TestEqual("Pinned message parent channel ID should match", PinnedThreadMessage->GetParentChannelID(), CreateChannelResult.Channel->GetChannelID());
			}
		}
	}, 0.3f));
	
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

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests pinning a thread message when parent channel doesn't exist - should fail gracefully
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessagePinMessageToParentChannelParentChannelNotFoundTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.PinMessageToParentChannel.3Advanced.ParentChannelNotFound", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessagePinMessageToParentChannelParentChannelNotFoundTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_pin_no_parent_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_pin_no_parent";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread pin no parent");
	
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
	
	// Wait for thread message to be received
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
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
		
		const FString ThreadMessageText = TEXT("Thread message for pin no parent test");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() -> bool {
		return *bThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Delete parent channel before pinning
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		// Delete parent channel
		FPubnubChatOperationResult DeleteResult = Chat->DeleteChannel(TestChannelID, false);
		TestFalse("DeleteChannel should succeed", DeleteResult.Error);
		
		// Wait a bit for deletion to propagate
	}, 0.1f));
	
	// Try to pin message after parent channel is deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadMessage)->PinMessageToParentChannel();
		TestTrue("PinMessageToParentChannel should fail when parent channel doesn't exist", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
	}, 0.3f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(*ThreadChannel && *ReceivedMessage)
		{
			// Remove thread
			FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
			if(HasThreadResult.HasThread)
			{
				Chat->RemoveThreadChannel(*ReceivedMessage);
			}
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests pinning multiple thread messages - verifies that pinning a second message overrides the first
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessagePinMessageToParentChannelPinTwiceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.PinMessageToParentChannel.3Advanced.PinTwice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessagePinMessageToParentChannelPinTwiceTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_pin_twice_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_pin_twice";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread pin twice");
	
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
	
	// Wait for thread messages to be received
	TSharedPtr<bool> bFirstThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> FirstThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	TSharedPtr<bool> bSecondThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> SecondThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
	auto ThreadMessageLambda = [this, bFirstThreadMessageReceived, FirstThreadMessage, bSecondThreadMessageReceived, SecondThreadMessage](UPubnubChatThreadMessage* Message)
	{
		if(Message && !*FirstThreadMessage)
		{
			*bFirstThreadMessageReceived = true;
			*FirstThreadMessage = Message;
		}
		else if(Message && *FirstThreadMessage && !*SecondThreadMessage)
		{
			*bSecondThreadMessageReceived = true;
			*SecondThreadMessage = Message;
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
	
	// Send first text to thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString FirstThreadMessageText = TEXT("First thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(FirstThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until first thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstThreadMessageReceived]() -> bool {
		return *bFirstThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin first thread message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, FirstThreadMessage]()
	{
		if(!*FirstThreadMessage)
		{
			AddError("First ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*FirstThreadMessage)->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Send second text to thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString SecondThreadMessageText = TEXT("Second thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(SecondThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until second thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSecondThreadMessageReceived]() -> bool {
		return *bSecondThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin second thread message (should override first)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondThreadMessage]()
	{
		if(!*SecondThreadMessage)
		{
			AddError("Second ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*SecondThreadMessage)->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify second message is pinned (overriding first)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, FirstThreadMessage, SecondThreadMessage]()
	{
		if(!*FirstThreadMessage || !*SecondThreadMessage)
		{
			return;
		}
		
		// Verify GetPinnedMessage returns the second message (not the first)
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match second message", PinnedMessageResult.Message->GetMessageTimetoken(), (*SecondThreadMessage)->GetMessageTimetoken());
			TestNotEqual("Pinned message timetoken should not match first message", PinnedMessageResult.Message->GetMessageTimetoken(), (*FirstThreadMessage)->GetMessageTimetoken());
		}
	}, 0.3f));
	
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

// ============================================================================
// UNPINMESSAGEFROMPARENTCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessageUnpinMessageFromParentChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.UnpinMessageFromParentChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessageUnpinMessageFromParentChannelNotInitializedTest::RunTest(const FString& Parameters)
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
			UPubnubChatThreadMessage* ThreadMessage = NewObject<UPubnubChatThreadMessage>();
			if(ThreadMessage)
			{
				FPubnubChatOperationResult UnpinResult = ThreadMessage->UnpinMessageFromParentChannel();
				
				TestTrue("UnpinMessageFromParentChannel should fail when ThreadMessage is not initialized", UnpinResult.Error);
				TestFalse("ErrorMessage should not be empty", UnpinResult.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessageUnpinMessageFromParentChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.UnpinMessageFromParentChannel.2HappyPath.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessageUnpinMessageFromParentChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_unpin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_unpin_happy";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread unpin happy path");
	
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
	
	// Wait for thread message to be received
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
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
		
		const FString ThreadMessageText = TEXT("Thread message for unpin test");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() -> bool {
		return *bThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the thread message to parent channel first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*ThreadMessage)->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			return;
		}
		
		// Verify message is pinned
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match", PinnedMessageResult.Message->GetMessageTimetoken(), (*ThreadMessage)->GetMessageTimetoken());
		}
	}, 0.3f));
	
	// Unpin the thread message from parent channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult UnpinResult = (*ThreadMessage)->UnpinMessageFromParentChannel();
		TestFalse("UnpinMessageFromParentChannel should succeed", UnpinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is unpinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		// Verify GetPinnedMessage returns no message
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNull("Pinned message should be null after unpin", PinnedMessageResult.Message);
	}, 0.3f));
	
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

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests unpinning a thread message when it's not pinned - should succeed without error (no-op)
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessageUnpinMessageFromParentChannelNotPinnedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.UnpinMessageFromParentChannel.3Advanced.NotPinned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessageUnpinMessageFromParentChannelNotPinnedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_unpin_not_pinned_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_unpin_not_pinned";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread unpin not pinned");
	
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
	
	// Wait for thread message to be received
	TSharedPtr<bool> bThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> ThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
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
		
		const FString ThreadMessageText = TEXT("Thread message for unpin not pinned test");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bThreadMessageReceived]() -> bool {
		return *bThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Verify no message is pinned initially
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNull("Pinned message should be null initially", PinnedMessageResult.Message);
	}, 0.1f));
	
	// Try to unpin message that's not pinned (should succeed without error)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadMessage]()
	{
		if(!*ThreadMessage)
		{
			AddError("ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult UnpinResult = (*ThreadMessage)->UnpinMessageFromParentChannel();
		// Should succeed even if message is not pinned (no-op)
		TestFalse("UnpinMessageFromParentChannel should succeed even when message is not pinned", UnpinResult.Error);
	}, 0.1f));
	
	// Verify still no message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult]()
	{
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNull("Pinned message should still be null", PinnedMessageResult.Message);
	}, 0.1f));
	
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

/**
 * Tests unpinning a thread message when a different message is pinned - should succeed without error (no-op)
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessageUnpinMessageFromParentChannelDifferentMessagePinnedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.UnpinMessageFromParentChannel.3Advanced.DifferentMessagePinned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessageUnpinMessageFromParentChannelDifferentMessagePinnedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_msg_unpin_diff_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_msg_unpin_diff";
	
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
	
	// Create parent channel
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
	const FString TestMessageText = TEXT("Test message for thread unpin diff");
	
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
	
	// Wait for thread messages to be received
	TSharedPtr<bool> bFirstThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> FirstThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	TSharedPtr<bool> bSecondThreadMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatThreadMessage*> SecondThreadMessage = MakeShared<UPubnubChatThreadMessage*>(nullptr);
	
	auto ThreadMessageLambda = [this, bFirstThreadMessageReceived, FirstThreadMessage, bSecondThreadMessageReceived, SecondThreadMessage](UPubnubChatThreadMessage* Message)
	{
		if(Message && !*FirstThreadMessage)
		{
			*bFirstThreadMessageReceived = true;
			*FirstThreadMessage = Message;
		}
		else if(Message && *FirstThreadMessage && !*SecondThreadMessage)
		{
			*bSecondThreadMessageReceived = true;
			*SecondThreadMessage = Message;
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
	
	// Send first text to thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString FirstThreadMessageText = TEXT("First thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(FirstThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until first thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstThreadMessageReceived]() -> bool {
		return *bFirstThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin first thread message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, FirstThreadMessage]()
	{
		if(!*FirstThreadMessage)
		{
			AddError("First ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult PinResult = (*FirstThreadMessage)->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
	}, 0.1f));
	
	// Send second text to thread channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		
		const FString SecondThreadMessageText = TEXT("Second thread message");
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(SecondThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until second thread message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSecondThreadMessageReceived]() -> bool {
		return *bSecondThreadMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Verify first message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, FirstThreadMessage]()
	{
		if(!*FirstThreadMessage)
		{
			return;
		}
		
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match first message", PinnedMessageResult.Message->GetMessageTimetoken(), (*FirstThreadMessage)->GetMessageTimetoken());
		}
	}, 0.3f));
	
	// Try to unpin second message (which is not pinned) - should succeed without error (no-op)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondThreadMessage]()
	{
		if(!*SecondThreadMessage)
		{
			AddError("Second ThreadMessage was not received");
			return;
		}
		
		FPubnubChatOperationResult UnpinResult = (*SecondThreadMessage)->UnpinMessageFromParentChannel();
		// Should succeed even if this specific message is not pinned (no-op)
		TestFalse("UnpinMessageFromParentChannel should succeed even when this message is not pinned", UnpinResult.Error);
	}, 0.1f));
	
	// Verify first message is still pinned (unpinning second shouldn't affect it)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, FirstThreadMessage, SecondThreadMessage]()
	{
		if(!*FirstThreadMessage || !*SecondThreadMessage)
		{
			return;
		}
		
		FPubnubChatMessageResult PinnedMessageResult = CreateChannelResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should still be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should still match first message", PinnedMessageResult.Message->GetMessageTimetoken(), (*FirstThreadMessage)->GetMessageTimetoken());
			TestNotEqual("Pinned message timetoken should not match second message", PinnedMessageResult.Message->GetMessageTimetoken(), (*SecondThreadMessage)->GetMessageTimetoken());
		}
	}, 0.1f));
	
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

// ============================================================================
// ASYNC FULL PARAMETER TESTS (ThreadMessage)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessagePinMessageToParentChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.PinMessageToParentChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessagePinMessageToParentChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_message_pin_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_message_pin_async_full";
	const FString TestMessageText = TEXT("Thread parent message");
	const FString TestThreadText = TEXT("Thread reply message");
	
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
			AddError("Received message is invalid when creating thread channel for ThreadMessage.PinMessageToParentChannelAsync");
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
		ThreadMessage->Get()->PinMessageToParentChannelAsync(OnOperationResponse);
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
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatThreadMessageUnpinMessageFromParentChannelAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ThreadMessage.UnpinMessageFromParentChannelAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatThreadMessageUnpinMessageFromParentChannelAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_thread_message_unpin_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_thread_message_unpin_async_full";
	const FString TestMessageText = TEXT("Thread parent message unpin");
	const FString TestThreadText = TEXT("Thread reply message unpin");
	
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
			AddError("Received message is invalid when creating thread channel for ThreadMessage.UnpinMessageFromParentChannelAsync");
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
		FPubnubChatOperationResult PinResult = ThreadMessage->Get()->PinMessageToParentChannel();
		TestFalse("PinMessageToParentChannel should succeed", PinResult.Error);
		*bUnpinRequestSent = true;
		ThreadMessage->Get()->UnpinMessageFromParentChannelAsync(OnOperationResponse);
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
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

