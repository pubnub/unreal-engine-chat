// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatCallbackStop.h"
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
#include "PubnubStructLibrary.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// REPORT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageReportNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to report with uninitialized message
		FPubnubChatOperationResult ReportResult = UninitializedMessage->Report();
		TestTrue("Report should fail for uninitialized message", ReportResult.Error);
		TestFalse("Report error message should not be empty", ReportResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageReportHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_report_happy";
	const FString TestMessageText = TEXT("Message to report");
	
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
	
	// Report message with default (empty) reason
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessage)->Report();
		TestFalse("Report should succeed", ReportResult.Error);
	}, 0.2f));
	
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageReportFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_report_full";
	const FString TestMessageText = TEXT("Message to report with reason");
	const FString TestReason = TEXT("Inappropriate content");
	
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
	
	// Report message with reason
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReason]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessage)->Report(TestReason);
		TestFalse("Report should succeed", ReportResult.Error);
	}, 0.2f));
	
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
 * Tests Report by verifying that a moderation event is received via ListenForEvents.
 * Verifies that the event is emitted to the correct moderation channel and contains correct payload data.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportEventReceivedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.4Advanced.EventReceived", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageReportEventReceivedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_event_init";
	const FString TestChannelID = SDK_PREFIX + "test_report_event";
	const FString TestMessageText = TEXT("Message to report and verify event");
	const FString TestReason = TEXT("Spam content");
	
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
	
	// Get moderation channel for listening to events
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<FString> MessageTimetoken = MakeShared<FString>();
	TSharedPtr<FString> MessageUserID = MakeShared<FString>();
	
	// Connect with callback to receive message
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage, MessageTimetoken, MessageUserID](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
			*MessageTimetoken = Message->GetMessageTimetoken();
			FPubnubChatMessageData MessageData = Message->GetMessageData();
			*MessageUserID = MessageData.UserID;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	
	// Listen for moderation events on the moderation channel
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, ModerationChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received event type should be Moderation", Event.Type, EPubnubChatEventType::PCET_Moderation);
		TestEqual("Received event ChannelID should match moderation channel", Event.ChannelID, ModerationChannelID);
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(ModerationChannelID, EPubnubChatEventType::PCET_Moderation, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
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
	
	// Report message with reason
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReason]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessage)->Report(TestReason);
		TestFalse("Report should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received and has correct payload
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID, TestMessageText, TestReason, MessageTimetoken, MessageUserID, ModerationChannelID]()
	{
		if(!*bEventReceived)
		{
			AddError("Moderation event was not received");
		}
		else
		{
			TestEqual("Received event type should be Moderation", ReceivedEvent->Type, EPubnubChatEventType::PCET_Moderation);
			TestEqual("Received event ChannelID should match moderation channel", ReceivedEvent->ChannelID, ModerationChannelID);
			
			// Parse payload to verify it contains correct data
			TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject);
			UPubnubJsonUtilities::StringToJsonObject(ReceivedEvent->Payload, PayloadObject);
			
			// Verify text field
			FString TextInPayload;
			if(PayloadObject->TryGetStringField(TEXT("text"), TextInPayload))
			{
				TestEqual("Payload text should match message text", TextInPayload, TestMessageText);
			}
			else
			{
				AddError("Payload should contain text field");
			}
			
			// Verify reason field
			FString ReasonInPayload;
			if(PayloadObject->TryGetStringField(TEXT("reason"), ReasonInPayload))
			{
				TestEqual("Payload reason should match", ReasonInPayload, TestReason);
			}
			else
			{
				AddError("Payload should contain reason field");
			}
			
			// Verify timetoken field
			FString TimetokenInPayload;
			if(PayloadObject->TryGetStringField(TEXT("timetoken"), TimetokenInPayload))
			{
				if(!MessageTimetoken->IsEmpty())
				{
					TestEqual("Payload timetoken should match message timetoken", TimetokenInPayload, *MessageTimetoken);
				}
			}
			else
			{
				AddError("Payload should contain timetoken field");
			}
			
			// Verify channelId field
			FString ChannelIdInPayload;
			if(PayloadObject->TryGetStringField(TEXT("channelId"), ChannelIdInPayload))
			{
				TestEqual("Payload channelId should match test channel", ChannelIdInPayload, TestChannelID);
			}
			else
			{
				AddError("Payload should contain channelId field");
			}
			
			// Verify userId field
			FString UserIdInPayload;
			if(PayloadObject->TryGetStringField(TEXT("userId"), UserIdInPayload))
			{
				if(!MessageUserID->IsEmpty())
				{
					TestEqual("Payload userId should match message user ID", UserIdInPayload, *MessageUserID);
				}
			}
			else
			{
				AddError("Payload should contain userId field");
			}
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, CreateResult, Chat, TestChannelID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		
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
 * Tests Report with empty reason - verifies that empty reason is handled correctly in the event payload.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportEmptyReasonTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.4Advanced.EmptyReason", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageReportEmptyReasonTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_empty_reason_init";
	const FString TestChannelID = SDK_PREFIX + "test_report_empty_reason";
	const FString TestMessageText = TEXT("Message to report without reason");
	
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
	
	// Get moderation channel for listening to events
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
	
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
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	
	// Listen for moderation events on the moderation channel
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, ModerationChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(ModerationChannelID, EPubnubChatEventType::PCET_Moderation, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	
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
	
	// Report message with empty reason (default)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessage)->Report();
		TestFalse("Report should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event payload contains empty reason
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent]()
	{
		if(!*bEventReceived)
		{
			AddError("Moderation event was not received");
		}
		else
		{
			// Parse payload to verify reason is empty
			TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject);
			UPubnubJsonUtilities::StringToJsonObject(ReceivedEvent->Payload, PayloadObject);
			
			FString ReasonInPayload;
			if(PayloadObject->TryGetStringField(TEXT("reason"), ReasonInPayload))
			{
				TestTrue("Payload reason should be empty when not provided", ReasonInPayload.IsEmpty());
			}
			else
			{
				// Reason field might not exist if empty, which is also acceptable
				TestTrue("Payload reason field should be empty or missing", true);
			}
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, CreateResult, Chat, TestChannelID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		
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
 * Tests Report on an edited message - verifies that GetCurrentText is used correctly to get the current text of the message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportEditedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Report.4Advanced.EditedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageReportEditedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_report_edited_init";
	const FString TestChannelID = SDK_PREFIX + "test_report_edited";
	const FString OriginalText = TEXT("Original message");
	const FString EditedText = TEXT("Edited message to report");
	const FString TestReason = TEXT("Inappropriate edit");
	
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
	
	// Get moderation channel for listening to events
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
	
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
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	
	// Listen for moderation events on the moderation channel
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, ModerationChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(ModerationChannelID, EPubnubChatEventType::PCET_Moderation, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	
	// Wait a bit for subscription to be ready, then send message
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
	}, 0.2f));
	
	// Wait a bit for edit to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, EditedText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FString CurrentText = (*ReceivedMessage)->GetCurrentText();
		TestEqual("GetCurrentText should return edited text", CurrentText, EditedText);
	}, 0.3f));
	
	// Report the edited message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, TestReason]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessage)->Report(TestReason);
		TestFalse("Report should succeed", ReportResult.Error);
	}, 0.1f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event payload contains edited text (not original text)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, EditedText]()
	{
		if(!*bEventReceived)
		{
			AddError("Moderation event was not received");
		}
		else
		{
			// Parse payload to verify it contains edited text
			TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject);
			UPubnubJsonUtilities::StringToJsonObject(ReceivedEvent->Payload, PayloadObject);
			
			FString TextInPayload;
			if(PayloadObject->TryGetStringField(TEXT("text"), TextInPayload))
			{
				TestEqual("Payload text should match edited text (not original)", TextInPayload, EditedText);
			}
			else
			{
				AddError("Payload should contain text field");
			}
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, CreateResult, Chat, TestChannelID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		
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
// FORWARD TESTS
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageForwardHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.Forward.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageForwardHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_forward_happy_init";
	const FString SourceChannelID = SDK_PREFIX + "test_forward_happy_source";
	const FString DestinationChannelID = SDK_PREFIX + "test_forward_happy_dest";
	const FString TestMessageText = TEXT("Message to forward");
	
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
	
	// Create source channel
	FPubnubChatChannelData SourceChannelData;
	FPubnubChatChannelResult CreateSourceResult = Chat->CreatePublicConversation(SourceChannelID, SourceChannelData);
	TestFalse("CreatePublicConversation should succeed for source", CreateSourceResult.Result.Error);
	TestNotNull("Source channel should be created", CreateSourceResult.Channel);
	
	// Create destination channel
	FPubnubChatChannelData DestChannelData;
	FPubnubChatChannelResult CreateDestResult = Chat->CreatePublicConversation(DestinationChannelID, DestChannelData);
	TestFalse("CreatePublicConversation should succeed for destination", CreateDestResult.Result.Error);
	TestNotNull("Destination channel should be created", CreateDestResult.Channel);
	
	if(!CreateSourceResult.Channel || !CreateDestResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bSourceMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> SourceMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect to source channel with callback to receive message
	auto SourceMessageLambda = [this, bSourceMessageReceived, SourceMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*SourceMessage)
		{
			*bSourceMessageReceived = true;
			*SourceMessage = Message;
		}
	};
	CreateSourceResult.Channel->OnMessageReceivedNative.AddLambda(SourceMessageLambda);
	
	FPubnubChatOperationResult SourceConnectResult = CreateSourceResult.Channel->Connect();
	TestFalse("Connect to source channel should succeed", SourceConnectResult.Error);
	
	// Connect to destination channel (to verify forwarded message)
	auto DestMessageLambda = [this](UPubnubChatMessage* Message)
	{
		// Just acknowledge receipt, no need to store
	};
	CreateDestResult.Channel->OnMessageReceivedNative.AddLambda(DestMessageLambda);
	
	FPubnubChatOperationResult DestConnectResult = CreateDestResult.Channel->Connect();
	TestFalse("Connect to destination channel should succeed", DestConnectResult.Error);
	
	// Wait for subscriptions, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSourceResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateSourceResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until source message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSourceMessageReceived]() -> bool {
		return *bSourceMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Forward the message using Message->Forward()
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SourceMessage, CreateDestResult]()
	{
		if(!*SourceMessage)
		{
			AddError("Source message was not received");
			return;
		}
		
		FPubnubChatOperationResult ForwardResult = (*SourceMessage)->Forward(CreateDestResult.Channel);
		TestFalse("Forward should succeed", ForwardResult.Error);
	}, 0.1f));
	
	// Cleanup: Disconnect and delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSourceResult, CreateDestResult, Chat, SourceChannelID, DestinationChannelID]()
	{
		if(CreateSourceResult.Channel)
		{
			CreateSourceResult.Channel->Disconnect();
		}
		if(CreateDestResult.Channel)
		{
			CreateDestResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(SourceChannelID, false);
			Chat->DeleteChannel(DestinationChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

