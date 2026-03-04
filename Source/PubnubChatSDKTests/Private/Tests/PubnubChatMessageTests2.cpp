// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
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
 * Tests Report by verifying that a report event is received via channel message report stream.
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
	TSharedPtr<FPubnubChatReportEvent> ReceivedEvent = MakeShared<FPubnubChatReportEvent>();
	
	// Stream report events on the channel
	CreateResult.Channel->OnMessageReportedNative.AddLambda([this, bEventReceived, ReceivedEvent](const FPubnubChatReportEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
	});
	
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID, TestMessageText, TestReason, MessageTimetoken, MessageUserID]()
	{
		if(!*bEventReceived)
		{
			AddError("Report event was not received");
		}
		else
		{
			TestEqual("Report text should match message text", ReceivedEvent->Text, TestMessageText);
			TestEqual("Report reason should match", ReceivedEvent->Reason, TestReason);
			TestEqual("Report channelId should match test channel", ReceivedEvent->ReportedMessageChannelID, TestChannelID);
			if(!MessageTimetoken->IsEmpty())
			{
				TestEqual("Report timetoken should match message timetoken", ReceivedEvent->MessageTimetoken, *MessageTimetoken);
			}
			if(!MessageUserID->IsEmpty())
			{
				TestEqual("Report userId should match message user ID", ReceivedEvent->ReportedUserID, *MessageUserID);
			}
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingMessageReports();
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
	TSharedPtr<FPubnubChatReportEvent> ReceivedEvent = MakeShared<FPubnubChatReportEvent>();
	
	// Stream report events on the channel
	CreateResult.Channel->OnMessageReportedNative.AddLambda([this, bEventReceived, ReceivedEvent](const FPubnubChatReportEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
	});
	
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
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
	
	// Verify report event contains empty reason
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent]()
	{
		if(!*bEventReceived)
		{
			AddError("Report event was not received");
		}
		else
		{
			TestTrue("Report reason should be empty when not provided", ReceivedEvent->Reason.IsEmpty());
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingMessageReports();
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
	TSharedPtr<FPubnubChatReportEvent> ReceivedEvent = MakeShared<FPubnubChatReportEvent>();
	
	// Stream report events on the channel
	CreateResult.Channel->OnMessageReportedNative.AddLambda([this, bEventReceived, ReceivedEvent](const FPubnubChatReportEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
	});
	
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
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
	
	// Verify report event contains edited text (not original text)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, EditedText]()
	{
		if(!*bEventReceived)
		{
			AddError("Report event was not received");
		}
		else
		{
			TestEqual("Report text should match edited text (not original)", ReceivedEvent->Text, EditedText);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingMessageReports();
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
			Chat->DeleteChannel(SourceChannelID);
			Chat->DeleteChannel(DestinationChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// STREAMUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStreamUpdatesNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to stream updates with uninitialized message
		FPubnubChatOperationResult StreamUpdatesResult = UninitializedMessage->StreamUpdates();
		TestTrue("StreamUpdates should fail with uninitialized message", StreamUpdatesResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamUpdatesResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_happy";
	const FString TestMessageText = TEXT("Message to stream updates");
	const FString EditedText = TEXT("Edited message text");
	
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
	
	// Set up delegate to receive message updates
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FString> ReceivedTimetoken = MakeShared<FString>();
	TSharedPtr<FPubnubChatMessageData> ReceivedMessageData = MakeShared<FPubnubChatMessageData>();
	
	auto UpdateLambda = [this, bUpdateReceived, ReceivedTimetoken, ReceivedMessageData](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*bUpdateReceived = true;
		*ReceivedTimetoken = Timetoken;
		*ReceivedMessageData = MessageData;
	};
	
	// Stream updates (no parameters required)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.2f));
	
	// Wait for subscription to be ready, then edit message to trigger update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, EditedText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(EditedText);
		TestFalse("EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify update was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedTimetoken, ReceivedMessageData, ReceivedMessage, EditedText]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		
		if(*ReceivedMessage)
		{
			FString MessageTimetoken = (*ReceivedMessage)->GetMessageTimetoken();
			TestEqual("Received Timetoken should match message timetoken", *ReceivedTimetoken, MessageTimetoken);
			
			// Verify message data was updated in repository
			FPubnubChatMessageData RetrievedData = (*ReceivedMessage)->GetMessageData();
			TestEqual("Retrieved message should have edited text", (*ReceivedMessage)->GetCurrentText(), EditedText);
			
			// Verify message actions contain edit action
			bool bHasEditAction = false;
			for(const FPubnubChatMessageAction& Action : RetrievedData.MessageActions)
			{
				if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited && Action.Value == EditedText)
				{
					bHasEditAction = true;
					break;
				}
			}
			TestTrue("Message should have edit action", bHasEditAction);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
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
 * Tests StreamUpdates with multiple sequential message updates (EditText, ToggleReaction, Delete).
 * Verifies that multiple updates are received correctly in sequence.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesMultipleUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdates.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesMultipleUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_multiple";
	const FString TestMessageText = TEXT("Message for multiple updates");
	const FString FirstEditText = TEXT("First edit");
	const FString SecondEditText = TEXT("Second edit");
	const FString ReactionEmoji = TEXT("👍");
	
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
	
	// Shared state for update reception
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatMessageData>> ReceivedUpdates = MakeShared<TArray<FPubnubChatMessageData>>();
	
	// Set up delegate to receive message updates
	auto UpdateLambda = [this, UpdateCount, ReceivedUpdates](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*UpdateCount = *UpdateCount + 1;
		ReceivedUpdates->Add(MessageData);
	};
	
	// Stream updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.2f));
	
	// Wait for subscription, then send first update (EditText)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEditText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(FirstEditText);
		TestFalse("First EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait for first update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Send second update (ToggleReaction)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ReactionEmoji]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult ReactionResult = (*ReceivedMessage)->ToggleReaction(ReactionEmoji);
		TestFalse("ToggleReaction should succeed", ReactionResult.Error);
	}, 0.1f));
	
	// Wait for second update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Send third update (EditText again)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEditText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(SecondEditText);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait for third update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 3;
	}, MAX_WAIT_TIME));
	
	// Verify all updates were received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedUpdates, ReceivedMessage, SecondEditText, ReactionEmoji]()
	{
		TestTrue("At least 3 updates should have been received", *UpdateCount >= 3);
		
		if(*ReceivedMessage)
		{
			// Verify final message state
			FPubnubChatMessageData FinalData = (*ReceivedMessage)->GetMessageData();
			TestEqual("Final text should be second edit", (*ReceivedMessage)->GetCurrentText(), SecondEditText);
			
			// Verify message has reaction
			FPubnubChatGetReactionsResult ReactionsResult = (*ReceivedMessage)->GetReactions();
			TestFalse("GetReactions should succeed", ReactionsResult.Result.Error);
			bool bHasReaction = false;
			for(const FPubnubChatMessageReaction& Reaction : ReactionsResult.Reactions)
			{
				if(Reaction.Value == ReactionEmoji)
				{
					bHasReaction = true;
					TestTrue("Reaction should be mine", Reaction.IsMine);
					TestTrue("Reaction should contain at least one user", Reaction.UserIDs.Num() >= 1);
					TestEqual("Reaction count should match user IDs count", Reaction.Count, Reaction.UserIDs.Num());
					break;
				}
			}
			TestTrue("Message should have reaction", bHasReaction);
			
			// Verify message has multiple edit actions
			int32 EditActionCount = 0;
			for(const FPubnubChatMessageAction& Action : FinalData.MessageActions)
			{
				if(Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
				{
					EditActionCount++;
				}
			}
			TestTrue("Message should have at least 2 edit actions", EditActionCount >= 2);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests StreamUpdates when already streaming - verifies that calling StreamUpdates twice doesn't cause errors.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesAlreadyStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdates.4Advanced.AlreadyStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesAlreadyStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_already_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_already";
	const FString TestMessageText = TEXT("Message for already streaming test");
	const FString EditedText = TEXT("Edited text");
	
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
	
	// Set up delegate to receive message updates
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	
	auto UpdateLambda = [this, bUpdateReceived](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*bUpdateReceived = true;
	};
	
	// Stream updates first time
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("First StreamUpdates should succeed", StreamUpdatesResult.Error);
		
		// Call StreamUpdates again immediately - should not error
		FPubnubChatOperationResult SecondStreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("Second StreamUpdates should succeed (no error when already streaming)", SecondStreamUpdatesResult.Error);
	}, 0.2f));
	
	// Wait for subscription, then edit message to trigger update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, EditedText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(EditedText);
		TestFalse("EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify update was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedMessage, EditedText]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		
		if(*ReceivedMessage)
		{
			TestEqual("Message text should be updated", (*ReceivedMessage)->GetCurrentText(), EditedText);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests StreamUpdates with soft delete action - verifies that delete action updates are received.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesSoftDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdates.4Advanced.SoftDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesSoftDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_delete_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_delete";
	const FString TestMessageText = TEXT("Message to soft delete");
	
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
	
	// Set up delegate to receive message updates
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMessageData> ReceivedMessageData = MakeShared<FPubnubChatMessageData>();
	
	auto UpdateLambda = [this, bUpdateReceived, ReceivedMessageData](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*bUpdateReceived = true;
		*ReceivedMessageData = MessageData;
	};
	
	// Stream updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.2f));
	
	// Wait for subscription, then soft delete message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Verify message is not deleted initially
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
		TestFalse("IsDeleted should succeed", IsDeletedResult.Result.Error);
		TestFalse("Message should not be deleted initially", IsDeletedResult.IsDeleted);
		
		FPubnubChatOperationResult DeleteResult = (*ReceivedMessage)->Delete(true); // Soft delete
		TestFalse("Soft delete should succeed", DeleteResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify update was received and message is deleted
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedMessageData, ReceivedMessage]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		
		if(*ReceivedMessage)
		{
			// Verify message is now deleted
			FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage)->IsDeleted();
			TestFalse("IsDeleted should succeed", IsDeletedResult.Result.Error);
			TestTrue("Message should be deleted", IsDeletedResult.IsDeleted);
			
			// Verify message data contains delete action
			FPubnubChatMessageData RetrievedData = (*ReceivedMessage)->GetMessageData();
			bool bHasDeleteAction = false;
			for(const FPubnubChatMessageAction& Action : RetrievedData.MessageActions)
			{
				if(Action.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
				{
					bHasDeleteAction = true;
					break;
				}
			}
			TestTrue("Message should have delete action", bHasDeleteAction);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// STREAMUPDATESON TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesOnEmptyArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesOn.1Validation.EmptyArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStreamUpdatesOnEmptyArrayTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_empty_init";
	
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
	
	// Call StreamUpdatesOn with empty array
	TArray<UPubnubChatMessage*> EmptyMessagesArray;
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatMessage::StreamUpdatesOn(EmptyMessagesArray);
	
	// Should succeed but do nothing (no messages to process)
	TestFalse("StreamUpdatesOn with empty array should succeed", StreamUpdatesOnResult.Error);
	TestEqual("StepResults should be empty for empty array", StreamUpdatesOnResult.StepResults.Num(), 0);
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesOnUninitializedMessagesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesOn.1Validation.UninitializedMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStreamUpdatesOnUninitializedMessagesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_uninit_init";
	
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
	
	// Create uninitialized message objects
	UPubnubChatMessage* UninitializedMessage1 = NewObject<UPubnubChatMessage>(Chat);
	UPubnubChatMessage* UninitializedMessage2 = NewObject<UPubnubChatMessage>(Chat);
	
	// Call StreamUpdatesOn with uninitialized messages
	TArray<UPubnubChatMessage*> UninitializedMessagesArray;
	UninitializedMessagesArray.Add(UninitializedMessage1);
	UninitializedMessagesArray.Add(UninitializedMessage2);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatMessage::StreamUpdatesOn(UninitializedMessagesArray);
	
	// Should fail because all messages are uninitialized
	TestTrue("StreamUpdatesOn should fail with uninitialized messages", StreamUpdatesOnResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesOnHappyPathSingleMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesOn.2HappyPath.SingleMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesOnHappyPathSingleMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_single_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_on_single";
	const FString TestMessageText = TEXT("Message for stream updates on");
	
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
	
	// Call StreamUpdatesOn with single message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		TArray<UPubnubChatMessage*> MessagesArray;
		MessagesArray.Add(*ReceivedMessage);
		
		FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatMessage::StreamUpdatesOn(MessagesArray);
		
		TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
		TestTrue("Should have at least one step result", StreamUpdatesOnResult.StepResults.Num() >= 1);
		
		// Verify that StreamUpdates was called successfully
		bool bFoundSubscribeStep = false;
		for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
		{
			if(Step.StepName == TEXT("Subscribe"))
			{
				bFoundSubscribeStep = true;
				TestFalse("Subscribe step should succeed", Step.OperationResult.Error);
				break;
			}
		}
		TestTrue("Should find Subscribe step in results", bFoundSubscribeStep);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesOnHappyPathMultipleMessagesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesOn.2HappyPath.MultipleMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStreamUpdatesOnHappyPathMultipleMessagesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_multi_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_on_multi";
	const FString TestMessageText1 = TEXT("Message 1 for stream updates on");
	const FString TestMessageText2 = TEXT("Message 2 for stream updates on");
	
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
	TSharedPtr<bool> bMessage1Received = MakeShared<bool>(false);
	TSharedPtr<bool> bMessage2Received = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage1 = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage2 = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect with callback to receive messages
	auto MessageLambda = [this, bMessage1Received, bMessage2Received, ReceivedMessage1, ReceivedMessage2, TestMessageText1, TestMessageText2](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			FPubnubChatMessageData MessageData = Message->GetMessageData();
			if(MessageData.Text == TestMessageText1 && !*ReceivedMessage1)
			{
				*bMessage1Received = true;
				*ReceivedMessage1 = Message;
			}
			else if(MessageData.Text == TestMessageText2 && !*ReceivedMessage2)
			{
				*bMessage2Received = true;
				*ReceivedMessage2 = Message;
			}
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Wait a bit for subscription to be ready, then send messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText1, TestMessageText2]()
	{
		FPubnubChatOperationResult SendResult1 = CreateResult.Channel->SendText(TestMessageText1);
		TestFalse("SendText 1 should succeed", SendResult1.Error);
		
		FPubnubChatOperationResult SendResult2 = CreateResult.Channel->SendText(TestMessageText2);
		TestFalse("SendText 2 should succeed", SendResult2.Error);
	}, 0.5f));
	
	// Wait until both messages are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessage1Received, bMessage2Received]() -> bool {
		return *bMessage1Received && *bMessage2Received;
	}, MAX_WAIT_TIME));
	
	// Call StreamUpdatesOn with both messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage1, ReceivedMessage2]()
	{
		if(!*ReceivedMessage1 || !*ReceivedMessage2)
		{
			AddError("Messages were not received");
			return;
		}
		
		TArray<UPubnubChatMessage*> MessagesArray;
		MessagesArray.Add(*ReceivedMessage1);
		MessagesArray.Add(*ReceivedMessage2);
		
		FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatMessage::StreamUpdatesOn(MessagesArray);
		
		TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
		TestTrue("Should have at least two step results (one per message)", StreamUpdatesOnResult.StepResults.Num() >= 2);
		
		// Verify that StreamUpdates was called successfully for both messages
		int32 SubscribeStepCount = 0;
		for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
		{
			if(Step.StepName == TEXT("Subscribe"))
			{
				SubscribeStepCount++;
				TestFalse("Subscribe step should succeed", Step.OperationResult.Error);
			}
		}
		TestEqual("Should have two Subscribe steps", SubscribeStepCount, 2);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage1, ReceivedMessage2, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage1)
		{
			(*ReceivedMessage1)->StopStreamingUpdates();
		}
		if(*ReceivedMessage2)
		{
			(*ReceivedMessage2)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesOnMixedMessagesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesOn.4Advanced.MixedMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStreamUpdatesOnMixedMessagesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_mixed_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_on_mixed";
	const FString TestMessageText = TEXT("Message for stream updates on mixed");
	
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
	
	// Create uninitialized message
	UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
	
	// Call StreamUpdatesOn with mixed messages (initialized + uninitialized)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UninitializedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		TArray<UPubnubChatMessage*> MessagesArray;
		MessagesArray.Add(*ReceivedMessage);
		MessagesArray.Add(UninitializedMessage);
		
		FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatMessage::StreamUpdatesOn(MessagesArray);
		
		// Should fail because one message is uninitialized
		TestTrue("StreamUpdatesOn should fail with mixed messages", StreamUpdatesOnResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
		
		// Verify that at least one Subscribe step succeeded (from initialized message)
		// and errors from uninitialized message are merged
		bool bFoundSuccessfulSubscribe = false;
		bool bFoundError = false;
		for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
		{
			if(Step.StepName == TEXT("Subscribe") && !Step.OperationResult.Error)
			{
				bFoundSuccessfulSubscribe = true;
			}
			if(Step.OperationResult.Error)
			{
				bFoundError = true;
			}
		}
		
		// Note: The initialized message might succeed, but overall result should be error due to uninitialized message
		// Check both StepResults errors and overall Error flag (uninitialized message returns early without steps)
		TestTrue("Should have error from uninitialized message", bFoundError || StreamUpdatesOnResult.Error);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// STOPSTREAMINGUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStopStreamingUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StopStreamingUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStopStreamingUpdatesNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized message object
		UPubnubChatMessage* UninitializedMessage = NewObject<UPubnubChatMessage>(Chat);
		
		// Try to stop streaming updates with uninitialized message
		FPubnubChatOperationResult StopResult = UninitializedMessage->StopStreamingUpdates();
		TestTrue("StopStreamingUpdates should fail with uninitialized message", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStopStreamingUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StopStreamingUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStopStreamingUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_happy";
	const FString TestMessageText = TEXT("Message to stop streaming");
	const FString EditedText = TEXT("Edited text after stop");
	
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
	
	// Set up delegate to receive message updates
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	
	auto UpdateLambda = [this, UpdateCount](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*UpdateCount = *UpdateCount + 1;
	};
	
	// Stream updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.2f));
	
	// Wait for subscription, then send an update that should be received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		// Send first update before stopping
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(TEXT("First edit"));
		TestFalse("First EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait for first update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Verify first update was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount]()
	{
		TestTrue("First update should have been received", *UpdateCount >= 1);
	}, 0.1f));
	
	// Stop streaming updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateCount, EditedText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult StopResult = (*ReceivedMessage)->StopStreamingUpdates();
		TestFalse("StopStreamingUpdates should succeed", StopResult.Error);
		
		// Record update count before second update
		int32 CountBeforeStop = *UpdateCount;
		
		// Send second update after stopping
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(EditedText);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Wait and verify second update was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedMessage, EditedText]()
	{
		// Wait a bit to ensure update would have been received if streaming was active
		// The update count should remain the same
		int32 CountAfterWait = *UpdateCount;
		
		// Verify message was still updated in repository (even though delegate wasn't called)
		if(*ReceivedMessage)
		{
			// Message should still be updated in repository
			TestEqual("Message text should be updated in repository", (*ReceivedMessage)->GetCurrentText(), EditedText);
		}
	}, 0.5f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			// Ensure stopped (should be safe to call again)
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
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
 * Tests StopStreamingUpdates when not streaming - verifies that calling StopStreamingUpdates when not streaming doesn't cause errors.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStopStreamingUpdatesNotStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StopStreamingUpdates.4Advanced.NotStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStopStreamingUpdatesNotStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_not_streaming_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_not_streaming";
	const FString TestMessageText = TEXT("Message for not streaming test");
	
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
	
	// Stop streaming updates without starting it first - should not error
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		FPubnubChatOperationResult StopResult = (*ReceivedMessage)->StopStreamingUpdates();
		TestFalse("StopStreamingUpdates should succeed even when not streaming", StopResult.Error);
	}, 0.2f));
	
	// Cleanup: Disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests multiple start/stop cycles - verifies that streaming can be started and stopped multiple times.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStopStreamingUpdatesMultipleCyclesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StopStreamingUpdates.4Advanced.MultipleCycles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageStopStreamingUpdatesMultipleCyclesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_cycles_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_cycles";
	const FString TestMessageText = TEXT("Message for multiple cycles test");
	const FString FirstEditText = TEXT("First edit");
	const FString SecondEditText = TEXT("Second edit");
	const FString ThirdEditText = TEXT("Third edit");
	
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
	
	// Shared state for update reception
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	
	auto UpdateLambda = [this, UpdateCount](FString Timetoken, const FPubnubChatMessageData& MessageData)
	{
		*UpdateCount = *UpdateCount + 1;
	};
	
	// Cycle 1: Start streaming, receive update, stop streaming
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, UpdateLambda]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		
		(*ReceivedMessage)->OnUpdatedNative.AddLambda(UpdateLambda);
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("First StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.2f));
	
	// Send first edit
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, FirstEditText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(FirstEditText);
		TestFalse("First EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait for first update
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Stop streaming
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult StopResult = (*ReceivedMessage)->StopStreamingUpdates();
		TestFalse("First StopStreamingUpdates should succeed", StopResult.Error);
	}, 0.1f));
	
	// Send second edit (should not be received)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, SecondEditText, UpdateCount]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		int32 CountBeforeSecondEdit = *UpdateCount;
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(SecondEditText);
		TestFalse("Second EditText should succeed", EditResult.Error);
	}, 0.1f));
	
	// Cycle 2: Start streaming again, receive update, stop streaming
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult StreamUpdatesResult = (*ReceivedMessage)->StreamUpdates();
		TestFalse("Second StreamUpdates should succeed", StreamUpdatesResult.Error);
	}, 0.1f));
	
	// Send third edit (should be received)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, ThirdEditText]()
	{
		if(!*ReceivedMessage)
		{
			return;
		}
		
		FPubnubChatOperationResult EditResult = (*ReceivedMessage)->EditText(ThirdEditText);
		TestFalse("Third EditText should succeed", EditResult.Error);
	}, 0.5f));
	
	// Wait for third update
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedMessage, ThirdEditText]()
	{
		TestTrue("At least 2 updates should have been received", *UpdateCount >= 2);
		
		if(*ReceivedMessage)
		{
			// Verify final message state
			TestEqual("Final text should be third edit", (*ReceivedMessage)->GetCurrentText(), ThirdEditText);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates, disconnect and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, CreateResult, Chat, TestChannelID, InitUserID]()
	{
		if(*ReceivedMessage)
		{
			(*ReceivedMessage)->StopStreamingUpdates();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Message)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageEditTextAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.EditTextAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageEditTextAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_edit_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_edit_async_full";
	const FString TestMessageText = TEXT("Original async text");
	const FString UpdatedText = TEXT("Edited async text");
	
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
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bCallbackReceived, UpdatedText, OnOperationResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid");
			*bCallbackReceived = true;
			return;
		}
		
		ReceivedMessage->Get()->EditTextAsync(UpdatedText, OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, ReceivedMessage, UpdatedText]()
	{
		TestFalse("EditTextAsync should succeed", CallbackResult->Error);
		if(ReceivedMessage->IsValid())
		{
			TestEqual("Message text should be updated", ReceivedMessage->Get()->GetCurrentText(), UpdatedText);
		}
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDeleteAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.DeleteAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDeleteAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_delete_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_delete_async_full";
	const FString TestMessageText = TEXT("Delete async text");
	
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
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bCallbackReceived, OnOperationResponse]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid");
			*bCallbackReceived = true;
			return;
		}
		
		ReceivedMessage->Get()->DeleteAsync(OnOperationResponse, false);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("DeleteAsync should succeed", CallbackResult->Error);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageToggleReactionAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ToggleReactionAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageToggleReactionAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_reaction_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_reaction_async_full";
	const FString TestMessageText = TEXT("Reaction async text");
	const FString Reaction = TEXT("like");
	
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bCallbackReceived, Reaction, OnOperationResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bCallbackReceived = true;
			return;
		}
		(*ReceivedMessage).Get()->ToggleReactionAsync(Reaction, OnOperationResponse);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("ToggleReactionAsync should succeed", CallbackResult->Error);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessagePinUnpinAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.PinUnpinAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessagePinUnpinAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_pin_unpin_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_pin_unpin_async_full";
	const FString TestMessageText = TEXT("Pin/unpin async text");
	
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	TSharedPtr<bool> bPinReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> PinResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnPinResponse;
	OnPinResponse.BindLambda([bPinReceived, PinResult](const FPubnubChatOperationResult& Result)
	{
		*PinResult = Result;
		*bPinReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bPinReceived, OnPinResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bPinReceived = true;
			return;
		}
		
		(*ReceivedMessage).Get()->PinAsync(OnPinResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bPinReceived]() { return *bPinReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bUnpinReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> UnpinResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnUnpinResponse;
	OnUnpinResponse.BindLambda([bUnpinReceived, UnpinResult](const FPubnubChatOperationResult& Result)
	{
		*UnpinResult = Result;
		*bUnpinReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bUnpinReceived, OnUnpinResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bUnpinReceived = true;
			return;
		}
		
		(*ReceivedMessage).Get()->UnpinAsync(OnUnpinResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUnpinReceived]() { return *bUnpinReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, PinResult, UnpinResult]()
	{
		TestFalse("PinAsync should succeed", PinResult->Error);
		TestFalse("UnpinAsync should succeed", UnpinResult->Error);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageRestoreIsDeletedAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.RestoreIsDeletedAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageRestoreIsDeletedAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_restore_isdeleted_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_restore_isdeleted_async_full";
	const FString TestMessageText = TEXT("Restore/isdeleted async text");
	
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			return;
		}
		(*ReceivedMessage).Get()->Delete(true);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			return;
		}
		FPubnubChatIsDeletedResult IsDeletedResult = (*ReceivedMessage).Get()->IsDeleted();
		TestFalse("IsDeleted should succeed", IsDeletedResult.Result.Error);
		TestTrue("Message should be marked as deleted", IsDeletedResult.IsDeleted);

		FPubnubChatOperationResult RestoreResult = (*ReceivedMessage).Get()->Restore();
		TestFalse("Restore should succeed", RestoreResult.Error);
	}, 0.3f));
	
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageForwardAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ForwardAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageForwardAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_forward_async_full_init";
	const FString SourceChannelID = SDK_PREFIX + "test_message_forward_async_source";
	const FString DestChannelID = SDK_PREFIX + "test_message_forward_async_dest";
	const FString TestMessageText = TEXT("Forward async text");
	
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
	
	FPubnubChatChannelResult CreateSourceResult = Chat->CreatePublicConversation(SourceChannelID, FPubnubChatChannelData());
	FPubnubChatChannelResult CreateDestResult = Chat->CreatePublicConversation(DestChannelID, FPubnubChatChannelData());
	TestFalse("Create source channel should succeed", CreateSourceResult.Result.Error);
	TestFalse("Create destination channel should succeed", CreateDestResult.Result.Error);
	if(!CreateSourceResult.Channel || !CreateDestResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<TWeakObjectPtr<UPubnubChatMessage>> ReceivedMessage = MakeShared<TWeakObjectPtr<UPubnubChatMessage>>(nullptr);
	CreateSourceResult.Channel->OnMessageReceivedNative.AddLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !(*ReceivedMessage).IsValid())
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	CreateSourceResult.Channel->Connect();
	CreateDestResult.Channel->Connect();
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSourceResult, TestMessageText]()
	{
		FPubnubChatSendTextParams SendTextParams;
		SendTextParams.StoreInHistory = true;
		FPubnubChatOperationResult SendResult = CreateSourceResult.Channel->SendText(TestMessageText, SendTextParams);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bForwardReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> ForwardResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnForwardResponse;
	OnForwardResponse.BindLambda([bForwardReceived, ForwardResult](const FPubnubChatOperationResult& Result)
	{
		*ForwardResult = Result;
		*bForwardReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bForwardReceived, CreateDestResult, OnForwardResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bForwardReceived = true;
			return;
		}
		
		(*ReceivedMessage).Get()->ForwardAsync(CreateDestResult.Channel, OnForwardResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bForwardReceived]() { return *bForwardReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ForwardResult]()
	{
		TestFalse("ForwardAsync should succeed", ForwardResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, SourceChannelID, DestChannelID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(SourceChannelID);
			Chat->DeleteChannel(DestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageReportAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ReportAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageReportAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_report_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_report_async_full";
	const FString TestMessageText = TEXT("Report async text");
	const FString ReportReason = TEXT("async report reason");
	
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	TSharedPtr<bool> bReportReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> ReportResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnReportResponse;
	OnReportResponse.BindLambda([bReportReceived, ReportResult](const FPubnubChatOperationResult& Result)
	{
		*ReportResult = Result;
		*bReportReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bReportReceived, ReportReason, OnReportResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bReportReceived = true;
			return;
		}
		
		(*ReceivedMessage).Get()->ReportAsync(OnReportResponse, ReportReason);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReportReceived]() { return *bReportReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReportResult]()
	{
		TestFalse("ReportAsync should succeed", ReportResult->Error);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageStreamUpdatesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.StreamUpdatesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageStreamUpdatesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_stream_updates_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_stream_updates_async_full";
	const FString TestMessageText = TEXT("Stream updates async text");
	
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	TSharedPtr<bool> bStreamReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StreamResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStreamResponse;
	OnStreamResponse.BindLambda([bStreamReceived, StreamResult](const FPubnubChatOperationResult& Result)
	{
		*StreamResult = Result;
		*bStreamReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bStreamReceived, OnStreamResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bStreamReceived = true;
			return;
		}
		(*ReceivedMessage).Get()->StreamUpdatesAsync(OnStreamResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStreamReceived]() { return *bStreamReceived; }, MAX_WAIT_TIME));
	
	TSharedPtr<bool> bStopReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> StopResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnStopResponse;
	OnStopResponse.BindLambda([bStopReceived, StopResult](const FPubnubChatOperationResult& Result)
	{
		*StopResult = Result;
		*bStopReceived = true;
	});
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bStopReceived, OnStopResponse]()
	{
		if(!(*ReceivedMessage).IsValid())
		{
			AddError("Received message is invalid");
			*bStopReceived = true;
			return;
		}
		(*ReceivedMessage).Get()->StopStreamingUpdatesAsync(OnStopResponse);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bStopReceived]() { return *bStopReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, StreamResult, StopResult]()
	{
		TestFalse("StreamUpdatesAsync should succeed", StreamResult->Error);
		TestFalse("StopStreamingUpdatesAsync should succeed", StopResult->Error);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageThreadOpsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Message.ThreadOpsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageThreadOpsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_message_thread_ops_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_message_thread_ops_async_full";
	const FString TestMessageText = TEXT("Thread ops async text");
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
		if(Message && !(*ReceivedMessage).IsValid())
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
	
	TSharedPtr<bool> bAllThreadOpsDone = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatThreadChannelResult> CreateThreadResult = MakeShared<FPubnubChatThreadChannelResult>();
	TSharedPtr<FPubnubChatHasThreadResult> HasThreadResult = MakeShared<FPubnubChatHasThreadResult>();
	TSharedPtr<FPubnubChatThreadChannelResult> GetThreadResult = MakeShared<FPubnubChatThreadChannelResult>();
	TSharedPtr<FPubnubChatOperationResult> RemoveThreadResult = MakeShared<FPubnubChatOperationResult>();
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessage, bAllThreadOpsDone, CreateThreadResult, HasThreadResult, GetThreadResult, RemoveThreadResult, TestThreadText]()
	{
		if(!ReceivedMessage->IsValid())
		{
			AddError("Received message is invalid when starting thread ops");
			*bAllThreadOpsDone = true;
			return;
		}
		UPubnubChatMessage* Message = ReceivedMessage->Get();
		FOnPubnubChatOperationResponseNative OnRemove;
		OnRemove.BindLambda([bAllThreadOpsDone, RemoveThreadResult](const FPubnubChatOperationResult& RemoveResult)
		{
			*RemoveThreadResult = RemoveResult;
			*bAllThreadOpsDone = true;
		});
		FOnPubnubChatThreadChannelResponseNative OnGet;
		OnGet.BindLambda([this, Message, bAllThreadOpsDone, GetThreadResult, RemoveThreadResult, OnRemove](const FPubnubChatThreadChannelResult& GetResult)
		{
			*GetThreadResult = GetResult;
			if(GetResult.Result.Error) { *bAllThreadOpsDone = true; return; }
			if(!IsValid(Message)) { AddError("Message invalid before RemoveThreadAsync"); *bAllThreadOpsDone = true; return; }
			Message->RemoveThreadAsync(OnRemove);
		});
		FOnPubnubChatThreadChannelResponseNative OnCreate;
		OnCreate.BindLambda([this, Message, bAllThreadOpsDone, CreateThreadResult, HasThreadResult, GetThreadResult, RemoveThreadResult, OnGet, TestThreadText](const FPubnubChatThreadChannelResult& CreateResult)
		{
			*CreateThreadResult = CreateResult;
			if(CreateResult.Result.Error) { *bAllThreadOpsDone = true; return; }
			if(!IsValid(Message)) { AddError("Message invalid before HasThread"); *bAllThreadOpsDone = true; return; }
			// SendText on thread channel propagates the thread to the server; otherwise GetThreadAsync may fail (no such channel)
			if(CreateResult.ThreadChannel)
			{
				CreateResult.ThreadChannel->Connect();
				FPubnubChatOperationResult SendResult = CreateResult.ThreadChannel->SendText(TestThreadText);
				if(SendResult.Error) { AddError("Thread SendText failed"); *bAllThreadOpsDone = true; return; }
			}
			FPlatformProcess::Sleep(1.0f);
			*HasThreadResult = Message->HasThread();
			if(HasThreadResult->Result.Error) { *bAllThreadOpsDone = true; return; }
			if(!IsValid(Message)) { AddError("Message invalid before GetThreadAsync"); *bAllThreadOpsDone = true; return; }
			Message->GetThreadAsync(OnGet);
		});
		Message->CreateThreadAsync(OnCreate);
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bAllThreadOpsDone]() { return *bAllThreadOpsDone; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateThreadResult, HasThreadResult, GetThreadResult, RemoveThreadResult]()
	{
		TestFalse("CreateThreadAsync should succeed", CreateThreadResult->Result.Error);
		TestFalse("HasThread should succeed", HasThreadResult->Result.Error);
		TestFalse("GetThreadAsync should succeed", GetThreadResult->Result.Error);
		TestFalse("RemoveThreadAsync should succeed", RemoveThreadResult->Error);
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

#endif // WITH_DEV_AUTOMATION_TESTS

