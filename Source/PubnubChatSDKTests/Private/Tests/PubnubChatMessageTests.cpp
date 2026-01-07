// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, MessageTimetoken](UPubnubChatMessage* Message)
	{
		if(Message && MessageTimetoken->IsEmpty())
		{
			*bMessageReceived = true;
			*MessageTimetoken = Message->GetMessageTimetoken();
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
		FPubnubChatChannelResult DeleteResult = Chat->DeleteChannel(TestChannelID, false);
		TestFalse("DeleteChannel should succeed", DeleteResult.Result.Error);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bFirstMessageReceived, bSecondMessageReceived, FirstMessage, SecondMessage](UPubnubChatMessage* Message)
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
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
		FPubnubChatChannelResult DeleteResult = Chat->DeleteChannel(TestChannelID, false);
		TestFalse("DeleteChannel should succeed", DeleteResult.Result.Error);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
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
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bFirstMessageReceived, bSecondMessageReceived, FirstMessage, SecondMessage](UPubnubChatMessage* Message)
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
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
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
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

