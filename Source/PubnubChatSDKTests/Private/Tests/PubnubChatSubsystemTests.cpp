// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatEmptyPublishKeyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.1Validation.EmptyPublishKey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatEmptyPublishKeyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}
	
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_empty_pubkey";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TEXT(""), TestSubscribeKey, TestUserID, ChatConfig);
	
	TestTrue("InitChat should fail with empty PublishKey", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatEmptySubscribeKeyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.1Validation.EmptySubscribeKey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatEmptySubscribeKeyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}
	
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestUserID = SDK_PREFIX + "test_user_empty_subkey";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TEXT(""), TestUserID, ChatConfig);
	
	TestTrue("InitChat should fail with empty SubscribeKey", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}
	
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TEXT(""), ChatConfig);
	
	TestTrue("InitChat should fail with empty UserID", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatAllEmptyFieldsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.1Validation.AllEmptyFields", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatAllEmptyFieldsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TEXT(""), TEXT(""), TEXT(""), ChatConfig);
	
	TestTrue("InitChat should fail with all empty fields", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_happy_path";
	
	// Use default Config (empty struct)
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	TestEqual("Chat object should match GetChat", ChatSubsystem->GetChat(), InitResult.Chat);
	
	// Verify Chat is properly initialized using reflection
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	TestNotNull("Chat should exist", Chat);
	
	if(Chat)
	{
		// Verify CurrentUser exists
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("Current user should exist", CurrentUser);
		
		// Verify using reflection that CurrentUser matches
		UPubnubChatUser* ReflectedUser = GetCurrentUserFromChat(Chat);
		TestEqual("Reflected CurrentUser should match GetCurrentUser", ReflectedUser, CurrentUser);
		
		// Verify PubnubClient was created
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		TestNotNull("PubnubClient should be created", PubnubClient);
		
		// Verify IsInitialized flag
		bool bIsInitialized = GetIsInitializedFromChat(Chat);
		TestTrue("Chat should be initialized", bIsInitialized);
		
		// Verify repository is created during InitChat
		UPubnubChatObjectsRepository* Repository = GetObjectsRepositoryFromChat(Chat);
		TestNotNull("Repository should be created during InitChat", Repository);
		
		// Verify subsystem's internal Chat pointer matches
		UPubnubChat* SubsystemChat = GetChatFromSubsystem(ChatSubsystem);
		TestEqual("Subsystem's internal Chat should match returned Chat", SubsystemChat, Chat);
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Config Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatFullConfigTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.3FullConfig.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatFullConfigTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_full_config";
	
	// Set all Config parameters
	FPubnubChatConfig ChatConfig;
	ChatConfig.AuthKey = TEXT("test_auth_key");
	ChatConfig.TypingTimeout = 10000;
	ChatConfig.TypingTimeoutDifference = 2000;
	ChatConfig.StoreUserActivityInterval = 300000;
	ChatConfig.StoreUserActivityTimestamps = true;
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed with full config", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify Chat is properly initialized
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("Current user should exist", CurrentUser);
		
		// Use reflection helper to verify config was set correctly
		FPubnubChatConfig StoredConfig = GetChatConfigFromChat(Chat);
		TestEqual("AuthKey should be stored correctly", StoredConfig.AuthKey, ChatConfig.AuthKey);
		TestEqual("TypingTimeout should be stored correctly", StoredConfig.TypingTimeout, ChatConfig.TypingTimeout);
		TestEqual("TypingTimeoutDifference should be stored correctly", StoredConfig.TypingTimeoutDifference, ChatConfig.TypingTimeoutDifference);
		TestEqual("StoreUserActivityInterval should be stored correctly", StoredConfig.StoreUserActivityInterval, ChatConfig.StoreUserActivityInterval);
		TestEqual("StoreUserActivityTimestamps should be stored correctly", StoredConfig.StoreUserActivityTimestamps, ChatConfig.StoreUserActivityTimestamps);
		
		// Verify PubnubClient was created using reflection
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		TestNotNull("PubnubClient should be created", PubnubClient);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests calling InitChat multiple times with the same parameters.
 * Verifies that subsequent calls return the existing Chat object and indicate that chat already exists.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatDuplicateCallTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.DuplicateCall", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatDuplicateCallTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_duplicate";
	
	FPubnubChatConfig ChatConfig;
	
	// First InitChat call
	FPubnubChatInitChatResult FirstResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("First InitChat should succeed", FirstResult.Result.Error);
	TestNotNull("First Chat object should be created", FirstResult.Chat);
	
	UPubnubChat* FirstChat = FirstResult.Chat;
	
	// Second InitChat call with same parameters
	FPubnubChatInitChatResult SecondResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	// Should return existing chat (currently sets Error=true but returns Chat)
	TestNotNull("Second InitChat should return existing Chat", SecondResult.Chat);
	TestEqual("Second result should return same Chat object", SecondResult.Chat, FirstChat);
	TestEqual("GetChat should return same object", ChatSubsystem->GetChat(), FirstChat);
	
	// Verify the result indicates it's an existing chat
	// Note: Current implementation sets Error=true when chat exists, but returns the chat
	TestTrue("Result should indicate existing chat", SecondResult.Result.Error);
	TestFalse("ErrorMessage should indicate existing chat", SecondResult.Result.ErrorMessage.IsEmpty());

	CleanUp();
	return true;
}

/**
 * Tests that InitChat properly tracks step results for internal operations.
 * Verifies that step results contain expected operations like GetUserMetadata and SetUserMetadata.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatStepResultsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.StepResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatStepResultsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_step_results";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	// Verify step results are tracked
	TestTrue("StepResults should contain at least one step", InitResult.Result.StepResults.Num() > 0);
	
	// Check that step results contain expected operations
	bool bFoundGetUserMetadata = false;
	bool bFoundSetUserMetadata = false;
	
	for(const FPubnubChatOperationStepResult& Step : InitResult.Result.StepResults)
	{
		if(Step.StepName == TEXT("GetUserMetadata"))
		{
			bFoundGetUserMetadata = true;
			TestFalse("GetUserMetadata step should not have error", Step.OperationResult.Error);
		}
		else if(Step.StepName == TEXT("SetUserMetadata"))
		{
			bFoundSetUserMetadata = true;
			TestFalse("SetUserMetadata step should not have error", Step.OperationResult.Error);
		}
	}
	
	// At least one of these should be present (depending on whether user exists)
	TestTrue("Should have either GetUserMetadata or SetUserMetadata step", bFoundGetUserMetadata || bFoundSetUserMetadata);

	CleanUp();
	return true;
}

/**
 * Tests InitChat with different UserIDs after destroying previous chat.
 * Verifies that destroying and reinitializing with a different UserID creates a new Chat object with the correct user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatDifferentUserIDsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.DifferentUserIDs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatDifferentUserIDsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	
	FPubnubChatConfig ChatConfig;
	
	// First InitChat with UserID1
	const FString TestUserID1 = SDK_PREFIX + "test_user_1";
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestNotNull("First Chat should be created", Result1.Chat);
	
	UPubnubChat* Chat1 = ChatSubsystem->GetChat();
	if(Chat1)
	{
		UPubnubChatUser* User1 = Chat1->GetCurrentUser();
		TestNotNull("User1 should exist", User1);
	}
	
	// Destroy first chat
	ChatSubsystem->DestroyChat();
	
	// Second InitChat with UserID2
	const FString TestUserID2 = SDK_PREFIX + "test_user_2";
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	TestNotNull("Second Chat should be created", Result2.Chat);
	
	UPubnubChat* Chat2 = ChatSubsystem->GetChat();
	if(Chat2)
	{
		UPubnubChatUser* User2 = Chat2->GetCurrentUser();
		TestNotNull("User2 should exist", User2);
	}
	
	// Verify they are different objects
	TestNotEqual("Chat objects should be different", Chat1, Chat2);

	CleanUp();
	return true;
}

/**
 * Tests that ChatConfig parameters are preserved and stored correctly after InitChat.
 * Verifies that all config values (AuthKey, TypingTimeout, etc.) are stored and can be retrieved from the Chat object.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatConfigPreservationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.ConfigPreservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatConfigPreservationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_config_preservation";
	
	// Set specific config values
	FPubnubChatConfig ChatConfig;
	ChatConfig.AuthKey = TEXT("custom_auth_key_123");
	ChatConfig.TypingTimeout = 15000;
	ChatConfig.TypingTimeoutDifference = 3000;
	ChatConfig.StoreUserActivityInterval = 900000;
	ChatConfig.StoreUserActivityTimestamps = true;
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify config was preserved using reflection
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	TestNotNull("Chat should exist", Chat);
	
	if(Chat)
	{
		// Use reflection to verify config was stored correctly
		FPubnubChatConfig StoredConfig = GetChatConfigFromChat(Chat);
		TestEqual("AuthKey should be preserved", StoredConfig.AuthKey, ChatConfig.AuthKey);
		TestEqual("TypingTimeout should be preserved", StoredConfig.TypingTimeout, ChatConfig.TypingTimeout);
		TestEqual("TypingTimeoutDifference should be preserved", StoredConfig.TypingTimeoutDifference, ChatConfig.TypingTimeoutDifference);
		TestEqual("StoreUserActivityInterval should be preserved", StoredConfig.StoreUserActivityInterval, ChatConfig.StoreUserActivityInterval);
		TestEqual("StoreUserActivityTimestamps should be preserved", StoredConfig.StoreUserActivityTimestamps, ChatConfig.StoreUserActivityTimestamps);
		
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("Current user should exist", CurrentUser);
	}

	CleanUp();
	return true;
}

/**
 * Tests subsystem state management throughout the Chat lifecycle.
 * Verifies that GetChat returns null before InitChat, returns Chat after InitChat, and returns null after DestroyChat.
 * Also verifies internal subsystem state using reflection.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatSubsystemStateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.SubsystemState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatSubsystemStateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_subsystem_state";
	
	FPubnubChatConfig ChatConfig;
	
	// Test that GetChat returns null before InitChat
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat();
	TestNull("GetChat should return null before InitChat", ChatBeforeInit);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatBefore = GetChatFromSubsystem(ChatSubsystem);
	TestNull("Subsystem's internal Chat should be null before InitChat", SubsystemChatBefore);
	
	// InitChat
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Test that GetChat returns the Chat after InitChat
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat();
	TestNotNull("GetChat should return Chat after InitChat", ChatAfterInit);
	TestEqual("GetChat should return same Chat as InitChat result", ChatAfterInit, InitResult.Chat);
	
	// Verify subsystem's internal Chat matches using reflection
	UPubnubChat* SubsystemChatAfter = GetChatFromSubsystem(ChatSubsystem);
	TestNotNull("Subsystem's internal Chat should exist after InitChat", SubsystemChatAfter);
	TestEqual("Subsystem's internal Chat should match GetChat", SubsystemChatAfter, ChatAfterInit);
	TestEqual("Subsystem's internal Chat should match InitChat result", SubsystemChatAfter, InitResult.Chat);
	
	// Destroy Chat
	ChatSubsystem->DestroyChat();
	
	// Test that GetChat returns null after DestroyChat
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat();
	TestNull("GetChat should return null after DestroyChat", ChatAfterDestroy);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatAfterDestroy = GetChatFromSubsystem(ChatSubsystem);
	TestNull("Subsystem's internal Chat should be null after DestroyChat", SubsystemChatAfterDestroy);

	CleanUp();
	return true;
}

/**
 * Tests that CurrentUser is properly created and accessible after InitChat.
 * Verifies that CurrentUser exists both via public API and reflection, and that both references match.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatCurrentUserVerificationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChat.4Advanced.CurrentUserVerification", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatCurrentUserVerificationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_verification";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Verify CurrentUser exists via public API
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("CurrentUser should exist", CurrentUser);
		
		// Verify CurrentUser exists via reflection
		UPubnubChatUser* ReflectedUser = GetCurrentUserFromChat(Chat);
		TestNotNull("Reflected CurrentUser should exist", ReflectedUser);
		TestEqual("Reflected CurrentUser should match GetCurrentUser", ReflectedUser, CurrentUser);
		
		// Verify CurrentUser's UserID matches the input UserID
		if(CurrentUser)
		{
			// Note: We'd need to check if UPubnubChatUser has a GetUserID method or use reflection
			// For now, we verify the user exists and matches
			TestTrue("CurrentUser should be valid", CurrentUser->IsValidLowLevel());
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// GETCHAT TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemGetChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.GetChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemGetChatTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// GetChat should return nullptr before InitChat
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat();
	TestNull("Chat is null before InitChat", ChatBeforeInit);
	
	// Initialize Chat - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_get";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		return false;
	}

	// GetChat should return the chat object after InitChat
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat();
	TestNotNull("Chat exists after InitChat", ChatAfterInit);
	TestEqual("GetChat returns same object", ChatAfterInit, InitResult.Chat);

	CleanUp();
	return true;
}

// ============================================================================
// DESTROYCHAT TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.DestroyChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyChatTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Initialize Chat - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_destroy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		return false;
	}

	// Verify chat exists
	UPubnubChat* ChatBeforeDestroy = ChatSubsystem->GetChat();
	TestNotNull("Chat exists before destroy", ChatBeforeDestroy);
	
	// Verify subsystem's internal Chat exists using reflection
	UPubnubChat* SubsystemChatBefore = GetChatFromSubsystem(ChatSubsystem);
	TestNotNull("Subsystem's internal Chat should exist before destroy", SubsystemChatBefore);
	TestEqual("Subsystem's internal Chat should match GetChat", SubsystemChatBefore, ChatBeforeDestroy);

	// Destroy chat
	ChatSubsystem->DestroyChat();

	// Verify chat is destroyed
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat();
	TestNull("Chat is null after destroy", ChatAfterDestroy);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatAfter = GetChatFromSubsystem(ChatSubsystem);
	TestNull("Subsystem's internal Chat should be null after destroy", SubsystemChatAfter);

	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

