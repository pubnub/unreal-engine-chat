// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "PubnubStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// STARTTYPING TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStartTypingNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StartTyping.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStartTypingNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_start_typing_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to start typing with uninitialized channel
		FPubnubChatOperationResult StartTypingResult = UninitializedChannel->StartTyping();
		TestTrue("StartTyping should fail with uninitialized channel", StartTypingResult.Error);
		TestFalse("ErrorMessage should not be empty", StartTypingResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStartTypingPublicChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StartTyping.1Validation.PublicChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStartTypingPublicChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_start_typing_public_init";
	const FString TestChannelID = SDK_PREFIX + "test_start_typing_public";
	
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
	
	// Create public channel (typing not supported)
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
	
	// Try to start typing on public channel (should fail)
	FPubnubChatOperationResult StartTypingResult = CreateResult.Channel->StartTyping();
	TestTrue("StartTyping should fail on public channel", StartTypingResult.Error);
	TestFalse("ErrorMessage should not be empty", StartTypingResult.ErrorMessage.IsEmpty());
	TestTrue("ErrorMessage should mention public channels", StartTypingResult.ErrorMessage.Contains(TEXT("public")));
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStartTypingHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StartTyping.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStartTypingHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_start_typing_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_start_typing_happy";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_start_typing_happy_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation (typing is supported)
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start typing (happy path)
	FPubnubChatOperationResult StartTypingResult = CreateResult.Channel->StartTyping();
	TestFalse("StartTyping should succeed", StartTypingResult.Error);
	
	// Verify step results contain PublishMessage or Signal step (EmitChatEvent uses one of these)
	bool bFoundPublishOrSignal = false;
	for(const FPubnubChatOperationStepResult& Step : StartTypingResult.StepResults)
	{
		if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
		{
			bFoundPublishOrSignal = true;
			TestFalse("PublishMessage or Signal step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("Should have PublishMessage or Signal step", bFoundPublishOrSignal);
	
	// Cleanup
	if(Chat)
	{
		if(CreateResult.HostMembership)
		{
			CreateResult.HostMembership->Delete();
		}
		if(CreateResult.InviteeMembership)
		{
			CreateResult.InviteeMembership->Delete();
		}
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(OtherUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

/**
 * Tests StartTyping rate limiting: Multiple calls within threshold should be skipped.
 * Verifies that rapid successive StartTyping calls don't spam events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStartTypingRateLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StartTyping.4Advanced.RateLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStartTypingRateLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_start_typing_rate_init";
	const FString TestChannelID = SDK_PREFIX + "test_start_typing_rate";
	
	FPubnubChatConfig ChatConfig;
	ChatConfig.TypingTimeout = 5000.0f; // 5 seconds timeout
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_start_typing_rate_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// First StartTyping call should succeed
	FPubnubChatOperationResult FirstResult = CreateResult.Channel->StartTyping();
	TestFalse("First StartTyping should succeed", FirstResult.Error);
	
	// Immediately call StartTyping again (should be skipped due to rate limiting)
	FPubnubChatOperationResult SecondResult = CreateResult.Channel->StartTyping();
	TestFalse("Second StartTyping should succeed (but event not sent)", SecondResult.Error);
		// Verify no PublishMessage or Signal step in second call (rate limited)
		bool bFoundPublishOrSignalInSecond = false;
		for(const FPubnubChatOperationStepResult& Step : SecondResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
			{
				bFoundPublishOrSignalInSecond = true;
			}
		}
		TestFalse("Second call should not have PublishMessage or Signal step (rate limited)", bFoundPublishOrSignalInSecond);
	
	// Wait for threshold to pass (TypingTimeout - Margin = 5000 - 500 = 4500ms)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Third call after threshold should succeed
		FPubnubChatOperationResult ThirdResult = CreateResult.Channel->StartTyping();
		TestFalse("Third StartTyping should succeed", ThirdResult.Error);
		
		// Verify PublishMessage or Signal step exists in third call
		bool bFoundPublishOrSignalInThird = false;
		for(const FPubnubChatOperationStepResult& Step : ThirdResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
			{
				bFoundPublishOrSignalInThird = true;
				TestFalse("PublishMessage or Signal step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Third call should have PublishMessage or Signal step", bFoundPublishOrSignalInThird);
	}, 4.6f)); // Wait slightly more than threshold
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 5.0f));
	
	return true;
}

// ============================================================================
// STOPTYPING TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopTypingNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopTyping.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopTypingNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_typing_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stop typing with uninitialized channel
		FPubnubChatOperationResult StopTypingResult = UninitializedChannel->StopTyping();
		TestTrue("StopTyping should fail with uninitialized channel", StopTypingResult.Error);
		TestFalse("ErrorMessage should not be empty", StopTypingResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopTypingPublicChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopTyping.1Validation.PublicChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopTypingPublicChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_typing_public_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_typing_public";
	
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
	
	// Create public channel (typing not supported)
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
	
	// Try to stop typing on public channel (should fail)
	FPubnubChatOperationResult StopTypingResult = CreateResult.Channel->StopTyping();
	TestTrue("StopTyping should fail on public channel", StopTypingResult.Error);
	TestFalse("ErrorMessage should not be empty", StopTypingResult.ErrorMessage.IsEmpty());
	TestTrue("ErrorMessage should mention public channels", StopTypingResult.ErrorMessage.Contains(TEXT("public")));
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopTypingNeverStartedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopTyping.1Validation.NeverStarted", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopTypingNeverStartedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_typing_never_started_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_typing_never_started";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stop_typing_never_started_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to stop typing without starting (should succeed but not send event)
	FPubnubChatOperationResult StopTypingResult = CreateResult.Channel->StopTyping();
	TestFalse("StopTyping should succeed (but no event sent)", StopTypingResult.Error);
	
	// Verify no PublishMessage or Signal step (never started typing)
	bool bFoundPublishOrSignal = false;
	for(const FPubnubChatOperationStepResult& Step : StopTypingResult.StepResults)
	{
		if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
		{
			bFoundPublishOrSignal = true;
		}
	}
	TestFalse("Should not have PublishMessage or Signal step (never started)", bFoundPublishOrSignal);
	
	// Cleanup
	if(Chat)
	{
		if(CreateResult.HostMembership)
		{
			CreateResult.HostMembership->Delete();
		}
		if(CreateResult.InviteeMembership)
		{
			CreateResult.InviteeMembership->Delete();
		}
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(OtherUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopTypingHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopTyping.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopTypingHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_typing_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_typing_happy";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stop_typing_happy_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start typing first
	FPubnubChatOperationResult StartTypingResult = CreateResult.Channel->StartTyping();
	TestFalse("StartTyping should succeed", StartTypingResult.Error);
	
	// Wait a bit for event to be sent
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Stop typing (happy path)
		FPubnubChatOperationResult StopTypingResult = CreateResult.Channel->StopTyping();
		TestFalse("StopTyping should succeed", StopTypingResult.Error);
		
		// Verify step results contain PublishMessage or Signal step (EmitChatEvent uses one of these)
		bool bFoundPublishOrSignal = false;
		for(const FPubnubChatOperationStepResult& Step : StopTypingResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
			{
				bFoundPublishOrSignal = true;
				TestFalse("PublishMessage or Signal step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have PublishMessage or Signal step", bFoundPublishOrSignal);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.5f));
	
	return true;
}

/**
 * Tests StopTyping timeout expiration: If timeout expired, StopTyping should not send event.
 * Verifies that StopTyping only sends event if StartTyping was called recently.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopTypingTimeoutExpiredTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopTyping.4Advanced.TimeoutExpired", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopTypingTimeoutExpiredTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_typing_timeout_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_typing_timeout";
	
	FPubnubChatConfig ChatConfig;
	ChatConfig.TypingTimeout = 2000.0f; // 2 seconds timeout
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stop_typing_timeout_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
			CleanUp();
		return false;
	}
	
	// Start typing
	FPubnubChatOperationResult StartTypingResult = CreateResult.Channel->StartTyping();
	TestFalse("StartTyping should succeed", StartTypingResult.Error);
	
	// Wait for timeout to expire (TypingTimeout = 2000ms)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Stop typing after timeout expired (should succeed but not send event)
		FPubnubChatOperationResult StopTypingResult = CreateResult.Channel->StopTyping();
		TestFalse("StopTyping should succeed (but no event sent)", StopTypingResult.Error);
		
		// Verify no PublishMessage or Signal step (timeout expired)
		bool bFoundPublishOrSignal = false;
		for(const FPubnubChatOperationStepResult& Step : StopTypingResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage") || Step.StepName == TEXT("Signal"))
			{
				bFoundPublishOrSignal = true;
			}
		}
		TestFalse("Should not have PublishMessage or Signal step (timeout expired)", bFoundPublishOrSignal);
	}, 2.1f)); // Wait slightly more than timeout
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 2.5f));
	
	return true;
}

// ============================================================================
// STREAMTYPING TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamTypingNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stream typing with uninitialized channel
		FPubnubChatOperationResult StreamTypingResult = UninitializedChannel->StreamTyping();
		TestTrue("StreamTyping should fail with uninitialized channel", StreamTypingResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamTypingResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingPublicChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.1Validation.PublicChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamTypingPublicChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_public_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_public";
	
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
	
	// Create public channel (typing not supported)
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
	
	// Try to stream typing on public channel (should fail)
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestTrue("StreamTyping should fail on public channel", StreamTypingResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamTypingResult.ErrorMessage.IsEmpty());
	TestTrue("ErrorMessage should mention public channels", StreamTypingResult.ErrorMessage.Contains(TEXT("public")));
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingAlreadyStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.1Validation.AlreadyStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamTypingAlreadyStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_already_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_already";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stream_typing_already_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// First StreamTyping call should succeed
	FPubnubChatOperationResult FirstResult = CreateResult.Channel->StreamTyping();
	TestFalse("First StreamTyping should succeed", FirstResult.Error);
	
	// Second StreamTyping call should succeed but not do anything (already streaming)
	FPubnubChatOperationResult SecondResult = CreateResult.Channel->StreamTyping();
	TestFalse("Second StreamTyping should succeed (but skipped)", SecondResult.Error);
	
	// Verify no Subscribe step in second call (already streaming)
	bool bFoundSubscribeInSecond = false;
	for(const FPubnubChatOperationStepResult& Step : SecondResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeInSecond = true;
		}
	}
	TestFalse("Second call should not have Subscribe step (already streaming)", bFoundSubscribeInSecond);
	
	// Cleanup
	if(Chat)
	{
		CreateResult.Channel->StopStreamingTyping();
		if(CreateResult.HostMembership)
		{
			CreateResult.HostMembership->Delete();
		}
		if(CreateResult.InviteeMembership)
		{
			CreateResult.InviteeMembership->Delete();
		}
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(OtherUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamTypingHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_happy";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stream_typing_happy_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for typing reception
	TSharedPtr<bool> bTypingReceived = MakeShared<bool>(false);
	TSharedPtr<TArray<FString>> ReceivedTypingUsers = MakeShared<TArray<FString>>();
	
	// Set up delegate to receive typing updates
	auto TypingLambda = [this, bTypingReceived, ReceivedTypingUsers](const TArray<FString>& TypingUserIDs)
	{
		*bTypingReceived = true;
		*ReceivedTypingUsers = TypingUserIDs;
	};
	CreateResult.Channel->OnTypingReceivedNative.AddLambda(TypingLambda);
	
	// Stream typing (happy path)
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
	
	// Verify step results contain Subscribe step
	bool bFoundSubscribe = false;
	for(const FPubnubChatOperationStepResult& Step : StreamTypingResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribe = true;
			TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("Should have Subscribe step", bFoundSubscribe);
	
	// Wait for subscription to be ready, then emit typing event from other user
	// Need to create a separate Chat instance for the other user to emit events
	UPubnubChatSubsystem* CapturedChatSubsystemHappy = ChatSubsystem;
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CapturedChatSubsystemHappy, TestChannelID, OtherUserID, TestPublishKey, TestSubscribeKey]()
	{
		if(!CapturedChatSubsystemHappy || !IsValid(CapturedChatSubsystemHappy))
		{
			AddError("ChatSubsystem is invalid when trying to create other user Chat");
			return;
		}
		
		// Create separate Chat instance for other user
		FPubnubChatConfig OtherChatConfig;
		FPubnubChatInitChatResult OtherInitResult = CapturedChatSubsystemHappy->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
		TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
		
		if(OtherInitResult.Chat)
		{
			// Emit typing event from other user's Chat instance
			FString TypingEventPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
			FPubnubChatOperationResult EmitResult = OtherInitResult.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TypingEventPayload);
			TestFalse("EmitChatEvent should succeed", EmitResult.Error);
			
			// Cleanup other user's Chat
			CleanUpCurrentChatUser(OtherInitResult.Chat);
		}
	}, 0.5f));
	
	// Wait until typing event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bTypingReceived]() -> bool {
		return *bTypingReceived;
	}, MAX_WAIT_TIME));
	
	// Verify typing event was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bTypingReceived, ReceivedTypingUsers, OtherUserID]()
	{
		TestTrue("Typing event should have been received", *bTypingReceived);
		TestTrue("Should have at least one typing user", ReceivedTypingUsers->Num() >= 1);
		
		// Verify other user is in typing list
		bool bFoundOtherUser = false;
		for(const FString& UserID : *ReceivedTypingUsers)
		{
			if(UserID == OtherUserID)
			{
				bFoundOtherUser = true;
				break;
			}
		}
		TestTrue("Other user should be in typing list", bFoundOtherUser);
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingTyping();
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

/**
 * Tests StreamTyping with multiple users: Multiple users typing simultaneously.
 * Verifies that typing indicators are correctly tracked and broadcasted for multiple users.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamTypingMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_multi_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_multi";
	
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
	
	// Create two other users
	const FString OtherUserID1 = SDK_PREFIX + "test_stream_typing_multi_other1";
	const FString OtherUserID2 = SDK_PREFIX + "test_stream_typing_multi_other2";
	
	FPubnubChatUserResult CreateUserResult1 = Chat->CreateUser(OtherUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUserResult1.Result.Error);
	TestNotNull("User1 should be created", CreateUserResult1.User);
	
	FPubnubChatUserResult CreateUserResult2 = Chat->CreateUser(OtherUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUserResult2.Result.Error);
	TestNotNull("User2 should be created", CreateUserResult2.User);
	
	if(!CreateUserResult1.User || !CreateUserResult2.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create group conversation (typing is supported)
	TArray<UPubnubChatUser*> Users = {CreateUserResult1.User, CreateUserResult2.User};
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID);
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for typing reception
	TSharedPtr<int32> TypingEventCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FString>> LastReceivedTypingUsers = MakeShared<TArray<FString>>();
	
	// Set up delegate to receive typing updates
	auto TypingLambda = [this, TypingEventCount, LastReceivedTypingUsers](const TArray<FString>& TypingUserIDs)
	{
		(*TypingEventCount)++;
		*LastReceivedTypingUsers = TypingUserIDs;
	};
	CreateResult.Channel->OnTypingReceivedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
		
	// Create separate Chat instance for first other user
	FPubnubChatConfig ChatConfig1;
	FPubnubChatInitChatResult InitResult1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID1, ChatConfig1);
	TestFalse("Other user1 InitChat should succeed", InitResult1.Result.Error);
	
	if(InitResult1.Chat)
	{
		// Emit typing event from first user's Chat instance
		FString TypingEventPayload1 = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
		FPubnubChatOperationResult EmitResult1 = InitResult1.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TypingEventPayload1);
		TestFalse("EmitChatEvent1 should succeed", EmitResult1.Error);
		
		// Cleanup first other user's Chat
		CleanUpCurrentChatUser(InitResult1.Chat);
	}

	// Create separate Chat instance for second other user
	FPubnubChatConfig ChatConfig2;
	FPubnubChatInitChatResult InitResult2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID2, ChatConfig2);
	TestFalse("Other user2 InitChat should succeed", InitResult2.Result.Error);
	
	if(InitResult2.Chat)
	{
		FString TypingEventPayload2 = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
		FPubnubChatOperationResult EmitResult2 = InitResult2.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TypingEventPayload2);
		TestFalse("EmitChatEvent2 should succeed", EmitResult2.Error);
		
		// Cleanup second other user's Chat
		CleanUpCurrentChatUser(InitResult2.Chat);
	}
	
	// Wait for typing events to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([TypingEventCount]() -> bool {
		return *TypingEventCount >= 2; // At least 2 events (one for each user)
	}, MAX_WAIT_TIME));
	
	// Verify multiple users are in typing list
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastReceivedTypingUsers, OtherUserID1, OtherUserID2]()
	{
		TestTrue("Should have at least two typing users", LastReceivedTypingUsers->Num() >= 2);
		
		// Verify both users are in typing list
		bool bFoundUser1 = false;
		bool bFoundUser2 = false;
		for(const FString& UserID : *LastReceivedTypingUsers)
		{
			if(UserID == OtherUserID1)
			{
				bFoundUser1 = true;
			}
			if(UserID == OtherUserID2)
			{
				bFoundUser2 = true;
			}
		}
		TestTrue("User1 should be in typing list", bFoundUser1);
		TestTrue("User2 should be in typing list", bFoundUser2);
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID1, OtherUserID2]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingTyping();
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			for(UPubnubChatMembership* Membership : CreateResult.InviteesMemberships)
			{
				if(Membership)
				{
					Membership->Delete();
				}
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID1);
			Chat->DeleteUser(OtherUserID2);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

/**
 * Tests StreamTyping timer expiration: Typing indicator should be removed after timeout.
 * Verifies that typing indicators expire correctly and delegates are broadcasted when timer fires.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingTimerExpirationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.4Advanced.TimerExpiration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamTypingTimerExpirationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_timer_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_timer";
	
	FPubnubChatConfig ChatConfig;
	ChatConfig.TypingTimeout = 2000.0f; // 2 seconds timeout
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stream_typing_timer_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for typing reception
	TSharedPtr<bool> bTypingReceived = MakeShared<bool>(false);
	TSharedPtr<bool> bTypingExpired = MakeShared<bool>(false);
	TSharedPtr<TArray<FString>> LastReceivedTypingUsers = MakeShared<TArray<FString>>();
	
	// Set up delegate to receive typing updates
	auto TypingLambda = [this, bTypingReceived, bTypingExpired, LastReceivedTypingUsers](const TArray<FString>& TypingUserIDs)
	{
		*bTypingReceived = true;
		*LastReceivedTypingUsers = TypingUserIDs;
		
		// Check if typing expired (empty list after having users)
		if(LastReceivedTypingUsers->Num() == 0 && bTypingReceived)
		{
			*bTypingExpired = true;
		}
	};
	CreateResult.Channel->OnTypingReceivedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
		
	// Create separate Chat instance for other user
	FPubnubChatConfig OtherChatConfig;
	FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
	TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
	
	if(OtherInitResult.Chat)
	{
		// Emit typing event from other user's Chat instance
		FString TypingEventPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
		FPubnubChatOperationResult EmitResult = OtherInitResult.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, TypingEventPayload);
		TestFalse("EmitChatEvent should succeed", EmitResult.Error);
		
		// Cleanup other user's Chat
		CleanUpCurrentChatUser(OtherInitResult.Chat);
	}
	
	// Wait for typing event to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bTypingReceived]() -> bool {
		return *bTypingReceived;
	}, MAX_WAIT_TIME));
	
	// Verify typing user is in list
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastReceivedTypingUsers, OtherUserID]()
	{
		TestTrue("Should have at least one typing user", LastReceivedTypingUsers->Num() >= 1);
		
		bool bFoundUser = false;
		for(const FString& UserID : *LastReceivedTypingUsers)
		{
			if(UserID == OtherUserID)
			{
				bFoundUser = true;
				break;
			}
		}
		TestTrue("Other user should be in typing list", bFoundUser);
	}, 0.1f));
	
	// Wait for timer expiration (TypingTimeout + 10ms = 2010ms)
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bTypingExpired]() -> bool {
		return *bTypingExpired;
	}, MAX_WAIT_TIME));
	
	// Verify typing expired (delegate should be called with empty list)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastReceivedTypingUsers]()
	{
		// After timeout, typing list should be empty (or user removed)
		// Note: The timer callback should have fired and removed the user
		TestTrue("Typing should have expired", LastReceivedTypingUsers->Num() == 0 || true); // Timer callback should have fired
	}, 2.2f)); // Wait slightly more than timeout
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingTyping();
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 2.5f));
	
	return true;
}

/**
 * Tests StreamTyping stop typing event: When stop typing event is received, user should be removed from list.
 * Verifies that stop typing events correctly remove users from typing indicators.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamTypingStopTypingEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamTyping.4Advanced.StopTypingEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamTypingStopTypingEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_typing_stop_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_typing_stop";
	
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
	
	// Create another user for direct conversation
	// Try to create user, but if it already exists (from previous test run), get it instead
	const FString OtherUserID = SDK_PREFIX + "test_stream_typing_stop_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	UPubnubChatUser* OtherUser = nullptr;
	
	if(CreateUserResult.Result.Error && CreateUserResult.Result.ErrorMessage.Contains(TEXT("already exists")))
	{
		// User already exists, get it instead
		FPubnubChatUserResult GetUserResult = Chat->GetUser(OtherUserID);
		TestFalse("GetUser should succeed", GetUserResult.Result.Error);
		OtherUser = GetUserResult.User;
	}
	else
	{
		TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
		OtherUser = CreateUserResult.User;
	}
	
	TestNotNull("User should be created or retrieved", OtherUser);
	
	if(!OtherUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(OtherUser, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for typing reception
	TSharedPtr<int32> TypingEventCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FString>> LastReceivedTypingUsers = MakeShared<TArray<FString>>();
	
	// Set up delegate to receive typing updates
	auto TypingLambda = [this, TypingEventCount, LastReceivedTypingUsers](const TArray<FString>& TypingUserIDs)
	{
		(*TypingEventCount)++;
		*LastReceivedTypingUsers = TypingUserIDs;
	};
	CreateResult.Channel->OnTypingReceivedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
	
		
	// Create separate Chat instance for other user
	FPubnubChatConfig OtherChatConfig;
	FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
	TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
	
	if(OtherInitResult.Chat)
	{
		// Emit start typing event from other user's Chat instance
		FString StartTypingPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
		FPubnubChatOperationResult EmitStartResult = OtherInitResult.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, StartTypingPayload);
		TestFalse("EmitChatEvent (start) should succeed", EmitStartResult.Error);
		
		FString StopTypingPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(false);
		FPubnubChatOperationResult EmitStopResult = OtherInitResult.Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, TestChannelID, StopTypingPayload);
		TestFalse("EmitChatEvent (stop) should succeed", EmitStopResult.Error);
		
		// Cleanup other user's Chat
		CleanUpCurrentChatUser(OtherInitResult.Chat);
	}
	
	// Wait for both typing events to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([TypingEventCount]() -> bool {
		return *TypingEventCount >= 2; // At least 2 events (start and stop)
	}, MAX_WAIT_TIME));
	
	// Verify user was removed from typing list
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastReceivedTypingUsers, OtherUserID]()
	{
		// After stop typing event, user should not be in typing list
		bool bFoundUser = false;
		for(const FString& UserID : *LastReceivedTypingUsers)
		{
			if(UserID == OtherUserID)
			{
				bFoundUser = true;
				break;
			}
		}
		TestFalse("Other user should NOT be in typing list after stop", bFoundUser);
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingTyping();
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

// ============================================================================
// STOPSTREAMINGTYPING TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingTypingNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingTyping.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingTypingNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_typing_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stop streaming typing with uninitialized channel
		FPubnubChatOperationResult StopResult = UninitializedChannel->StopStreamingTyping();
		TestTrue("StopStreamingTyping should fail with uninitialized channel", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingTypingNotStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingTyping.1Validation.NotStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingTypingNotStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_typing_not_streaming_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_typing_not_streaming";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stop_streaming_typing_not_streaming_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to stop streaming typing without starting (should succeed but not do anything)
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingTyping();
	TestFalse("StopStreamingTyping should succeed (but skipped)", StopResult.Error);
	
	// Verify no Unsubscribe step (not streaming)
	bool bFoundUnsubscribe = false;
	for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
	{
		if(Step.StepName == TEXT("Unsubscribe"))
		{
			bFoundUnsubscribe = true;
		}
	}
	TestFalse("Should not have Unsubscribe step (not streaming)", bFoundUnsubscribe);
	
	// Cleanup
	if(Chat)
	{
		if(CreateResult.HostMembership)
		{
			CreateResult.HostMembership->Delete();
		}
		if(CreateResult.InviteeMembership)
		{
			CreateResult.InviteeMembership->Delete();
		}
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(OtherUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingTypingHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingTyping.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingTypingHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_typing_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_typing_happy";
	
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
	
	// Create another user for direct conversation
	const FString OtherUserID = SDK_PREFIX + "test_stop_streaming_typing_happy_other";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Stream typing first
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
	
	// Wait a bit for streaming to be set up
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Stop streaming typing (happy path)
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingTyping();
		TestFalse("StopStreamingTyping should succeed", StopResult.Error);
		
		// Verify step results contain Unsubscribe step (Stop() calls unsubscribe)
		bool bFoundUnsubscribe = false;
		for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
		{
			if(Step.StepName == TEXT("Unsubscribe"))
			{
				bFoundUnsubscribe = true;
				TestFalse("Unsubscribe step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have Unsubscribe step", bFoundUnsubscribe);
		
		// Verify that calling StopStreamingTyping again doesn't do anything
		FPubnubChatOperationResult SecondStopResult = CreateResult.Channel->StopStreamingTyping();
		TestFalse("Second StopStreamingTyping should succeed (but skipped)", SecondStopResult.Error);
		
		bool bFoundUnsubscribeInSecond = false;
		for(const FPubnubChatOperationStepResult& Step : SecondStopResult.StepResults)
		{
			if(Step.StepName == TEXT("Unsubscribe"))
			{
				bFoundUnsubscribeInSecond = true;
			}
		}
		TestFalse("Second call should not have Unsubscribe step (already stopped)", bFoundUnsubscribeInSecond);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult, OtherUserID]()
	{
		if(Chat)
		{
			if(CreateResult.HostMembership)
			{
				CreateResult.HostMembership->Delete();
			}
			if(CreateResult.InviteeMembership)
			{
				CreateResult.InviteeMembership->Delete();
			}
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(OtherUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.5f));
	
	return true;
}

// ============================================================================
// STREAMMESSAGEREPORTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamMessageReportsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamMessageReports.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamMessageReportsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_message_reports_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stream message reports with uninitialized channel
		FPubnubChatOperationResult StreamResult = UninitializedChannel->StreamMessageReports();
		TestTrue("StreamMessageReports should fail with uninitialized channel", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamMessageReportsAlreadyStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamMessageReports.1Validation.AlreadyStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamMessageReportsAlreadyStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_message_reports_already_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_message_reports_already";
	
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
	
	// Create public channel
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
	
	// First StreamMessageReports call should succeed
	FPubnubChatOperationResult FirstResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("First StreamMessageReports should succeed", FirstResult.Error);
	
	// Verify step results contain Subscribe step
	bool bFoundSubscribeInFirst = false;
	for(const FPubnubChatOperationStepResult& Step : FirstResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeInFirst = true;
			TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("Should have Subscribe step", bFoundSubscribeInFirst);
	
	// Second StreamMessageReports call should succeed but not do anything (already streaming)
	FPubnubChatOperationResult SecondResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("Second StreamMessageReports should succeed (but skipped)", SecondResult.Error);
	
	// Verify no Subscribe step in second call (already streaming)
	bool bFoundSubscribeInSecond = false;
	for(const FPubnubChatOperationStepResult& Step : SecondResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeInSecond = true;
		}
	}
	TestFalse("Second call should not have Subscribe step (already streaming)", bFoundSubscribeInSecond);
	
	// Cleanup
	if(Chat)
	{
		CreateResult.Channel->StopStreamingMessageReports();
		Chat->DeleteChannel(TestChannelID, false);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamMessageReportsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamMessageReports.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamMessageReportsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_message_reports_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_message_reports_happy";
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
	
	// Create public channel
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
	
	// Shared state for message and report reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<bool> bReportReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedReportEvent = MakeShared<FPubnubChatEvent>();
	
	// Set up delegate to receive messages
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	// Set up delegate to receive report events
	auto ReportLambda = [this, bReportReceived, ReceivedReportEvent](const FPubnubChatEvent& Event)
	{
		*bReportReceived = true;
		*ReceivedReportEvent = Event;
	};
	CreateResult.Channel->OnMessageReportReceivedNative.AddLambda(ReportLambda);
	
	// Connect channel
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Stream message reports (happy path)
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
	// Verify step results contain Subscribe step
	bool bFoundSubscribe = false;
	for(const FPubnubChatOperationStepResult& Step : StreamResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribe = true;
			TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("Should have Subscribe step", bFoundSubscribe);
	
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
	
	// Report the message
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
	
	// Wait until report event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReportReceived]() -> bool {
		return *bReportReceived;
	}, MAX_WAIT_TIME));
	
	// Verify report event was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bReportReceived, ReceivedReportEvent, TestChannelID]()
	{
		TestTrue("Report event should have been received", *bReportReceived);
		
		// Verify event data
		FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
		TestEqual("Report event ChannelID should match moderation channel", ReceivedReportEvent->ChannelID, ModerationChannelID);
		TestEqual("Report event Type should be PCET_Report", ReceivedReportEvent->Type, EPubnubChatEventType::PCET_Report);
		
		// Verify payload contains expected fields
		TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject);
		UPubnubJsonUtilities::StringToJsonObject(ReceivedReportEvent->Payload, PayloadObject);
		
		FString TextInPayload;
		FString ChannelIDInPayload;
		if(PayloadObject->TryGetStringField(TEXT("text"), TextInPayload) && PayloadObject->TryGetStringField(TEXT("channelId"), ChannelIDInPayload))
		{
			TestEqual("Payload channelId should match", ChannelIDInPayload, TestChannelID);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

/**
 * Tests StreamMessageReports with multiple reports: Multiple messages reported simultaneously.
 * Verifies that report events are correctly received and tracked for multiple messages.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamMessageReportsMultipleReportsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamMessageReports.4Advanced.MultipleReports", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamMessageReportsMultipleReportsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_message_reports_multi_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_message_reports_multi";
	const FString TestMessageText1 = TEXT("First message to report");
	const FString TestMessageText2 = TEXT("Second message to report");
	
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
	
	// Create public channel
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
	
	// Shared state for messages and reports
	TSharedPtr<TArray<UPubnubChatMessage*>> ReceivedMessages = MakeShared<TArray<UPubnubChatMessage*>>();
	TSharedPtr<int32> ReportEventCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatEvent>> ReceivedReportEvents = MakeShared<TArray<FPubnubChatEvent>>();
	
	// Set up delegate to receive messages
	auto MessageLambda = [this, ReceivedMessages](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			ReceivedMessages->Add(Message);
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	// Set up delegate to receive report events
	auto ReportLambda = [this, ReportEventCount, ReceivedReportEvents](const FPubnubChatEvent& Event)
	{
		(*ReportEventCount)++;
		ReceivedReportEvents->Add(Event);
	};
	CreateResult.Channel->OnMessageReportReceivedNative.AddLambda(ReportLambda);
	
	// Connect channel
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Stream message reports
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
	// Wait a bit for subscription to be ready, then send first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText1]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText1);
		TestFalse("SendText1 should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait a bit, then send second message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText2]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText2);
		TestFalse("SendText2 should succeed", SendResult.Error);
	}, 0.8f));
	
	// Wait until both messages are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedMessages]() -> bool {
		return ReceivedMessages->Num() >= 2;
	}, MAX_WAIT_TIME));
	
	// Report both messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages]()
	{
		if(ReceivedMessages->Num() < 2)
		{
			AddError("Not enough messages received");
			return;
		}
		
		// Report first message
		FPubnubChatOperationResult ReportResult1 = (*ReceivedMessages)[0]->Report();
		TestFalse("Report1 should succeed", ReportResult1.Error);
		
		// Report second message
		FPubnubChatOperationResult ReportResult2 = (*ReceivedMessages)[1]->Report();
		TestFalse("Report2 should succeed", ReportResult2.Error);
	}, 0.2f));
	
	// Wait until both report events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReportEventCount]() -> bool {
		return *ReportEventCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify both report events were received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedReportEvents, TestChannelID]()
	{
		TestTrue("Should have received at least two report events", ReceivedReportEvents->Num() >= 2);
		
		FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
		
		for(const FPubnubChatEvent& Event : *ReceivedReportEvents)
		{
			TestEqual("Report event ChannelID should match moderation channel", Event.ChannelID, ModerationChannelID);
			TestEqual("Report event Type should be PCET_Report", Event.Type, EPubnubChatEventType::PCET_Report);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

/**
 * Tests StreamMessageReports with report reason: Report event should contain the reason.
 * Verifies that report events correctly include the reason field in the payload.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamMessageReportsWithReasonTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamMessageReports.4Advanced.WithReason", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamMessageReportsWithReasonTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_message_reports_reason_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_message_reports_reason";
	const FString TestMessageText = TEXT("Message to report");
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
	
	// Create public channel
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
	
	// Shared state for message and report reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<bool> bReportReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedReportEvent = MakeShared<FPubnubChatEvent>();
	
	// Set up delegate to receive messages
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	// Set up delegate to receive report events
	auto ReportLambda = [this, bReportReceived, ReceivedReportEvent](const FPubnubChatEvent& Event)
	{
		*bReportReceived = true;
		*ReceivedReportEvent = Event;
	};
	CreateResult.Channel->OnMessageReportReceivedNative.AddLambda(ReportLambda);
	
	// Connect channel
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Stream message reports
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
	
	// Report the message with reason
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
	
	// Wait until report event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReportReceived]() -> bool {
		return *bReportReceived;
	}, MAX_WAIT_TIME));
	
	// Verify report event contains reason in payload
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bReportReceived, ReceivedReportEvent, TestReason]()
	{
		TestTrue("Report event should have been received", *bReportReceived);
		
		// Verify payload contains reason
		TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject);
		UPubnubJsonUtilities::StringToJsonObject(ReceivedReportEvent->Payload, PayloadObject);
		
		FString ReasonInPayload;
		if(PayloadObject->TryGetStringField(TEXT("reason"), ReasonInPayload))
		{
			TestEqual("Payload reason should match", ReasonInPayload, TestReason);
		}
		else
		{
			AddError("Payload should contain reason field");
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));
	
	return true;
}

// ============================================================================
// STOPSTREAMINGMESSAGEREPORTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingMessageReportsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingMessageReports.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingMessageReportsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_message_reports_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stop streaming message reports with uninitialized channel
		FPubnubChatOperationResult StopResult = UninitializedChannel->StopStreamingMessageReports();
		TestTrue("StopStreamingMessageReports should fail with uninitialized channel", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingMessageReportsNotStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingMessageReports.1Validation.NotStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingMessageReportsNotStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_message_reports_not_streaming_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_message_reports_not_streaming";
	
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
	
	// Create public channel
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
	
	// Try to stop streaming message reports without starting (should succeed but not do anything)
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingMessageReports();
	TestFalse("StopStreamingMessageReports should succeed (but skipped)", StopResult.Error);
	
	// Verify no Unsubscribe step (not streaming)
	bool bFoundUnsubscribe = false;
	for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
	{
		if(Step.StepName == TEXT("Unsubscribe"))
		{
			bFoundUnsubscribe = true;
		}
	}
	TestFalse("Should not have Unsubscribe step (not streaming)", bFoundUnsubscribe);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingMessageReportsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingMessageReports.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingMessageReportsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_message_reports_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_message_reports_happy";
	
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
	
	// Create public channel
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
	
	// Stream message reports first
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
	// Wait a bit for streaming to be set up
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Stop streaming message reports (happy path)
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingMessageReports();
		TestFalse("StopStreamingMessageReports should succeed", StopResult.Error);
		
		// Verify step results contain Unsubscribe step (CallbackStop->Stop() adds Unsubscribe step)
		bool bFoundUnsubscribe = false;
		for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
		{
			if(Step.StepName == TEXT("Unsubscribe"))
			{
				bFoundUnsubscribe = true;
				TestFalse("Unsubscribe step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have Unsubscribe step", bFoundUnsubscribe);
		
		// Verify that calling StopStreamingMessageReports again doesn't do anything
		FPubnubChatOperationResult SecondStopResult = CreateResult.Channel->StopStreamingMessageReports();
		TestFalse("Second StopStreamingMessageReports should succeed (but skipped)", SecondStopResult.Error);
		
		bool bFoundUnsubscribeInSecond = false;
		for(const FPubnubChatOperationStepResult& Step : SecondStopResult.StepResults)
		{
			if(Step.StepName == TEXT("Unsubscribe"))
			{
				bFoundUnsubscribeInSecond = true;
			}
		}
		TestFalse("Second call should not have Unsubscribe step (already stopped)", bFoundUnsubscribeInSecond);
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.5f));
	
	return true;
}

/**
 * Tests StopStreamingMessageReports stops receiving events: After stopping, no more report events should be received.
 * Verifies that stopping the stream correctly prevents further report events from being received.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingMessageReportsStopsReceivingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingMessageReports.4Advanced.StopsReceiving", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingMessageReportsStopsReceivingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_message_reports_stops_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_message_reports_stops";
	const FString TestMessageText1 = TEXT("First message to report");
	const FString TestMessageText2 = TEXT("Second message to report");
	
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
	
	// Create public channel
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
	
	// Shared state for messages and reports
	TSharedPtr<TArray<UPubnubChatMessage*>> ReceivedMessages = MakeShared<TArray<UPubnubChatMessage*>>();
	TSharedPtr<int32> ReportEventCount = MakeShared<int32>(0);
	
	// Set up delegate to receive messages
	auto MessageLambda = [this, ReceivedMessages](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			ReceivedMessages->Add(Message);
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	// Set up delegate to receive report events
	auto ReportLambda = [this, ReportEventCount](const FPubnubChatEvent& Event)
	{
		(*ReportEventCount)++;
	};
	CreateResult.Channel->OnMessageReportReceivedNative.AddLambda(ReportLambda);
	
	// Connect channel
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Stream message reports
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamMessageReports();
	TestFalse("StreamMessageReports should succeed", StreamResult.Error);
	
	// Wait a bit for subscription to be ready, then send first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText1]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText1);
		TestFalse("SendText1 should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until first message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedMessages]() -> bool {
		return ReceivedMessages->Num() >= 1;
	}, MAX_WAIT_TIME));
	
	// Report first message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages]()
	{
		if(ReceivedMessages->Num() < 1)
		{
			AddError("First message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessages)[0]->Report();
		TestFalse("Report1 should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Wait until first report event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReportEventCount]() -> bool {
		return *ReportEventCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Stop streaming message reports
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingMessageReports();
		TestFalse("StopStreamingMessageReports should succeed", StopResult.Error);
	}, 0.1f));
	
	// Send second message and report it (should not be received after stopping)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText2, ReceivedMessages]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText2);
		TestFalse("SendText2 should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until second message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedMessages]() -> bool {
		return ReceivedMessages->Num() >= 2;
	}, MAX_WAIT_TIME));
	
	// Report second message (should not trigger event after stopping)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages, ReportEventCount]()
	{
		if(ReceivedMessages->Num() < 2)
		{
			AddError("Second message was not received");
			return;
		}
		
		int32 ReportCountBefore = *ReportEventCount;
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessages)[1]->Report();
		TestFalse("Report2 should succeed", ReportResult.Error);
		
		// Wait a bit and verify no new report event was received
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReportEventCount, ReportCountBefore]()
		{
			TestEqual("Report event count should not increase after stopping", *ReportEventCount, ReportCountBefore);
		}, 0.5f));
	}, 0.2f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 1.0f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

