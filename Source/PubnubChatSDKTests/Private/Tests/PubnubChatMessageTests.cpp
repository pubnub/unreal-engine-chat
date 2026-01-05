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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageGetCurrentTextMultipleEditsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.GetCurrentText.3Advanced.MultipleEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextMultipleEditsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditText.3Advanced.MultipleEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

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

#endif // WITH_DEV_AUTOMATION_TESTS

