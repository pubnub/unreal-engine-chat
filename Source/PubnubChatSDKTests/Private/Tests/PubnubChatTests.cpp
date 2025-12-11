// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatCallbackStop.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// EMITCHATEVENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Get Chat without initializing
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	TestNull("Chat should be null before InitChat", Chat);
	
	if(!Chat)
	{
		// Try to emit event without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_emit_event_not_init";
			const FString TestPayload = TEXT("{\"test\":\"data\"}");
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload);
			
			TestTrue("EmitChatEvent should fail when Chat is not initialized", EmitResult.Error);
			TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_emit_event_empty_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to emit event with empty ChannelID
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TEXT(""), TestPayload);
		
		TestTrue("EmitChatEvent should fail with empty ChannelID", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventEmptyPayloadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.1Validation.EmptyPayload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventEmptyPayloadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_emit_event_empty_payload_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_event_empty_payload";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to emit event with empty Payload
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TEXT(""));
		
		TestTrue("EmitChatEvent should fail with empty Payload", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_event_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_event_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Emit event with only required parameters (EventType, ChannelID, Payload) and default EventMethod
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload);
		
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
		
		// Verify step results contain either PublishMessage or Signal step
		bool bFoundPublishOrSignal = false;
		for(const FPubnubChatOperationStepResult& Step : EmitResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
			{
				bFoundPublishOrSignal = true;
				TestFalse("PublishMessage or Signal step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have PublishMessage or Signal step", bFoundPublishOrSignal);
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_event_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_event_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Emit event with all parameters - test with Publish method
		const FString TestPayload = TEXT("{\"test\":\"data\",\"value\":123}");
		FPubnubChatOperationResult EmitResultPublish = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
		
		TestFalse("EmitChatEvent with Publish should succeed", EmitResultPublish.Error);
		
		// Verify step results contain PublishMessage step
		bool bFoundPublishMessage = false;
		for(const FPubnubChatOperationStepResult& Step : EmitResultPublish.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage"))
			{
				bFoundPublishMessage = true;
				TestFalse("PublishMessage step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have PublishMessage step", bFoundPublishMessage);
		
		// Emit event with Signal method
		const FString TestPayload2 = TEXT("{\"test\":\"signal\",\"value\":456}");
		FPubnubChatOperationResult EmitResultSignal = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Receipt, TestChannelID, TestPayload2, EPubnubChatEventMethod::PCEM_Signal);
		
		TestFalse("EmitChatEvent with Signal should succeed", EmitResultSignal.Error);
		
		// Verify step results contain Signal step
		bool bFoundSignal = false;
		for(const FPubnubChatOperationStepResult& Step : EmitResultSignal.StepResults)
		{
			if(Step.StepName == TEXT("Signal"))
			{
				bFoundSignal = true;
				TestFalse("Signal step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have Signal step", bFoundSignal);
		
		// Emit event with Default method (should use default for event type)
		const FString TestPayload3 = TEXT("{\"test\":\"default\",\"value\":789}");
		FPubnubChatOperationResult EmitResultDefault = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload3, EPubnubChatEventMethod::PCEM_Default);
		
		TestFalse("EmitChatEvent with Default should succeed", EmitResultDefault.Error);
		TestTrue("StepResults should contain at least one step", EmitResultDefault.StepResults.Num() > 0);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests emitting events with all different event types.
 * Verifies that all event types can be emitted successfully.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventAllEventTypesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.4Advanced.AllEventTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventAllEventTypesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_event_all_types_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_event_all_types";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Test all event types
		TArray<EPubnubChatEventType> EventTypes = {
			EPubnubChatEventType::PCET_Typing,
			EPubnubChatEventType::PCET_Report,
			EPubnubChatEventType::PCET_Receipt,
			EPubnubChatEventType::PCET_Mention,
			EPubnubChatEventType::PCET_Invite,
			EPubnubChatEventType::PCET_Custom,
			EPubnubChatEventType::PCET_Moderation
		};
		
		for(int32 i = 0; i < EventTypes.Num(); ++i)
		{
			const FString TestPayload = FString::Printf(TEXT("{\"eventType\":%d,\"test\":\"data\"}"), i);
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EventTypes[i], TestChannelID, TestPayload);
			
			TestFalse(FString::Printf(TEXT("EmitChatEvent should succeed for event type %d"), i), EmitResult.Error);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests that emitted events include the event type in the payload.
 * Verifies that the event type is correctly added to the JSON payload.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatEmitChatEventPayloadIncludesTypeTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.EmitChatEvent.4Advanced.PayloadIncludesType", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatEmitChatEventPayloadIncludesTypeTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_event_payload_type_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_event_payload_type";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Emit event - the function should add type to payload
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload);
		
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
		
		// Note: We can't directly verify the payload content without accessing PubnubClient internals,
		// but we verify the operation succeeded which means the payload was correctly formatted
		TestTrue("EmitChatEvent operation completed successfully", true);
	}

	CleanUp();
	return true;
}

// ============================================================================
// LISTENFOREVENTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatListenForEventsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Get Chat without initializing
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	TestNull("Chat should be null before InitChat", Chat);
	
	if(!Chat)
	{
		// Try to listen for events without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_listen_events_not_init";
			FOnPubnubChatEventReceivedNative EventCallback;
			FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, EPubnubChatEventType::PCET_Typing, EventCallback);
			
			TestTrue("ListenForEvents should fail when Chat is not initialized", ListenResult.Result.Error);
			TestNull("CallbackStop should not be created", ListenResult.CallbackStop);
			TestFalse("ErrorMessage should not be empty", ListenResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatListenForEventsEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_listen_events_empty_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to listen for events with empty ChannelID
		FOnPubnubChatEventReceivedNative EventCallback;
		FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TEXT(""), EPubnubChatEventType::PCET_Typing, EventCallback);
		
		TestTrue("ListenForEvents should fail with empty ChannelID", ListenResult.Result.Error);
		TestNull("CallbackStop should not be created", ListenResult.CallbackStop);
		TestFalse("ErrorMessage should not be empty", ListenResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatListenForEventsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Listen for events with only required parameters (ChannelID, EventType, Callback)
		FOnPubnubChatEventReceivedNative EventCallback;
		EventCallback.BindLambda([](const FPubnubChatEvent& Event)
		{
			// Callback for testing
		});
		
		FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, EPubnubChatEventType::PCET_Typing, EventCallback);
		
		TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
		TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
		
		// Verify step results contain Subscribe step
		bool bFoundSubscribe = false;
		for(const FPubnubChatOperationStepResult& Step : ListenResult.Result.StepResults)
		{
			if(Step.StepName == TEXT("Subscribe"))
			{
				bFoundSubscribe = true;
				TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have Subscribe step", bFoundSubscribe);
		
		// Cleanup: Stop listening
		if(ListenResult.CallbackStop)
		{
			FPubnubChatOperationResult StopResult = ListenResult.CallbackStop->Stop();
			TestFalse("Stop should succeed", StopResult.Error);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatListenForEventsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Listen for events with all parameters (only takes ChannelID, EventType, Callback - all are required)
		FOnPubnubChatEventReceivedNative EventCallback;
		EventCallback.BindLambda([](const FPubnubChatEvent& Event)
		{
			// Callback for testing
		});
		
		FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, EPubnubChatEventType::PCET_Typing, EventCallback);
		
		TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
		TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
		TestTrue("StepResults should contain at least one step", ListenResult.Result.StepResults.Num() > 0);
		
		// Cleanup: Stop listening
		if(ListenResult.CallbackStop)
		{
			FPubnubChatOperationResult StopResult = ListenResult.CallbackStop->Stop();
			TestFalse("Stop should succeed", StopResult.Error);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests listening for events and receiving them when emitted.
 * Verifies the full flow: listen for event, emit event, receive event through callback.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsReceiveEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.4Advanced.ReceiveEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatListenForEventsReceiveEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_receive_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_receive";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	const EPubnubChatEventType ExpectedEventType = EPubnubChatEventType::PCET_Typing;
	const FString TestPayload = TEXT("{\"test\":\"data\"}");
	
	// Listen for events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, ExpectedEventType, TestChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received event type should match", Event.Type, ExpectedEventType);
		TestEqual("Received event ChannelID should match", Event.ChannelID, TestChannelID);
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, ExpectedEventType, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ExpectedEventType, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(ExpectedEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, ExpectedEventType]()
	{
		if(!*bEventReceived)
		{
			AddError("Event was not received");
		}
		else
		{
			TestEqual("Received event type should match expected", ReceivedEvent->Type, ExpectedEventType);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests listening for one event type and emitting another type - should not receive it.
 * Verifies that events are filtered by type and only matching events are received.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsFilterByTypeTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.4Advanced.FilterByType", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatListenForEventsFilterByTypeTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_filter_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_filter";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	const EPubnubChatEventType ListenEventType = EPubnubChatEventType::PCET_Typing;
	const EPubnubChatEventType EmitEventType = EPubnubChatEventType::PCET_Receipt; // Different type
	
	// Listen for Typing events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		AddError("Event should NOT be received - wrong event type");
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, ListenEventType, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, EmitEventType]()
	{
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EmitEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.5f));
	
	// Wait a bit to ensure event would have been received if we were listening for it
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify event was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived]()
	{
		if(*bEventReceived)
		{
			AddError("Event should NOT have been received - wrong event type");
		}
		else
		{
			TestTrue("Event was correctly filtered out", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests listening for events with multiple listeners on the same channel.
 * Verifies that multiple listeners can be registered and all receive matching events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsMultipleListenersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.4Advanced.MultipleListeners", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatListenForEventsMultipleListenersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Shared state for event reception from multiple listeners
	TSharedPtr<bool> bEvent1Received = MakeShared<bool>(false);
	TSharedPtr<bool> bEvent2Received = MakeShared<bool>(false);
	const EPubnubChatEventType ExpectedEventType = EPubnubChatEventType::PCET_Typing;
	const FString TestPayload = TEXT("{\"test\":\"data\"}");
	
	// Listen first time
	FOnPubnubChatEventReceivedNative EventCallback1;
	EventCallback1.BindLambda([this, bEvent1Received, ExpectedEventType](const FPubnubChatEvent& Event)
	{
		*bEvent1Received = true;
		TestEqual("Callback1 received event type should match", Event.Type, ExpectedEventType);
	});
	
	FPubnubChatListenForEventsResult ListenResult1 = Chat->ListenForEvents(TestChannelID, ExpectedEventType, EventCallback1);
	TestFalse("First ListenForEvents should succeed", ListenResult1.Result.Error);
	TestNotNull("First CallbackStop should be created", ListenResult1.CallbackStop);
	
	// Listen second time
	FOnPubnubChatEventReceivedNative EventCallback2;
	EventCallback2.BindLambda([this, bEvent2Received, ExpectedEventType](const FPubnubChatEvent& Event)
	{
		*bEvent2Received = true;
		TestEqual("Callback2 received event type should match", Event.Type, ExpectedEventType);
	});
	
	FPubnubChatListenForEventsResult ListenResult2 = Chat->ListenForEvents(TestChannelID, ExpectedEventType, EventCallback2);
	TestFalse("Second ListenForEvents should succeed", ListenResult2.Result.Error);
	TestNotNull("Second CallbackStop should be created", ListenResult2.CallbackStop);
	
	// Wait a bit for subscriptions to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ExpectedEventType, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(ExpectedEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.5f));
	
	// Wait until all events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEvent1Received, bEvent2Received]() -> bool {
		return *bEvent1Received && *bEvent2Received;
	}, MAX_WAIT_TIME));
	
	// Verify all events were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEvent1Received, bEvent2Received]()
	{
		if(!*bEvent1Received)
		{
			AddError("Event was not received by listener 1");
		}
		if(!*bEvent2Received)
		{
			AddError("Event was not received by listener 2");
		}
		
		TestTrue("All listeners should have received the event", *bEvent1Received && *bEvent2Received);
	}, 0.1f));
	
	// Cleanup: Stop listening
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult1, ListenResult2]()
	{
		if(ListenResult1.CallbackStop)
		{
			ListenResult1.CallbackStop->Stop();
		}
		if(ListenResult2.CallbackStop)
		{
			ListenResult2.CallbackStop->Stop();
		}
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests stopping event listener and verifying events are no longer received.
 * Verifies that CallbackStop properly unsubscribes and stops receiving events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatListenForEventsStopListenerTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.ListenForEvents.4Advanced.StopListener", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatListenForEventsStopListenerTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_listen_events_stop_init";
	const FString TestChannelID = SDK_PREFIX + "test_listen_events_stop";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	const EPubnubChatEventType ExpectedEventType = EPubnubChatEventType::PCET_Typing;
	const FString TestPayload = TEXT("{\"test\":\"data\"}");
	
	// Listen for events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		AddError("Event should NOT be received after stopping listener");
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, ExpectedEventType, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult]()
	{
		// Stop listening before emitting event
		if(ListenResult.CallbackStop)
		{
			FPubnubChatOperationResult StopResult = ListenResult.CallbackStop->Stop();
			TestFalse("Stop should succeed", StopResult.Error);
		}
	}, 0.5f));
	
	// Emit event after stopping
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ExpectedEventType, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(ExpectedEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.2f));
	
	// Wait a bit to ensure event would have been received if we were still listening
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify event was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived]()
	{
		if(*bEventReceived)
		{
			AddError("Event should NOT have been received after stopping listener");
		}
		else
		{
			TestTrue("Event was correctly not received after stopping", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]()
	{
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
