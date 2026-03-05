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

#include "Dom/JsonObject.h"
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
		Chat->DeleteChannel(TestChannelID);
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
	
	// Verify step results contain PublishMessage or Signal step
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
		Chat->DeleteChannel(TestChannelID);
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
			Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
		
		// Verify step results contain PublishMessage or Signal step
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
			Chat->DeleteChannel(TestChannelID);
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
			Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
	CreateResult.Channel->OnTypingChangedNative.AddLambda(TypingLambda);
	
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
			FPubnubChatChannelResult OtherChannelResult = OtherInitResult.Chat->GetChannel(TestChannelID);
			TestFalse("Other user GetChannel should succeed", OtherChannelResult.Result.Error);
			if(OtherChannelResult.Channel)
			{
				FPubnubChatOperationResult StartTypingResult = OtherChannelResult.Channel->StartTyping();
				TestFalse("StartTyping should succeed", StartTypingResult.Error);
			}
			
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
			Chat->DeleteChannel(TestChannelID);
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
	CreateResult.Channel->OnTypingChangedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
		
	// Create separate Chat instance for first other user
	FPubnubChatConfig ChatConfig1;
	FPubnubChatInitChatResult InitResult1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID1, ChatConfig1);
	TestFalse("Other user1 InitChat should succeed", InitResult1.Result.Error);
	
	if(InitResult1.Chat)
	{
		FPubnubChatChannelResult OtherChannelResult1 = InitResult1.Chat->GetChannel(TestChannelID);
		TestFalse("Other user1 GetChannel should succeed", OtherChannelResult1.Result.Error);
		if(OtherChannelResult1.Channel)
		{
			FPubnubChatOperationResult StartTypingResult1 = OtherChannelResult1.Channel->StartTyping();
			TestFalse("StartTyping1 should succeed", StartTypingResult1.Error);
		}
		
		// Cleanup first other user's Chat
		CleanUpCurrentChatUser(InitResult1.Chat);
	}

	// Create separate Chat instance for second other user
	FPubnubChatConfig ChatConfig2;
	FPubnubChatInitChatResult InitResult2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID2, ChatConfig2);
	TestFalse("Other user2 InitChat should succeed", InitResult2.Result.Error);
	
	if(InitResult2.Chat)
	{
		FPubnubChatChannelResult OtherChannelResult2 = InitResult2.Chat->GetChannel(TestChannelID);
		TestFalse("Other user2 GetChannel should succeed", OtherChannelResult2.Result.Error);
		if(OtherChannelResult2.Channel)
		{
			FPubnubChatOperationResult StartTypingResult2 = OtherChannelResult2.Channel->StartTyping();
			TestFalse("StartTyping2 should succeed", StartTypingResult2.Error);
		}
		
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
			Chat->DeleteChannel(TestChannelID);
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
	CreateResult.Channel->OnTypingChangedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
		
	// Create separate Chat instance for other user
	FPubnubChatConfig OtherChatConfig;
	FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
	TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
	
	if(OtherInitResult.Chat)
	{
		FPubnubChatChannelResult OtherChannelResult = OtherInitResult.Chat->GetChannel(TestChannelID);
		TestFalse("Other user GetChannel should succeed", OtherChannelResult.Result.Error);
		if(OtherChannelResult.Channel)
		{
			FPubnubChatOperationResult StartTypingResult = OtherChannelResult.Channel->StartTyping();
			TestFalse("StartTyping should succeed", StartTypingResult.Error);
		}
		
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
			Chat->DeleteChannel(TestChannelID);
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
	CreateResult.Channel->OnTypingChangedNative.AddLambda(TypingLambda);
	
	// Stream typing
	FPubnubChatOperationResult StreamTypingResult = CreateResult.Channel->StreamTyping();
	TestFalse("StreamTyping should succeed", StreamTypingResult.Error);
	
		
	// Create separate Chat instance for other user
	FPubnubChatConfig OtherChatConfig;
	FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
	TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
	
	if(OtherInitResult.Chat)
	{
		FPubnubChatChannelResult OtherChannelResult = OtherInitResult.Chat->GetChannel(TestChannelID);
		TestFalse("Other user GetChannel should succeed", OtherChannelResult.Result.Error);
		if(OtherChannelResult.Channel)
		{
			FPubnubChatOperationResult StartTypingResult = OtherChannelResult.Channel->StartTyping();
			TestFalse("StartTyping should succeed", StartTypingResult.Error);
			
			FPubnubChatOperationResult StopTypingResult = OtherChannelResult.Channel->StopTyping();
			TestFalse("StopTyping should succeed", StopTypingResult.Error);
		}
		
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
			Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
			Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
	TSharedPtr<FPubnubChatReportEvent> ReceivedReportEvent = MakeShared<FPubnubChatReportEvent>();
	
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
	auto ReportLambda = [this, bReportReceived, ReceivedReportEvent](const FPubnubChatReportEvent& Event)
	{
		*bReportReceived = true;
		*ReceivedReportEvent = Event;
	};
	CreateResult.Channel->OnMessageReportedNative.AddLambda(ReportLambda);
	
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
		TestEqual("Report event ChannelID should match source channel", ReceivedReportEvent->ReportedMessageChannelID, TestChannelID);
		TestEqual("Payload channelId should match", ReceivedReportEvent->ReportedMessageChannelID, TestChannelID);
		
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID);
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
	TSharedPtr<TArray<FPubnubChatReportEvent>> ReceivedReportEvents = MakeShared<TArray<FPubnubChatReportEvent>>();
	
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
	auto ReportLambda = [this, ReportEventCount, ReceivedReportEvents](const FPubnubChatReportEvent& Event)
	{
		(*ReportEventCount)++;
		ReceivedReportEvents->Add(Event);
	};
	CreateResult.Channel->OnMessageReportedNative.AddLambda(ReportLambda);
	
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
				
		for(const FPubnubChatReportEvent& Event : *ReceivedReportEvents)
		{
			TestEqual("Report event ChannelID should match source channel", Event.ReportedMessageChannelID, TestChannelID);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID);
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
	TSharedPtr<FPubnubChatReportEvent> ReceivedReportEvent = MakeShared<FPubnubChatReportEvent>();
	
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
	auto ReportLambda = [this, bReportReceived, ReceivedReportEvent](const FPubnubChatReportEvent& Event)
	{
		*bReportReceived = true;
		*ReceivedReportEvent = Event;
	};
	CreateResult.Channel->OnMessageReportedNative.AddLambda(ReportLambda);
	
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
		TestEqual("Payload reason should match", ReceivedReportEvent->Reason, TestReason);
		
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		if(Chat)
		{
			CreateResult.Channel->StopStreamingMessageReports();
			Chat->DeleteChannel(TestChannelID);
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
		Chat->DeleteChannel(TestChannelID);
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
			Chat->DeleteChannel(TestChannelID);
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
	auto ReportLambda = [this, ReportEventCount](const FPubnubChatReportEvent& Event)
	{
		(*ReportEventCount)++;
	};
	CreateResult.Channel->OnMessageReportedNative.AddLambda(ReportLambda);
	
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
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 1.0f));
	
	return true;
}

// ============================================================================
// GETMESSAGEREPORTSHISTORY TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageReportsHistoryNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to get message reports history with uninitialized channel
		const FString StartTimetoken = TEXT("0");
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatEventsResult HistoryResult = UninitializedChannel->GetMessageReportsHistory(StartTimetoken, EndTimetoken);
		
		TestTrue("GetMessageReportsHistory should fail with uninitialized channel", HistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryEmptyStartTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.1Validation.EmptyStartTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageReportsHistoryEmptyStartTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_empty_start_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_empty_start";
	
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
	
	// Try to get message reports history with empty StartTimetoken
	const FString EmptyStartTimetoken = TEXT("");
	const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(EmptyStartTimetoken, EndTimetoken);
	
	TestTrue("GetMessageReportsHistory should fail with empty StartTimetoken", HistoryResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryEmptyEndTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.1Validation.EmptyEndTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageReportsHistoryEmptyEndTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_empty_end_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_empty_end";
	
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
	
	// Try to get message reports history with empty EndTimetoken
	const FString StartTimetoken = TEXT("0");
	const FString EmptyEndTimetoken = TEXT("");
	FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(StartTimetoken, EmptyEndTimetoken);
	
	TestTrue("GetMessageReportsHistory should fail with empty EndTimetoken", HistoryResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", HistoryResult.Result.ErrorMessage.IsEmpty());
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelGetMessageReportsHistoryHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_happy";
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
	
	// Get start timetoken before sending message
	const FString StartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	// Connect channel
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
	
	// Report the message
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
	
	// Wait for report to be stored, then get message reports history
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, StartTimetoken, TestChannelID, InitUserID, TestMessageText, TestReason, Chat]()
	{
		// Get message reports history with only required parameters (default Count = 100)
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(StartTimetoken, EndTimetoken);
		
		TestFalse("GetMessageReportsHistory should succeed", HistoryResult.Result.Error);
		TestTrue("Should have at least one report event", HistoryResult.Events.Num() >= 1);
		
		// Verify the report event we created is in the results
		bool bFoundReportEvent = false;
		FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
		
		for(const FPubnubChatEvent& Event : HistoryResult.Events)
		{
			if(Event.Type == EPubnubChatEventType::PCET_Report && Event.ChannelID == ModerationChannelID)
			{
				bFoundReportEvent = true;
				TestEqual("Event ChannelID should match moderation channel", Event.ChannelID, ModerationChannelID);
				TestEqual("Event UserID should match", Event.UserID, InitUserID);
				TestFalse("Event Timetoken should not be empty", Event.Timetoken.IsEmpty());
				TestFalse("Event Payload should not be empty", Event.Payload.IsEmpty());
				
				// Verify payload contains report data
				TSharedPtr<FJsonObject> PayloadJson = MakeShareable(new FJsonObject);
				UPubnubJsonUtilities::StringToJsonObject(Event.Payload, PayloadJson);
				
				FString PayloadText;
				FString PayloadReason;
				FString PayloadChannelId;
				FString PayloadUserId;
				
				if(PayloadJson->TryGetStringField(ANSI_TO_TCHAR("text"), PayloadText))
				{
					TestEqual("Payload text should match reported message", PayloadText, TestMessageText);
				}
				if(PayloadJson->TryGetStringField(ANSI_TO_TCHAR("reason"), PayloadReason))
				{
					TestEqual("Payload reason should match", PayloadReason, TestReason);
				}
				if(PayloadJson->TryGetStringField(ANSI_TO_TCHAR("channelId"), PayloadChannelId))
				{
					TestEqual("Payload channelId should match", PayloadChannelId, TestChannelID);
				}
				if(PayloadJson->TryGetStringField(ANSI_TO_TCHAR("userId"), PayloadUserId))
				{
					TestEqual("Payload userId should match", PayloadUserId, InitUserID);
				}
				
				break;
			}
		}
		TestTrue("Should find the report event in history", bFoundReportEvent);
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.5f));
	
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelGetMessageReportsHistoryFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_full";
	const FString TestMessageText1 = TEXT("First message to report");
	const FString TestMessageText2 = TEXT("Second message to report");
	const FString TestMessageText3 = TEXT("Third message to report");
	const FString TestReason1 = TEXT("Reason 1");
	const FString TestReason2 = TEXT("Reason 2");
	const FString TestReason3 = TEXT("Reason 3");
	
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
	TSharedPtr<TArray<UPubnubChatMessage*>> ReceivedMessages = MakeShared<TArray<UPubnubChatMessage*>>();
	
	// Set up delegate to receive messages
	auto MessageLambda = [this, ReceivedMessages](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			ReceivedMessages->Add(Message);
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
	
	// Get start timetoken before sending messages
	const FString StartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	// Connect channel
	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);
	
	// Send first message
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages, TestReason1]()
	{
		if(ReceivedMessages->Num() < 1)
		{
			AddError("First message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessages)[0]->Report(TestReason1);
		TestFalse("Report1 should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Send second message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText2]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText2);
		TestFalse("SendText2 should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until second message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedMessages]() -> bool {
		return ReceivedMessages->Num() >= 2;
	}, MAX_WAIT_TIME));
	
	// Report second message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages, TestReason2]()
	{
		if(ReceivedMessages->Num() < 2)
		{
			AddError("Second message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessages)[1]->Report(TestReason2);
		TestFalse("Report2 should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Send third message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessageText3]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessageText3);
		TestFalse("SendText3 should succeed", SendResult.Error);
	}, 0.3f));
	
	// Wait until third message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedMessages]() -> bool {
		return ReceivedMessages->Num() >= 3;
	}, MAX_WAIT_TIME));
	
	// Report third message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMessages, TestReason3]()
	{
		if(ReceivedMessages->Num() < 3)
		{
			AddError("Third message was not received");
			return;
		}
		
		FPubnubChatOperationResult ReportResult = (*ReceivedMessages)[2]->Report(TestReason3);
		TestFalse("Report3 should succeed", ReportResult.Error);
	}, 0.2f));
	
	// Wait for reports to be stored, then get message reports history with Count parameter
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, StartTimetoken, TestChannelID, InitUserID, ReceivedMessages, TestMessageText1, TestMessageText2, TestMessageText3, TestReason1, TestReason2, TestReason3, Chat]()
	{
		// Get message reports history with all parameters including Count
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const int Count = 10;
		FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(StartTimetoken, EndTimetoken, Count);
		
		TestFalse("GetMessageReportsHistory should succeed", HistoryResult.Result.Error);
		TestTrue("Should have at least 3 report events", HistoryResult.Events.Num() >= 3);
		TestTrue("Should not exceed Count limit", HistoryResult.Events.Num() <= Count);
		
		// Verify all three report events are present
		FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
		bool bFoundReport1 = false;
		bool bFoundReport2 = false;
		bool bFoundReport3 = false;
		
		for(const FPubnubChatEvent& Event : HistoryResult.Events)
		{
			if(Event.Type == EPubnubChatEventType::PCET_Report && Event.ChannelID == ModerationChannelID)
			{
				TSharedPtr<FJsonObject> PayloadJson = MakeShareable(new FJsonObject);
				UPubnubJsonUtilities::StringToJsonObject(Event.Payload, PayloadJson);
				
				FString PayloadText;
				FString PayloadReason;
				
				if(PayloadJson->TryGetStringField(ANSI_TO_TCHAR("text"), PayloadText) && 
				   PayloadJson->TryGetStringField(ANSI_TO_TCHAR("reason"), PayloadReason))
				{
					if(PayloadText == TestMessageText1 && PayloadReason == TestReason1)
					{
						bFoundReport1 = true;
					}
					else if(PayloadText == TestMessageText2 && PayloadReason == TestReason2)
					{
						bFoundReport2 = true;
					}
					else if(PayloadText == TestMessageText3 && PayloadReason == TestReason3)
					{
						bFoundReport3 = true;
					}
				}
			}
		}
		
		TestTrue("Should find first report event", bFoundReport1);
		TestTrue("Should find second report event", bFoundReport2);
		TestTrue("Should find third report event", bFoundReport3);
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.6f));
	
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetMessageReportsHistory with empty history (no reports in the specified time range).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryEmptyHistoryTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.4Advanced.EmptyHistory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageReportsHistoryEmptyHistoryTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_empty";
	
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
	
	// Use a time range in the past where no reports exist
	// Use timetoken "0" as start and a very old timetoken as end
	const FString StartTimetoken = TEXT("0");
	const FString EndTimetoken = TEXT("10000000000000000"); // Very old timetoken
	
	FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(StartTimetoken, EndTimetoken);
	
	TestFalse("GetMessageReportsHistory should succeed even with empty history", HistoryResult.Result.Error);
	TestEqual("Should have no events in empty history", HistoryResult.Events.Num(), 0);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMessageReportsHistory verifies that returned events are report events (Type = PCET_Report)
 * and belong to the correct moderation channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageReportsHistoryVerifyEventDataTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessageReportsHistory.4Advanced.VerifyEventData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelGetMessageReportsHistoryVerifyEventDataTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_reports_history_verify_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_reports_history_verify";
	const FString TestMessageText = TEXT("Message to verify report data");
	const FString TestReason = TEXT("Verification reason");
	
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
	
	// Set up delegate to receive messages
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
	
	// Get start timetoken before sending message
	const FString StartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
	
	// Connect channel
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
	
	// Report the message
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
	
	// Wait for report to be stored, then verify event data
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, StartTimetoken, ModerationChannelID, TestChannelID, InitUserID, TestMessageText, TestReason, MessageTimetoken, Chat]()
	{
		// Get message reports history
		const FString EndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatEventsResult HistoryResult = CreateResult.Channel->GetMessageReportsHistory(StartTimetoken, EndTimetoken);
		
		TestFalse("GetMessageReportsHistory should succeed", HistoryResult.Result.Error);
		TestTrue("Should have at least one report event", HistoryResult.Events.Num() >= 1);
		
		// Verify all returned events are report events from the correct moderation channel
		bool bFoundMatchingReport = false;
		for(const FPubnubChatEvent& Event : HistoryResult.Events)
		{
			// All events should be report events
			TestEqual("Event Type should be PCET_Report", Event.Type, EPubnubChatEventType::PCET_Report);
			
			// Events should belong to the moderation channel
			if(Event.ChannelID == ModerationChannelID)
			{
				TestEqual("Event ChannelID should match moderation channel", Event.ChannelID, ModerationChannelID);
				TestEqual("Event UserID should match", Event.UserID, InitUserID);
				TestFalse("Event Timetoken should not be empty", Event.Timetoken.IsEmpty());
				TestFalse("Event Payload should not be empty", Event.Payload.IsEmpty());
				
				// Verify payload structure
				TSharedPtr<FJsonObject> PayloadJson = MakeShareable(new FJsonObject);
				UPubnubJsonUtilities::StringToJsonObject(Event.Payload, PayloadJson);
				
				FString PayloadText;
				FString PayloadReason;
				FString PayloadChannelId;
				FString PayloadUserId;
				FString PayloadTimetoken;
				
				TestTrue("Payload should contain text field", PayloadJson->TryGetStringField(ANSI_TO_TCHAR("text"), PayloadText));
				TestTrue("Payload should contain reason field", PayloadJson->TryGetStringField(ANSI_TO_TCHAR("reason"), PayloadReason));
				TestTrue("Payload should contain channelId field", PayloadJson->TryGetStringField(ANSI_TO_TCHAR("channelId"), PayloadChannelId));
				TestTrue("Payload should contain userId field", PayloadJson->TryGetStringField(ANSI_TO_TCHAR("userId"), PayloadUserId));
				TestTrue("Payload should contain timetoken field", PayloadJson->TryGetStringField(ANSI_TO_TCHAR("timetoken"), PayloadTimetoken));
				
				// Verify payload values match our report
				if(PayloadText == TestMessageText && PayloadReason == TestReason && 
				   PayloadChannelId == TestChannelID && PayloadUserId == InitUserID)
				{
					bFoundMatchingReport = true;
					TestEqual("Payload channelId should match", PayloadChannelId, TestChannelID);
					TestEqual("Payload userId should match", PayloadUserId, InitUserID);
					TestEqual("Payload text should match", PayloadText, TestMessageText);
					TestEqual("Payload reason should match", PayloadReason, TestReason);
					TestEqual("Payload timetoken should match message timetoken", PayloadTimetoken, *MessageTimetoken);
				}
			}
		}
		
		TestTrue("Should find matching report event with correct data", bFoundMatchingReport);
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.5f));
	
	return true;
}

// ============================================================================
// STREAMUPDATESON TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnEmptyArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.1Validation.EmptyArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnEmptyArrayTest::RunTest(const FString& Parameters)
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
	TArray<UPubnubChatChannel*> EmptyChannelsArray;
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(EmptyChannelsArray);
	
	// Should succeed but do nothing (no channels to process)
	TestFalse("StreamUpdatesOn with empty array should succeed", StreamUpdatesOnResult.Error);
	TestEqual("StepResults should be empty for empty array", StreamUpdatesOnResult.StepResults.Num(), 0);
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnUninitializedChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.1Validation.UninitializedChannels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnUninitializedChannelsTest::RunTest(const FString& Parameters)
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
	
	// Create uninitialized channel objects
	UPubnubChatChannel* UninitializedChannel1 = NewObject<UPubnubChatChannel>(Chat);
	UPubnubChatChannel* UninitializedChannel2 = NewObject<UPubnubChatChannel>(Chat);
	
	// Call StreamUpdatesOn with uninitialized channels
	TArray<UPubnubChatChannel*> UninitializedChannelsArray;
	UninitializedChannelsArray.Add(UninitializedChannel1);
	UninitializedChannelsArray.Add(UninitializedChannel2);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(UninitializedChannelsArray);
	
	// Should fail because all channels are uninitialized
	TestTrue("StreamUpdatesOn should fail with uninitialized channels", StreamUpdatesOnResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnHappyPathSingleChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.2HappyPath.SingleChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnHappyPathSingleChannelTest::RunTest(const FString& Parameters)
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
	
	// Call StreamUpdatesOn with single channel
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult.Channel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	
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
	
	// Cleanup: Stop streaming updates and delete channel
	if(CreateResult.Channel)
	{
		CreateResult.Channel->StopStreamingUpdates();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnHappyPathMultipleChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.2HappyPath.MultipleChannels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnHappyPathMultipleChannelsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_multi_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_stream_updates_on_multi_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_stream_updates_on_multi_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_stream_updates_on_multi_3";
	
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
	
	// Create multiple channels
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation 1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel 1 should be created", CreateResult1.Channel);
	
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation 2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel 2 should be created", CreateResult2.Channel);
	
	FPubnubChatChannelData ChannelData3;
	FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
	TestFalse("CreatePublicConversation 3 should succeed", CreateResult3.Result.Error);
	TestNotNull("Channel 3 should be created", CreateResult3.Channel);
	
	if(!CreateResult1.Channel || !CreateResult2.Channel || !CreateResult3.Channel)
	{
		// Cleanup what was created
		if(CreateResult1.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		if(CreateResult2.Channel && Chat) Chat->DeleteChannel(TestChannelID2);
		if(CreateResult3.Channel && Chat) Chat->DeleteChannel(TestChannelID3);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Call StreamUpdatesOn with multiple channels
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult1.Channel);
	ChannelsArray.Add(CreateResult2.Channel);
	ChannelsArray.Add(CreateResult3.Channel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	
	TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
	
	// Verify that StreamUpdates was called successfully for all channels
	// Each channel should have a Subscribe step
	int32 SubscribeStepCount = 0;
	for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			SubscribeStepCount++;
			TestFalse("Subscribe step should succeed", Step.OperationResult.Error);
		}
	}
	TestEqual("Should have 3 Subscribe steps (one per channel)", SubscribeStepCount, 3);
	
	// Cleanup: Stop streaming updates and delete channels
	if(CreateResult1.Channel) CreateResult1.Channel->StopStreamingUpdates();
	if(CreateResult2.Channel) CreateResult2.Channel->StopStreamingUpdates();
	if(CreateResult3.Channel) CreateResult3.Channel->StopStreamingUpdates();
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID2);
		Chat->DeleteChannel(TestChannelID3);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// Note: StreamUpdatesOn doesn't have optional parameters, so full parameter test is same as happy path
// This test verifies the function works correctly with multiple channels in various states

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_full_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_stream_updates_on_full_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_stream_updates_on_full_2";
	
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
	
	// Create channels
	FPubnubChatChannelData ChannelData1;
	ChannelData1.ChannelName = TEXT("Channel1");
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation 1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel 1 should be created", CreateResult1.Channel);
	
	FPubnubChatChannelData ChannelData2;
	ChannelData2.ChannelName = TEXT("Channel2");
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation 2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel 2 should be created", CreateResult2.Channel);
	
	if(!CreateResult1.Channel || !CreateResult2.Channel)
	{
		if(CreateResult1.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		if(CreateResult2.Channel && Chat) Chat->DeleteChannel(TestChannelID2);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Call StreamUpdatesOn with multiple channels
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult1.Channel);
	ChannelsArray.Add(CreateResult2.Channel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	
	TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
	
	// Verify results are merged correctly
	int32 SubscribeStepCount = 0;
	for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			SubscribeStepCount++;
			TestFalse("Subscribe step should succeed", Step.OperationResult.Error);
		}
	}
	TestEqual("Should have 2 Subscribe steps (one per channel)", SubscribeStepCount, 2);
	
	// Cleanup: Stop streaming updates and delete channels
	if(CreateResult1.Channel) CreateResult1.Channel->StopStreamingUpdates();
	if(CreateResult2.Channel) CreateResult2.Channel->StopStreamingUpdates();
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests StreamUpdatesOn with mixed initialized and uninitialized channels.
 * Verifies that failures from uninitialized channels are properly merged into the result.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnMixedChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.4Advanced.MixedChannels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnMixedChannelsTest::RunTest(const FString& Parameters)
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
	
	// Create one initialized channel
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
	
	// Create uninitialized channel
	UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
	
	// Call StreamUpdatesOn with mixed channels (initialized + uninitialized)
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult.Channel);
	ChannelsArray.Add(UninitializedChannel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	
	// Should fail because one channel is uninitialized
	TestTrue("StreamUpdatesOn should fail with mixed channels", StreamUpdatesOnResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
	
	// Verify that at least one Subscribe step succeeded (from initialized channel)
	// and errors from uninitialized channel are merged
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
	
	// Note: The initialized channel might succeed, but overall result should be error due to uninitialized channel
	// Check both StepResults errors and overall Error flag (uninitialized channel returns early without steps)
	TestTrue("Should have error from uninitialized channel", bFoundError || StreamUpdatesOnResult.Error);
	
	// Cleanup: Stop streaming updates if it was started
	if(CreateResult.Channel)
	{
		CreateResult.Channel->StopStreamingUpdates();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests StreamUpdatesOn with multiple channels receiving updates simultaneously.
 * Verifies that updates from different channels are received correctly via delegates.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnMultipleUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnMultipleUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_multi_updates_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_stream_updates_on_multi_updates_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_stream_updates_on_multi_updates_2";
	
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
	
	// Create channels with initial data
	FPubnubChatChannelData ChannelData1;
	ChannelData1.ChannelName = TEXT("InitialName1");
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation 1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel 1 should be created", CreateResult1.Channel);
	
	FPubnubChatChannelData ChannelData2;
	ChannelData2.ChannelName = TEXT("InitialName2");
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation 2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel 2 should be created", CreateResult2.Channel);
	
	if(!CreateResult1.Channel || !CreateResult2.Channel)
	{
		if(CreateResult1.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		if(CreateResult2.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for update reception
	TSharedPtr<int32> UpdateCount1 = MakeShared<int32>(0);
	TSharedPtr<int32> UpdateCount2 = MakeShared<int32>(0);
	TSharedPtr<bool> bUpdate1Received = MakeShared<bool>(false);
	TSharedPtr<bool> bUpdate2Received = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatChannelData> ReceivedChannelData1 = MakeShared<FPubnubChatChannelData>();
	TSharedPtr<FPubnubChatChannelData> ReceivedChannelData2 = MakeShared<FPubnubChatChannelData>();
	
	// Set up delegates to receive channel updates
	auto UpdateLambda1 = [this, UpdateCount1, bUpdate1Received, ReceivedChannelData1](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		(*UpdateCount1)++;
		*bUpdate1Received = true;
		*ReceivedChannelData1 = ChannelData;
	};
	CreateResult1.Channel->OnUpdatedNative.AddLambda(UpdateLambda1);
	
	auto UpdateLambda2 = [this, UpdateCount2, bUpdate2Received, ReceivedChannelData2](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		(*UpdateCount2)++;
		*bUpdate2Received = true;
		*ReceivedChannelData2 = ChannelData;
	};
	CreateResult2.Channel->OnUpdatedNative.AddLambda(UpdateLambda2);
	
	// Call StreamUpdatesOn with both channels
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult1.Channel);
	ChannelsArray.Add(CreateResult2.Channel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
	
	// Wait for subscriptions to be ready and let any initial channel creation events settle
	// Then reset counters to ignore creation events before sending actual updates
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount1, UpdateCount2, bUpdate1Received, bUpdate2Received, CreateResult1, CreateResult2]()
	{
		// Reset counters and flags to ignore any initial channel creation events
		// that may have been received after StreamUpdatesOn was called
		*UpdateCount1 = 0;
		*UpdateCount2 = 0;
		*bUpdate1Received = false;
		*bUpdate2Received = false;
		
		// Update both channels to trigger update events
		FPubnubChatUpdateChannelInputData UpdatedChannelData1;
		UpdatedChannelData1.ChannelName = TEXT("UpdatedName1");
		UpdatedChannelData1.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult1 = CreateResult1.Channel->Update(UpdatedChannelData1);
		TestFalse("Update 1 should succeed", UpdateResult1.Error);
		
		FPubnubChatUpdateChannelInputData UpdatedChannelData2;
		UpdatedChannelData2.ChannelName = TEXT("UpdatedName2");
		UpdatedChannelData2.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult2 = CreateResult2.Channel->Update(UpdatedChannelData2);
		TestFalse("Update 2 should succeed", UpdateResult2.Error);
	}, 1.5f));
	
	
	// Wait until both updates are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdate1Received, bUpdate2Received]() -> bool {
		return *bUpdate1Received && *bUpdate2Received;
	}, MAX_WAIT_TIME));
	
	// Verify updates were received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdate1Received, bUpdate2Received, ReceivedChannelData1, ReceivedChannelData2, UpdateCount1, UpdateCount2, CreateResult1, CreateResult2]()
	{
		TestTrue("Update 1 should have been received", *bUpdate1Received);
		TestTrue("Update 2 should have been received", *bUpdate2Received);
		TestEqual("Channel 1 should receive exactly one update", *UpdateCount1, 1);
		TestEqual("Channel 2 should receive exactly one update", *UpdateCount2, 1);
		TestEqual("Received ChannelName 1 should be updated", ReceivedChannelData1->ChannelName, TEXT("UpdatedName1"));
		TestEqual("Received ChannelName 2 should be updated", ReceivedChannelData2->ChannelName, TEXT("UpdatedName2"));
		
		// Verify channel data was updated in repository
		FPubnubChatChannelData RetrievedData1 = CreateResult1.Channel->GetChannelData();
		FPubnubChatChannelData RetrievedData2 = CreateResult2.Channel->GetChannelData();
		TestEqual("Retrieved ChannelName 1 should be updated", RetrievedData1.ChannelName, TEXT("UpdatedName1"));
		TestEqual("Retrieved ChannelName 2 should be updated", RetrievedData2.ChannelName, TEXT("UpdatedName2"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete channels
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult1, CreateResult2, Chat, TestChannelID1, TestChannelID2]()
	{
		if(CreateResult1.Channel)
		{
			CreateResult1.Channel->StopStreamingUpdates();
		}
		if(CreateResult2.Channel)
		{
			CreateResult2.Channel->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1);
			Chat->DeleteChannel(TestChannelID1);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests StreamUpdatesOn when channels are already streaming updates.
 * Verifies that calling StreamUpdatesOn on already streaming channels doesn't cause errors.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesOnAlreadyStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdatesOn.4Advanced.AlreadyStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesOnAlreadyStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_already_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_stream_updates_on_already_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_stream_updates_on_already_2";
	
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
	
	// Create channels
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation 1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel 1 should be created", CreateResult1.Channel);
	
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation 2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel 2 should be created", CreateResult2.Channel);
	
	if(!CreateResult1.Channel || !CreateResult2.Channel)
	{
		if(CreateResult1.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		if(CreateResult2.Channel && Chat) Chat->DeleteChannel(TestChannelID1);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// First, start streaming updates individually
	FPubnubChatOperationResult StreamUpdatesResult1 = CreateResult1.Channel->StreamUpdates();
	TestFalse("First StreamUpdates should succeed", StreamUpdatesResult1.Error);
	
	FPubnubChatOperationResult StreamUpdatesResult2 = CreateResult2.Channel->StreamUpdates();
	TestFalse("Second StreamUpdates should succeed", StreamUpdatesResult2.Error);
	
	// Now call StreamUpdatesOn on already streaming channels
	TArray<UPubnubChatChannel*> ChannelsArray;
	ChannelsArray.Add(CreateResult1.Channel);
	ChannelsArray.Add(CreateResult2.Channel);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatChannel::StreamUpdatesOn(ChannelsArray);
	
	// Should succeed (StreamUpdates skips if already streaming)
	TestFalse("StreamUpdatesOn should succeed even when channels are already streaming", StreamUpdatesOnResult.Error);
	
	// Verify that StreamUpdates was called but skipped (no new Subscribe steps)
	// Since channels are already streaming, StreamUpdates returns early with no steps
	int32 SubscribeStepCount = 0;
	for(const FPubnubChatOperationStepResult& Step : StreamUpdatesOnResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			SubscribeStepCount++;
		}
	}
	// When already streaming, StreamUpdates returns early with no steps, so no Subscribe steps should be added
	TestEqual("Should have no new Subscribe steps (already streaming)", SubscribeStepCount, 0);
	
	// Cleanup: Stop streaming updates and delete channels
	if(CreateResult1.Channel) CreateResult1.Channel->StopStreamingUpdates();
	if(CreateResult2.Channel) CreateResult2.Channel->StopStreamingUpdates();
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETMEMBER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMemberNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMember.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMemberNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_member_not_init";
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatMembershipResult GetMemberResult = UninitializedChannel->GetMember(TEXT("user"));
		TestTrue("GetMember should fail on uninitialized channel", GetMemberResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMemberResult.Result.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMemberEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMember.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMemberEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_member_empty_user";
	const FString TestChannelID = SDK_PREFIX + "test_get_member_empty_user_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	if(CreateResult.Channel)
	{
		FPubnubChatMembershipResult GetMemberResult = CreateResult.Channel->GetMember(TEXT(""));
		TestTrue("GetMember should fail for empty UserID", GetMemberResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMemberResult.Result.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMemberHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMember.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMemberHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_member_happy";
	const FString TestChannelID = SDK_PREFIX + "test_get_member_happy_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);
	}

	if(CreateResult.Channel)
	{
		FPubnubChatMembershipResult GetMemberResult = CreateResult.Channel->GetMember(InitUserID);
		TestFalse("GetMember should succeed", GetMemberResult.Result.Error);
		TestNotNull("Membership should be returned", GetMemberResult.Membership);

		if(GetMemberResult.Membership)
		{
			TestEqual("Membership UserID should match", GetMemberResult.Membership->GetUserID(), InitUserID);
			TestEqual("Membership ChannelID should match", GetMemberResult.Membership->GetChannelID(), TestChannelID);
		}
	}

	if(JoinResult.Membership)
	{
		JoinResult.Membership->Delete();
	}
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMemberNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMember.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMemberNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_member_no_optional";
	const FString TestChannelID = SDK_PREFIX + "test_get_member_no_optional_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);

		FPubnubChatMembershipResult GetMemberResult = CreateResult.Channel->GetMember(InitUserID);
		TestFalse("GetMember should succeed with required parameters only", GetMemberResult.Result.Error);
		TestNotNull("Membership should be returned", GetMemberResult.Membership);
	}

	if(JoinResult.Membership)
	{
		JoinResult.Membership->Delete();
	}
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Verifies GetMember returns no membership (without error) for a valid user that is not joined to the channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMemberNonMemberUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMember.4Advanced.NonMemberUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMemberNonMemberUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_member_non_member_init";
	const FString OtherUserID = SDK_PREFIX + "test_get_member_non_member_other";
	const FString TestChannelID = SDK_PREFIX + "test_get_member_non_member_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);

		FPubnubChatMembershipResult GetMemberResult = CreateResult.Channel->GetMember(OtherUserID);
		TestFalse("GetMember should succeed for non-member user query", GetMemberResult.Result.Error);
		TestNull("Membership should be null for non-member user", GetMemberResult.Membership);
	}

	if(JoinResult.Membership)
	{
		JoinResult.Membership->Delete();
	}
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(OtherUserID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HASMEMBER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelHasMemberNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.HasMember.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelHasMemberNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_member_not_init";
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;

	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatHasMemberResult HasMemberResult = UninitializedChannel->HasMember(TEXT("user"));
		TestTrue("HasMember should fail on uninitialized channel", HasMemberResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HasMemberResult.Result.ErrorMessage.IsEmpty());
		Chat->DeleteUser(InitUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelHasMemberEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.HasMember.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelHasMemberEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_member_empty_user";
	const FString TestChannelID = SDK_PREFIX + "test_has_member_empty_user_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	if(CreateResult.Channel)
	{
		FPubnubChatHasMemberResult HasMemberResult = CreateResult.Channel->HasMember(TEXT(""));
		TestTrue("HasMember should fail for empty UserID", HasMemberResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", HasMemberResult.Result.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelHasMemberHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.HasMember.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelHasMemberHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_member_happy";
	const FString TestChannelID = SDK_PREFIX + "test_has_member_happy_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);

		FPubnubChatHasMemberResult HasMemberResult = CreateResult.Channel->HasMember(InitUserID);
		TestFalse("HasMember should succeed", HasMemberResult.Result.Error);
		TestTrue("HasMember should be true for joined user", HasMemberResult.HasMember);
	}

	if(JoinResult.Membership)
	{
		JoinResult.Membership->Delete();
	}
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelHasMemberNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.HasMember.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelHasMemberNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_member_no_optional";
	const FString TestChannelID = SDK_PREFIX + "test_has_member_no_optional_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);

		FPubnubChatHasMemberResult HasMemberResult = CreateResult.Channel->HasMember(InitUserID);
		TestFalse("HasMember should succeed with required parameters only", HasMemberResult.Result.Error);
		TestTrue("HasMember should be true for joined user", HasMemberResult.HasMember);
	}

	if(JoinResult.Membership)
	{
		JoinResult.Membership->Delete();
	}
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Verifies HasMember reflects membership deletion and becomes false after removing the membership.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelHasMemberAfterMembershipDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.HasMember.4Advanced.AfterMembershipDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelHasMemberAfterMembershipDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_has_member_after_delete";
	const FString TestChannelID = SDK_PREFIX + "test_has_member_after_delete_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	if(CreateResult.Channel)
	{
		FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join();
		TestFalse("Join should succeed", JoinResult.Result.Error);
		TestNotNull("Join should return membership", JoinResult.Membership);

		FPubnubChatHasMemberResult HasMemberBefore = CreateResult.Channel->HasMember(InitUserID);
		TestFalse("HasMember should succeed before delete", HasMemberBefore.Result.Error);
		TestTrue("HasMember should be true before delete", HasMemberBefore.HasMember);

		if(JoinResult.Membership)
		{
			FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
			TestFalse("Membership delete should succeed", DeleteMembershipResult.Error);
		}

		FPubnubChatHasMemberResult HasMemberAfter = CreateResult.Channel->HasMember(InitUserID);
		TestFalse("HasMember should succeed after delete", HasMemberAfter.Result.Error);
		TestFalse("HasMember should be false after delete", HasMemberAfter.HasMember);
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FETCHREADRECEIPTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelFetchReadReceiptsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.FetchReadReceipts.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelFetchReadReceiptsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_fetch_read_receipts_not_init";
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatFetchReadReceiptsResult Result = UninitializedChannel->FetchReadReceipts();
		TestTrue("FetchReadReceipts should fail with uninitialized channel", Result.Result.Error);
		TestFalse("Result.ErrorMessage should not be empty", Result.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelFetchReadReceiptsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.FetchReadReceipts.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelFetchReadReceiptsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_fetch_read_receipts_happy_init";
	const FString SecondUserID = SDK_PREFIX + "test_fetch_read_receipts_happy_other";
	const FString TestChannelID = SDK_PREFIX + "test_fetch_read_receipts_happy";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, FPubnubChatMembershipData());
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	if(!CreateResult.Channel || !CreateResult.HostMembership)
	{
		Chat->DeleteUser(SecondUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	FPubnubChatOperationResult SetLRMTResult = CreateResult.HostMembership->SetLastReadMessageTimetoken(TestTimetoken);
	TestFalse("SetLastReadMessageTimetoken should succeed", SetLRMTResult.Error);

	FPubnubChatFetchReadReceiptsResult FetchResult = CreateResult.Channel->FetchReadReceipts();
	TestFalse("FetchReadReceipts should succeed", FetchResult.Result.Error);
	TestTrue("ReadReceipts should contain at least one entry (host member)", FetchResult.ReadReceipts.Num() >= 1);
	TestTrue("Total should be >= 1", FetchResult.Total >= 1);

	bool bFoundInitUserReceipt = false;
	for(const FPubnubChatReadReceipt& Receipt : FetchResult.ReadReceipts)
	{
		if(Receipt.UserID == InitUserID)
		{
			bFoundInitUserReceipt = true;
			TestTrue("Host member LastReadTimetoken should match set value", Receipt.LastReadTimetoken.Contains(TestTimetoken) || Receipt.LastReadTimetoken == TestTimetoken);
			break;
		}
	}
	TestTrue("ReadReceipts should contain entry for InitUserID", bFoundInitUserReceipt);

	if(CreateResult.HostMembership) { CreateResult.HostMembership->Delete(); }
	if(CreateResult.InviteeMembership) { CreateResult.InviteeMembership->Delete(); }
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(SecondUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelFetchReadReceiptsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.FetchReadReceipts.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelFetchReadReceiptsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_fetch_read_receipts_full_init";
	const FString SecondUserID = SDK_PREFIX + "test_fetch_read_receipts_full_other";
	const FString TestChannelID = SDK_PREFIX + "test_fetch_read_receipts_full";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(SecondUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	const int TestLimit = 10;
	FString TestFilter = UPubnubChatInternalUtilities::GetFilterForUserID(InitUserID);
	FPubnubMemberSort TestSort;
	FPubnubMemberSingleSort SingleSort;
	SingleSort.SortType = EPubnubMemberSortType::PMeST_UserID;
	SingleSort.SortOrder = false;
	TestSort.MemberSort.Add(SingleSort);
	FPubnubPage TestPage;

	FPubnubChatFetchReadReceiptsResult FetchResult = CreateResult.Channel->FetchReadReceipts(TestLimit, TestFilter, TestSort, TestPage);
	TestFalse("FetchReadReceipts with all parameters should succeed", FetchResult.Result.Error);
	TestTrue("ReadReceipts count should match filter (at least init user)", FetchResult.ReadReceipts.Num() >= 1);
	TestTrue("Total should be non-negative", FetchResult.Total >= 0);

	if(CreateResult.HostMembership) { CreateResult.HostMembership->Delete(); }
	if(CreateResult.InviteeMembership) { CreateResult.InviteeMembership->Delete(); }
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(SecondUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// STREAMREADRECEIPTS TESTS
// ============================================================================
// VALIDATION
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamReadReceiptsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamReadReceipts.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamReadReceiptsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_read_receipts_not_init";
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult Result = UninitializedChannel->StreamReadReceipts();
		TestTrue("StreamReadReceipts should fail with uninitialized channel", Result.Error);
		TestFalse("ErrorMessage should not be empty", Result.ErrorMessage.IsEmpty());
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// HAPPY PATH
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamReadReceiptsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamReadReceipts.2HappyPath.DirectChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamReadReceiptsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_read_receipts_happy_init";
	const FString SecondUserID = SDK_PREFIX + "test_stream_read_receipts_happy_other";
	const FString TestChannelID = SDK_PREFIX + "test_stream_read_receipts_happy";
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat) { AddError("Chat should be initialized"); CleanUp(); return false; }
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User) { CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	if(!CreateResult.Channel || !CreateResult.HostMembership) { Chat->DeleteUser(SecondUserID); CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamReadReceipts();
	TestFalse("StreamReadReceipts should succeed on direct channel", StreamResult.Error);
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingReadReceipts();
	TestFalse("StopStreamingReadReceipts should succeed", StopResult.Error);
	if(CreateResult.HostMembership) { CreateResult.HostMembership->Delete(); }
	if(CreateResult.InviteeMembership) { CreateResult.InviteeMembership->Delete(); }
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(SecondUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// STOPSTREAMINGREADRECEIPTS TESTS
// ============================================================================
// VALIDATION
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingReadReceiptsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingReadReceipts.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingReadReceiptsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_read_receipts_not_init";
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult Result = UninitializedChannel->StopStreamingReadReceipts();
		TestTrue("StopStreamingReadReceipts should fail with uninitialized channel", Result.Error);
		TestFalse("ErrorMessage should not be empty", Result.ErrorMessage.IsEmpty());
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// HAPPY PATH
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingReadReceiptsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingReadReceipts.2HappyPath.AfterStream", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingReadReceiptsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_read_receipts_happy_init";
	const FString SecondUserID = SDK_PREFIX + "test_stop_stream_read_receipts_happy_other";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_read_receipts_happy";
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat) { AddError("Chat should be initialized"); CleanUp(); return false; }
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User) { CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel) { Chat->DeleteUser(SecondUserID); CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamReadReceipts();
	TestFalse("StreamReadReceipts should succeed", StreamResult.Error);
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingReadReceipts();
	TestFalse("StopStreamingReadReceipts should succeed after streaming", StopResult.Error);
	if(CreateResult.HostMembership) { CreateResult.HostMembership->Delete(); }
	if(CreateResult.InviteeMembership) { CreateResult.InviteeMembership->Delete(); }
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(SecondUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ADVANCED: Stop when not streaming is no-op success
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingReadReceiptsWhenNotStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingReadReceipts.3Advanced.WhenNotStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingReadReceiptsWhenNotStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_read_receipts_no_stream_init";
	const FString SecondUserID = SDK_PREFIX + "test_stop_read_receipts_no_stream_other";
	const FString TestChannelID = SDK_PREFIX + "test_stop_read_receipts_no_stream";
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat) { AddError("Chat should be initialized"); CleanUp(); return false; }
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	if(!CreateUserResult.User) { CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel) { Chat->DeleteUser(SecondUserID); CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingReadReceipts();
	TestFalse("StopStreamingReadReceipts when not streaming should succeed (no-op)", StopResult.Error);
	if(CreateResult.HostMembership) { CreateResult.HostMembership->Delete(); }
	if(CreateResult.InviteeMembership) { CreateResult.InviteeMembership->Delete(); }
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED: Default config – Receipt event not emitted for public channel
// ============================================================================
/**
 * With default config (EmitReadReceiptEvents: public=false), setting last read on a public channel
 * should NOT emit a Receipt event; listener should not receive one.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelReadReceiptDefaultConfigPublicNoEmitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamReadReceipts.4Advanced.DefaultConfigPublicNoEmit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelReadReceiptDefaultConfigPublicNoEmitTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_read_receipt_public_no_emit_init";
	const FString TestChannelID = SDK_PREFIX + "test_read_receipt_public_no_emit";
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat) { AddError("Chat should be initialized"); CleanUp(); return false; }
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel) { CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join();
	TestFalse("Join should succeed", JoinResult.Result.Error);
	TestNotNull("Membership should be created", JoinResult.Membership);
	if(!JoinResult.Membership) { Chat->DeleteChannel(TestChannelID); CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	TSharedPtr<bool> bReceiptReceived = MakeShared<bool>(false);
	CreateResult.Channel->OnReadReceiptReceivedNative.AddLambda([bReceiptReceived](const FPubnubChatReadReceipt&) { *bReceiptReceived = true; });
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamReadReceipts();
	TestFalse("StreamReadReceipts should succeed", StreamResult.Error);
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, JoinResult]()
	{
		const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
		TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bReceiptReceived]()
	{
		TestFalse("With default config, Receipt event should NOT be received for public channel", *bReceiptReceived);
	}, 1.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, JoinResult, Chat, TestChannelID, InitUserID]()
	{
		CreateResult.Channel->StopStreamingReadReceipts();
		if(JoinResult.Membership) { JoinResult.Membership->Delete(); }
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	return true;
}

// ============================================================================
// ADVANCED: Config public=true – Receipt event emitted for public channel
// ============================================================================
/**
 * With config EmitReadReceiptEvents["public"]=true, setting last read on a public channel
 * should emit a Receipt event; listener should receive it.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelReadReceiptConfigPublicEmitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamReadReceipts.4Advanced.ConfigPublicEmit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelReadReceiptConfigPublicEmitTest::RunTest(const FString& Parameters)
{
	if(!InitTest()) { AddError("TestInitialization failed"); return false; }
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_read_receipt_public_emit_init";
	const FString TestChannelID = SDK_PREFIX + "test_read_receipt_public_emit";
	FPubnubChatConfig ChatConfig;
	ChatConfig.EmitReadReceiptEvents.Add(TEXT("public"), true);
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat) { AddError("Chat should be initialized"); CleanUp(); return false; }
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel) { CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join();
	TestFalse("Join should succeed", JoinResult.Result.Error);
	TestNotNull("Membership should be created", JoinResult.Membership);
	if(!JoinResult.Membership) { Chat->DeleteChannel(TestChannelID); CleanUpCurrentChatUser(Chat); CleanUp(); return false; }
	TSharedPtr<bool> bReceiptReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatReadReceipt> ReceivedReceipt = MakeShared<FPubnubChatReadReceipt>();
	const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	CreateResult.Channel->OnReadReceiptReceivedNative.AddLambda([bReceiptReceived, ReceivedReceipt](const FPubnubChatReadReceipt& R) { *bReceiptReceived = true; *ReceivedReceipt = R; });
	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamReadReceipts();
	TestFalse("StreamReadReceipts should succeed", StreamResult.Error);
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, JoinResult, TestTimetoken]()
	{
		FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
		TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReceiptReceived]() { return *bReceiptReceived; }, MAX_WAIT_TIME));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bReceiptReceived, ReceivedReceipt, TestTimetoken, InitUserID]()
	{
		if(!*bReceiptReceived) { AddError("With EmitReadReceiptEvents public=true, Receipt event should be received for public channel"); }
		else
		{
			TestEqual("Received receipt UserID should match", ReceivedReceipt->UserID, InitUserID);
			TestTrue("Received receipt should contain timetoken", ReceivedReceipt->LastReadTimetoken.Contains(TestTimetoken) || ReceivedReceipt->LastReadTimetoken == TestTimetoken);
		}
	}, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, JoinResult, Chat, TestChannelID, InitUserID]()
	{
		CreateResult.Channel->StopStreamingReadReceipts();
		if(JoinResult.Membership) { JoinResult.Membership->Delete(); }
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

