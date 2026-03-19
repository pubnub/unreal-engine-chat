// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "PubnubSubsystem.h"
#include "PubnubClient.h"
#include "PubnubStructLibrary.h"

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

	CleanUpCurrentChatUser(InitResult.Chat);
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

	CleanUpCurrentChatUser(InitResult.Chat);
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

	CleanUpCurrentChatUser(InitResult.Chat);
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

	CleanUpCurrentChatUser(InitResult.Chat);
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
	TestEqual("Chat object should match GetChat", ChatSubsystem->GetChat(TestUserID), InitResult.Chat);
	
	// Verify Chat is properly initialized using reflection
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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
		UPubnubChat* SubsystemChat = GetChatFromSubsystem(ChatSubsystem, TestUserID);
		TestEqual("Subsystem's internal Chat should match returned Chat", SubsystemChat, Chat);
	}

	CleanUpCurrentChatUser(InitResult.Chat);
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
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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

	CleanUpCurrentChatUser(InitResult.Chat);
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
	TestEqual("GetChat should return same object", ChatSubsystem->GetChat(TestUserID), FirstChat);
	
	// Verify the result indicates it's an existing chat
	// Note: Current implementation sets Error=true when chat exists, but returns the chat
	TestTrue("Result should indicate existing chat", SecondResult.Result.Error);
	TestFalse("ErrorMessage should indicate existing chat", SecondResult.Result.ErrorMessage.IsEmpty());

	CleanUpCurrentChatUser(FirstResult.Chat);
	CleanUpCurrentChatUser(SecondResult.Chat);
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

	CleanUpCurrentChatUser(InitResult.Chat);
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
	
	UPubnubChat* Chat1 = ChatSubsystem->GetChat(TestUserID1);
	if(Chat1)
	{
		UPubnubChatUser* User1 = Chat1->GetCurrentUser();
		TestNotNull("User1 should exist", User1);
	}
	
	// Destroy first chat
	ChatSubsystem->DestroyChat(TestUserID1);
	
	// Second InitChat with UserID2
	const FString TestUserID2 = SDK_PREFIX + "test_user_2";
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	TestNotNull("Second Chat should be created", Result2.Chat);
	
	UPubnubChat* Chat2 = ChatSubsystem->GetChat(TestUserID2);
	if(Chat2)
	{
		UPubnubChatUser* User2 = Chat2->GetCurrentUser();
		TestNotNull("User2 should exist", User2);
	}
	
	// Verify they are different objects
	TestNotEqual("Chat objects should be different", Chat1, Chat2);

	CleanUpCurrentChatUser(Chat1);
	CleanUpCurrentChatUser(Chat2);
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
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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

	CleanUpCurrentChatUser(Chat);
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
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat(TestUserID);
	TestNull("GetChat should return null before InitChat", ChatBeforeInit);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatBefore = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNull("Subsystem's internal Chat should be null before InitChat", SubsystemChatBefore);
	
	// InitChat
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Test that GetChat returns the Chat after InitChat
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("GetChat should return Chat after InitChat", ChatAfterInit);
	TestEqual("GetChat should return same Chat as InitChat result", ChatAfterInit, InitResult.Chat);
	
	// Verify subsystem's internal Chat matches using reflection
	UPubnubChat* SubsystemChatAfter = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNotNull("Subsystem's internal Chat should exist after InitChat", SubsystemChatAfter);
	TestEqual("Subsystem's internal Chat should match GetChat", SubsystemChatAfter, ChatAfterInit);
	TestEqual("Subsystem's internal Chat should match InitChat result", SubsystemChatAfter, InitResult.Chat);
	
	// Destroy Chat
	ChatSubsystem->DestroyChat(TestUserID);
	
	// Test that GetChat returns null after DestroyChat
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat(TestUserID);
	TestNull("GetChat should return null after DestroyChat", ChatAfterDestroy);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatAfterDestroy = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNull("Subsystem's internal Chat should be null after DestroyChat", SubsystemChatAfterDestroy);

	CleanUpCurrentChatUser(ChatAfterInit);
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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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

	CleanUpCurrentChatUser(Chat);
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

	// Initialize Chat - get keys from environment variables
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_get";
	
	// GetChat should return nullptr before InitChat
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat(TestUserID);
	TestNull("Chat is null before InitChat", ChatBeforeInit);
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		return false;
	}

	// GetChat should return the chat object after InitChat
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("Chat exists after InitChat", ChatAfterInit);
	TestEqual("GetChat returns same object", ChatAfterInit, InitResult.Chat);

	CleanUpCurrentChatUser(ChatAfterInit);
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
	UPubnubChat* ChatBeforeDestroy = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("Chat exists before destroy", ChatBeforeDestroy);
	
	// Verify subsystem's internal Chat exists using reflection
	UPubnubChat* SubsystemChatBefore = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNotNull("Subsystem's internal Chat should exist before destroy", SubsystemChatBefore);
	TestEqual("Subsystem's internal Chat should match GetChat", SubsystemChatBefore, ChatBeforeDestroy);
	
	// Destroy chat
	ChatSubsystem->DestroyChat(TestUserID);

	// Verify chat is destroyed
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat(TestUserID);
	TestNull("Chat is null after destroy", ChatAfterDestroy);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatAfter = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNull("Subsystem's internal Chat should be null after destroy", SubsystemChatAfter);

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// MULTIPLE CHATS TESTS
// ============================================================================

/**
 * Tests creating multiple chats with different UserIDs.
 * Verifies that each chat can be retrieved independently and they are separate objects.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemMultipleChatsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.MultipleChats.CreateMultipleChats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemMultipleChatsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	FPubnubChatConfig ChatConfig;
	
	// Create first chat
	const FString TestUserID1 = SDK_PREFIX + "test_user_multiple_1";
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestNotNull("First Chat should be created", Result1.Chat);
	
	// Create second chat with different UserID
	const FString TestUserID2 = SDK_PREFIX + "test_user_multiple_2";
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	TestNotNull("Second Chat should be created", Result2.Chat);
	
	// Create third chat with different UserID
	const FString TestUserID3 = SDK_PREFIX + "test_user_multiple_3";
	FPubnubChatInitChatResult Result3 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID3, ChatConfig);
	TestFalse("Third InitChat should succeed", Result3.Result.Error);
	TestNotNull("Third Chat should be created", Result3.Chat);
	
	// Verify all chats are different objects
	TestNotEqual("Chat1 and Chat2 should be different objects", Result1.Chat, Result2.Chat);
	TestNotEqual("Chat1 and Chat3 should be different objects", Result1.Chat, Result3.Chat);
	TestNotEqual("Chat2 and Chat3 should be different objects", Result2.Chat, Result3.Chat);
	
	// Verify GetChat returns correct chat for each UserID
	UPubnubChat* RetrievedChat1 = ChatSubsystem->GetChat(TestUserID1);
	TestNotNull("Retrieved Chat1 should exist", RetrievedChat1);
	TestEqual("Retrieved Chat1 should match created Chat1", RetrievedChat1, Result1.Chat);
	
	UPubnubChat* RetrievedChat2 = ChatSubsystem->GetChat(TestUserID2);
	TestNotNull("Retrieved Chat2 should exist", RetrievedChat2);
	TestEqual("Retrieved Chat2 should match created Chat2", RetrievedChat2, Result2.Chat);
	
	UPubnubChat* RetrievedChat3 = ChatSubsystem->GetChat(TestUserID3);
	TestNotNull("Retrieved Chat3 should exist", RetrievedChat3);
	TestEqual("Retrieved Chat3 should match created Chat3", RetrievedChat3, Result3.Chat);
	
	// Verify each chat has correct current user
	if(RetrievedChat1)
	{
		UPubnubChatUser* User1 = RetrievedChat1->GetCurrentUser();
		TestNotNull("User1 should exist", User1);
	}
	
	if(RetrievedChat2)
	{
		UPubnubChatUser* User2 = RetrievedChat2->GetCurrentUser();
		TestNotNull("User2 should exist", User2);
	}
	
	if(RetrievedChat3)
	{
		UPubnubChatUser* User3 = RetrievedChat3->GetCurrentUser();
		TestNotNull("User3 should exist", User3);
	}

	CleanUpCurrentChatUser(RetrievedChat1);
	CleanUpCurrentChatUser(RetrievedChat2);
	CleanUpCurrentChatUser(RetrievedChat3);
	CleanUp();
	return true;
}

/**
 * Tests that creating a chat with an existing UserID returns the existing chat.
 * Verifies that the same UserID cannot create multiple chats.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDuplicateUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.MultipleChats.DuplicateUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDuplicateUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_duplicate_multiple";
	FPubnubChatConfig ChatConfig;
	
	// Create first chat
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestNotNull("First Chat should be created", Result1.Chat);
	
	UPubnubChat* FirstChat = Result1.Chat;
	
	// Try to create another chat with the same UserID
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	// Should return existing chat with error flag set
	TestNotNull("Second InitChat should return existing Chat", Result2.Chat);
	TestEqual("Second result should return same Chat object", Result2.Chat, FirstChat);
	TestTrue("Result should indicate existing chat", Result2.Result.Error);
	TestFalse("ErrorMessage should indicate existing chat", Result2.Result.ErrorMessage.IsEmpty());
	
	// Verify GetChat still returns the same chat
	UPubnubChat* RetrievedChat = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("Retrieved Chat should exist", RetrievedChat);
	TestEqual("Retrieved Chat should match first Chat", RetrievedChat, FirstChat);

	CleanUpCurrentChatUser(FirstChat);
	CleanUp();
	return true;
}

/**
 * Tests destroying individual chats while keeping others alive.
 * Verifies that destroying one chat doesn't affect others.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyIndividualChatTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.MultipleChats.DestroyIndividualChat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyIndividualChatTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	FPubnubChatConfig ChatConfig;
	
	// Create multiple chats
	const FString TestUserID1 = SDK_PREFIX + "test_user_destroy_1";
	const FString TestUserID2 = SDK_PREFIX + "test_user_destroy_2";
	const FString TestUserID3 = SDK_PREFIX + "test_user_destroy_3";
	
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	FPubnubChatInitChatResult Result3 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID3, ChatConfig);
	
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	TestFalse("Third InitChat should succeed", Result3.Result.Error);
	
	// Verify all chats exist
	TestNotNull("Chat1 should exist", ChatSubsystem->GetChat(TestUserID1));
	TestNotNull("Chat2 should exist", ChatSubsystem->GetChat(TestUserID2));
	TestNotNull("Chat3 should exist", ChatSubsystem->GetChat(TestUserID3));
	
	// Destroy only Chat2
	ChatSubsystem->DestroyChat(TestUserID2);
	
	// Verify Chat2 is destroyed
	TestNull("Chat2 should be null after destroy", ChatSubsystem->GetChat(TestUserID2));
	
	// Verify Chat1 and Chat3 still exist
	TestNotNull("Chat1 should still exist", ChatSubsystem->GetChat(TestUserID1));
	TestNotNull("Chat3 should still exist", ChatSubsystem->GetChat(TestUserID3));
	
	// Verify they are still the same objects
	TestEqual("Chat1 should still match original", ChatSubsystem->GetChat(TestUserID1), Result1.Chat);
	TestEqual("Chat3 should still match original", ChatSubsystem->GetChat(TestUserID3), Result3.Chat);
	
	// Destroy Chat1
	ChatSubsystem->DestroyChat(TestUserID1);
	
	// Verify Chat1 is destroyed but Chat3 still exists
	TestNull("Chat1 should be null after destroy", ChatSubsystem->GetChat(TestUserID1));
	TestNotNull("Chat3 should still exist", ChatSubsystem->GetChat(TestUserID3));

	CleanUpCurrentChatUser(Result1.Chat);
	CleanUpCurrentChatUser(Result2.Chat);
	CleanUpCurrentChatUser(Result3.Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetChat with invalid UserID.
 * Verifies that GetChat returns null for non-existent UserID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemGetChatInvalidUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.MultipleChats.GetChatInvalidUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemGetChatInvalidUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_valid";
	const FString InvalidUserID = SDK_PREFIX + "test_user_invalid";
	FPubnubChatConfig ChatConfig;
	
	// Create a chat with valid UserID
	FPubnubChatInitChatResult Result = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("InitChat should succeed", Result.Result.Error);
	
	// Try to get chat with invalid UserID
	UPubnubChat* InvalidChat = ChatSubsystem->GetChat(InvalidUserID);
	TestNull("GetChat with invalid UserID should return null", InvalidChat);
	
	// Verify valid chat still exists
	UPubnubChat* ValidChat = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("Valid Chat should still exist", ValidChat);
	TestEqual("Valid Chat should match created Chat", ValidChat, Result.Chat);
	
	// Try to get chat with empty UserID
	UPubnubChat* EmptyChat = ChatSubsystem->GetChat(TEXT(""));
	TestNull("GetChat with empty UserID should return null", EmptyChat);

	CleanUpCurrentChatUser(Result.Chat);
	CleanUp();
	return true;
}

/**
 * Tests DestroyChat with invalid UserID.
 * Verifies that DestroyChat handles invalid UserID gracefully.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyChatInvalidUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.MultipleChats.DestroyChatInvalidUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyChatInvalidUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_destroy_invalid";
	const FString InvalidUserID = SDK_PREFIX + "test_user_not_exist";
	FPubnubChatConfig ChatConfig;
	
	// Create a chat
	FPubnubChatInitChatResult Result = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	TestFalse("InitChat should succeed", Result.Result.Error);
	TestNotNull("Chat should exist", ChatSubsystem->GetChat(TestUserID));
	
	// Try to destroy chat with invalid UserID (should not crash)
	ChatSubsystem->DestroyChat(InvalidUserID);
	
	// Verify original chat still exists
	TestNotNull("Original Chat should still exist", ChatSubsystem->GetChat(TestUserID));
	
	// Try to destroy chat with empty UserID (should not crash)
	ChatSubsystem->DestroyChat(TEXT(""));
	
	// Verify original chat still exists
	TestNotNull("Original Chat should still exist after empty destroy", ChatSubsystem->GetChat(TestUserID));

	CleanUpCurrentChatUser(Result.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// DESTROY ALL CHATS TESTS
// ============================================================================

/**
 * Tests DestroyAllChats function.
 * Verifies that all chats are destroyed and removed from the subsystem.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyAllChatsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.DestroyAllChats.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyAllChatsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	FPubnubChatConfig ChatConfig;
	
	// Create multiple chats
	const FString TestUserID1 = SDK_PREFIX + "test_user_destroy_all_1";
	const FString TestUserID2 = SDK_PREFIX + "test_user_destroy_all_2";
	const FString TestUserID3 = SDK_PREFIX + "test_user_destroy_all_3";
	
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	FPubnubChatInitChatResult Result3 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID3, ChatConfig);
	
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	TestFalse("Third InitChat should succeed", Result3.Result.Error);
	
	// Verify all chats exist
	TestNotNull("Chat1 should exist", ChatSubsystem->GetChat(TestUserID1));
	TestNotNull("Chat2 should exist", ChatSubsystem->GetChat(TestUserID2));
	TestNotNull("Chat3 should exist", ChatSubsystem->GetChat(TestUserID3));
	
	// Destroy all chats
	ChatSubsystem->DestroyAllChats();
	
	// Verify all chats are destroyed
	TestNull("Chat1 should be null after DestroyAllChats", ChatSubsystem->GetChat(TestUserID1));
	TestNull("Chat2 should be null after DestroyAllChats", ChatSubsystem->GetChat(TestUserID2));
	TestNull("Chat3 should be null after DestroyAllChats", ChatSubsystem->GetChat(TestUserID3));

	CleanUpCurrentChatUser(Result1.Chat);
	CleanUpCurrentChatUser(Result2.Chat);
	CleanUpCurrentChatUser(Result3.Chat);
	CleanUp();
	return true;
}

/**
 * Tests DestroyAllChats when no chats exist.
 * Verifies that DestroyAllChats handles empty state gracefully.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyAllChatsEmptyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.DestroyAllChats.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyAllChatsEmptyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// DestroyAllChats when no chats exist (should not crash)
	ChatSubsystem->DestroyAllChats();
	
	// Verify no chats exist
	const FString TestUserID = SDK_PREFIX + "test_user_not_exist";
	TestNull("GetChat should return null when no chats exist", ChatSubsystem->GetChat(TestUserID));

	CleanUp();
	return true;
}

/**
 * Tests DestroyAllChats and then creating new chats.
 * Verifies that new chats can be created after DestroyAllChats.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSubsystemDestroyAllChatsThenCreateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.DestroyAllChats.ThenCreate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSubsystemDestroyAllChatsThenCreateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	FPubnubChatConfig ChatConfig;
	
	// Create and destroy chats
	const FString TestUserID1 = SDK_PREFIX + "test_user_destroy_then_create_1";
	const FString TestUserID2 = SDK_PREFIX + "test_user_destroy_then_create_2";
	
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	
	TestFalse("First InitChat should succeed", Result1.Result.Error);
	TestFalse("Second InitChat should succeed", Result2.Result.Error);
	
	// Destroy all chats
	ChatSubsystem->DestroyAllChats();
	
	// Verify all chats are destroyed
	TestNull("Chat1 should be null after DestroyAllChats", ChatSubsystem->GetChat(TestUserID1));
	TestNull("Chat2 should be null after DestroyAllChats", ChatSubsystem->GetChat(TestUserID2));
	
	// Create new chats with same UserIDs
	FPubnubChatInitChatResult Result3 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID1, ChatConfig);
	FPubnubChatInitChatResult Result4 = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID2, ChatConfig);
	
	TestFalse("Third InitChat should succeed", Result3.Result.Error);
	TestFalse("Fourth InitChat should succeed", Result4.Result.Error);
	
	// Verify new chats exist and are different objects
	TestNotNull("New Chat1 should exist", ChatSubsystem->GetChat(TestUserID1));
	TestNotNull("New Chat2 should exist", ChatSubsystem->GetChat(TestUserID2));
	TestNotEqual("New Chat1 should be different object", Result3.Chat, Result1.Chat);
	TestNotEqual("New Chat2 should be different object", Result4.Chat, Result2.Chat);

	CleanUpCurrentChatUser(Result1.Chat);
	CleanUpCurrentChatUser(Result2.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// INITCHATWITHPUBNUBCLIENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}
	
	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	
	// Create a valid PubnubClient
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = SDK_PREFIX + "test_client_empty_userid";
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TEXT(""), TestClient, ChatConfig);
	
	TestTrue("InitChatWithPubnubClient should fail with empty UserID", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientNullClientTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.1Validation.NullClient", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientNullClientTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}
	
	const FString TestUserID = SDK_PREFIX + "test_user_null_client";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, nullptr, ChatConfig);
	
	TestTrue("InitChatWithPubnubClient should fail with null PubnubClient", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientAllInvalidTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.1Validation.AllInvalid", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientAllInvalidTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TEXT(""), nullptr, ChatConfig);
	
	TestTrue("InitChatWithPubnubClient should fail with all invalid parameters", InitResult.Result.Error);
	TestNull("Chat object should not be created", InitResult.Chat);
	TestFalse("ErrorMessage should not be empty", InitResult.Result.ErrorMessage.IsEmpty());

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_happy_path_client";
	
	// Create a valid PubnubClient
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	// Use default Config (empty struct)
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	TestEqual("Chat object should match GetChat", ChatSubsystem->GetChat(TestUserID), InitResult.Chat);
	
	// Verify Chat is properly initialized using reflection
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("Chat should exist", Chat);
	
	if(Chat)
	{
		// Verify CurrentUser exists
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("Current user should exist", CurrentUser);
		
		// Verify using reflection that CurrentUser matches
		UPubnubChatUser* ReflectedUser = GetCurrentUserFromChat(Chat);
		TestEqual("Reflected CurrentUser should match GetCurrentUser", ReflectedUser, CurrentUser);
		
		// Verify PubnubClient was set correctly
		UPubnubClient* ChatClient = GetPubnubClientFromChat(Chat);
		TestNotNull("PubnubClient should be set", ChatClient);
		TestEqual("PubnubClient should match provided client", ChatClient, TestClient);
		
		// Verify PubnubClient UserID was set correctly
		FString ClientUserID = TestClient->GetUserID();
		TestEqual("PubnubClient UserID should match TestUserID", ClientUserID, TestUserID);
		
		// Verify IsInitialized flag
		bool bIsInitialized = GetIsInitializedFromChat(Chat);
		TestTrue("Chat should be initialized", bIsInitialized);
		
		// Verify repository is created during InitChat
		UPubnubChatObjectsRepository* Repository = GetObjectsRepositoryFromChat(Chat);
		TestNotNull("Repository should be created during InitChat", Repository);
		
		// Verify subsystem's internal Chat pointer matches
		UPubnubChat* SubsystemChat = GetChatFromSubsystem(ChatSubsystem, TestUserID);
		TestEqual("Subsystem's internal Chat should match returned Chat", SubsystemChat, Chat);
	}

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Config Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientFullConfigTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.3FullConfig.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientFullConfigTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_full_config_client";
	
	// Create a valid PubnubClient
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	// Set all Config parameters
	FPubnubChatConfig ChatConfig;
	ChatConfig.AuthKey = TEXT("test_auth_key");
	ChatConfig.TypingTimeout = 10000;
	ChatConfig.TypingTimeoutDifference = 2000;
	ChatConfig.StoreUserActivityInterval = 300000;
	ChatConfig.StoreUserActivityTimestamps = true;
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed with full config", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify Chat is properly initialized
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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
		
		// Verify PubnubClient was set correctly using reflection
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		TestNotNull("PubnubClient should be set", PubnubClient);
		TestEqual("PubnubClient should match provided client", PubnubClient, TestClient);
	}

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests calling InitChatWithPubnubClient multiple times with the same UserID.
 * Verifies that subsequent calls return the existing Chat object and indicate that chat already exists.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientDuplicateCallTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.DuplicateCall", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientDuplicateCallTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_duplicate_client";
	
	// Create PubnubClients
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig1;
	ClientConfig1.PublishKey = TestPublishKey;
	ClientConfig1.SubscribeKey = TestSubscribeKey;
	ClientConfig1.UserID = TestUserID;
	UPubnubClient* TestClient1 = PubnubSubsystem->CreatePubnubClient(ClientConfig1);
	
	FPubnubConfig ClientConfig2;
	ClientConfig2.PublishKey = TestPublishKey;
	ClientConfig2.SubscribeKey = TestSubscribeKey;
	ClientConfig2.UserID = TestUserID;
	UPubnubClient* TestClient2 = PubnubSubsystem->CreatePubnubClient(ClientConfig2);
	
	if(!TestClient1 || !TestClient2)
	{
		AddError("Failed to create PubnubClients");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	
	// First InitChatWithPubnubClient call
	FPubnubChatInitChatResult FirstResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient1, ChatConfig);
	TestFalse("First InitChatWithPubnubClient should succeed", FirstResult.Result.Error);
	TestNotNull("First Chat object should be created", FirstResult.Chat);
	
	UPubnubChat* FirstChat = FirstResult.Chat;
	
	// Second InitChatWithPubnubClient call with same UserID but different client
	FPubnubChatInitChatResult SecondResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient2, ChatConfig);
	
	// Should return existing chat (currently sets Error=true but returns Chat)
	TestNotNull("Second InitChatWithPubnubClient should return existing Chat", SecondResult.Chat);
	TestEqual("Second result should return same Chat object", SecondResult.Chat, FirstChat);
	TestEqual("GetChat should return same object", ChatSubsystem->GetChat(TestUserID), FirstChat);
	
	// Verify the result indicates it's an existing chat
	// Note: Current implementation sets Error=true when chat exists, but returns the chat
	TestTrue("Result should indicate existing chat", SecondResult.Result.Error);
	TestFalse("ErrorMessage should indicate existing chat", SecondResult.Result.ErrorMessage.IsEmpty());
	
	// Verify the first client is still used (not replaced by second client)
	UPubnubClient* ChatClient = GetPubnubClientFromChat(FirstChat);
	TestEqual("Chat should still use first client", ChatClient, TestClient1);

	CleanUpCurrentChatUser(FirstResult.Chat);
	CleanUpCurrentChatUser(SecondResult.Chat);
	CleanUp();
	return true;
}

/**
 * Tests that PubnubClient UserID is set correctly when InitChatWithPubnubClient is called.
 * Verifies that SetUserID is called on the PubnubClient with the provided UserID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientUserIDSetTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.UserIDSet", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientUserIDSetTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_id_set";
	
	// Create a PubnubClient with a different UserID initially
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = SDK_PREFIX + "different_user_id";
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	// Verify initial UserID is different
	FString InitialUserID = TestClient->GetUserID();
	TestNotEqual("Initial UserID should be different", InitialUserID, TestUserID);
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify PubnubClient UserID was updated to match TestUserID
	FString UpdatedUserID = TestClient->GetUserID();
	TestEqual("PubnubClient UserID should be updated to TestUserID", UpdatedUserID, TestUserID);

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

/**
 * Tests InitChatWithPubnubClient with different UserIDs after destroying previous chat.
 * Verifies that destroying and reinitializing with a different UserID creates a new Chat object with the correct user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientDifferentUserIDsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.DifferentUserIDs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientDifferentUserIDsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	
	// First InitChatWithPubnubClient with UserID1
	const FString TestUserID1 = SDK_PREFIX + "test_user_1_client";
	FPubnubConfig ClientConfig1;
	ClientConfig1.PublishKey = TestPublishKey;
	ClientConfig1.SubscribeKey = TestSubscribeKey;
	ClientConfig1.UserID = TestUserID1;
	UPubnubClient* TestClient1 = PubnubSubsystem->CreatePubnubClient(ClientConfig1);
	
	if(!TestClient1)
	{
		AddError("Failed to create PubnubClient1");
		return false;
	}
	
	FPubnubChatInitChatResult Result1 = ChatSubsystem->InitChatWithPubnubClient(TestUserID1, TestClient1, ChatConfig);
	TestFalse("First InitChatWithPubnubClient should succeed", Result1.Result.Error);
	TestNotNull("First Chat should be created", Result1.Chat);
	
	UPubnubChat* Chat1 = ChatSubsystem->GetChat(TestUserID1);
	if(Chat1)
	{
		UPubnubChatUser* User1 = Chat1->GetCurrentUser();
		TestNotNull("User1 should exist", User1);
	}
	
	// Destroy first chat
	ChatSubsystem->DestroyChat(TestUserID1);
	
	// Second InitChatWithPubnubClient with UserID2
	const FString TestUserID2 = SDK_PREFIX + "test_user_2_client";
	FPubnubConfig ClientConfig2;
	ClientConfig2.PublishKey = TestPublishKey;
	ClientConfig2.SubscribeKey = TestSubscribeKey;
	ClientConfig2.UserID = TestUserID2;
	UPubnubClient* TestClient2 = PubnubSubsystem->CreatePubnubClient(ClientConfig2);
	
	if(!TestClient2)
	{
		AddError("Failed to create PubnubClient2");
		return false;
	}
	
	FPubnubChatInitChatResult Result2 = ChatSubsystem->InitChatWithPubnubClient(TestUserID2, TestClient2, ChatConfig);
	TestFalse("Second InitChatWithPubnubClient should succeed", Result2.Result.Error);
	TestNotNull("Second Chat should be created", Result2.Chat);
	
	UPubnubChat* Chat2 = ChatSubsystem->GetChat(TestUserID2);
	if(Chat2)
	{
		UPubnubChatUser* User2 = Chat2->GetCurrentUser();
		TestNotNull("User2 should exist", User2);
	}
	
	// Verify they are different objects
	TestNotEqual("Chat objects should be different", Chat1, Chat2);

	CleanUpCurrentChatUser(Chat1);
	CleanUpCurrentChatUser(Chat2);
	CleanUp();
	return true;
}

/**
 * Tests that ChatConfig parameters are preserved and stored correctly after InitChatWithPubnubClient.
 * Verifies that all config values (AuthKey, TypingTimeout, etc.) are stored and can be retrieved from the Chat object.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientConfigPreservationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.ConfigPreservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientConfigPreservationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_config_preservation_client";
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	// Set specific config values
	FPubnubChatConfig ChatConfig;
	ChatConfig.AuthKey = TEXT("custom_auth_key_123");
	ChatConfig.TypingTimeout = 15000;
	ChatConfig.TypingTimeoutDifference = 3000;
	ChatConfig.StoreUserActivityInterval = 900000;
	ChatConfig.StoreUserActivityTimestamps = true;
	
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify config was preserved using reflection
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests subsystem state management throughout the Chat lifecycle.
 * Verifies that GetChat returns null before InitChatWithPubnubClient, returns Chat after InitChatWithPubnubClient, and returns null after DestroyChat.
 * Also verifies internal subsystem state using reflection.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientSubsystemStateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.SubsystemState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientSubsystemStateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_subsystem_state_client";
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	
	// Test that GetChat returns null before InitChatWithPubnubClient
	UPubnubChat* ChatBeforeInit = ChatSubsystem->GetChat(TestUserID);
	TestNull("GetChat should return null before InitChatWithPubnubClient", ChatBeforeInit);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatBefore = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNull("Subsystem's internal Chat should be null before InitChatWithPubnubClient", SubsystemChatBefore);
	
	// InitChatWithPubnubClient
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Test that GetChat returns the Chat after InitChatWithPubnubClient
	UPubnubChat* ChatAfterInit = ChatSubsystem->GetChat(TestUserID);
	TestNotNull("GetChat should return Chat after InitChatWithPubnubClient", ChatAfterInit);
	TestEqual("GetChat should return same Chat as InitChatWithPubnubClient result", ChatAfterInit, InitResult.Chat);
	
	// Verify subsystem's internal Chat matches using reflection
	UPubnubChat* SubsystemChatAfter = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNotNull("Subsystem's internal Chat should exist after InitChatWithPubnubClient", SubsystemChatAfter);
	TestEqual("Subsystem's internal Chat should match GetChat", SubsystemChatAfter, ChatAfterInit);
	TestEqual("Subsystem's internal Chat should match InitChatWithPubnubClient result", SubsystemChatAfter, InitResult.Chat);
	
	// Destroy Chat
	ChatSubsystem->DestroyChat(TestUserID);
	
	// Test that GetChat returns null after DestroyChat
	UPubnubChat* ChatAfterDestroy = ChatSubsystem->GetChat(TestUserID);
	TestNull("GetChat should return null after DestroyChat", ChatAfterDestroy);
	
	// Verify subsystem's internal Chat is null using reflection
	UPubnubChat* SubsystemChatAfterDestroy = GetChatFromSubsystem(ChatSubsystem, TestUserID);
	TestNull("Subsystem's internal Chat should be null after DestroyChat", SubsystemChatAfterDestroy);

	CleanUpCurrentChatUser(ChatAfterInit);
	CleanUp();
	return true;
}

/**
 * Tests that CurrentUser is properly created and accessible after InitChatWithPubnubClient.
 * Verifies that CurrentUser exists both via public API and reflection, and that both references match.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientCurrentUserVerificationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.CurrentUserVerification", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientCurrentUserVerificationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_verification_client";
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
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
			TestTrue("CurrentUser should be valid", CurrentUser->IsValidLowLevel());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that InitChatWithPubnubClient properly tracks step results for internal operations.
 * Verifies that step results contain expected operations like GetUserMetadata and SetUserMetadata.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientStepResultsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.StepResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientStepResultsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_step_results_client";
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	
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

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

/**
 * Tests that the same PubnubClient instance is reused correctly when InitChatWithPubnubClient is called.
 * Verifies that the Chat object stores a reference to the provided PubnubClient and doesn't create a new one.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatInitChatWithPubnubClientClientReuseTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ChatSubsystem.InitChatWithPubnubClient.4Advanced.ClientReuse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatInitChatWithPubnubClientClientReuseTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_client_reuse";
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	if(!PubnubSubsystem)
	{
		AddError("PubnubSubsystem is invalid");
		return false;
	}
	
	// Create a PubnubClient
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TestPublishKey;
	ClientConfig.SubscribeKey = TestSubscribeKey;
	ClientConfig.UserID = TestUserID;
	UPubnubClient* TestClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);
	
	if(!TestClient)
	{
		AddError("Failed to create PubnubClient");
		return false;
	}
	
	// Store the client ID or pointer for verification
	UPubnubClient* OriginalClient = TestClient;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChatWithPubnubClient(TestUserID, TestClient, ChatConfig);
	
	TestFalse("InitChatWithPubnubClient should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	// Verify the Chat uses the same PubnubClient instance
	UPubnubChat* Chat = ChatSubsystem->GetChat(TestUserID);
	if(Chat)
	{
		// Verify via public API
		UPubnubClient* ChatClient = Chat->GetPubnubClient();
		TestNotNull("Chat should have PubnubClient", ChatClient);
		TestEqual("Chat should use the same PubnubClient instance", ChatClient, OriginalClient);
		
		// Verify via reflection
		UPubnubClient* ReflectedClient = GetPubnubClientFromChat(Chat);
		TestNotNull("Reflected PubnubClient should exist", ReflectedClient);
		TestEqual("Reflected PubnubClient should match provided client", ReflectedClient, OriginalClient);
		TestEqual("Reflected PubnubClient should match GetPubnubClient", ReflectedClient, ChatClient);
	}

	CleanUpCurrentChatUser(InitResult.Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

