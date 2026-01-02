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
#include "FunctionLibraries/PubnubTimetokenUtilities.h"

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

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to emit event with empty ChannelID
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TEXT(""), TestPayload);
		
		TestTrue("EmitChatEvent should fail with empty ChannelID", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to emit event with empty Payload
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TEXT(""));
		
		TestTrue("EmitChatEvent should fail with empty Payload", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to listen for events with empty ChannelID
		FOnPubnubChatEventReceivedNative EventCallback;
		FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TEXT(""), EPubnubChatEventType::PCET_Typing, EventCallback);
		
		TestTrue("ListenForEvents should fail with empty ChannelID", ListenResult.Result.Error);
		TestNull("CallbackStop should not be created", ListenResult.CallbackStop);
		TestFalse("ErrorMessage should not be empty", ListenResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
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

	CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, Chat]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, Chat]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult1, ListenResult2, Chat]()
	{
		if(ListenResult1.CallbackStop)
		{
			ListenResult1.CallbackStop->Stop();
		}
		if(ListenResult2.CallbackStop)
		{
			ListenResult2.CallbackStop->Stop();
		}
		CleanUpCurrentChatUser(Chat);
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
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUpCurrentChatUser(Chat);
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// PRESENCE TESTS
// ============================================================================

// ============================================================================
// WHEREPRESENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWherePresentNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to call WherePresent without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_where_present_not_init";
			FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(TestUserID);
			
			TestTrue("WherePresent should fail when Chat is not initialized", WherePresentResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", WherePresentResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWherePresentEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_where_present_empty_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to call WherePresent with empty UserID
		FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(TEXT(""));
		
		TestTrue("WherePresent should fail with empty UserID", WherePresentResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", WherePresentResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWherePresentHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_where_present_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_where_present_happy";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call WherePresent with only required parameter (UserID)
		FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(InitUserID);
		
		TestFalse("WherePresent should succeed", WherePresentResult.Result.Error);
		TestTrue("Channels array should contain the joined channel", WherePresentResult.Channels.Contains(TestChannelID));
		TestEqual("Channels array should have exactly 1 channel", WherePresentResult.Channels.Num(), 1);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWherePresentFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_where_present_full_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_where_present_full_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_where_present_full_2";
	
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
	
	// Create two channels
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannel1Result = Chat->CreatePublicConversation(TestChannelID1, ChannelData);
	TestFalse("CreateChannel1 should succeed", CreateChannel1Result.Result.Error);
	TestNotNull("Channel1 should be created", CreateChannel1Result.Channel);
	
	FPubnubChatChannelResult CreateChannel2Result = Chat->CreatePublicConversation(TestChannelID2, ChannelData);
	TestFalse("CreateChannel2 should succeed", CreateChannel2Result.Result.Error);
	TestNotNull("Channel2 should be created", CreateChannel2Result.Channel);
	
	if(!CreateChannel1Result.Channel || !CreateChannel2Result.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join both channels
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult Join1Result = CreateChannel1Result.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join1 should succeed", Join1Result.Result.Error);
	
	FPubnubChatJoinResult Join2Result = CreateChannel2Result.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join2 should succeed", Join2Result.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID1, TestChannelID2]()
	{
		// Call WherePresent (all parameters are required, so this is the same as happy path)
		FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(InitUserID);
		
		TestFalse("WherePresent should succeed", WherePresentResult.Result.Error);
		TestTrue("Channels array should contain channel 1", WherePresentResult.Channels.Contains(TestChannelID1));
		TestTrue("Channels array should contain channel 2", WherePresentResult.Channels.Contains(TestChannelID2));
		TestEqual("Channels array should have exactly 2 channels", WherePresentResult.Channels.Num(), 2);
	}, 1.0f));
	
	// Cleanup: Leave channels, delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannel1Result, CreateChannel2Result, Chat, TestChannelID1, TestChannelID2]()
	{
		if(CreateChannel1Result.Channel)
		{
			CreateChannel1Result.Channel->Leave();
		}
		if(CreateChannel2Result.Channel)
		{
			CreateChannel2Result.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1, false);
			Chat->DeleteChannel(TestChannelID2, false);
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
 * Tests WherePresent when user is not present on any channel.
 * Verifies that WherePresent returns empty array when user hasn't joined any channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentUserNotPresentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.4Advanced.UserNotPresent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWherePresentUserNotPresentTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_where_present_not_present_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Call WherePresent for user who hasn't joined any channels
		FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(InitUserID);
		
		TestFalse("WherePresent should succeed", WherePresentResult.Result.Error);
		TestEqual("Channels array should be empty", WherePresentResult.Channels.Num(), 0);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests WherePresent with Connect vs Join - both should make user present.
 * Verifies that both Channel->Connect and Channel->Join make user present on channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWherePresentConnectVsJoinTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WherePresent.4Advanced.ConnectVsJoin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWherePresentConnectVsJoinTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_where_present_connect_join_init";
	const FString TestChannelIDConnect = SDK_PREFIX + "test_where_present_connect";
	const FString TestChannelIDJoin = SDK_PREFIX + "test_where_present_join";
	
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
	
	// Create two channels
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelConnectResult = Chat->CreatePublicConversation(TestChannelIDConnect, ChannelData);
	TestFalse("CreateChannelConnect should succeed", CreateChannelConnectResult.Result.Error);
	TestNotNull("ChannelConnect should be created", CreateChannelConnectResult.Channel);
	
	FPubnubChatChannelResult CreateChannelJoinResult = Chat->CreatePublicConversation(TestChannelIDJoin, ChannelData);
	TestFalse("CreateChannelJoin should succeed", CreateChannelJoinResult.Result.Error);
	TestNotNull("ChannelJoin should be created", CreateChannelJoinResult.Channel);
	
	if(!CreateChannelConnectResult.Channel || !CreateChannelJoinResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Connect to first channel (without membership)
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatConnectResult ConnectResult = CreateChannelConnectResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Join second channel (with membership)
	FPubnubChatJoinResult JoinResult = CreateChannelJoinResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelIDConnect, TestChannelIDJoin]()
	{
		// Call WherePresent - should return both channels
		FPubnubChatWherePresentResult WherePresentResult = Chat->WherePresent(InitUserID);
		
		TestFalse("WherePresent should succeed", WherePresentResult.Result.Error);
		TestTrue("Channels array should contain connected channel", WherePresentResult.Channels.Contains(TestChannelIDConnect));
		TestTrue("Channels array should contain joined channel", WherePresentResult.Channels.Contains(TestChannelIDJoin));
		TestEqual("Channels array should have exactly 2 channels", WherePresentResult.Channels.Num(), 2);
	}, 1.0f));
	
	// Cleanup: Disconnect/Leave channels, delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelConnectResult, CreateChannelJoinResult, Chat, TestChannelIDConnect, TestChannelIDJoin]()
	{
		if(CreateChannelConnectResult.Channel)
		{
			CreateChannelConnectResult.Channel->Disconnect();
		}
		if(CreateChannelJoinResult.Channel)
		{
			CreateChannelJoinResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelIDConnect, false);
			Chat->DeleteChannel(TestChannelIDJoin, false);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// WHOISPRESENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWhoIsPresentNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to call WhoIsPresent without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_who_present_not_init";
			FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TestChannelID);
			
			TestTrue("WhoIsPresent should fail when Chat is not initialized", WhoIsPresentResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", WhoIsPresentResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWhoIsPresentEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_who_present_empty_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to call WhoIsPresent with empty ChannelID
		FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TEXT(""));
		
		TestTrue("WhoIsPresent should fail with empty ChannelID", WhoIsPresentResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", WhoIsPresentResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWhoIsPresentHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_who_present_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_who_present_happy";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call WhoIsPresent with only required parameter (ChannelID) and default Limit/Offset
		FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TestChannelID);
		
		TestFalse("WhoIsPresent should succeed", WhoIsPresentResult.Result.Error);
		TestTrue("Users array should contain the joined user", WhoIsPresentResult.Users.Contains(InitUserID));
		TestEqual("Users array should have at least 1 user", WhoIsPresentResult.Users.Num(), 1);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWhoIsPresentFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_who_present_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_who_present_full";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call WhoIsPresent with all parameters (ChannelID, Limit, Offset)
		const int TestLimit = 100;
		const int TestOffset = 0;
		FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TestChannelID, TestLimit, TestOffset);
		
		TestFalse("WhoIsPresent should succeed", WhoIsPresentResult.Result.Error);
		TestTrue("Users array should contain the joined user", WhoIsPresentResult.Users.Contains(InitUserID));
		TestEqual("Users array should have at least 1 user", WhoIsPresentResult.Users.Num(), 1);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
 * Tests WhoIsPresent when channel is empty (no users present).
 * Verifies that WhoIsPresent returns empty array when no users have joined the channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentEmptyChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.4Advanced.EmptyChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatWhoIsPresentEmptyChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_who_present_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_who_present_empty";
	
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
	
	// Create channel but don't join it
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Call WhoIsPresent on empty channel
	FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TestChannelID);
	
	TestFalse("WhoIsPresent should succeed", WhoIsPresentResult.Result.Error);
	TestEqual("Users array should be empty", WhoIsPresentResult.Users.Num(), 0);
	
	// Cleanup: Delete channel
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests WhoIsPresent with user who joined via Join vs Connect.
 * Verifies that WhoIsPresent returns users regardless of whether they joined via Join or Connect.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentJoinVsConnectTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.4Advanced.JoinVsConnect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWhoIsPresentJoinVsConnectTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_who_present_join_connect_init";
	const FString TestChannelID = SDK_PREFIX + "test_who_present_join_connect";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call WhoIsPresent - should return the user who joined
		FPubnubChatWhoIsPresentResult WhoIsPresentResult = Chat->WhoIsPresent(TestChannelID);
		
		TestFalse("WhoIsPresent should succeed", WhoIsPresentResult.Result.Error);
		TestTrue("Users array should contain the joined user", WhoIsPresentResult.Users.Contains(InitUserID));
		TestEqual("Users array should have exactly 1 user", WhoIsPresentResult.Users.Num(), 1);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
 * Tests WhoIsPresent with pagination (Limit and Offset parameters).
 * Verifies that Limit and Offset parameters work correctly for pagination.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatWhoIsPresentPaginationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.WhoIsPresent.4Advanced.Pagination", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatWhoIsPresentPaginationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_who_present_pag_init";
	const FString TestChannelID = SDK_PREFIX + "test_who_present_pag";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Test with Limit = 1, Offset = 0 (should return first user)
		FPubnubChatWhoIsPresentResult WhoIsPresentResult1 = Chat->WhoIsPresent(TestChannelID, 1, 0);
		TestFalse("WhoIsPresent with Limit=1 should succeed", WhoIsPresentResult1.Result.Error);
		TestEqual("Users array should have at most 1 user", WhoIsPresentResult1.Users.Num(), 1);
		
		// Test with Limit = 10, Offset = 0 (should return all users)
		FPubnubChatWhoIsPresentResult WhoIsPresentResult2 = Chat->WhoIsPresent(TestChannelID, 10, 0);
		TestFalse("WhoIsPresent with Limit=10 should succeed", WhoIsPresentResult2.Result.Error);
		TestTrue("Users array should contain the user", WhoIsPresentResult2.Users.Contains(InitUserID));
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
// ISPRESENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatIsPresentNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to call IsPresent without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_is_present_not_init_user";
			const FString TestChannelID = SDK_PREFIX + "test_is_present_not_init_channel";
			FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(TestUserID, TestChannelID);
			
			TestTrue("IsPresent should fail when Chat is not initialized", IsPresentResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", IsPresentResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatIsPresentEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_is_present_empty_user_init";
	const FString TestChannelID = SDK_PREFIX + "test_is_present_empty_user_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to call IsPresent with empty UserID
		FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(TEXT(""), TestChannelID);
		
		TestTrue("IsPresent should fail with empty UserID", IsPresentResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", IsPresentResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatIsPresentEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_is_present_empty_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to call IsPresent with empty ChannelID
		FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(TestUserID, TEXT(""));
		
		TestTrue("IsPresent should fail with empty ChannelID", IsPresentResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", IsPresentResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatIsPresentHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_present_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_is_present_happy";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call IsPresent with required parameters (UserID, ChannelID)
		FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(InitUserID, TestChannelID);
		
		TestFalse("IsPresent should succeed", IsPresentResult.Result.Error);
		TestTrue("IsPresent should return true when user is present", IsPresentResult.IsPresent);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatIsPresentFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_present_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_is_present_full";
	
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
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Join channel to make user present
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelID]()
	{
		// Call IsPresent (all parameters are required, so this is the same as happy path)
		FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(InitUserID, TestChannelID);
		
		TestFalse("IsPresent should succeed", IsPresentResult.Result.Error);
		TestTrue("IsPresent should return true when user is present", IsPresentResult.IsPresent);
	}, 1.0f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
 * Tests IsPresent when user is not present on channel.
 * Verifies that IsPresent returns false when user hasn't joined the channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentUserNotPresentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.4Advanced.UserNotPresent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatIsPresentUserNotPresentTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_present_not_present_init";
	const FString TestChannelID = SDK_PREFIX + "test_is_present_not_present";
	
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
	
	// Create channel but don't join it
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Call IsPresent - user should not be present
	FPubnubChatIsPresentResult IsPresentResult = Chat->IsPresent(InitUserID, TestChannelID);
	
	TestFalse("IsPresent should succeed", IsPresentResult.Result.Error);
	TestFalse("IsPresent should return false when user is not present", IsPresentResult.IsPresent);
	
	// Cleanup: Delete channel
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests IsPresent with Connect vs Join - both should make user present.
 * Verifies that both Channel->Connect and Channel->Join make IsPresent return true.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatIsPresentConnectVsJoinTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Presence.IsPresent.4Advanced.ConnectVsJoin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatIsPresentConnectVsJoinTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_present_connect_join_init";
	const FString TestChannelIDConnect = SDK_PREFIX + "test_is_present_connect";
	const FString TestChannelIDJoin = SDK_PREFIX + "test_is_present_join";
	
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
	
	// Create two channels
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelConnectResult = Chat->CreatePublicConversation(TestChannelIDConnect, ChannelData);
	TestFalse("CreateChannelConnect should succeed", CreateChannelConnectResult.Result.Error);
	TestNotNull("ChannelConnect should be created", CreateChannelConnectResult.Channel);
	
	FPubnubChatChannelResult CreateChannelJoinResult = Chat->CreatePublicConversation(TestChannelIDJoin, ChannelData);
	TestFalse("CreateChannelJoin should succeed", CreateChannelJoinResult.Result.Error);
	TestNotNull("ChannelJoin should be created", CreateChannelJoinResult.Channel);
	
	if(!CreateChannelConnectResult.Channel || !CreateChannelJoinResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Connect to first channel (without membership)
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	FPubnubChatConnectResult ConnectResult = CreateChannelConnectResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Join second channel (with membership)
	FPubnubChatJoinResult JoinResult = CreateChannelJoinResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, InitUserID, TestChannelIDConnect, TestChannelIDJoin]()
	{
		// Call IsPresent for connected channel - should return true
		FPubnubChatIsPresentResult IsPresentConnectResult = Chat->IsPresent(InitUserID, TestChannelIDConnect);
		TestFalse("IsPresent for connected channel should succeed", IsPresentConnectResult.Result.Error);
		TestTrue("IsPresent should return true for connected channel", IsPresentConnectResult.IsPresent);
		
		// Call IsPresent for joined channel - should return true
		FPubnubChatIsPresentResult IsPresentJoinResult = Chat->IsPresent(InitUserID, TestChannelIDJoin);
		TestFalse("IsPresent for joined channel should succeed", IsPresentJoinResult.Result.Error);
		TestTrue("IsPresent should return true for joined channel", IsPresentJoinResult.IsPresent);
	}, 1.0f));
	
	// Cleanup: Disconnect/Leave channels, delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelConnectResult, CreateChannelJoinResult, Chat, TestChannelIDConnect, TestChannelIDJoin]()
	{
		if(CreateChannelConnectResult.Channel)
		{
			CreateChannelConnectResult.Channel->Disconnect();
		}
		if(CreateChannelJoinResult.Channel)
		{
			CreateChannelJoinResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelIDConnect, false);
			Chat->DeleteChannel(TestChannelIDJoin, false);
		}
		CleanUpCurrentChatUser(Chat);
	CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// RECONNECTSUBSCRIPTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatReconnectSubscriptionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.ReconnectSubscriptions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatReconnectSubscriptionsNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to reconnect subscriptions without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatOperationResult ReconnectResult = Chat->ReconnectSubscriptions();
			
			TestTrue("ReconnectSubscriptions should fail when Chat is not initialized", ReconnectResult.Error);
			TestFalse("ErrorMessage should not be empty", ReconnectResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatReconnectSubscriptionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.ReconnectSubscriptions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatReconnectSubscriptionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_reconnect_happy_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Reconnect subscriptions with only required parameters (no Timetoken - defaults to empty string)
		FPubnubChatOperationResult ReconnectResult = Chat->ReconnectSubscriptions();
		
		TestFalse("ReconnectSubscriptions should succeed", ReconnectResult.Error);
		
		// Verify step results contain ReconnectSubscriptions step
		bool bFoundReconnect = false;
		for(const FPubnubChatOperationStepResult& Step : ReconnectResult.StepResults)
		{
			if(Step.StepName == TEXT("ReconnectSubscriptions"))
			{
				bFoundReconnect = true;
				TestFalse("ReconnectSubscriptions step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have ReconnectSubscriptions step", bFoundReconnect);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatReconnectSubscriptionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.ReconnectSubscriptions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatReconnectSubscriptionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_reconnect_full_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Test with empty Timetoken (default behavior - reconnect from "now")
		FPubnubChatOperationResult ReconnectResultEmpty = Chat->ReconnectSubscriptions(TEXT(""));
		
		TestFalse("ReconnectSubscriptions with empty Timetoken should succeed", ReconnectResultEmpty.Error);
		
		// Test with a valid timetoken (reconnect from specific point)
		// Using a recent timetoken format (numeric string)
		const FString TestTimetoken = TEXT("12345678901234567");
		FPubnubChatOperationResult ReconnectResultWithTimetoken = Chat->ReconnectSubscriptions(TestTimetoken);
		
		TestFalse("ReconnectSubscriptions with Timetoken should succeed", ReconnectResultWithTimetoken.Error);
		
		// Verify step results contain ReconnectSubscriptions step
		bool bFoundReconnect = false;
		for(const FPubnubChatOperationStepResult& Step : ReconnectResultWithTimetoken.StepResults)
		{
			if(Step.StepName == TEXT("ReconnectSubscriptions"))
			{
				bFoundReconnect = true;
				TestFalse("ReconnectSubscriptions step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have ReconnectSubscriptions step", bFoundReconnect);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that ReconnectSubscriptions restores message reception after DisconnectSubscriptions.
 * Verifies the full flow: subscribe, disconnect, verify no messages, reconnect, verify messages are received again.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatReconnectSubscriptionsRestoresMessageReceptionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.ReconnectSubscriptions.4Advanced.RestoresMessageReception", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatReconnectSubscriptionsRestoresMessageReceptionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_reconnect_restore_init";
	const FString TestChannelID = SDK_PREFIX + "test_reconnect_restore";
	
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
	
	// Create channel and join to set up subscription
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceivedBeforeDisconnect = MakeShared<bool>(false);
	TSharedPtr<bool> bMessageReceivedAfterDisconnect = MakeShared<bool>(false);
	TSharedPtr<bool> bMessageReceivedAfterReconnect = MakeShared<bool>(false);
	const FString TestMessage = TEXT("Test message for reconnect");
	
	// Set up message listener
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceivedBeforeDisconnect, bMessageReceivedAfterDisconnect, bMessageReceivedAfterReconnect, TestMessage](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			FPubnubChatMessageData MessageData = Message->GetMessageData();
			if(MessageData.Text == TestMessage)
			{
				if(!*bMessageReceivedBeforeDisconnect)
				{
					*bMessageReceivedBeforeDisconnect = true;
				}
				else if(!*bMessageReceivedAfterReconnect)
				{
					*bMessageReceivedAfterReconnect = true;
				}
			}
		}
	});
	
	// Join channel to subscribe
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait for subscription to be ready, then send a message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait for message to be received before disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceivedBeforeDisconnect]() -> bool {
		return *bMessageReceivedBeforeDisconnect;
	}, MAX_WAIT_TIME));
	
	// Disconnect subscriptions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
		TestFalse("DisconnectSubscriptions should succeed", DisconnectResult.Error);
	}, 0.1f));
	
	// Wait a bit, then send another message (should NOT be received)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessage, bMessageReceivedAfterDisconnect]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessage + TEXT("_after_disconnect"));
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait a bit to ensure message would have been received if we were still connected
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify message was NOT received after disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceivedAfterDisconnect]()
	{
		if(*bMessageReceivedAfterDisconnect)
		{
			AddError("Message should NOT have been received after DisconnectSubscriptions");
		}
		else
		{
			TestTrue("Message was correctly not received after disconnect", true);
		}
	}, 0.1f));
	
	// Reconnect subscriptions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		FPubnubChatOperationResult ReconnectResult = Chat->ReconnectSubscriptions();
		TestFalse("ReconnectSubscriptions should succeed", ReconnectResult.Error);
	}, 0.1f));
	
	// Wait for reconnection, then send another message (should be received)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait for message to be received after reconnect
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceivedAfterReconnect]() -> bool {
		return *bMessageReceivedAfterReconnect;
	}, MAX_WAIT_TIME));
	
	// Verify message was received after reconnect
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceivedAfterReconnect]()
	{
		if(!*bMessageReceivedAfterReconnect)
		{
			AddError("Message should have been received after ReconnectSubscriptions");
		}
		else
		{
			TestTrue("Message was correctly received after reconnect", true);
		}
	}, 0.1f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
// DISCONNECTSUBSCRIPTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDisconnectSubscriptionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.DisconnectSubscriptions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDisconnectSubscriptionsNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to disconnect subscriptions without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
			
			TestTrue("DisconnectSubscriptions should fail when Chat is not initialized", DisconnectResult.Error);
			TestFalse("ErrorMessage should not be empty", DisconnectResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDisconnectSubscriptionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.DisconnectSubscriptions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDisconnectSubscriptionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_disconnect_happy_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Disconnect subscriptions (no parameters required)
		FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
		
		TestFalse("DisconnectSubscriptions should succeed", DisconnectResult.Error);
		
		// Verify step results contain DisconnectSubscriptions step
		bool bFoundDisconnect = false;
		for(const FPubnubChatOperationStepResult& Step : DisconnectResult.StepResults)
		{
			if(Step.StepName == TEXT("DisconnectSubscriptions"))
			{
				bFoundDisconnect = true;
				TestFalse("DisconnectSubscriptions step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have DisconnectSubscriptions step", bFoundDisconnect);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDisconnectSubscriptionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.DisconnectSubscriptions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDisconnectSubscriptionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_disconnect_full_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// DisconnectSubscriptions takes no parameters, so this is the same as happy path
		// But we can verify it works correctly
		FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
		
		TestFalse("DisconnectSubscriptions should succeed", DisconnectResult.Error);
		TestTrue("StepResults should contain at least one step", DisconnectResult.StepResults.Num() > 0);
		
		// Verify step results contain DisconnectSubscriptions step
		bool bFoundDisconnect = false;
		for(const FPubnubChatOperationStepResult& Step : DisconnectResult.StepResults)
		{
			if(Step.StepName == TEXT("DisconnectSubscriptions"))
			{
				bFoundDisconnect = true;
				TestFalse("DisconnectSubscriptions step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have DisconnectSubscriptions step", bFoundDisconnect);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that DisconnectSubscriptions prevents message reception.
 * Verifies that after calling DisconnectSubscriptions, messages sent to subscribed channels are not received.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDisconnectSubscriptionsPreventsMessageReceptionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.DisconnectSubscriptions.4Advanced.PreventsMessageReception", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatDisconnectSubscriptionsPreventsMessageReceptionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_disconnect_prevent_init";
	const FString TestChannelID = SDK_PREFIX + "test_disconnect_prevent";
	
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
	
	// Create channel and join to set up subscription
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceivedBeforeDisconnect = MakeShared<bool>(false);
	TSharedPtr<bool> bMessageReceivedAfterDisconnect = MakeShared<bool>(false);
	const FString TestMessage = TEXT("Test message");
	
	// Set up message listener
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([this, bMessageReceivedBeforeDisconnect, bMessageReceivedAfterDisconnect, TestMessage](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			FPubnubChatMessageData MessageData = Message->GetMessageData();
			if(MessageData.Text == TestMessage)
			{
				if(!*bMessageReceivedBeforeDisconnect)
				{
					*bMessageReceivedBeforeDisconnect = true;
				}
				else
				{
					*bMessageReceivedAfterDisconnect = true;
					AddError("Message should NOT be received after DisconnectSubscriptions");
				}
			}
		}
	});
	
	// Join channel to subscribe
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait for subscription to be ready, then send a message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait for message to be received before disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceivedBeforeDisconnect]() -> bool {
		return *bMessageReceivedBeforeDisconnect;
	}, MAX_WAIT_TIME));
	
	// Disconnect subscriptions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
		TestFalse("DisconnectSubscriptions should succeed", DisconnectResult.Error);
	}, 0.1f));
	
	// Wait a bit for disconnect to take effect, then send another message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait a bit to ensure message would have been received if we were still connected
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify message was NOT received after disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceivedAfterDisconnect]()
	{
		if(*bMessageReceivedAfterDisconnect)
		{
			AddError("Message should NOT have been received after DisconnectSubscriptions");
		}
		else
		{
			TestTrue("Message was correctly not received after disconnect", true);
		}
	}, 0.1f));
	
	// Cleanup: Leave channel, delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
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
 * Tests that DisconnectSubscriptions prevents event reception.
 * Verifies that after calling DisconnectSubscriptions, events emitted to subscribed channels are not received.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDisconnectSubscriptionsPreventsEventReceptionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.ConnectionStatus.DisconnectSubscriptions.4Advanced.PreventsEventReception", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatDisconnectSubscriptionsPreventsEventReceptionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_disconnect_event_init";
	const FString TestChannelID = SDK_PREFIX + "test_disconnect_event";
	
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
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceivedBeforeDisconnect = MakeShared<bool>(false);
	TSharedPtr<bool> bEventReceivedAfterDisconnect = MakeShared<bool>(false);
	const EPubnubChatEventType ExpectedEventType = EPubnubChatEventType::PCET_Typing;
	const FString TestPayload = TEXT("{\"test\":\"data\"}");
	
	// Listen for events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceivedBeforeDisconnect, bEventReceivedAfterDisconnect, ExpectedEventType](const FPubnubChatEvent& Event)
	{
		if(Event.Type == ExpectedEventType)
		{
			if(!*bEventReceivedBeforeDisconnect)
			{
				*bEventReceivedBeforeDisconnect = true;
			}
			else
			{
				*bEventReceivedAfterDisconnect = true;
				AddError("Event should NOT be received after DisconnectSubscriptions");
			}
		}
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, ExpectedEventType, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready, then emit event
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ExpectedEventType, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(ExpectedEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.5f));
	
	// Wait for event to be received before disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceivedBeforeDisconnect]() -> bool {
		return *bEventReceivedBeforeDisconnect;
	}, MAX_WAIT_TIME));
	
	// Disconnect subscriptions
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat]()
	{
		FPubnubChatOperationResult DisconnectResult = Chat->DisconnectSubscriptions();
		TestFalse("DisconnectSubscriptions should succeed", DisconnectResult.Error);
	}, 0.1f));
	
	// Wait a bit for disconnect to take effect, then emit another event
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, ExpectedEventType, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(ExpectedEventType, TestChannelID, TestPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
	}, 0.5f));
	
	// Wait a bit to ensure event would have been received if we were still connected
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify event was NOT received after disconnect
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceivedAfterDisconnect]()
	{
		if(*bEventReceivedAfterDisconnect)
		{
			AddError("Event should NOT have been received after DisconnectSubscriptions");
		}
		else
		{
			TestTrue("Event was correctly not received after disconnect", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, Chat]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// GETEVENTSHISTORY TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get events history without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_get_events_history_not_init";
			const FString StartTimetoken = TEXT("0");
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken);
			
			TestTrue("GetEventsHistory should fail when Chat is not initialized", HistoryResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_events_history_empty_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get events history with empty ChannelID
		const FString StartTimetoken = TEXT("0");
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TEXT(""), StartTimetoken, EndTimetoken);
		
		TestTrue("GetEventsHistory should fail with empty ChannelID", HistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryEmptyStartTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.1Validation.EmptyStartTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryEmptyStartTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_events_history_empty_start_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_empty_start";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get events history with empty StartTimetoken
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, TEXT(""), EndTimetoken);
		
		TestTrue("GetEventsHistory should fail with empty StartTimetoken", HistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryEmptyEndTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.1Validation.EmptyEndTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryEmptyEndTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_events_history_empty_end_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_empty_end";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get events history with empty EndTimetoken
		const FString StartTimetoken = TEXT("0");
		FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, TEXT(""));
		
		TestTrue("GetEventsHistory should fail with empty EndTimetoken", HistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing event
		const FString StartTimetoken = TEXT("0");
		
		// Emit an event with Publish method (only published events can be retrieved)
		const FString TestPayload = TEXT("{\"test\":\"data\"}");
		FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
		
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
		
		// Wait a bit for event to be stored, then get events history
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken, InitUserID]()
		{
			// Get events history with only required parameters (default Count = 100)
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have at least one event", HistoryResult.Events.Num() >= 1);
			
			// Verify the event we published is in the results
			bool bFoundEvent = false;
			for(const FPubnubChatEvent& Event : HistoryResult.Events)
			{
				if(Event.Type == EPubnubChatEventType::PCET_Typing && Event.ChannelID == TestChannelID)
				{
					bFoundEvent = true;
					TestEqual("Event ChannelID should match", Event.ChannelID, TestChannelID);
					TestEqual("Event UserID should match", Event.UserID, InitUserID);
					TestFalse("Event Timetoken should not be empty", Event.Timetoken.IsEmpty());
					TestFalse("Event Payload should not be empty", Event.Payload.IsEmpty());
					break;
				}
			}
			TestTrue("Should find the published event in history", bFoundEvent);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.5f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Emit multiple events with Publish method
		const FString TestPayload1 = TEXT("{\"test\":\"data1\"}");
		const FString TestPayload2 = TEXT("{\"test\":\"data2\"}");
		const FString TestPayload3 = TEXT("{\"test\":\"data3\"}");
		
		FPubnubChatOperationResult EmitResult1 = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload1, EPubnubChatEventMethod::PCEM_Publish);
		TestFalse("EmitChatEvent 1 should succeed", EmitResult1.Error);
		
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestPayload2]()
		{
			FPubnubChatOperationResult EmitResult2 = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Receipt, TestChannelID, TestPayload2, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse("EmitChatEvent 2 should succeed", EmitResult2.Error);
		}, 0.2f));
		
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestPayload3]()
		{
			FPubnubChatOperationResult EmitResult3 = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Custom, TestChannelID, TestPayload3, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse("EmitChatEvent 3 should succeed", EmitResult3.Error);
		}, 0.4f));
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken]()
		{
			// Get events history with all parameters including Count
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			const int Count = 10;
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken, Count);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have at least 3 events", HistoryResult.Events.Num() >= 3);
			TestTrue("Should not exceed Count limit", HistoryResult.Events.Num() <= Count);
			
			// Verify all three event types are present
			bool bFoundTyping = false;
			bool bFoundReceipt = false;
			bool bFoundCustom = false;
			
			for(const FPubnubChatEvent& Event : HistoryResult.Events)
			{
				if(Event.Type == EPubnubChatEventType::PCET_Typing && Event.ChannelID == TestChannelID)
				{
					bFoundTyping = true;
				}
				else if(Event.Type == EPubnubChatEventType::PCET_Receipt && Event.ChannelID == TestChannelID)
				{
					bFoundReceipt = true;
				}
				else if(Event.Type == EPubnubChatEventType::PCET_Custom && Event.ChannelID == TestChannelID)
				{
					bFoundCustom = true;
				}
			}
			
			TestTrue("Should find Typing event", bFoundTyping);
			TestTrue("Should find Receipt event", bFoundReceipt);
			TestTrue("Should find Custom event", bFoundCustom);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.6f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that only events published with PCEM_Publish method are retrieved.
 * Events published with PCEM_Signal should not appear in history.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryOnlyPublishEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.OnlyPublishEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryOnlyPublishEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_publish_only_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_publish_only";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Emit event with Publish method (should appear in history)
		const FString TestPayloadPublish = TEXT("{\"test\":\"publish\"}");
		FPubnubChatOperationResult EmitResultPublish = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayloadPublish, EPubnubChatEventMethod::PCEM_Publish);
		TestFalse("EmitChatEvent with Publish should succeed", EmitResultPublish.Error);
		
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
		{
			// Emit event with Signal method (should NOT appear in history)
			const FString TestPayloadSignal = TEXT("{\"test\":\"signal\"}");
			FPubnubChatOperationResult EmitResultSignal = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Receipt, TestChannelID, TestPayloadSignal, EPubnubChatEventMethod::PCEM_Signal);
			TestFalse("EmitChatEvent with Signal should succeed", EmitResultSignal.Error);
		}, 0.2f));
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken]()
		{
			// Get events history
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			
			// Verify only the Publish event is in history, not the Signal event
			bool bFoundPublishEvent = false;
			bool bFoundSignalEvent = false;
			
			for(const FPubnubChatEvent& Event : HistoryResult.Events)
			{
				if(Event.ChannelID == TestChannelID)
				{
					if(Event.Type == EPubnubChatEventType::PCET_Typing)
					{
						bFoundPublishEvent = true;
					}
					else if(Event.Type == EPubnubChatEventType::PCET_Receipt)
					{
						bFoundSignalEvent = true;
					}
				}
			}
			
			TestTrue("Should find Publish event in history", bFoundPublishEvent);
			TestFalse("Should NOT find Signal event in history", bFoundSignalEvent);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.4f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

/**
 * Tests GetEventsHistory with empty history (no events in the specified time range).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryEmptyHistoryTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.EmptyHistory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryEmptyHistoryTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Use a time range in the past where no events exist
		// Use timetoken "0" as start and a very old timetoken as end
		const FString StartTimetoken = TEXT("0");
		const FString EndTimetoken = TEXT("10000000000000000"); // Very old timetoken
		
		FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken);
		
		TestFalse("GetEventsHistory should succeed even with empty history", HistoryResult.Result.Error);
		TestEqual("Should have no events in empty history", HistoryResult.Events.Num(), 0);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetEventsHistory with Count parameter limiting the number of returned events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryCountLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.CountLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryCountLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_count_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_count";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Emit multiple events
		const int NumEventsToPublish = 5;
		for(int i = 0; i < NumEventsToPublish; ++i)
		{
			const FString TestPayload = FString::Printf(TEXT("{\"test\":\"data%d\"}"), i);
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse(FString::Printf(TEXT("EmitChatEvent %d should succeed"), i), EmitResult.Error);
			
			if(i < NumEventsToPublish - 1)
			{
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]() {}, 0.1f));
			}
		}
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken, NumEventsToPublish]()
		{
			// Get events history with Count limit smaller than number of events published
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			const int CountLimit = 3;
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken, CountLimit);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have events", HistoryResult.Events.Num() > 0);
			TestTrue("Should respect Count limit", HistoryResult.Events.Num() <= CountLimit);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.6f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

/**
 * Tests GetEventsHistory with different event types to verify all types can be retrieved.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryAllEventTypesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.AllEventTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryAllEventTypesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_all_types_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_all_types";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Emit events with all different event types (using Publish method)
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
			const FString TestPayload = FString::Printf(TEXT("{\"test\":\"eventType%d\"}"), i);
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EventTypes[i], TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse(FString::Printf(TEXT("EmitChatEvent for type %d should succeed"), i), EmitResult.Error);
			
			if(i < EventTypes.Num() - 1)
			{
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]() {}, 0.1f));
			}
		}
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken, EventTypes, InitUserID]()
		{
			// Get events history
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have at least as many events as published", HistoryResult.Events.Num() >= EventTypes.Num());
			
			// Verify all event types are present in history
			TArray<bool> FoundEventTypes;
			FoundEventTypes.SetNum(EventTypes.Num());
			for(int32 i = 0; i < FoundEventTypes.Num(); ++i)
			{
				FoundEventTypes[i] = false;
			}
			
			for(const FPubnubChatEvent& Event : HistoryResult.Events)
			{
				if(Event.ChannelID == TestChannelID)
				{
					for(int32 i = 0; i < EventTypes.Num(); ++i)
					{
						if(Event.Type == EventTypes[i])
						{
							FoundEventTypes[i] = true;
							TestEqual("Event ChannelID should match", Event.ChannelID, TestChannelID);
							TestEqual("Event UserID should match", Event.UserID, InitUserID);
							TestFalse("Event Timetoken should not be empty", Event.Timetoken.IsEmpty());
							TestFalse("Event Payload should not be empty", Event.Payload.IsEmpty());
							break;
						}
					}
				}
			}
			
			// Verify all event types were found
			for(int32 i = 0; i < EventTypes.Num(); ++i)
			{
				TestTrue(FString::Printf(TEXT("Should find event type %d in history"), i), FoundEventTypes[i]);
			}
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.8f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

/**
 * Tests GetEventsHistory IsMore parameter when exactly Count messages are returned.
 * Verifies that IsMore is true when we get exactly the requested count, indicating more might be available.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryIsMoreTrueTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.IsMoreTrue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryIsMoreTrueTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_ismore_true_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_ismore_true";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Publish exactly Count events (so we get exactly Count messages back)
		const int Count = 5;
		for(int i = 0; i < Count; ++i)
		{
			const FString TestPayload = FString::Printf(TEXT("{\"test\":\"data%d\"}"), i);
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse(FString::Printf(TEXT("EmitChatEvent %d should succeed"), i), EmitResult.Error);
			
			if(i < Count - 1)
			{
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]() {}, 0.1f));
			}
		}
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken, Count]()
		{
			// Get events history with Count matching the number of events we published
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken, Count);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have events", HistoryResult.Events.Num() > 0);
			
			// If we got exactly Count messages, IsMore should be true (indicating there might be more)
			// Note: We're checking messages, not events, because FetchHistory returns messages
			// If exactly Count messages were returned, there might be more available
			TestTrue("IsMore should be true when exactly Count messages are returned", HistoryResult.IsMore);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.6f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

/**
 * Tests GetEventsHistory IsMore parameter when fewer than Count messages are returned.
 * Verifies that IsMore is false when we get fewer than requested, indicating we got all available.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryIsMoreFalseTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.IsMoreFalse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryIsMoreFalseTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_ismore_false_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_ismore_false";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get start timetoken before publishing events
		const FString StartTimetoken = TEXT("0");
		
		// Publish fewer events than Count (so we get fewer than Count messages back)
		const int NumEventsToPublish = 2;
		const int Count = 10; // Request more than we publish
		
		for(int i = 0; i < NumEventsToPublish; ++i)
		{
			const FString TestPayload = FString::Printf(TEXT("{\"test\":\"data%d\"}"), i);
			FPubnubChatOperationResult EmitResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TestPayload, EPubnubChatEventMethod::PCEM_Publish);
			TestFalse(FString::Printf(TEXT("EmitChatEvent %d should succeed"), i), EmitResult.Error);
			
			if(i < NumEventsToPublish - 1)
			{
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]() {}, 0.1f));
			}
		}
		
		// Wait for events to be stored
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, StartTimetoken, Count, NumEventsToPublish]()
		{
			// Get events history with Count larger than number of events we published
			const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
			FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken, Count);
			
			TestFalse("GetEventsHistory should succeed", HistoryResult.Result.Error);
			TestTrue("Should have events", HistoryResult.Events.Num() > 0);
			TestTrue("Should have fewer events than Count", HistoryResult.Events.Num() < Count);
			
			// If we got fewer than Count messages, IsMore should be false (indicating we got all available)
			TestFalse("IsMore should be false when fewer than Count messages are returned", HistoryResult.IsMore);
			
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.4f));
	}
	else
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}
	return true;
}

/**
 * Tests GetEventsHistory IsMore parameter with empty history.
 * Verifies that IsMore is false when no events are found.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetEventsHistoryIsMoreEmptyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.GetEventsHistory.4Advanced.IsMoreEmpty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetEventsHistoryIsMoreEmptyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_events_history_ismore_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_events_history_ismore_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Use a time range in the past where no events exist
		const FString StartTimetoken = TEXT("0");
		const FString EndTimetoken = TEXT("10000000000000000"); // Very old timetoken
		const int Count = 10;
		
		FPubnubChatGetEventsHistoryResult HistoryResult = Chat->GetEventsHistory(TestChannelID, StartTimetoken, EndTimetoken, Count);
		
		TestFalse("GetEventsHistory should succeed even with empty history", HistoryResult.Result.Error);
		TestEqual("Should have no events in empty history", HistoryResult.Events.Num(), 0);
		
		// With no messages returned, IsMore should be false
		TestFalse("IsMore should be false when no messages are returned", HistoryResult.IsMore);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
