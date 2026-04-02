// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "PubnubStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// GETCURRENTTEXT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetCurrentTextNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetCurrentText.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageGetCurrentTextNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getcurrenttext_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to get current text with uninitialized message
		FString CurrentText = UninitializedMessage->GetCurrentText();
		TestTrue("GetCurrentText should return empty string for uninitialized message", CurrentText.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetCurrentTextHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetCurrentText.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageGetCurrentTextHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getcurrenttext_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_getcurrenttext_happy";
	const FString TestMessageText = TEXT("Original message text");
	
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
	
	// Wait a bit for subscription to be ready, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Verify GetCurrentText returns original text
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestMessageText]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return original message text", CurrentText, TestMessageText);
		
		// Verify message data has no message actions
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		TestEqual("MessageActions should be empty for unedited message", MessageData.MessageActions.Num(), 0);
		TestEqual("MessageData.Text should match original text", MessageData.Text, TestMessageText);
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests GetCurrentText with multiple edits - verifies that the most recent edited text is returned.
 * Sends a message, edits it multiple times, and verifies GetCurrentText always returns the latest edit.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetCurrentTextMultipleEditsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetCurrentText.4Advanced.MultipleEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageGetCurrentTextMultipleEditsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getcurrenttext_multiple_edits_init";
	const FString TestChannelID = SDK_PREFIX + "test_getcurrenttext_multiple_edits";
	const FString OriginalText = TEXT("Original message");
	const FString FirstEdit = TEXT("First edit");
	const FString SecondEdit = TEXT("Second edit");
	const FString ThirdEdit = TEXT("Third edit");
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, OriginalText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(OriginalText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Perform first edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEdit]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(FirstEdit);
		TestFalse("First EditText should succeed", EditResult.Error);
	}, 0.2f));
	
	// Wait a bit, then verify first edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return first edit", CurrentText, FirstEdit);
	}, 0.3f));
	
	// Perform second edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(SecondEdit);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify second edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return second edit", CurrentText, SecondEdit);
	}, 0.3f));
	
	// Perform third edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThirdEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(ThirdEdit);
		TestFalse("Third EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify third edit and message actions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThirdEdit, OriginalText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return third edit (most recent)", CurrentText, ThirdEdit);
		
		// Verify message actions contain all edits
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		TestTrue("MessageActions should have at least 3 edits", MessageData.MessageActions.Num() >= 3);
		
		// Verify all edits are of type Edited
		int EditedCount = 0;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				EditedCount++;
			}
		}
		TestEqual("Should have 3 Edited message actions", EditedCount, 3);
		
		// Verify original text is still in MessageData.Text
		TestEqual("MessageData.Text should still contain original text", MessageData.Text, OriginalText);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests GetCurrentText after getting message via Channel->GetMessage() - verifies that GetMessage returns message with edited actions and GetCurrentText returns most recent edit.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetCurrentTextViaGetMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetCurrentText.4Advanced.ViaGetMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageGetCurrentTextViaGetMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getcurrenttext_getmessage_init";
	const FString TestChannelID = SDK_PREFIX + "test_getcurrenttext_getmessage";
	const FString OriginalText = TEXT("Original message for GetMessage test");
	const FString FirstEdit = TEXT("First edit via GetMessage");
	const FString SecondEdit = TEXT("Second edit via GetMessage");
	
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
	
	// Shared state
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, OriginalText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(OriginalText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Get message via Channel->GetMessage() and edit it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, FirstEdit, OriginalText]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			AddError("Message timetoken was not received");
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify original text
		FString CurrentText = GetMessageResult.Message->GetCurrentText();
		TestEqual("GetCurrentText should return original text before edit", CurrentText, OriginalText);
		
		// Edit the message
		FPubnubChatOperationResult EditResult = GetMessageResult.Message->EditText(FirstEdit);
		TestFalse("EditText should succeed", EditResult.Error);
	}, 0.2f));
	
	// Wait a bit, then verify first edit via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, FirstEdit]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed after edit", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved after edit", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify GetCurrentText returns first edit
		FString CurrentText = GetMessageResult.Message->GetCurrentText();
		TestEqual("GetCurrentText should return first edit", CurrentText, FirstEdit);
		
		// Verify message has edited actions
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		TestTrue("Message should have at least one message action", MessageData.MessageActions.Num() >= 1);
		
		// Verify there's an Edited action
		bool bHasEditedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				bHasEditedAction = true;
				TestEqual("Edited action value should match first edit", Action.Value, FirstEdit);
				break;
			}
		}
		TestTrue("Message should have Edited message action", bHasEditedAction);
	}, 0.3f));
	
	// Perform second edit via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, SecondEdit]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = GetMessageResult.Message->EditText(SecondEdit);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify second edit via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, SecondEdit, OriginalText]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed after second edit", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved after second edit", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify GetCurrentText returns second edit (most recent)
		FString CurrentText = GetMessageResult.Message->GetCurrentText();
		TestEqual("GetCurrentText should return second edit (most recent)", CurrentText, SecondEdit);
		
		// Verify message has multiple edited actions
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		TestTrue("Message should have at least 2 message actions", MessageData.MessageActions.Num() >= 2);
		
		// Count Edited actions
		int EditedCount = 0;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				EditedCount++;
			}
		}
		TestEqual("Should have 2 Edited message actions", EditedCount, 2);
		
		// Verify original text is still in MessageData.Text
		TestEqual("MessageData.Text should still contain original text", MessageData.Text, OriginalText);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// EDITTEXT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageEditTextNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_edittext_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to edit text with uninitialized message
		FPubnubChatOperationResult EditResult = UninitializedMessage->EditText(TEXT("New text"));
		TestTrue("EditText should fail with uninitialized message", EditResult.Error);
		TestFalse("ErrorMessage should not be empty", EditResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextEmptyNewTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.1Validation.EmptyNewText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageEditTextEmptyNewTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_edittext_empty_text_init";
	const FString TestChannelID = SDK_PREFIX + "test_edittext_empty_text";
	const FString TestMessageText = TEXT("Original message");
	
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
	
	// Try to edit with empty string
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(TEXT(""));
		TestTrue("EditText should fail with empty NewText", EditResult.Error);
		TestFalse("ErrorMessage should not be empty", EditResult.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageEditTextHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_edittext_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_edittext_happy";
	const FString OriginalText = TEXT("Original message");
	const FString EditedText = TEXT("Edited message");
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, OriginalText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(OriginalText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Edit the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, EditedText]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(EditedText);
		TestFalse("EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify edit was successful
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, EditedText, OriginalText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify GetCurrentText returns edited text
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return edited text", CurrentText, EditedText);
		
		// Verify message data has edited action
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		TestTrue("Message should have at least one message action", MessageData.MessageActions.Num() >= 1);
		
		// Verify there's an Edited action
		bool bHasEditedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				bHasEditedAction = true;
				TestEqual("Edited action value should match edited text", Action.Value, EditedText);
				break;
			}
		}
		TestTrue("Message should have Edited message action", bHasEditedAction);
		
		// Verify original text is still in MessageData.Text
		TestEqual("MessageData.Text should still contain original text", MessageData.Text, OriginalText);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests EditText with multiple sequential edits - verifies that each edit succeeds and GetCurrentText always returns the most recent edit.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextMultipleEditsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.4Advanced.MultipleEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageEditTextMultipleEditsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_edittext_multiple_edits_init";
	const FString TestChannelID = SDK_PREFIX + "test_edittext_multiple_edits";
	const FString OriginalText = TEXT("Original message");
	const FString FirstEdit = TEXT("First edit");
	const FString SecondEdit = TEXT("Second edit");
	const FString ThirdEdit = TEXT("Third edit");
	const FString FourthEdit = TEXT("Fourth edit");
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, OriginalText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(OriginalText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Perform first edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEdit]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(FirstEdit);
		TestFalse("First EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait and verify first edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return first edit", CurrentText, FirstEdit);
	}, 0.3f));
	
	// Perform second edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(SecondEdit);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait and verify second edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return second edit", CurrentText, SecondEdit);
	}, 0.3f));
	
	// Perform third edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThirdEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(ThirdEdit);
		TestFalse("Third EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait and verify third edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThirdEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return third edit", CurrentText, ThirdEdit);
	}, 0.3f));
	
	// Perform fourth edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FourthEdit]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(FourthEdit);
		TestFalse("Fourth EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait and verify final state
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FourthEdit, OriginalText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify GetCurrentText returns most recent edit
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return fourth edit (most recent)", CurrentText, FourthEdit);
		
		// Verify message actions contain all edits
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		TestTrue("Message should have at least 4 message actions", MessageData.MessageActions.Num() >= 4);
		
		// Count Edited actions
		int EditedCount = 0;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				EditedCount++;
			}
		}
		TestEqual("Should have 4 Edited message actions", EditedCount, 4);
		
		// Verify original text is still in MessageData.Text
		TestEqual("MessageData.Text should still contain original text", MessageData.Text, OriginalText);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests EditText and then retrieving message via Channel->GetMessage() - verifies that GetMessage returns message with edited actions and GetCurrentText returns the edited text.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextViaGetMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.4Advanced.ViaGetMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageEditTextViaGetMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_edittext_getmessage_init";
	const FString TestChannelID = SDK_PREFIX + "test_edittext_getmessage";
	const FString OriginalText = TEXT("Original message for EditText GetMessage test");
	const FString EditedText = TEXT("Edited message via GetMessage");
	
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
	
	// Shared state
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, OriginalText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(OriginalText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Get message via Channel->GetMessage() and edit it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, EditedText, OriginalText]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			AddError("Message timetoken was not received");
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify original text before edit
		FString CurrentText = GetMessageResult.Message->GetCurrentText();
		TestEqual("GetCurrentText should return original text before edit", CurrentText, OriginalText);
		
		// Edit the message
		FPubnubChatOperationResult EditResult = GetMessageResult.Message->EditText(EditedText);
		TestFalse("EditText should succeed", EditResult.Error);
		
		// Verify step results contain AddMessageAction step
		bool bFoundAddMessageAction = false;
		for(const FPubnubChatOperationStepResult& Step : EditResult.StepResults)
		{
			if(Step.StepName == TEXT("AddMessageAction"))
			{
				bFoundAddMessageAction = true;
				TestFalse("AddMessageAction step should not have error", Step.OperationResult.Error);
				break;
			}
		}
		TestTrue("Should have AddMessageAction step", bFoundAddMessageAction);
	}, 0.2f));
	
	// Wait a bit, then verify edit via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken, EditedText, OriginalText]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed after edit", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved after edit", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify GetCurrentText returns edited text
		FString CurrentText = GetMessageResult.Message->GetCurrentText();
		TestEqual("GetCurrentText should return edited text", CurrentText, EditedText);
		
		// Verify message has edited actions
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		TestTrue("Message should have at least one message action", MessageData.MessageActions.Num() >= 1);
		
		// Verify there's an Edited action
		bool bHasEditedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
			{
				bHasEditedAction = true;
				TestEqual("Edited action value should match edited text", Action.Value, EditedText);
				TestFalse("Edited action timetoken should not be empty", Action.Timetoken.IsEmpty());
				TestFalse("Edited action userID should not be empty", Action.UserID.IsEmpty());
				break;
			}
		}
		TestTrue("Message should have Edited message action", bHasEditedAction);
		
		// Verify original text is still in MessageData.Text
		TestEqual("MessageData.Text should still contain original text", MessageData.Text, OriginalText);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// DELETE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDeleteNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Delete.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDeleteNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to delete with uninitialized message
		FPubnubChatOperationResult DeleteResult = UninitializedMessage->Delete(false);
		TestTrue("Delete should fail with uninitialized message", DeleteResult.Error);
		TestFalse("ErrorMessage should not be empty", DeleteResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDeleteHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Delete.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDeleteHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_delete_happy";
	const FString TestMessageText = TEXT("Message to delete");
	
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
	
	// Soft delete the message (default parameter)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is not deleted before delete
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestFalse("Message should not be deleted before delete", IsDeletedBeforeResult.IsDeleted);
		
		// Soft delete the message (explicitly set Soft=true)
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify soft delete was successful
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is marked as deleted (repository should be updated in place)
		FPubnubChatIsDeletedResult IsDeletedAfterResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after delete", IsDeletedAfterResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has deleted action
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				TestEqual("Deleted action value should match soft deleted value", Action.Value, Pubnub_Chat_Soft_Deleted_Action_Value);
				break;
			}
		}
		TestTrue("Message should have Deleted message action", bHasDeletedAction);
		
		// Verify message still exists (soft delete doesn't remove it)
		TestFalse("Message timetoken should not be empty", (*ReceivedMessage)->GetMessageTimetoken().IsEmpty());
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDeleteHardDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Delete.3FullParameters.HardDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDeleteHardDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_hard_init";
	const FString TestChannelID = SDK_PREFIX + "test_delete_hard";
	const FString TestMessageText = TEXT("Message to hard delete");
	
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
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
			*MessageTimetoken = Message->GetMessageTimetoken();
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
	
	// Hard delete the message (Soft=false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Hard delete with Soft=false
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(false);
		TestFalse("Hard delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify hard delete removed message from repository
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		// Try to get message via GetMessage - should fail or return null after hard delete
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		// After hard delete, message should not be retrievable
		TestTrue("GetMessage should fail or return null after hard delete", GetMessageResult.Result.Error || !GetMessageResult.Message);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests Delete with soft delete and then retrieving message via GetMessage - verifies that soft-deleted message can still be retrieved and IsDeleted returns true.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDeleteSoftViaGetMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Delete.4Advanced.SoftDeleteViaGetMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDeleteSoftViaGetMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_soft_getmessage_init";
	const FString TestChannelID = SDK_PREFIX + "test_delete_soft_getmessage";
	const FString TestMessageText = TEXT("Message to soft delete via GetMessage");
	
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
	
	// Shared state
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
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
	
	// Get message via GetMessage and soft delete it
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			AddError("Message timetoken was not received");
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify message is not deleted before delete
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = GetMessageResult.Message->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestFalse("Message should not be deleted before delete", IsDeletedBeforeResult.IsDeleted);
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = GetMessageResult.Message->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.2f));
	
	// Wait a bit, then verify soft delete via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed after soft delete", GetMessageResult.Result.Error);
		TestNotNull("Message should still be retrievable after soft delete", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify message is marked as deleted
		FPubnubChatIsDeletedResult IsDeletedAfterResult = GetMessageResult.Message->IsDeleted();
		TestFalse("IsDeleted check should succeed after delete", IsDeletedAfterResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has deleted action
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				TestEqual("Deleted action value should match soft deleted value", Action.Value, Pubnub_Chat_Soft_Deleted_Action_Value);
				TestFalse("Deleted action timetoken should not be empty", Action.Timetoken.IsEmpty());
				break;
			}
		}
		TestTrue("Message should have Deleted message action", bHasDeletedAction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// RESTORE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Restore.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageRestoreNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to restore with uninitialized message
		FPubnubChatOperationResult RestoreResult = UninitializedMessage->Restore();
		TestTrue("Restore should fail with uninitialized message", RestoreResult.Error);
		TestFalse("ErrorMessage should not be empty", RestoreResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Restore.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageRestoreHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_restore_happy";
	const FString TestMessageText = TEXT("Message to restore");
	
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
	
	// Soft delete the message first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify delete and restore
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is marked as deleted (repository should be updated in place)
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedBeforeResult.IsDeleted);
		
		// Restore the message
		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage)->Restore();
		TestFalse("Restore should succeed", RestoreResult.Error);
	}, 0.3f));
	
	// Wait a bit, then verify restore was successful
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is no longer marked as deleted (repository should be updated in place)
		FPubnubChatIsDeletedResult IsDeletedAfterResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterResult.Result.Error);
		TestFalse("Message should not be marked as deleted after restore", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has no deleted actions
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				break;
			}
		}
		TestFalse("Message should not have Deleted message action after restore", bHasDeletedAction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests Restore on a non-deleted message - verifies that Restore succeeds even when message is not deleted.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreNonDeletedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Restore.4Advanced.RestoreNonDeleted", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageRestoreNonDeletedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_non_deleted_init";
	const FString TestChannelID = SDK_PREFIX + "test_restore_non_deleted";
	const FString TestMessageText = TEXT("Message that is not deleted");
	
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
	
	// Restore a non-deleted message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is not deleted
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestFalse("Message should not be marked as deleted", IsDeletedBeforeResult.IsDeleted);
		
		// Restore a non-deleted message (should still succeed)
		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage)->Restore();
		TestFalse("Restore should succeed even for non-deleted message", RestoreResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is still not deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is still not deleted (repository should be updated in place)
		FPubnubChatIsDeletedResult IsDeletedAfterResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterResult.Result.Error);
		TestFalse("Message should still not be marked as deleted", IsDeletedAfterResult.IsDeleted);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests Restore with multiple delete actions - verifies that Restore removes all deleted message actions.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreMultipleDeletesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Restore.4Advanced.MultipleDeletes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageRestoreMultipleDeletesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_multiple_deletes_init";
	const FString TestChannelID = SDK_PREFIX + "test_restore_multiple_deletes";
	const FString TestMessageText = TEXT("Message with multiple deletes");
	
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
	
	// Add multiple delete actions by calling Delete multiple times
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// First soft delete
		FPubnubChatOperationResult DeleteResult1 = (*ReceivedMessage)->Delete(true);
		TestFalse("First soft delete should succeed", DeleteResult1.Error);
	}, 0.1f));
	
	// Wait a bit, then add second delete
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Second soft delete (adds another delete action) - this may fail if server doesn't allow multiple deletes
		FPubnubChatOperationResult DeleteResult2 = (*ReceivedMessage)->Delete(true);
		// Note: Second delete may succeed or fail depending on server behavior
		// We'll verify that at least one delete action exists
	}, 0.3f));
	
	// Wait a bit, then verify multiple deletes and restore
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message has at least one deleted action (repository should be updated in place)
		FPubnubChatMessageData MessageDataBefore = (*ReceivedMessage)->GetMessageData();
		int DeletedActionCount = 0;
		for(const FPubnubChatMessageAction& Action : MessageDataBefore.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				DeletedActionCount++;
			}
		}
		TestTrue("Message should have at least one deleted action", DeletedActionCount >= 1);
		
		// Restore the message (should remove all deleted actions)
		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage)->Restore();
		TestFalse("Restore should succeed", RestoreResult.Error);
	}, 0.3f));
	
	// Wait a bit, then verify all deleted actions were removed
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is no longer marked as deleted (repository should be updated in place)
		FPubnubChatIsDeletedResult IsDeletedAfterResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterResult.Result.Error);
		TestFalse("Message should not be marked as deleted after restore", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has no deleted actions
		FPubnubChatMessageData MessageDataAfter = (*ReceivedMessage)->GetMessageData();
		int DeletedActionCountAfter = 0;
		for(const FPubnubChatMessageAction& Action : MessageDataAfter.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				DeletedActionCountAfter++;
			}
		}
		TestEqual("Message should have no deleted actions after restore", DeletedActionCountAfter, 0);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// ISDELETED TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageIsDeletedNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.IsDeleted.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageIsDeletedNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_isdeleted_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to check IsDeleted with uninitialized message
		FPubnubChatIsDeletedResult IsDeletedResult = UninitializedMessage->IsDeleted();
		TestTrue("IsDeleted should fail with uninitialized message", IsDeletedResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", IsDeletedResult.Result.ErrorMessage.IsEmpty());
		TestFalse("IsDeleted should be false when result has error", IsDeletedResult.IsDeleted);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageIsDeletedHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.IsDeleted.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageIsDeletedHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_isdeleted_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_isdeleted_happy";
	const FString TestMessageText = TEXT("Message to check IsDeleted");
	
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
	
	// Check IsDeleted on non-deleted message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Check IsDeleted on non-deleted message
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedResult.Result.Error);
		TestFalse("Message should not be marked as deleted", IsDeletedResult.IsDeleted);
		
		// Verify message data has no deleted actions
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				break;
			}
		}
		TestFalse("Message should not have Deleted message action", bHasDeletedAction);
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageIsDeletedDeletedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.IsDeleted.3FullParameters.DeletedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageIsDeletedDeletedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_isdeleted_deleted_init";
	const FString TestChannelID = SDK_PREFIX + "test_isdeleted_deleted";
	const FString TestMessageText = TEXT("Message that will be deleted");
	
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
	
	// Soft delete the message first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message is not deleted before delete
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestFalse("Message should not be marked as deleted before delete", IsDeletedBeforeResult.IsDeleted);
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then check IsDeleted on deleted message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Check IsDeleted on deleted message
		FPubnubChatIsDeletedResult IsDeletedAfterResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after delete", IsDeletedAfterResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has deleted action
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				TestEqual("Deleted action value should match soft deleted value", Action.Value, Pubnub_Chat_Soft_Deleted_Action_Value);
				break;
			}
		}
		TestTrue("Message should have Deleted message action", bHasDeletedAction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests IsDeleted after Restore - verifies that IsDeleted returns false after message is restored.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageIsDeletedAfterRestoreTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.IsDeleted.4Advanced.AfterRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageIsDeletedAfterRestoreTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_isdeleted_restore_init";
	const FString TestChannelID = SDK_PREFIX + "test_isdeleted_restore";
	const FString TestMessageText = TEXT("Message to delete and restore");
	
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
	
	// Soft delete the message first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify delete and restore
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is marked as deleted
		FPubnubChatIsDeletedResult IsDeletedBeforeRestoreResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeRestoreResult.Result.Error);
		TestTrue("Message should be marked as deleted before restore", IsDeletedBeforeRestoreResult.IsDeleted);
		
		// Restore the message
		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage)->Restore();
		TestFalse("Restore should succeed", RestoreResult.Error);
	}, 0.3f));
	
	// Wait a bit, then verify IsDeleted after restore
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is no longer marked as deleted
		FPubnubChatIsDeletedResult IsDeletedAfterRestoreResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterRestoreResult.Result.Error);
		TestFalse("Message should not be marked as deleted after restore", IsDeletedAfterRestoreResult.IsDeleted);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests IsDeleted via GetMessage - verifies that IsDeleted works correctly when message is retrieved via Channel->GetMessage().
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageIsDeletedViaGetMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.IsDeleted.4Advanced.ViaGetMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageIsDeletedViaGetMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_isdeleted_getmessage_init";
	const FString TestChannelID = SDK_PREFIX + "test_isdeleted_getmessage";
	const FString TestMessageText = TEXT("Message to check IsDeleted via GetMessage");
	
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
	
	// Shared state
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
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
	
	// Soft delete the message via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			AddError("Message timetoken was not received");
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify message is not deleted before delete
		FPubnubChatIsDeletedResult IsDeletedBeforeResult = GetMessageResult.Message->IsDeleted();
		TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
		TestFalse("Message should not be marked as deleted before delete", IsDeletedBeforeResult.IsDeleted);
		
		// Soft delete the message
		FPubnubChatOperationResult DeleteResult = GetMessageResult.Message->Delete(true);
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.2f));
	
	// Wait a bit, then verify IsDeleted via GetMessage
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, MessageTimetoken]()
	{
		if(MessageTimetoken->IsEmpty())
		{
			return;
		}
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(*MessageTimetoken);
		TestFalse("GetMessage should succeed after delete", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved after delete", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// Verify message is marked as deleted
		FPubnubChatIsDeletedResult IsDeletedAfterResult = GetMessageResult.Message->IsDeleted();
		TestFalse("IsDeleted check should succeed after delete", IsDeletedAfterResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedAfterResult.IsDeleted);
		
		// Verify message data has deleted action
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		bool bHasDeletedAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
			{
				bHasDeletedAction = true;
				TestEqual("Deleted action value should match soft deleted value", Action.Value, Pubnub_Chat_Soft_Deleted_Action_Value);
				break;
			}
		}
		TestTrue("Message should have Deleted message action", bHasDeletedAction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// PIN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessagePinNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Pin.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessagePinNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to pin with uninitialized message
		FPubnubChatOperationResult PinResult = UninitializedMessage->Pin();
		TestTrue("Pin should fail with uninitialized message", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessagePinChannelDoesNotExistTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Pin.1Validation.ChannelDoesNotExist", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessagePinChannelDoesNotExistTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_channel_not_exist_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_channel_not_exist";
	const FString NonExistentChannelID = SDK_PREFIX + "test_pin_nonexistent_channel";
	const FString TestMessageText = TEXT("Test message");
	
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
	
	// Delete the channel to make it non-existent
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Disconnect first
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		
		// Delete the channel
		FPubnubChatOperationResult DeleteResult = Chat->DeleteChannel(TestChannelID);
		TestFalse("DeleteChannel should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then try to pin message from deleted channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Try to pin message from non-existent channel
		FPubnubChatOperationResult PinResult = (*ReceivedMessage)->Pin();
		TestTrue("Pin should fail when channel doesn't exist", PinResult.Error);
		TestFalse("ErrorMessage should not be empty", PinResult.ErrorMessage.IsEmpty());
	}, 0.3f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessagePinHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Pin.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessagePinHappyPathTest::RunTest(const FString& Parameters)
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
	const FString TestMessageText = TEXT("Test message to pin");
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Pin the message
		FPubnubChatOperationResult PinResult = (*ReceivedMessage)->Pin();
		TestFalse("Pin should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify pinned message is stored in channel data
		FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
		TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		
		// Verify GetPinnedMessage returns the correct message
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match", PinnedMessageResult.Message->GetMessageTimetoken(), (*ReceivedMessage)->GetMessageTimetoken());
			TestEqual("Pinned message text should match", PinnedMessageResult.Message->GetCurrentText(), (*ReceivedMessage)->GetCurrentText());
		}
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests pinning a message twice - verifies that pinning a message twice overrides the previously pinned message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessagePinTwiceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Pin.4Advanced.PinTwiceOverride", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessagePinTwiceTest::RunTest(const FString& Parameters)
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
	const FString FirstMessageText = TEXT("First message to pin");
	const FString SecondMessageText = TEXT("Second message to pin");
	
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
	TSharedPtr<bool> bFirstMessageReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bSecondMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> FirstMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> SecondMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect with callback to receive messages
	auto MessageLambda = [this, bFirstMessageReceived, bSecondMessageReceived, FirstMessage, SecondMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*FirstMessage)
		{
			*bFirstMessageReceived = true;
			*FirstMessage = Message;
		}
		else if(Message && *FirstMessage && !*SecondMessage)
		{
			*bSecondMessageReceived = true;
			*SecondMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, FirstMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(FirstMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until first message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstMessageReceived]() -> bool {
		return *bFirstMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, FirstMessage]()
	{
		if(!*FirstMessage)
		{
			AddError("First message was not received");
			return;
		}
		
		// Pin the first message
		FPubnubChatOperationResult PinResult = (*FirstMessage)->Pin();
		TestFalse("Pin first message should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then send second message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, SecondMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(SecondMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until second message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSecondMessageReceived]() -> bool {
		return *bSecondMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin second message (should override first)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondMessage]()
	{
		if(!*SecondMessage)
		{
			AddError("Second message was not received");
			return;
		}
		
		// Pin the second message (should override first)
		FPubnubChatOperationResult PinResult = (*SecondMessage)->Pin();
		TestFalse("Pin second message should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify second message is pinned (overriding first)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, FirstMessage, SecondMessage, SecondMessageText]()
	{
		if(!*FirstMessage || !*SecondMessage)
		{
			return;
		}
		
		// Verify GetPinnedMessage returns the second message (not the first)
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match second message", PinnedMessageResult.Message->GetMessageTimetoken(), (*SecondMessage)->GetMessageTimetoken());
			TestNotEqual("Pinned message timetoken should not match first message", PinnedMessageResult.Message->GetMessageTimetoken(), (*FirstMessage)->GetMessageTimetoken());
			TestEqual("Pinned message text should match second message", PinnedMessageResult.Message->GetCurrentText(), SecondMessageText);
		}
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// UNPIN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageUnpinNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Unpin.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageUnpinNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to unpin with uninitialized message
		FPubnubChatOperationResult UnpinResult = UninitializedMessage->Unpin();
		TestTrue("Unpin should fail with uninitialized message", UnpinResult.Error);
		TestFalse("ErrorMessage should not be empty", UnpinResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageUnpinChannelDoesNotExistTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Unpin.1Validation.ChannelDoesNotExist", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageUnpinChannelDoesNotExistTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_channel_not_exist_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_channel_not_exist";
	const FString TestMessageText = TEXT("Test message");
	
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
	
	// Delete the channel to make it non-existent
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Disconnect first
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		
		// Delete the channel
		FPubnubChatOperationResult DeleteResult = Chat->DeleteChannel(TestChannelID);
		TestFalse("DeleteChannel should succeed", DeleteResult.Error);
	}, 0.1f));
	
	// Wait a bit, then try to unpin message from deleted channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Try to unpin message from non-existent channel
		FPubnubChatOperationResult UnpinResult = (*ReceivedMessage)->Unpin();
		TestTrue("Unpin should fail when channel doesn't exist", UnpinResult.Error);
		TestFalse("ErrorMessage should not be empty", UnpinResult.ErrorMessage.IsEmpty());
	}, 0.3f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageUnpinHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Unpin.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageUnpinHappyPathTest::RunTest(const FString& Parameters)
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
	const FString TestMessageText = TEXT("Test message to pin and unpin");
	
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
	
	// Pin the message first
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Pin the message first
		FPubnubChatOperationResult PinResult = (*ReceivedMessage)->Pin();
		TestFalse("Pin should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is pinned
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("Pinned message should be retrieved", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match", PinnedMessageResult.Message->GetMessageTimetoken(), (*ReceivedMessage)->GetMessageTimetoken());
		}
	}, 0.3f));
	
	// Unpin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Unpin the message
		FPubnubChatOperationResult UnpinResult = (*ReceivedMessage)->Unpin();
		TestFalse("Unpin should succeed", UnpinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then verify message is unpinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Verify GetPinnedMessage returns no message
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNull("Pinned message should be null after unpin", PinnedMessageResult.Message);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests unpinning when no message is pinned - verifies that Unpin succeeds silently when there's no pinned message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageUnpinNoPinnedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Unpin.4Advanced.NoPinnedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageUnpinNoPinnedMessageTest::RunTest(const FString& Parameters)
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
	const FString TestMessageText = TEXT("Test message");
	
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
	
	// Verify no message is pinned
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify no message is pinned
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNull("No message should be pinned initially", PinnedMessageResult.Message);
		
		// Try to unpin when no message is pinned (should succeed silently)
		FPubnubChatOperationResult UnpinResult = (*ReceivedMessage)->Unpin();
		TestFalse("Unpin should succeed even when no message is pinned", UnpinResult.Error);
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests unpinning when a different message is pinned - verifies that Unpin succeeds silently without unpinning when the message is not the pinned one.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageUnpinDifferentMessagePinnedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Unpin.4Advanced.DifferentMessagePinned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageUnpinDifferentMessagePinnedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_different_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_different";
	const FString FirstMessageText = TEXT("First message");
	const FString SecondMessageText = TEXT("Second message");
	
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
	TSharedPtr<bool> bFirstMessageReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bSecondMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> FirstMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> SecondMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect with callback to receive messages
	auto MessageLambda = [this, bFirstMessageReceived, bSecondMessageReceived, FirstMessage, SecondMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*FirstMessage)
		{
			*bFirstMessageReceived = true;
			*FirstMessage = Message;
		}
		else if(Message && *FirstMessage && !*SecondMessage)
		{
			*bSecondMessageReceived = true;
			*SecondMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait for subscription, then send first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, FirstMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(FirstMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until first message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstMessageReceived]() -> bool {
		return *bFirstMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, FirstMessage]()
	{
		if(!*FirstMessage)
		{
			AddError("First message was not received");
			return;
		}
		
		// Pin the first message
		FPubnubChatOperationResult PinResult = (*FirstMessage)->Pin();
		TestFalse("Pin first message should succeed", PinResult.Error);
	}, 0.1f));
	
	// Wait a bit, then send second message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, SecondMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(SecondMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until second message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSecondMessageReceived]() -> bool {
		return *bSecondMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Try to unpin second message when first is pinned (should succeed silently without unpinning)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, FirstMessage, SecondMessage]()
	{
		if(!*FirstMessage || !*SecondMessage)
		{
			return;
		}
		
		// Try to unpin second message (should succeed but not unpin first)
		FPubnubChatOperationResult UnpinResult = (*SecondMessage)->Unpin();
		TestFalse("Unpin should succeed even when different message is pinned", UnpinResult.Error);
		
		// Verify first message is still pinned
		FPubnubChatMessageResult PinnedMessageResult = CreateResult.Channel->GetPinnedMessage();
		TestFalse("GetPinnedMessage should succeed", PinnedMessageResult.Result.Error);
		TestNotNull("First message should still be pinned", PinnedMessageResult.Message);
		
		if(PinnedMessageResult.Message)
		{
			TestEqual("Pinned message timetoken should match first message", PinnedMessageResult.Message->GetMessageTimetoken(), (*FirstMessage)->GetMessageTimetoken());
			TestNotEqual("Pinned message timetoken should not match second message", PinnedMessageResult.Message->GetMessageTimetoken(), (*SecondMessage)->GetMessageTimetoken());
		}
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
// TOGGLEREACTION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReaction.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageToggleReactionNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_togglereaction_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to toggle reaction with uninitialized message
		FPubnubChatOperationResult ToggleResult = UninitializedMessage->ToggleReaction(TEXT("👍"));
		TestTrue("ToggleReaction should fail with uninitialized message", ToggleResult.Error);
		TestFalse("ErrorMessage should not be empty", ToggleResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionEmptyReactionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReaction.1Validation.EmptyReaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageToggleReactionEmptyReactionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_togglereaction_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_togglereaction_empty";
	const FString TestMessageText = TEXT("Test message");
	
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
	
	// Try to toggle reaction with empty string
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TEXT(""));
		TestTrue("ToggleReaction should fail with empty reaction", ToggleResult.Error);
		TestFalse("ErrorMessage should not be empty", ToggleResult.ErrorMessage.IsEmpty());
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReaction.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageToggleReactionHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_togglereaction_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_togglereaction_happy";
	const FString TestMessageText = TEXT("Test message for reactions");
	const FString TestReaction = TEXT("👍");
	
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
	
	// Add reaction (first toggle)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify no reaction exists before toggle
		FPubnubChatHasReactionResult HasReactionBefore = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionBefore.Result.Error);
		TestFalse("User should not have reaction before toggle", HasReactionBefore.HasReaction);
		
		// Toggle reaction (should add it)
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed", ToggleResult.Error);
	}, 0.1f));
	
	// Verify reaction was added
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify user has the reaction
		FPubnubChatHasReactionResult HasReactionAfter = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionAfter.Result.Error);
		TestTrue("User should have reaction after toggle", HasReactionAfter.HasReaction);
		
		// Verify reaction appears in GetReactions
		FPubnubChatGetReactionsResult GetReactionsResult = (*ReceivedMessage)->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least one reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		bool bFoundReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundReaction = true;
				TestTrue("Reaction should contain at least one user", Reaction.UserIDs.Num() >= 1);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				TestTrue("Reaction should be mine for current user", Reaction.IsMine);
				break;
			}
		}
		TestTrue("Reaction should be found in GetReactions", bFoundReaction);
	}, 0.3f));
	
	// Remove reaction (second toggle)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Toggle reaction again (should remove it)
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed on second toggle", ToggleResult.Error);
	}, 0.1f));
	
	// Verify reaction was removed
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify user no longer has the reaction
		FPubnubChatHasReactionResult HasReactionAfter = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionAfter.Result.Error);
		TestFalse("User should not have reaction after second toggle", HasReactionAfter.HasReaction);
		
		// Verify message data is updated correctly
		FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
		bool bHasReactionAction = false;
		for(const FPubnubChatMessageAction& Action : MessageData.MessageActions)
		{
			if(Action.Value == TestReaction && Action.Type == EPubnubChatMessageActionType::PCMAT_Reaction)
			{
				bHasReactionAction = true;
				break;
			}
		}
		TestFalse("Message should not have reaction action after removal", bHasReactionAction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests ToggleReaction with multiple different reactions from the same user.
 * Verifies that a user can add multiple different reactions and toggle them independently.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionMultipleReactionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReaction.4Advanced.MultipleReactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageToggleReactionMultipleReactionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_togglereaction_multi_init";
	const FString TestChannelID = SDK_PREFIX + "test_togglereaction_multi";
	const FString TestMessageText = TEXT("Test message for multiple reactions");
	const FString Reaction1 = TEXT("👍");
	const FString Reaction2 = TEXT("❤️");
	const FString Reaction3 = TEXT("😊");
	
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
	
	// Add first reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction1]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(Reaction1);
		TestFalse("ToggleReaction should succeed for reaction 1", ToggleResult.Error);
	}, 0.1f));
	
	// Add second reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction2]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(Reaction2);
		TestFalse("ToggleReaction should succeed for reaction 2", ToggleResult.Error);
	}, 0.3f));
	
	// Add third reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction3]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(Reaction3);
		TestFalse("ToggleReaction should succeed for reaction 3", ToggleResult.Error);
	}, 0.3f));
	
	// Verify all reactions are present
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction1, Reaction2, Reaction3]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify user has all reactions
		FPubnubChatHasReactionResult HasReaction1 = (*ReceivedMessage)->HasUserReaction(Reaction1);
		TestFalse("HasUserReaction check should succeed for reaction 1", HasReaction1.Result.Error);
		TestTrue("User should have reaction 1", HasReaction1.HasReaction);
		
		FPubnubChatHasReactionResult HasReaction2 = (*ReceivedMessage)->HasUserReaction(Reaction2);
		TestFalse("HasUserReaction check should succeed for reaction 2", HasReaction2.Result.Error);
		TestTrue("User should have reaction 2", HasReaction2.HasReaction);
		
		FPubnubChatHasReactionResult HasReaction3 = (*ReceivedMessage)->HasUserReaction(Reaction3);
		TestFalse("HasUserReaction check should succeed for reaction 3", HasReaction3.Result.Error);
		TestTrue("User should have reaction 3", HasReaction3.HasReaction);
		
		// Verify GetReactions returns all three reactions
		FPubnubChatGetReactionsResult GetReactionsResult = (*ReceivedMessage)->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least 3 reactions", GetReactionsResult.Reactions.Num() >= 3);
		
		int FoundReactions = 0;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == Reaction1 || Reaction.Value == Reaction2 || Reaction.Value == Reaction3)
			{
				FoundReactions++;
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				TestTrue("Reaction should belong to current user", Reaction.IsMine);
			}
		}
		TestEqual("Should find all 3 reactions", FoundReactions, 3);
	}, 0.3f));
	
	// Remove one reaction (middle one)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction2]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(Reaction2);
		TestFalse("ToggleReaction should succeed to remove reaction 2", ToggleResult.Error);
	}, 0.1f));
	
	// Verify only two reactions remain
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, Reaction1, Reaction2, Reaction3]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify user still has reactions 1 and 3, but not 2
		FPubnubChatHasReactionResult HasReaction1 = (*ReceivedMessage)->HasUserReaction(Reaction1);
		TestTrue("User should still have reaction 1", HasReaction1.HasReaction);
		
		FPubnubChatHasReactionResult HasReaction2 = (*ReceivedMessage)->HasUserReaction(Reaction2);
		TestFalse("User should not have reaction 2", HasReaction2.HasReaction);
		
		FPubnubChatHasReactionResult HasReaction3 = (*ReceivedMessage)->HasUserReaction(Reaction3);
		TestTrue("User should still have reaction 3", HasReaction3.HasReaction);
		
		// Verify GetReactions returns only 2 reactions
		FPubnubChatGetReactionsResult GetReactionsResult = (*ReceivedMessage)->GetReactions();
		int FoundReactions = 0;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == Reaction1 || Reaction.Value == Reaction3)
			{
				FoundReactions++;
				TestTrue("Remaining reactions should belong to current user", Reaction.IsMine);
			}
			if(Reaction.Value == Reaction2)
			{
				AddError("Reaction 2 should not be present");
			}
		}
		TestEqual("Should find exactly 2 reactions", FoundReactions, 2);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests ToggleReaction with multiple users reacting to the same message.
 * Verifies that reactions from different users are tracked independently.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReaction.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageToggleReactionMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_togglereaction_multi_user_init";
	const FString SecondUserID = SDK_PREFIX + "test_togglereaction_multi_user_second";
	const FString TestChannelID = SDK_PREFIX + "test_togglereaction_multi_user";
	const FString TestMessageText = TEXT("Test message for multiple users");
	const FString TestReaction = TEXT("👍");
	
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
	
	// Create second user
	FPubnubChatUserResult CreateSecondUserResult = Chat->CreateUser(SecondUserID);
	TestFalse("CreateUser should succeed for second user", CreateSecondUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		if(Chat)
		{
			Chat->DeleteUser(SecondUserID);
		}
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
	
	// First user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for first user", ToggleResult.Error);
	}, 0.1f));
	
	// Create second Chat instance for second user and add reaction
	TSharedPtr<UPubnubChat*> SecondChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> SecondChannel = MakeShared<UPubnubChatChannel*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> SecondUserMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestChannelID, TestReaction, SecondUserID, SecondChat, SecondChannel, SecondUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Create second Chat instance for second user
		const FString TestPublishKey = GetTestPublishKey();
		const FString TestSubscribeKey = GetTestSubscribeKey();
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult SecondInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SecondUserID, ChatConfig);
		TestFalse("InitChat should succeed for second user", SecondInitResult.Result.Error);
		TestNotNull("Second Chat should be created", SecondInitResult.Chat);
		
		if(!SecondInitResult.Chat)
		{
			AddError("Failed to create second Chat instance");
			return;
		}
		
		*SecondChat = SecondInitResult.Chat;
		
		// Get channel for second user
		FPubnubChatChannelResult GetChannelResult = (*SecondChat)->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed for second user", GetChannelResult.Result.Error);
		TestNotNull("Channel should be found", GetChannelResult.Channel);
		
		if(!GetChannelResult.Channel)
		{
			AddError("Failed to get channel for second user");
			return;
		}
		
		*SecondChannel = GetChannelResult.Channel;
		
		// Connect second user to channel (they won't receive the message via subscription since it was already sent)
		FPubnubChatOperationResult SecondConnectResult = (*SecondChannel)->Connect();
		TestFalse("Connect should succeed for second user", SecondConnectResult.Error);
	}, 0.3f));
	
	// Get message for second user using GetMessage (since message was sent before they connected)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondChannel, SecondUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		
		// Get message for second user
		FPubnubChatMessageResult GetMessageResult = (*SecondChannel)->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed for second user", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved for second user", GetMessageResult.Message);
		if(GetMessageResult.Message)
		{
			*SecondUserMessage = GetMessageResult.Message;
		}
	}, 0.3f));
	
	// Second user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondUserMessage, TestReaction]()
	{
		if(!*SecondUserMessage)
		{
			AddError("Second user message was not retrieved");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*SecondUserMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for second user", ToggleResult.Error);
	}, 0.1f));
	
	// Refresh message data from server and verify both users' reactions are present
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, TestReaction, InitUserID]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Refresh message from server to get latest reactions
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// First user should have the reaction
		FPubnubChatHasReactionResult HasReactionResult = GetMessageResult.Message->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestTrue("First user should have reaction", HasReactionResult.HasReaction);
		
		// GetReactions should return a single aggregated reaction with both users
		FPubnubChatGetReactionsResult GetReactionsResult = GetMessageResult.Message->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least 1 reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		// Validate aggregated reaction for TestReaction
		bool bFoundReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundReaction = true;
				TestTrue("Aggregated reaction should contain at least 2 users", Reaction.UserIDs.Num() >= 2);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				TestTrue("First user should have IsMine=true", Reaction.IsMine);
			}
		}
		TestTrue("Should find aggregated reaction entry", bFoundReaction);
	}, 0.5f));
	
	// First user removes their reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed to remove first user's reaction", ToggleResult.Error);
	}, 0.1f));
	
	// Refresh message data and verify first user no longer has reaction, but second user's reaction remains
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Refresh message from server to get latest reactions
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// First user should no longer have the reaction
		FPubnubChatHasReactionResult HasReactionResult = GetMessageResult.Message->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestFalse("First user should not have reaction after toggle", HasReactionResult.HasReaction);
		
		// GetReactions should still return at least one reaction (from second user)
		FPubnubChatGetReactionsResult GetReactionsResult = GetMessageResult.Message->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should still return at least 1 reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		// Verify at least one reaction with the same value exists (from second user)
		bool bFoundOtherUserReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundOtherUserReaction = true;
				TestFalse("Remaining reaction should not be mine for first user", Reaction.IsMine);
				TestTrue("Remaining reaction should have at least one user", Reaction.UserIDs.Num() >= 1);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				break;
			}
		}
		TestTrue("Should still find reaction from second user", bFoundOtherUserReaction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, SecondUserID, SecondChat, SecondChannel]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(*SecondChannel)
		{
			(*SecondChannel)->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(SecondUserID);
		}
		if(*SecondChat)
		{
			CleanUpCurrentChatUser(*SecondChat);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// GETREACTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetReactionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetReactions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageGetReactionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getreactions_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to get reactions with uninitialized message
		FPubnubChatGetReactionsResult GetReactionsResult = UninitializedMessage->GetReactions();
		TestTrue("GetReactions should fail with uninitialized message", GetReactionsResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetReactionsResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Reactions array should be empty", GetReactionsResult.Reactions.Num(), 0);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetReactionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetReactions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageGetReactionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getreactions_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_getreactions_happy";
	const FString TestMessageText = TEXT("Test message for GetReactions");
	const FString TestReaction = TEXT("👍");
	
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
	
	// Get reactions before adding any (should be empty)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatGetReactionsResult GetReactionsResult = (*ReceivedMessage)->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestEqual("GetReactions should return empty array for message without reactions", GetReactionsResult.Reactions.Num(), 0);
	}, 0.1f));
	
	// Add reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed", ToggleResult.Error);
	}, 0.1f));
	
	// Get reactions after adding (should have one)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatGetReactionsResult GetReactionsResult = (*ReceivedMessage)->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least one reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		// Verify reaction details
		bool bFoundReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundReaction = true;
				TestTrue("Reaction should be mine", Reaction.IsMine);
				TestTrue("Reaction should contain at least one user", Reaction.UserIDs.Num() >= 1);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				break;
			}
		}
		TestTrue("Reaction should be found in GetReactions", bFoundReaction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests GetReactions with multiple reactions from different users.
 * Verifies that GetReactions correctly returns all reactions regardless of which user added them.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetReactionsMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetReactions.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageGetReactionsMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_getreactions_multi_user_init";
	const FString SecondUserID = SDK_PREFIX + "test_getreactions_multi_user_second";
	const FString ThirdUserID = SDK_PREFIX + "test_getreactions_multi_user_third";
	const FString TestChannelID = SDK_PREFIX + "test_getreactions_multi_user";
	const FString TestMessageText = TEXT("Test message for multiple users reactions");
	const FString TestReaction = TEXT("👍");
	
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
	
	// Create additional users
	FPubnubChatUserResult CreateSecondUserResult = Chat->CreateUser(SecondUserID);
	TestFalse("CreateUser should succeed for second user", CreateSecondUserResult.Result.Error);
	
	FPubnubChatUserResult CreateThirdUserResult = Chat->CreateUser(ThirdUserID);
	TestFalse("CreateUser should succeed for third user", CreateThirdUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		if(Chat)
		{
			Chat->DeleteUser(SecondUserID);
			Chat->DeleteUser(ThirdUserID);
		}
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
	
	// First user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for first user", ToggleResult.Error);
	}, 0.1f));
	
	// Create second and third Chat instances and add reactions
	TSharedPtr<UPubnubChat*> SecondChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChat*> ThirdChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> SecondChannel = MakeShared<UPubnubChatChannel*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> ThirdChannel = MakeShared<UPubnubChatChannel*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> SecondUserMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> ThirdUserMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestChannelID, TestReaction, SecondUserID, ThirdUserID, SecondChat, ThirdChat, SecondChannel, ThirdChannel, SecondUserMessage, ThirdUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Create second Chat instance for second user
		const FString TestPublishKey = GetTestPublishKey();
		const FString TestSubscribeKey = GetTestSubscribeKey();
		FPubnubChatConfig ChatConfig;
		
		FPubnubChatInitChatResult SecondInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SecondUserID, ChatConfig);
		TestFalse("InitChat should succeed for second user", SecondInitResult.Result.Error);
		TestNotNull("Second Chat should be created", SecondInitResult.Chat);
		if(!SecondInitResult.Chat) return;
		*SecondChat = SecondInitResult.Chat;
		
		FPubnubChatInitChatResult ThirdInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, ThirdUserID, ChatConfig);
		TestFalse("InitChat should succeed for third user", ThirdInitResult.Result.Error);
		TestNotNull("Third Chat should be created", ThirdInitResult.Chat);
		if(!ThirdInitResult.Chat) return;
		*ThirdChat = ThirdInitResult.Chat;
		
		// Get channels for second and third users
		FPubnubChatChannelResult GetChannelResult2 = (*SecondChat)->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed for second user", GetChannelResult2.Result.Error);
		TestNotNull("Channel should be found for second user", GetChannelResult2.Channel);
		if(!GetChannelResult2.Channel) return;
		*SecondChannel = GetChannelResult2.Channel;
		
		FPubnubChatChannelResult GetChannelResult3 = (*ThirdChat)->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed for third user", GetChannelResult3.Result.Error);
		TestNotNull("Channel should be found for third user", GetChannelResult3.Channel);
		if(!GetChannelResult3.Channel) return;
		*ThirdChannel = GetChannelResult3.Channel;
		
		// Connect second and third users to channel (they won't receive the message via subscription since it was already sent)
		FPubnubChatOperationResult SecondConnectResult = (*SecondChannel)->Connect();
		TestFalse("Connect should succeed for second user", SecondConnectResult.Error);
		
		FPubnubChatOperationResult ThirdConnectResult = (*ThirdChannel)->Connect();
		TestFalse("Connect should succeed for third user", ThirdConnectResult.Error);
	}, 0.3f));
	
	// Get message for second and third users using GetMessage (since message was sent before they connected)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondChannel, ThirdChannel, SecondUserMessage, ThirdUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		
		// Get message for second user
		FPubnubChatMessageResult GetMessageResult2 = (*SecondChannel)->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed for second user", GetMessageResult2.Result.Error);
		TestNotNull("Message should be retrieved for second user", GetMessageResult2.Message);
		if(GetMessageResult2.Message)
		{
			*SecondUserMessage = GetMessageResult2.Message;
		}
		
		// Get message for third user
		FPubnubChatMessageResult GetMessageResult3 = (*ThirdChannel)->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed for third user", GetMessageResult3.Result.Error);
		TestNotNull("Message should be retrieved for third user", GetMessageResult3.Message);
		if(GetMessageResult3.Message)
		{
			*ThirdUserMessage = GetMessageResult3.Message;
		}
	}, 0.3f));
	
	// Second user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondUserMessage, TestReaction]()
	{
		if(!*SecondUserMessage)
		{
			AddError("Second user message was not retrieved");
			return;
		}
		FPubnubChatOperationResult ToggleResult = (*SecondUserMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for second user", ToggleResult.Error);
	}, 0.1f));
	
	// Third user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThirdUserMessage, TestReaction]()
	{
		if(!*ThirdUserMessage)
		{
			AddError("Third user message was not retrieved");
			return;
		}
		FPubnubChatOperationResult ToggleResult = (*ThirdUserMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for third user", ToggleResult.Error);
	}, 0.1f));
	
	// Refresh message from server and verify GetReactions returns all reactions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Refresh message from server to get latest reactions from all users
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		FPubnubChatGetReactionsResult GetReactionsResult = GetMessageResult.Message->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least 1 reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		// Validate aggregated reaction for TestReaction
		bool bFoundReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundReaction = true;
				TestTrue("Aggregated reaction should contain at least 3 users", Reaction.UserIDs.Num() >= 3);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				TestTrue("Current user should have IsMine=true", Reaction.IsMine);
			}
		}
		TestTrue("Should find aggregated reaction entry", bFoundReaction);
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, SecondUserID, ThirdUserID, SecondChat, ThirdChat, SecondChannel, ThirdChannel]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(*SecondChannel)
		{
			(*SecondChannel)->Disconnect();
		}
		if(*ThirdChannel)
		{
			(*ThirdChannel)->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(SecondUserID);
			Chat->DeleteUser(ThirdUserID);
		}
		if(*SecondChat)
		{
			CleanUpCurrentChatUser(*SecondChat);
		}
		if(*ThirdChat)
		{
			CleanUpCurrentChatUser(*ThirdChat);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// HASUSERREACTION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasUserReactionNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasUserReaction.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageHasUserReactionNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_hasuserreaction_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to check reaction with uninitialized message
		FPubnubChatHasReactionResult HasReactionResult = UninitializedMessage->HasUserReaction(TEXT("👍"));
		TestTrue("HasUserReaction should fail with uninitialized message", HasReactionResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HasReactionResult.Result.ErrorMessage.IsEmpty());
		TestFalse("HasReaction should be false for uninitialized message", HasReactionResult.HasReaction);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasUserReactionEmptyReactionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasUserReaction.1Validation.EmptyReaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageHasUserReactionEmptyReactionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_hasuserreaction_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_hasuserreaction_empty";
	const FString TestMessageText = TEXT("Test message");
	
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
	
	// Try to check reaction with empty string
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasReactionResult HasReactionResult = (*ReceivedMessage)->HasUserReaction(TEXT(""));
		TestTrue("HasUserReaction should fail with empty reaction", HasReactionResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HasReactionResult.Result.ErrorMessage.IsEmpty());
		TestFalse("HasReaction should be false for empty reaction", HasReactionResult.HasReaction);
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channel
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
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasUserReactionHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasUserReaction.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageHasUserReactionHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_hasuserreaction_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_hasuserreaction_happy";
	const FString TestMessageText = TEXT("Test message for HasUserReaction");
	const FString TestReaction = TEXT("👍");
	const FString OtherReaction = TEXT("❤️");
	
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
	
	// Check reaction before adding (should be false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatHasReactionResult HasReactionResult = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestFalse("User should not have reaction before adding", HasReactionResult.HasReaction);
	}, 0.1f));
	
	// Add reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed", ToggleResult.Error);
	}, 0.1f));
	
	// Check reaction after adding (should be true)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction, OtherReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// User should have the reaction they added
		FPubnubChatHasReactionResult HasReactionResult = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestTrue("User should have reaction after adding", HasReactionResult.HasReaction);
		
		// User should not have a different reaction
		FPubnubChatHasReactionResult HasOtherReactionResult = (*ReceivedMessage)->HasUserReaction(OtherReaction);
		TestFalse("HasUserReaction check should succeed for other reaction", HasOtherReactionResult.Result.Error);
		TestFalse("User should not have other reaction", HasOtherReactionResult.HasReaction);
	}, 0.3f));
	
	// Remove reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed to remove", ToggleResult.Error);
	}, 0.1f));
	
	// Check reaction after removing (should be false)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatHasReactionResult HasReactionResult = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestFalse("User should not have reaction after removing", HasReactionResult.HasReaction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
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
 * Tests HasUserReaction with multiple users - verifies that each user's reactions are tracked independently.
 * User A adds a reaction, User B adds the same reaction, but HasUserReaction only returns true for the user who actually added it.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageHasUserReactionMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.HasUserReaction.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageHasUserReactionMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_hasuserreaction_multi_user_init";
	const FString SecondUserID = SDK_PREFIX + "test_hasuserreaction_multi_user_second";
	const FString TestChannelID = SDK_PREFIX + "test_hasuserreaction_multi_user";
	const FString TestMessageText = TEXT("Test message for multiple users HasUserReaction");
	const FString TestReaction = TEXT("👍");
	
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
	
	// Create second user
	FPubnubChatUserResult CreateSecondUserResult = Chat->CreateUser(SecondUserID);
	TestFalse("CreateUser should succeed for second user", CreateSecondUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		if(Chat)
		{
			Chat->DeleteUser(SecondUserID);
		}
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
	
	// First user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for first user", ToggleResult.Error);
	}, 0.1f));
	
	// Verify first user has the reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// First user should have the reaction
		FPubnubChatHasReactionResult HasReactionResult = (*ReceivedMessage)->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestTrue("First user should have reaction", HasReactionResult.HasReaction);
	}, 0.3f));
	
	// Create second Chat instance for second user and add reaction
	TSharedPtr<UPubnubChat*> SecondChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> SecondChannel = MakeShared<UPubnubChatChannel*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> SecondUserMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestChannelID, TestReaction, SecondUserID, SecondChat, SecondChannel, SecondUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Create second Chat instance for second user
		const FString TestPublishKey = GetTestPublishKey();
		const FString TestSubscribeKey = GetTestSubscribeKey();
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult SecondInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SecondUserID, ChatConfig);
		TestFalse("InitChat should succeed for second user", SecondInitResult.Result.Error);
		TestNotNull("Second Chat should be created", SecondInitResult.Chat);
		if(!SecondInitResult.Chat) return;
		*SecondChat = SecondInitResult.Chat;
		
		// Get channel for second user
		FPubnubChatChannelResult GetChannelResult = (*SecondChat)->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed for second user", GetChannelResult.Result.Error);
		TestNotNull("Channel should be found", GetChannelResult.Channel);
		if(!GetChannelResult.Channel) return;
		*SecondChannel = GetChannelResult.Channel;
		
		// Connect second user to channel (they won't receive the message via subscription since it was already sent)
		FPubnubChatOperationResult SecondConnectResult = (*SecondChannel)->Connect();
		TestFalse("Connect should succeed for second user", SecondConnectResult.Error);
	}, 0.3f));
	
	// Get message for second user using GetMessage (since message was sent before they connected)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondChannel, SecondUserMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		
		// Get message for second user
		FPubnubChatMessageResult GetMessageResult = (*SecondChannel)->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed for second user", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved for second user", GetMessageResult.Message);
		if(GetMessageResult.Message)
		{
			*SecondUserMessage = GetMessageResult.Message;
		}
	}, 0.3f));
	
	// Second user adds reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondUserMessage, TestReaction]()
	{
		if(!*SecondUserMessage)
		{
			AddError("Second user message was not retrieved");
			return;
		}
		FPubnubChatOperationResult ToggleResult = (*SecondUserMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed for second user", ToggleResult.Error);
	}, 0.1f));
	
	// Refresh message from server and verify first user still has the reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Refresh message from server to get latest reactions from all users
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// First user should still have the reaction
		FPubnubChatHasReactionResult HasReactionResult = GetMessageResult.Message->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestTrue("First user should still have reaction", HasReactionResult.HasReaction);
		
		// GetReactions should show one aggregated reaction from both users
		FPubnubChatGetReactionsResult GetReactionsResult = GetMessageResult.Message->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should return at least 1 reaction", GetReactionsResult.Reactions.Num() >= 1);
	}, 0.5f));
	
	// First user removes their reaction
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ToggleResult = (*ReceivedMessage)->ToggleReaction(TestReaction);
		TestFalse("ToggleReaction should succeed to remove first user's reaction", ToggleResult.Error);
	}, 0.1f));
	
	// Refresh message from server and verify first user no longer has the reaction (but second user's reaction still exists)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, TestReaction]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Refresh message from server to get latest reactions from all users
		FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
		TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
		TestNotNull("Message should be retrieved", GetMessageResult.Message);
		
		if(!GetMessageResult.Message)
		{
			return;
		}
		
		// First user should no longer have the reaction
		FPubnubChatHasReactionResult HasReactionResult = GetMessageResult.Message->HasUserReaction(TestReaction);
		TestFalse("HasUserReaction check should succeed", HasReactionResult.Result.Error);
		TestFalse("First user should not have reaction after removal", HasReactionResult.HasReaction);
		
		// GetReactions should still show at least one reaction (from second user)
		FPubnubChatGetReactionsResult GetReactionsResult = GetMessageResult.Message->GetReactions();
		TestFalse("GetReactions should succeed", GetReactionsResult.Result.Error);
		TestTrue("GetReactions should still return at least 1 reaction", GetReactionsResult.Reactions.Num() >= 1);
		
		// Verify at least one reaction with the same value exists (from second user)
		bool bFoundOtherUserReaction = false;
		for(const FPubnubChatMessageReaction& Reaction : GetReactionsResult.Reactions)
		{
			if(Reaction.Value == TestReaction)
			{
				bFoundOtherUserReaction = true;
				TestFalse("Remaining reaction should not be mine for first user", Reaction.IsMine);
				TestTrue("Remaining reaction should have at least one user", Reaction.UserIDs.Num() >= 1);
				TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
				break;
			}
		}
		TestTrue("Should still find reaction from second user", bFoundOtherUserReaction);
	}, 0.3f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID, SecondUserID, SecondChat, SecondChannel]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(*SecondChannel)
		{
			(*SecondChannel)->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(SecondUserID);
		}
		if(*SecondChat)
		{
			CleanUpCurrentChatUser(*SecondChat);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// CREATETHREAD TESTS (Message)
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageCreateThreadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.CreateThread.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageCreateThreadHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_create_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_create_thread_happy";
	
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
	const FString TestMessageText = TEXT("Test message for create thread");
	
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
	
	// Create thread using Message->CreateThread()
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult CreateResult = (*ReceivedMessage)->CreateThread();
		
		TestFalse("CreateThread should succeed", CreateResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateResult.ThreadChannel);
		
		if(CreateResult.ThreadChannel)
		{
			// Verify thread channel properties
			TestEqual("ParentChannelID should match", CreateResult.ThreadChannel->GetParentChannelID(), (*ReceivedMessage)->GetMessageData().ChannelID);
			TestEqual("ParentMessage should match", CreateResult.ThreadChannel->GetParentMessage(), *ReceivedMessage);
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
// GETTHREAD TESTS (Message)
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetThreadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetThread.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageGetThreadHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_get_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_get_thread_happy";
	
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
	const FString TestMessageText = TEXT("Test message for get thread");
	
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
	
	// Wait a bit for thread confirmation, then get thread using Message->GetThread()
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatThreadChannelResult GetResult = (*ReceivedMessage)->GetThread();
		
		TestFalse("GetThread should succeed", GetResult.Result.Error);
		TestNotNull("ThreadChannel should be returned", GetResult.ThreadChannel);
		
		if(GetResult.ThreadChannel)
		{
			// Verify thread channel properties
			TestEqual("ParentChannelID should match", GetResult.ThreadChannel->GetParentChannelID(), (*ReceivedMessage)->GetMessageData().ChannelID);
			TestEqual("ParentMessage should match", GetResult.ThreadChannel->GetParentMessage(), *ReceivedMessage);
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
// REMOVETHREAD TESTS (Message)
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRemoveThreadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.RemoveThread.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageRemoveThreadHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_remove_thread_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_remove_thread_happy";
	
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
	const FString TestMessageText = TEXT("Test message for remove thread");
	
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
	
	// Wait a bit for thread confirmation, then remove thread using Message->RemoveThread()
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		// Verify message has thread before removal
		FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
		if(!HasThreadResult.HasThread)
		{
			AddError("Message should have thread before removal - thread may not be confirmed yet");
			return;
		}
		TestTrue("Message should have thread before removal", HasThreadResult.HasThread);
		
		FPubnubChatOperationResult RemoveResult = (*ReceivedMessage)->RemoveThread();
		
		TestFalse("RemoveThread should succeed", RemoveResult.Error);
	}, 1.5f));
	
	// Wait a bit for removal to propagate, then verify message no longer has thread
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
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// INTERNAL UTILITIES UNIT TESTS - REACTION AGGREGATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReactionsFilterNonReactionActionsTest, "PubnubChat.Unit.Message.GetReactions.FilterNonReactionActions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageReactionsFilterNonReactionActionsTest::RunTest(const FString& Parameters)
{
	const FString CurrentUserID = TEXT("user_current");
	const FString OtherUserID = TEXT("user_other");

	TArray<FPubnubChatMessageAction> MessageActions;

	// Valid reaction action for current user
	FPubnubChatMessageAction CurrentUserReaction;
	CurrentUserReaction.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	CurrentUserReaction.Value = TEXT("thumbs_up");
	CurrentUserReaction.UserID = CurrentUserID;
	CurrentUserReaction.Timetoken = TEXT("1001");
	MessageActions.Add(CurrentUserReaction);

	// Valid reaction action for another user
	FPubnubChatMessageAction OtherUserReaction;
	OtherUserReaction.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	OtherUserReaction.Value = TEXT("laugh");
	OtherUserReaction.UserID = OtherUserID;
	OtherUserReaction.Timetoken = TEXT("1002");
	MessageActions.Add(OtherUserReaction);

	// Non-reaction actions that should be ignored by aggregation
	FPubnubChatMessageAction EditedAction;
	EditedAction.Type = EPubnubChatMessageActionType::PCMAT_Edited;
	EditedAction.Value = TEXT("thumbs_up"); // same value as reaction on purpose
	EditedAction.UserID = OtherUserID;
	EditedAction.Timetoken = TEXT("1003");
	MessageActions.Add(EditedAction);

	FPubnubChatMessageAction DeletedAction;
	DeletedAction.Type = EPubnubChatMessageActionType::PCMAT_Deleted;
	DeletedAction.Value = TEXT("laugh"); // same value as reaction on purpose
	DeletedAction.UserID = OtherUserID;
	DeletedAction.Timetoken = TEXT("1004");
	MessageActions.Add(DeletedAction);

	FPubnubChatMessageAction ThreadRootAction;
	ThreadRootAction.Type = EPubnubChatMessageActionType::PCMAT_ThreadRootId;
	ThreadRootAction.Value = TEXT("root_id");
	ThreadRootAction.UserID = OtherUserID;
	ThreadRootAction.Timetoken = TEXT("1005");
	MessageActions.Add(ThreadRootAction);

	const TArray<FPubnubChatMessageReaction> Reactions = UPubnubChatInternalUtilities::GetMessageReactionsFromMessageActions(CurrentUserID, MessageActions);

	TestEqual("Only reaction actions should be aggregated", Reactions.Num(), 2);

	const FPubnubChatMessageReaction ThumbsUpReaction = UPubnubChatInternalUtilities::GetReactionFromArrayByValue(TEXT("thumbs_up"), Reactions);
	TestEqual("thumbs_up reaction should exist", ThumbsUpReaction.Value, FString(TEXT("thumbs_up")));
	TestTrue("thumbs_up should be mine", ThumbsUpReaction.IsMine);
	TestEqual("thumbs_up count should match user IDs", ThumbsUpReaction.Count, ThumbsUpReaction.UserIDs.Num());
	TestEqual("thumbs_up should have exactly one user", ThumbsUpReaction.Count, 1);
	TestTrue("thumbs_up should contain current user", ThumbsUpReaction.UserIDs.Contains(CurrentUserID));

	const FPubnubChatMessageReaction LaughReaction = UPubnubChatInternalUtilities::GetReactionFromArrayByValue(TEXT("laugh"), Reactions);
	TestEqual("laugh reaction should exist", LaughReaction.Value, FString(TEXT("laugh")));
	TestFalse("laugh should not be mine", LaughReaction.IsMine);
	TestEqual("laugh count should match user IDs", LaughReaction.Count, LaughReaction.UserIDs.Num());
	TestEqual("laugh should have exactly one user", LaughReaction.Count, 1);
	TestTrue("laugh should contain other user", LaughReaction.UserIDs.Contains(OtherUserID));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReactionsCurrentUserNotPresentTest, "PubnubChat.Unit.Message.GetReactions.CurrentUserNotPresent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageReactionsCurrentUserNotPresentTest::RunTest(const FString& Parameters)
{
	const FString CurrentUserID = TEXT("user_current");
	const FString UserA = TEXT("user_a");
	const FString UserB = TEXT("user_b");

	TArray<FPubnubChatMessageAction> MessageActions;

	// Two different users react with the same value
	FPubnubChatMessageAction ReactionA;
	ReactionA.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	ReactionA.Value = TEXT("fire");
	ReactionA.UserID = UserA;
	ReactionA.Timetoken = TEXT("2001");
	MessageActions.Add(ReactionA);

	FPubnubChatMessageAction ReactionB;
	ReactionB.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	ReactionB.Value = TEXT("fire");
	ReactionB.UserID = UserB;
	ReactionB.Timetoken = TEXT("2002");
	MessageActions.Add(ReactionB);

	// Additional different reaction by another user
	FPubnubChatMessageAction HeartReaction;
	HeartReaction.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	HeartReaction.Value = TEXT("heart");
	HeartReaction.UserID = UserB;
	HeartReaction.Timetoken = TEXT("2003");
	MessageActions.Add(HeartReaction);

	const TArray<FPubnubChatMessageReaction> Reactions = UPubnubChatInternalUtilities::GetMessageReactionsFromMessageActions(CurrentUserID, MessageActions);

	TestEqual("Should return two aggregated reactions", Reactions.Num(), 2);

	const FPubnubChatMessageReaction FireReaction = UPubnubChatInternalUtilities::GetReactionFromArrayByValue(TEXT("fire"), Reactions);
	TestEqual("fire reaction should exist", FireReaction.Value, FString(TEXT("fire")));
	TestFalse("fire should not be mine when current user is absent", FireReaction.IsMine);
	TestEqual("fire count should match user IDs", FireReaction.Count, FireReaction.UserIDs.Num());
	TestEqual("fire should aggregate both users", FireReaction.Count, 2);
	TestTrue("fire should contain user A", FireReaction.UserIDs.Contains(UserA));
	TestTrue("fire should contain user B", FireReaction.UserIDs.Contains(UserB));
	TestFalse("fire should not contain current user", FireReaction.UserIDs.Contains(CurrentUserID));

	const FPubnubChatMessageReaction HeartReactionResult = UPubnubChatInternalUtilities::GetReactionFromArrayByValue(TEXT("heart"), Reactions);
	TestEqual("heart reaction should exist", HeartReactionResult.Value, FString(TEXT("heart")));
	TestFalse("heart should not be mine when current user is absent", HeartReactionResult.IsMine);
	TestEqual("heart count should match user IDs", HeartReactionResult.Count, HeartReactionResult.UserIDs.Num());
	TestEqual("heart should have one user", HeartReactionResult.Count, 1);
	TestTrue("heart should contain user B", HeartReactionResult.UserIDs.Contains(UserB));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

