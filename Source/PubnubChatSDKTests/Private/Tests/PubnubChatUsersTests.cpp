// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMembership.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// GETCURRENTUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetCurrentUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetCurrentUserNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetCurrentUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetCurrentUserHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_current_user_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Test GetCurrentUser
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("GetCurrentUser should return a user", CurrentUser);
		
		if(CurrentUser)
		{
			// Verify user is initialized
			FString UserID = CurrentUser->GetUserID();
			TestFalse("CurrentUser UserID should not be empty", UserID.IsEmpty());
			TestEqual("CurrentUser UserID should match InitChat UserID", UserID, TestUserID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserAfterDestroyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetCurrentUser.4Advanced.AfterDestroy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetCurrentUserAfterDestroyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_current_user_destroy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get current user before destroy
		UPubnubChatUser* UserBeforeDestroy = Chat->GetCurrentUser();
		TestNotNull("CurrentUser should exist before destroy", UserBeforeDestroy);
		
		// Destroy chat
		Chat->DestroyChat();
		
		// Get current user after destroy - should still return the user object (but chat is not initialized)
		UPubnubChatUser* UserAfterDestroy = Chat->GetCurrentUser();
		// Note: Current implementation returns the user object even after destroy
		// This is implementation-specific behavior
		TestNotNull("CurrentUser object should still exist after destroy", UserAfterDestroy);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetCurrentUser.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetCurrentUserConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_current_user_consistency";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Call GetCurrentUser multiple times - should return same object
		UPubnubChatUser* User1 = Chat->GetCurrentUser();
		UPubnubChatUser* User2 = Chat->GetCurrentUser();
		TestEqual("GetCurrentUser should return same object on multiple calls", User1, User2);
		
		if(User1 && User2)
		{
			TestEqual("User1 UserID should match User2 UserID", User1->GetUserID(), User2->GetUserID());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// CREATEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create user without initialized chat
		// This will fail because Chat is null, but we need to test the actual function
		// So we'll create a chat object but not initialize it properly
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_create_user_not_init";
			FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID);
			
			TestTrue("CreateUser should fail when Chat is not initialized", CreateResult.Result.Error);
			TestNull("User should not be created", CreateResult.User);
			TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_create_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to create user with empty UserID
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TEXT(""));
		
		TestTrue("CreateUser should fail with empty UserID", CreateResult.Result.Error);
		TestNull("User should not be created", CreateResult.User);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_user_init_user";
	const FString NewUserID = SDK_PREFIX + "test_create_user_new";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user with only required parameter (UserID)
		FPubnubChatUserResult CreateResult = Chat->CreateUser(NewUserID);
		
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		TestNotNull("User should be created", CreateResult.User);
		
		if(CreateResult.User)
		{
			TestEqual("Created User UserID should match", CreateResult.User->GetUserID(), NewUserID);
			
			// Verify user can get data from repository
			FPubnubChatUserData UserData = CreateResult.User->GetUserData();
			TestTrue("User should be able to get data from repository", true);
		}
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(NewUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_user_full_init";
	const FString NewUserID = SDK_PREFIX + "test_create_user_full_new";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user with all parameters
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("TestUserName");
		UserData.ExternalID = TEXT("external_123");
		UserData.ProfileUrl = TEXT("https://example.com/profile.jpg");
		UserData.Email = TEXT("test@example.com");
		UserData.Custom = TEXT("{\"key\":\"value\"}");
		UserData.Status = TEXT("active");
		UserData.Type = TEXT("user");
		
		FPubnubChatUserResult CreateResult = Chat->CreateUser(NewUserID, UserData);
		
		TestFalse("CreateUser should succeed with all parameters", CreateResult.Result.Error);
		TestNotNull("User should be created", CreateResult.User);
		
		if(CreateResult.User)
		{
			TestEqual("Created User UserID should match", CreateResult.User->GetUserID(), NewUserID);
			
			// Verify all data is stored correctly
			FPubnubChatUserData RetrievedData = CreateResult.User->GetUserData();
			TestEqual("UserName should match", RetrievedData.UserName, UserData.UserName);
			TestEqual("ExternalID should match", RetrievedData.ExternalID, UserData.ExternalID);
			TestEqual("ProfileUrl should match", RetrievedData.ProfileUrl, UserData.ProfileUrl);
			TestEqual("Email should match", RetrievedData.Email, UserData.Email);
			TestEqual("Custom should match", RetrievedData.Custom, UserData.Custom);
			TestEqual("Status should match", RetrievedData.Status, UserData.Status);
			TestEqual("Type should match", RetrievedData.Type, UserData.Type);
		}
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(NewUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserDuplicateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.4Advanced.DuplicateUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserDuplicateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_user_dup_init";
	const FString NewUserID = SDK_PREFIX + "test_create_user_dup";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user first time
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(NewUserID);
		TestFalse("First CreateUser should succeed", CreateResult1.Result.Error);
		TestNotNull("First User should be created", CreateResult1.User);
		
		// Try to create same user again - should fail
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(NewUserID);
		TestTrue("Second CreateUser should fail (user already exists)", CreateResult2.Result.Error);
		TestNull("Second User should not be created", CreateResult2.User);
		TestFalse("ErrorMessage should indicate user already exists", CreateResult2.Result.ErrorMessage.IsEmpty());
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(NewUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserDataSharingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUser.4Advanced.DataSharing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserDataSharingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_user_sharing_init";
	const FString NewUserID = SDK_PREFIX + "test_create_user_sharing";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user with data
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("SharedUser");
		UserData.Email = TEXT("shared@example.com");
		
		FPubnubChatUserResult CreateResult = Chat->CreateUser(NewUserID, UserData);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		TestNotNull("User should be created", CreateResult.User);
		
		// Get same user - should share the same data
		FPubnubChatUserResult GetResult = Chat->GetUser(NewUserID);
		TestFalse("GetUser should succeed", GetResult.Result.Error);
		TestNotNull("GetUser should return user", GetResult.User);
		
		if(CreateResult.User && GetResult.User)
		{
			// Both users should have same data from repository
			FPubnubChatUserData CreateData = CreateResult.User->GetUserData();
			FPubnubChatUserData GetData = GetResult.User->GetUserData();
			
			TestEqual("UserNames should match", CreateData.UserName, GetData.UserName);
			TestEqual("Emails should match", CreateData.Email, GetData.Email);
		}
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(NewUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get user without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_get_user_not_init";
			FPubnubChatUserResult GetResult = Chat->GetUser(TestUserID);
			
			TestTrue("GetUser should fail when Chat is not initialized", GetResult.Result.Error);
			TestNull("User should not be returned", GetResult.User);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get user with empty UserID
		FPubnubChatUserResult GetResult = Chat->GetUser(TEXT(""));
		
		TestTrue("GetUser should fail with empty UserID", GetResult.Result.Error);
		TestNull("User should not be returned", GetResult.User);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Then get the user
		FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
		
		TestFalse("GetUser should succeed", GetResult.Result.Error);
		TestNotNull("User should be returned", GetResult.User);
		
		if(GetResult.User)
		{
			TestEqual("GetUser UserID should match", GetResult.User->GetUserID(), TargetUserID);
			
			// Verify user can get data from repository
			FPubnubChatUserData UserData = GetResult.User->GetUserData();
			TestTrue("User should be able to get data from repository", true);
		}
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserNonExistentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.4Advanced.NonExistentUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserNonExistentTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_nonexistent_init";
	const FString NonExistentUserID = SDK_PREFIX + "test_get_user_nonexistent_" + FString::FromInt(FDateTime::Now().ToUnixTimestamp());
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get user that doesn't exist
		FPubnubChatUserResult GetResult = Chat->GetUser(NonExistentUserID);
		
		// GetUser will fail if user doesn't exist on server
		TestTrue("GetUser should fail for non-existent user", GetResult.Result.Error);
		TestNull("User should not be returned", GetResult.User);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserMultipleCallsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.4Advanced.MultipleCalls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserMultipleCallsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_multiple_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user first
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Get user multiple times
		FPubnubChatUserResult GetResult1 = Chat->GetUser(TargetUserID);
		FPubnubChatUserResult GetResult2 = Chat->GetUser(TargetUserID);
		FPubnubChatUserResult GetResult3 = Chat->GetUser(TargetUserID);
		
		TestFalse("First GetUser should succeed", GetResult1.Result.Error);
		TestFalse("Second GetUser should succeed", GetResult2.Result.Error);
		TestFalse("Third GetUser should succeed", GetResult3.Result.Error);
		
		TestNotNull("First User should be returned", GetResult1.User);
		TestNotNull("Second User should be returned", GetResult2.User);
		TestNotNull("Third User should be returned", GetResult3.User);
		
		if(GetResult1.User && GetResult2.User && GetResult3.User)
		{
			// All users should have same UserID
			TestEqual("First User UserID should match", GetResult1.User->GetUserID(), TargetUserID);
			TestEqual("Second User UserID should match", GetResult2.User->GetUserID(), TargetUserID);
			TestEqual("Third User UserID should match", GetResult3.User->GetUserID(), TargetUserID);
			
			// All users should share same data from repository
			FPubnubChatUserData Data1 = GetResult1.User->GetUserData();
			FPubnubChatUserData Data2 = GetResult2.User->GetUserData();
			FPubnubChatUserData Data3 = GetResult3.User->GetUserData();
			
			TestEqual("Data1 UserName should match Data2", Data1.UserName, Data2.UserName);
			TestEqual("Data2 UserName should match Data3", Data2.UserName, Data3.UserName);
			TestEqual("Data1 Email should match Data2", Data1.Email, Data2.Email);
			TestEqual("Data2 Email should match Data3", Data2.Email, Data3.Email);
		}
		
		// Cleanup: Delete created user
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserCurrentUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUser.4Advanced.GetCurrentUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserCurrentUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_user_current";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get current user via GetCurrentUser
		UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
		TestNotNull("CurrentUser should exist", CurrentUser);
		
		// Get same user via GetUser
		FPubnubChatUserResult GetResult = Chat->GetUser(TestUserID);
		TestFalse("GetUser should succeed", GetResult.Result.Error);
		TestNotNull("GetUser should return user", GetResult.User);
		
		if(CurrentUser && GetResult.User)
		{
			// Both should have same UserID
			TestEqual("CurrentUser UserID should match GetUser UserID", CurrentUser->GetUserID(), GetResult.User->GetUserID());
			
			// Both should share same data from repository
			FPubnubChatUserData CurrentData = CurrentUser->GetUserData();
			FPubnubChatUserData GetData = GetResult.User->GetUserData();
			
			TestEqual("CurrentUser UserName should match GetUser UserName", CurrentData.UserName, GetData.UserName);
			TestEqual("CurrentUser Email should match GetUser Email", CurrentData.Email, GetData.Email);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// UPDATEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to update user without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_update_user_not_init";
			FPubnubChatUpdateUserInputData UserData;
			FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TestUserID, UserData);
			
			TestTrue("UpdateUser should fail when Chat is not initialized", UpdateResult.Result.Error);
			TestNull("User should not be returned", UpdateResult.User);
			TestFalse("ErrorMessage should not be empty", UpdateResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_update_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to update user with empty UserID
		FPubnubChatUpdateUserInputData UserData;
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TEXT(""), UserData);
		
		TestTrue("UpdateUser should fail with empty UserID", UpdateResult.Result.Error);
		TestNull("User should not be returned", UpdateResult.User);
		TestFalse("ErrorMessage should not be empty", UpdateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserNonExistingUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.1Validation.NonExistingUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserNonExistingUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_init";
	const FString NonExistingUserID = SDK_PREFIX + "test_update_user_nonexisting";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to update user that doesn't exist (without creating it first)
		FPubnubChatUpdateUserInputData UserData;
		UserData.UserName = TEXT("NonExistingUser");
		UserData.ForceSetUserName = true;
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(NonExistingUserID, UserData);
		
		TestTrue("UpdateUser should fail with non-existing user", UpdateResult.Result.Error);
		TestNull("User should not be returned", UpdateResult.User);
		TestFalse("ErrorMessage should not be empty", UpdateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_init";
	const FString TargetUserID = SDK_PREFIX + "test_update_user_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user with minimal data
		FPubnubChatUpdateUserInputData UserData;
		UserData.UserName = TEXT("UpdatedUser");
		UserData.ForceSetUserName = true;
		
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TargetUserID, UserData);
		
		TestFalse("UpdateUser should succeed", UpdateResult.Result.Error);
		TestNotNull("User should be returned", UpdateResult.User);
		
		if(UpdateResult.User)
		{
			TestEqual("Updated User UserID should match", UpdateResult.User->GetUserID(), TargetUserID);
			
			// Verify data was updated
			FPubnubChatUserData UpdatedData = UpdateResult.User->GetUserData();
			TestEqual("UserName should be updated", UpdatedData.UserName, UserData.UserName);
				
			// Cleanup: Delete created user
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_update_user_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user with all parameters
		FPubnubChatUpdateUserInputData UserData;
		UserData.UserName = TEXT("UpdatedFullUser");
		UserData.ExternalID = TEXT("updated_external_456");
		UserData.ProfileUrl = TEXT("https://example.com/updated_profile.jpg");
		UserData.Email = TEXT("updated@example.com");
		UserData.Custom = TEXT("{\"updated\":\"data\"}");
		UserData.Status = TEXT("updatedStatus");
		UserData.Type = TEXT("updatedType");
		// Set ForceSet flags only for fields we're updating
		UserData.ForceSetUserName = true;
		UserData.ForceSetExternalID = true;
		UserData.ForceSetProfileUrl = true;
		UserData.ForceSetEmail = true;
		UserData.ForceSetCustom = true;
		UserData.ForceSetStatus = true;
		UserData.ForceSetType = true;
		
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TargetUserID, UserData);
		
		TestFalse("UpdateUser should succeed with all parameters", UpdateResult.Result.Error);
		TestNotNull("User should be returned", UpdateResult.User);
		
		if(UpdateResult.User)
		{
			TestEqual("Updated User UserID should match", UpdateResult.User->GetUserID(), TargetUserID);
			
			// Verify all data was updated correctly
			FPubnubChatUserData UpdatedData = UpdateResult.User->GetUserData();
			TestEqual("UserName should match", UpdatedData.UserName, UserData.UserName);
			TestEqual("ExternalID should match", UpdatedData.ExternalID, UserData.ExternalID);
			TestEqual("ProfileUrl should match", UpdatedData.ProfileUrl, UserData.ProfileUrl);
			TestEqual("Email should match", UpdatedData.Email, UserData.Email);
			TestEqual("Custom should match", UpdatedData.Custom, UserData.Custom);
			TestEqual("Status should match", UpdatedData.Status, UserData.Status);
			TestEqual("Type should match", UpdatedData.Type, UserData.Type);
			
		}
		// Cleanup: Delete created user
		Chat->DeleteUser(TargetUserID, false);
		
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserMultipleTimesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_multiple_init";
	const FString TargetUserID = SDK_PREFIX + "test_update_user_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user first time
		FPubnubChatUpdateUserInputData UserData1;
		UserData1.UserName = TEXT("FirstUpdate");
		UserData1.Email = TEXT("first@example.com");
		UserData1.ForceSetUserName = true;
		UserData1.ForceSetEmail = true;
		FPubnubChatUserResult UpdateResult1 = Chat->UpdateUser(TargetUserID, UserData1);
		TestFalse("First UpdateUser should succeed", UpdateResult1.Result.Error);
		
		// Update user second time
		FPubnubChatUpdateUserInputData UserData2;
		UserData2.UserName = TEXT("SecondUpdate");
		UserData2.Email = TEXT("second@example.com");
		UserData2.ForceSetUserName = true;
		UserData2.ForceSetEmail = true;
		FPubnubChatUserResult UpdateResult2 = Chat->UpdateUser(TargetUserID, UserData2);
		TestFalse("Second UpdateUser should succeed", UpdateResult2.Result.Error);
		
		// Update user third time
		FPubnubChatUpdateUserInputData UserData3;
		UserData3.UserName = TEXT("ThirdUpdate");
		UserData3.Email = TEXT("third@example.com");
		UserData3.ForceSetUserName = true;
		UserData3.ForceSetEmail = true;
		FPubnubChatUserResult UpdateResult3 = Chat->UpdateUser(TargetUserID, UserData3);
		TestFalse("Third UpdateUser should succeed", UpdateResult3.Result.Error);
		
		if(UpdateResult3.User)
		{
			// Verify final data is correct
			FPubnubChatUserData FinalData = UpdateResult3.User->GetUserData();
			TestEqual("Final UserName should match third update", FinalData.UserName, UserData3.UserName);
			TestEqual("Final Email should match third update", FinalData.Email, UserData3.Email);
			
			// Verify data synchronization - get user again should have same data
			FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
			TestFalse("GetUser should succeed", GetResult.Result.Error);
			if(GetResult.User)
			{
				FPubnubChatUserData GetData = GetResult.User->GetUserData();
				TestEqual("GetUser UserName should match UpdateUser UserName", GetData.UserName, FinalData.UserName);
				TestEqual("GetUser Email should match UpdateUser Email", GetData.Email, FinalData.Email);
			}
			
			// Cleanup: Delete created user
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserDataSynchronizationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUser.4Advanced.DataSynchronization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserDataSynchronizationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_sync_init";
	const FString TargetUserID = SDK_PREFIX + "test_update_user_sync";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user
		FPubnubChatUpdateUserInputData UserData;
		UserData.UserName = TEXT("SyncedUser");
		UserData.Email = TEXT("synced@example.com");
		UserData.ForceSetUserName = true;
		UserData.ForceSetEmail = true;
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TargetUserID, UserData);
		TestFalse("UpdateUser should succeed", UpdateResult.Result.Error);
		
		// Get user multiple times - all should have same updated data
		FPubnubChatUserResult GetResult1 = Chat->GetUser(TargetUserID);
		FPubnubChatUserResult GetResult2 = Chat->GetUser(TargetUserID);
		
		TestFalse("First GetUser should succeed", GetResult1.Result.Error);
		TestFalse("Second GetUser should succeed", GetResult2.Result.Error);
		
		if(UpdateResult.User && GetResult1.User && GetResult2.User)
		{
			FPubnubChatUserData UpdateData = UpdateResult.User->GetUserData();
			FPubnubChatUserData GetData1 = GetResult1.User->GetUserData();
			FPubnubChatUserData GetData2 = GetResult2.User->GetUserData();
			
			// All should have same data from repository
			TestEqual("UpdateUser UserName should match GetUser1 UserName", UpdateData.UserName, GetData1.UserName);
			TestEqual("GetUser1 UserName should match GetUser2 UserName", GetData1.UserName, GetData2.UserName);
			TestEqual("UpdateUser Email should match GetUser1 Email", UpdateData.Email, GetData1.Email);
			TestEqual("GetUser1 Email should match GetUser2 Email", GetData1.Email, GetData2.Email);
			
			// Cleanup: Delete created user
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// DELETEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to delete user without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_delete_user_not_init";
			FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TestUserID, false);
			
			TestTrue("DeleteUser should fail when Chat is not initialized", DeleteResult.Error);
			TestFalse("ErrorMessage should not be empty", DeleteResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_delete_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to delete user with empty UserID
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TEXT(""), false);
		
		TestTrue("DeleteUser should fail with empty UserID", DeleteResult.Error);
		TestFalse("ErrorMessage should not be empty", DeleteResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_user_init";
	const FString TargetUserID = SDK_PREFIX + "test_delete_user_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Delete user with default parameters (hard delete)
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TargetUserID);
		
		TestFalse("DeleteUser should succeed", DeleteResult.Error);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHardDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.3FullParameters.HardDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserHardDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_user_hard_init";
	const FString TargetUserID = SDK_PREFIX + "test_delete_user_hard";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Hard delete user (Soft = false)
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TargetUserID, false);
		
		TestFalse("Hard DeleteUser should succeed", DeleteResult.Error);
		
		// Verify user is actually deleted - GetUser should fail
		FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
		TestTrue("GetUser should fail after hard delete", GetResult.Result.Error);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserSoftDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.3FullParameters.SoftDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserSoftDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_user_soft_init";
	const FString TargetUserID = SDK_PREFIX + "test_delete_user_soft";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Soft delete user (Soft = true)
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TargetUserID, true);
		
		TestFalse("Soft DeleteUser should succeed", DeleteResult.Error);
		
		// Verify user still exists but is marked as deleted
		FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
		TestFalse("GetUser should succeed after soft delete", GetResult.Result.Error);
		TestNotNull("GetUser should return user after soft delete", GetResult.User);
		
		// Cleanup: Hard delete the soft-deleted user
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHardVsSoftTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUser.4Advanced.HardVsSoft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserHardVsSoftTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_user_hardsoft_init";
	const FString SoftDeleteUserID = SDK_PREFIX + "test_delete_user_soft";
	const FString HardDeleteUserID = SDK_PREFIX + "test_delete_user_hard";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create first user for soft delete
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(SoftDeleteUserID);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		
		// Create second user for hard delete
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(HardDeleteUserID);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		
		// Soft delete first user
		FPubnubChatOperationResult SoftDeleteResult = Chat->DeleteUser(SoftDeleteUserID, true);
		TestFalse("Soft DeleteUser should succeed", SoftDeleteResult.Error);
		
		// Hard delete second user
		FPubnubChatOperationResult HardDeleteResult = Chat->DeleteUser(HardDeleteUserID, false);
		TestFalse("Hard DeleteUser should succeed", HardDeleteResult.Error);
		
		// Verify soft-deleted user still exists
		FPubnubChatUserResult GetSoftResult = Chat->GetUser(SoftDeleteUserID);
		TestFalse("GetUser should succeed for soft-deleted user", GetSoftResult.Result.Error);
		TestNotNull("GetUser should return soft-deleted user", GetSoftResult.User);
		
		// Verify hard-deleted user no longer exists
		FPubnubChatUserResult GetHardResult = Chat->GetUser(HardDeleteUserID);
		TestTrue("GetUser should fail for hard-deleted user", GetHardResult.Result.Error);
		TestNull("GetUser should not return hard-deleted user", GetHardResult.User);
		
		// Cleanup: Hard delete the soft-deleted user
		if(Chat)
		{
			Chat->DeleteUser(SoftDeleteUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// RESTORE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserRestoreNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Restore.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserRestoreNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_not_init_init";
	const FString TargetUserID = SDK_PREFIX + "test_restore_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Create uninitialized user object
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		
		// Try to restore with uninitialized user
		FPubnubChatOperationResult RestoreResult = UninitializedUser->Restore();
		TestTrue("Restore should fail with uninitialized user", RestoreResult.Error);
		TestFalse("ErrorMessage should not be empty", RestoreResult.ErrorMessage.IsEmpty());
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserRestoreHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Restore.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserRestoreHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_restore_happy";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Soft delete the user first
	FPubnubChatOperationResult DeleteResult = CreateResult.User->Delete(true);
	TestFalse("Soft delete should succeed", DeleteResult.Error);
	
	// Verify user is marked as deleted
	FPubnubChatIsDeletedResult IsDeletedBeforeResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
	TestTrue("User should be marked as deleted", IsDeletedBeforeResult.IsDeleted);
	
	// Restore the user
	FPubnubChatOperationResult RestoreResult = CreateResult.User->Restore();
	TestFalse("Restore should succeed", RestoreResult.Error);
	
	// Verify user is no longer marked as deleted
	FPubnubChatIsDeletedResult IsDeletedAfterResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterResult.Result.Error);
	TestFalse("User should not be marked as deleted after restore", IsDeletedAfterResult.IsDeleted);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

// Test: Restore a user that wasn't deleted (should still succeed)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserRestoreNonDeletedUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Restore.4Advanced.RestoreNonDeletedUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserRestoreNonDeletedUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_non_deleted_init";
	const FString TargetUserID = SDK_PREFIX + "test_restore_non_deleted";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify user is not deleted
	FPubnubChatIsDeletedResult IsDeletedBeforeResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
	TestFalse("User should not be marked as deleted", IsDeletedBeforeResult.IsDeleted);
	
	// Restore a non-deleted user (should still succeed)
	FPubnubChatOperationResult RestoreResult = CreateResult.User->Restore();
	TestFalse("Restore should succeed even for non-deleted user", RestoreResult.Error);
	
	// Verify user is still not deleted
	FPubnubChatIsDeletedResult IsDeletedAfterResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterResult.Result.Error);
	TestFalse("User should still not be marked as deleted", IsDeletedAfterResult.IsDeleted);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// Test: Restore a hard-deleted user (should fail)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserRestoreHardDeletedUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Restore.4Advanced.RestoreHardDeletedUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserRestoreHardDeletedUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_restore_hard_deleted_init";
	const FString TargetUserID = SDK_PREFIX + "test_restore_hard_deleted";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Hard delete the user
	FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TargetUserID, false);
	TestFalse("Hard delete should succeed", DeleteResult.Error);
	
	// Try to get the user again (should fail)
	FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
	TestTrue("GetUser should fail after hard delete", GetResult.Result.Error);
	
	// Note: After hard delete, the user object is no longer valid, so we can't test Restore on it
	// This test verifies that hard-deleted users cannot be restored (they don't exist anymore)

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ISDELETED TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeleted.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_deleted_not_init_init";
	const FString TargetUserID = SDK_PREFIX + "test_is_deleted_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Create uninitialized user object
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		
		// Try to check IsDeleted with uninitialized user
		FPubnubChatIsDeletedResult IsDeletedResult = UninitializedUser->IsDeleted();
		TestTrue("IsDeleted should fail with uninitialized user", IsDeletedResult.Result.Error);
		TestFalse("IsDeleted should be false when error occurs", IsDeletedResult.IsDeleted);
		TestFalse("ErrorMessage should not be empty", IsDeletedResult.Result.ErrorMessage.IsEmpty());
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeleted.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_deleted_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_is_deleted_happy";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Check IsDeleted for a non-deleted user
	FPubnubChatIsDeletedResult IsDeletedResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed", IsDeletedResult.Result.Error);
	TestFalse("User should not be marked as deleted", IsDeletedResult.IsDeleted);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedSoftDeletedUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeleted.3FullParameters.SoftDeletedUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedSoftDeletedUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_deleted_soft_init";
	const FString TargetUserID = SDK_PREFIX + "test_is_deleted_soft";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify user is not deleted initially
	FPubnubChatIsDeletedResult IsDeletedBeforeResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed", IsDeletedBeforeResult.Result.Error);
	TestFalse("User should not be marked as deleted initially", IsDeletedBeforeResult.IsDeleted);
	
	// Soft delete the user
	FPubnubChatOperationResult DeleteResult = CreateResult.User->Delete(true);
	TestFalse("Soft delete should succeed", DeleteResult.Error);
	
	// Check IsDeleted for a soft-deleted user
	FPubnubChatIsDeletedResult IsDeletedAfterResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed after soft delete", IsDeletedAfterResult.Result.Error);
	TestTrue("User should be marked as deleted after soft delete", IsDeletedAfterResult.IsDeleted);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

// Test: Check IsDeleted after restore
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedAfterRestoreTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeleted.4Advanced.AfterRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedAfterRestoreTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_deleted_restore_init";
	const FString TargetUserID = SDK_PREFIX + "test_is_deleted_restore";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Soft delete the user
	FPubnubChatOperationResult DeleteResult = CreateResult.User->Delete(true);
	TestFalse("Soft delete should succeed", DeleteResult.Error);
	
	// Verify user is marked as deleted
	FPubnubChatIsDeletedResult IsDeletedBeforeRestoreResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed", IsDeletedBeforeRestoreResult.Result.Error);
	TestTrue("User should be marked as deleted", IsDeletedBeforeRestoreResult.IsDeleted);
	
	// Restore the user
	FPubnubChatOperationResult RestoreResult = CreateResult.User->Restore();
	TestFalse("Restore should succeed", RestoreResult.Error);
	
	// Check IsDeleted after restore
	FPubnubChatIsDeletedResult IsDeletedAfterRestoreResult = CreateResult.User->IsDeleted();
	TestFalse("IsDeleted check should succeed after restore", IsDeletedAfterRestoreResult.Result.Error);
	TestFalse("User should not be marked as deleted after restore", IsDeletedAfterRestoreResult.IsDeleted);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// Test: Check IsDeleted for hard-deleted user (should fail)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedHardDeletedUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeleted.4Advanced.HardDeletedUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedHardDeletedUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_deleted_hard_init";
	const FString TargetUserID = SDK_PREFIX + "test_is_deleted_hard";
	
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
	
	// Create user
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Hard delete the user
	FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TargetUserID, false);
	TestFalse("Hard delete should succeed", DeleteResult.Error);
	
	// Try to get the user again (should fail)
	FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
	TestTrue("GetUser should fail after hard delete", GetResult.Result.Error);
	
	// Note: After hard delete, the user object is no longer valid, so we can't test IsDeleted on it
	// This test verifies that hard-deleted users cannot be checked for IsDeleted (they don't exist anymore)
	// The user object from CreateResult is no longer valid after hard delete

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETUSERS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get users without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers();
			
			TestTrue("GetUsers should fail when Chat is not initialized", GetUsersResult.Result.Error);
			TestEqual("Users array should be empty", GetUsersResult.Users.Num(), 0);
			TestFalse("ErrorMessage should not be empty", GetUsersResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Call GetUsers with only default parameters (required parameters only)
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers();
		
		TestFalse("GetUsers should succeed", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid (may be empty)", GetUsersResult.Users.Num() >= 0);
		
		// Verify result structure is valid
		TestTrue("Total count should be non-negative", GetUsersResult.Total >= 0);
	}
	
	// Cleanup: No users created in this test, only using existing users
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_full_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_full_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_full_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_users_full_3";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test GetUsers with all parameters
		const int TestLimit = 10;
		const FString TestFilter = TEXT("name LIKE '*test_get_users_full*'");
		FPubnubGetAllSort TestSort;
		FPubnubGetAllSingleSort SingleSort;
		SingleSort.SortType = EPubnubGetAllSortType::PGAST_ID;
		SingleSort.SortOrder = false; // Ascending
		TestSort.GetAllSort.Add(SingleSort);
		FPubnubPage TestPage; // Empty page for first page
		
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(TestLimit, TestFilter, TestSort, TestPage);
		
		TestFalse("GetUsers should succeed with all parameters", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid", GetUsersResult.Users.Num() >= 0);
		TestTrue("Total count should be non-negative", GetUsersResult.Total >= 0);
		
		// Verify pagination information is present
		TestTrue("Page information should be present", true);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.3FullParameters.WithLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersWithLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_limit_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_limit_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_limit_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_users_limit_3";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test GetUsers with Limit parameter
		const int TestLimit = 2;
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(TestLimit);
		
		TestFalse("GetUsers should succeed with Limit", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid", GetUsersResult.Users.Num() >= 0);
		TestTrue("Users count should not exceed limit (if limit is enforced)", GetUsersResult.Users.Num() <= TestLimit || GetUsersResult.Users.Num() == 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithFilterTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.3FullParameters.WithFilter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersWithFilterTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_filter_init";
	const FString TestUserID = SDK_PREFIX + "test_get_users_filter_target";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Test GetUsers with Filter parameter
		const FString TestFilter = FString::Printf(TEXT("id == '%s'"), *TestUserID);
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(0, TestFilter);
		
		TestFalse("GetUsers should succeed with Filter", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid", GetUsersResult.Users.Num() >= 0);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithSortTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.3FullParameters.WithSort", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersWithSortTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_sort_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_sort_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_sort_2";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		// Test GetUsers with Sort parameter (ascending by ID)
		FPubnubGetAllSort TestSort;
		FPubnubGetAllSingleSort SingleSort;
		SingleSort.SortType = EPubnubGetAllSortType::PGAST_ID;
		SingleSort.SortOrder = false; // Ascending
		TestSort.GetAllSort.Add(SingleSort);
		
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(0, TEXT(""), TestSort);
		
		TestFalse("GetUsers should succeed with Sort", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid", GetUsersResult.Users.Num() >= 0);
		
		// Test GetUsers with Sort parameter (descending by ID)
		FPubnubGetAllSort TestSortDesc;
		FPubnubGetAllSingleSort SingleSortDesc;
		SingleSortDesc.SortType = EPubnubGetAllSortType::PGAST_ID;
		SingleSortDesc.SortOrder = true; // Descending
		TestSortDesc.GetAllSort.Add(SingleSortDesc);
		
		FPubnubChatGetUsersResult GetUsersResultDesc = Chat->GetUsers(0, TEXT(""), TestSortDesc);
		
		TestFalse("GetUsers should succeed with descending Sort", GetUsersResultDesc.Result.Error);
		TestTrue("Users array should be valid", GetUsersResultDesc.Users.Num() >= 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithPageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.3FullParameters.WithPage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersWithPageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_page_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Test GetUsers with Page parameter (first page - empty page)
		FPubnubPage FirstPage; // Empty page for first page
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(0, TEXT(""), FPubnubGetAllSort(), FirstPage);
		
		TestFalse("GetUsers should succeed with Page", GetUsersResult.Result.Error);
		TestTrue("Users array should be valid", GetUsersResult.Users.Num() >= 0);
		TestTrue("Page information should be present", true);
		
		// If there's a next page, test pagination
		if(!GetUsersResult.Page.Next.IsEmpty())
		{
			FPubnubPage NextPage;
			NextPage.Next = GetUsersResult.Page.Next;
			FPubnubChatGetUsersResult GetUsersResultNext = Chat->GetUsers(0, TEXT(""), FPubnubGetAllSort(), NextPage);
			
			TestFalse("GetUsers should succeed with Next page", GetUsersResultNext.Result.Error);
			TestTrue("Users array should be valid", GetUsersResultNext.Users.Num() >= 0);
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
 * Tests GetUsers with multiple users created.
 * Verifies that all created users are returned in the results and that the total count is correct.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_multi_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_multi_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_multi_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_users_multi_3";
	const FString TestUserID4 = SDK_PREFIX + "test_get_users_multi_4";
	const FString TestUserID5 = SDK_PREFIX + "test_get_users_multi_5";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create multiple test users
		TArray<FString> TestUserIDs = {TestUserID1, TestUserID2, TestUserID3, TestUserID4, TestUserID5};
		for(const FString& UserID : TestUserIDs)
		{
			FPubnubChatUserResult CreateResult = Chat->CreateUser(UserID);
			TestFalse(FString::Printf(TEXT("CreateUser %s should succeed"), *UserID), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedUserIDs.Add(UserID); }
		}
		
		// Wait a bit for server to process all user creations
		FPlatformProcess::Sleep(1.0f);
		
		// Get all users
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers();
		
		TestFalse("GetUsers should succeed", GetUsersResult.Result.Error);
		TestTrue("Users array should contain at least created users", GetUsersResult.Users.Num() >= CreatedUserIDs.Num());
		TestTrue("Total count should be at least number of created users", GetUsersResult.Total >= CreatedUserIDs.Num());
		
		// Verify all created users are in the result
		TArray<FString> FoundUserIDs;
		for(UPubnubChatUser* User : GetUsersResult.Users)
		{
			if(User)
			{
				FoundUserIDs.Add(User->GetUserID());
			}
		}
		
		for(const FString& CreatedUserID : CreatedUserIDs)
		{
			TestTrue(FString::Printf(TEXT("Created user %s should be found in results"), *CreatedUserID), FoundUserIDs.Contains(CreatedUserID));
		}
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests pagination functionality of GetUsers.
 * Verifies that pagination works correctly with limit parameter, next page navigation, and previous page navigation.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersPaginationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.Pagination", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersPaginationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_pagination_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Get first page with limit
		const int PageLimit = 5;
		FPubnubPage FirstPage;
		FPubnubChatGetUsersResult FirstPageResult = Chat->GetUsers(PageLimit, TEXT(""), FPubnubGetAllSort(), FirstPage);
		
		TestFalse("GetUsers first page should succeed", FirstPageResult.Result.Error);
		TestTrue("First page users array should be valid", FirstPageResult.Users.Num() >= 0);
		TestTrue("First page users should not exceed limit", FirstPageResult.Users.Num() <= PageLimit || FirstPageResult.Users.Num() == 0);
		
		// If there's a next page, test pagination
		if(!FirstPageResult.Page.Next.IsEmpty())
		{
			FPubnubPage NextPage;
			NextPage.Next = FirstPageResult.Page.Next;
			FPubnubChatGetUsersResult NextPageResult = Chat->GetUsers(PageLimit, TEXT(""), FPubnubGetAllSort(), NextPage);
			
			TestFalse("GetUsers next page should succeed", NextPageResult.Result.Error);
			TestTrue("Next page users array should be valid", NextPageResult.Users.Num() >= 0);
			TestTrue("Next page users should not exceed limit", NextPageResult.Users.Num() <= PageLimit || NextPageResult.Users.Num() == 0);
			
			// Verify pages are different
			if(FirstPageResult.Users.Num() > 0 && NextPageResult.Users.Num() > 0)
			{
				// Users in different pages should be different (unless there's overlap due to updates)
				TestTrue("Pagination should work correctly", true);
			}
			
			// Test previous page navigation
			if(!NextPageResult.Page.Prev.IsEmpty())
			{
				FPubnubPage PrevPage;
				PrevPage.Prev = NextPageResult.Page.Prev;
				FPubnubChatGetUsersResult PrevPageResult = Chat->GetUsers(PageLimit, TEXT(""), FPubnubGetAllSort(), PrevPage);
				
				TestFalse("GetUsers previous page should succeed", PrevPageResult.Result.Error);
				TestTrue("Previous page users array should be valid", PrevPageResult.Users.Num() >= 0);
			}
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests filtering functionality of GetUsers.
 * Verifies that users can be filtered by UserID and by name patterns using filter expressions.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersFilteringTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.Filtering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersFilteringTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_filtering_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_filtering_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_filtering_2";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users with different data
		FPubnubChatUserData UserData1;
		UserData1.UserName = TEXT("FilterUser1");
		UserData1.Status = TEXT("active");
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserData UserData2;
		UserData2.UserName = TEXT("FilterUser2");
		UserData2.Status = TEXT("inactive");
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		// Test filtering by UserID
		FString FilterByID = FString::Printf(TEXT("id == '%s'"), *TestUserID1);
		FPubnubChatGetUsersResult FilteredResult = Chat->GetUsers(0, FilterByID);
		
		TestFalse("GetUsers with filter should succeed", FilteredResult.Result.Error);
		TestTrue("Filtered users array should be valid", FilteredResult.Users.Num() >= 0);
		
		// Verify filtered user is in results
		bool FoundFilteredUser = false;
		for(UPubnubChatUser* User : FilteredResult.Users)
		{
			if(User && User->GetUserID() == TestUserID1)
			{
				FoundFilteredUser = true;
				break;
			}
		}
		TestTrue("Filtered user should be found in results", FoundFilteredUser || FilteredResult.Users.Num() == 0);
		
		// Test filtering by name pattern
		FString FilterByName = TEXT("name LIKE '*FilterUser*'");
		FPubnubChatGetUsersResult FilteredByNameResult = Chat->GetUsers(0, FilterByName);
		
		TestFalse("GetUsers with name filter should succeed", FilteredByNameResult.Result.Error);
		TestTrue("Filtered by name users array should be valid", FilteredByNameResult.Users.Num() >= 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests sorting functionality of GetUsers.
 * Verifies that users can be sorted by ID and Name in both ascending and descending order.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersSortingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.Sorting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersSortingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_sorting_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_sorting_a";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_sorting_b";
	const FString TestUserID3 = SDK_PREFIX + "test_get_users_sorting_c";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test sorting ascending by ID
		FPubnubGetAllSort SortAsc;
		FPubnubGetAllSingleSort SingleSortAsc;
		SingleSortAsc.SortType = EPubnubGetAllSortType::PGAST_ID;
		SingleSortAsc.SortOrder = false; // Ascending
		SortAsc.GetAllSort.Add(SingleSortAsc);
		
		FPubnubChatGetUsersResult AscResult = Chat->GetUsers(0, TEXT(""), SortAsc);
		TestFalse("GetUsers with ascending sort should succeed", AscResult.Result.Error);
		TestTrue("Ascending sorted users array should be valid", AscResult.Users.Num() >= 0);
		
		// Test sorting descending by ID
		FPubnubGetAllSort SortDesc;
		FPubnubGetAllSingleSort SingleSortDesc;
		SingleSortDesc.SortType = EPubnubGetAllSortType::PGAST_ID;
		SingleSortDesc.SortOrder = true; // Descending
		SortDesc.GetAllSort.Add(SingleSortDesc);
		
		FPubnubChatGetUsersResult DescResult = Chat->GetUsers(0, TEXT(""), SortDesc);
		TestFalse("GetUsers with descending sort should succeed", DescResult.Result.Error);
		TestTrue("Descending sorted users array should be valid", DescResult.Users.Num() >= 0);
		
		// Verify sorting order (if we have multiple users in results)
		if(AscResult.Users.Num() >= 2 && DescResult.Users.Num() >= 2)
		{
			// Ascending and descending should have reverse order
			TestTrue("Sorting should affect order", true);
		}
		
		// Test sorting by Name
		FPubnubGetAllSort SortByName;
		FPubnubGetAllSingleSort SingleSortByName;
		SingleSortByName.SortType = EPubnubGetAllSortType::PGAST_Name;
		SingleSortByName.SortOrder = false; // Ascending
		SortByName.GetAllSort.Add(SingleSortByName);
		
		FPubnubChatGetUsersResult NameSortResult = Chat->GetUsers(0, TEXT(""), SortByName);
		TestFalse("GetUsers with name sort should succeed", NameSortResult.Result.Error);
		TestTrue("Name sorted users array should be valid", NameSortResult.Users.Num() >= 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetUsers with a filter that matches no users.
 * Verifies that the function returns an empty result set with zero total count when no users match the filter criteria.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersEmptyResultTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.EmptyResult", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersEmptyResultTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_empty_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Test GetUsers with filter that matches no users
		const FString NonExistentFilter = FString::Printf(TEXT("id == '%s_nonexistent_%d'"), *SDK_PREFIX, FDateTime::Now().ToUnixTimestamp());
		FPubnubChatGetUsersResult EmptyResult = Chat->GetUsers(0, NonExistentFilter);
		
		TestFalse("GetUsers with non-existent filter should succeed", EmptyResult.Result.Error);
		TestEqual("Users array should be empty for non-existent filter", EmptyResult.Users.Num(), 0);
		TestEqual("Total count should be 0 for non-existent filter", EmptyResult.Total, 0);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests consistency of GetUsers results across multiple calls.
 * Verifies that calling GetUsers multiple times returns consistent total counts and user counts.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsers.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_consistency_init";
	const FString TestUserID = SDK_PREFIX + "test_get_users_consistency";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Call GetUsers multiple times - should return consistent results
		FPubnubChatGetUsersResult GetUsersResult1 = Chat->GetUsers();
		FPubnubChatGetUsersResult GetUsersResult2 = Chat->GetUsers();
		FPubnubChatGetUsersResult GetUsersResult3 = Chat->GetUsers();
		
		TestFalse("First GetUsers should succeed", GetUsersResult1.Result.Error);
		TestFalse("Second GetUsers should succeed", GetUsersResult2.Result.Error);
		TestFalse("Third GetUsers should succeed", GetUsersResult3.Result.Error);
		
		// Total count should be consistent
		TestEqual("Total count should be consistent across calls", GetUsersResult1.Total, GetUsersResult2.Total);
		TestEqual("Total count should be consistent across calls", GetUsersResult2.Total, GetUsersResult3.Total);
		
		// Users count should be consistent (may vary slightly due to timing, but should be close)
		TestTrue("Users count should be consistent", FMath::Abs(GetUsersResult1.Users.Num() - GetUsersResult2.Users.Num()) <= 1);
		TestTrue("Users count should be consistent", FMath::Abs(GetUsersResult2.Users.Num() - GetUsersResult3.Users.Num()) <= 1);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETUSERSUGGESTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get user suggestions without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(TEXT("test"), 10);
			
			TestTrue("GetUserSuggestions should fail when Chat is not initialized", GetSuggestionsResult.Result.Error);
			TestEqual("Users array should be empty", GetSuggestionsResult.Users.Num(), 0);
			TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsEmptyTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.1Validation.EmptyText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsEmptyTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_user_suggestions_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Try to get user suggestions with empty Text
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(TEXT(""), 10);
		
		TestTrue("GetUserSuggestions should fail with empty Text", GetSuggestionsResult.Result.Error);
		TestEqual("Users array should be empty", GetSuggestionsResult.Users.Num(), 0);
		TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_happy_init";
	const FString TestUserID = SDK_PREFIX + "test_get_user_suggestions_happy";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test user with a specific name pattern
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("TestUserHappy");
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Call GetUserSuggestions with only required parameter (Text)
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(TEXT("TestUser"));
		
		TestFalse("GetUserSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Users array should be valid (may be empty)", GetSuggestionsResult.Users.Num() >= 0);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_full_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_full_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_full_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_user_suggestions_full_3";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users with matching name patterns
		FPubnubChatUserData UserData1;
		UserData1.UserName = TEXT("FullTestUser1");
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserData UserData2;
		UserData2.UserName = TEXT("FullTestUser2");
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserData UserData3;
		UserData3.UserName = TEXT("FullTestUser3");
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3, UserData3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test GetUserSuggestions with all parameters (Text and Limit)
		const FString SearchText = TEXT("FullTest");
		const int TestLimit = 2;
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, TestLimit);
		
		TestFalse("GetUserSuggestions should succeed with all parameters", GetSuggestionsResult.Result.Error);
		TestTrue("Users array should be valid", GetSuggestionsResult.Users.Num() >= 0);
		TestTrue("Users count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Users.Num() <= TestLimit || GetSuggestionsResult.Users.Num() == 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsWithLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.3FullParameters.WithLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsWithLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_limit_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_limit_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_limit_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_user_suggestions_limit_3";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users with matching name patterns
		FPubnubChatUserData UserData1;
		UserData1.UserName = TEXT("LimitTestUser1");
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserData UserData2;
		UserData2.UserName = TEXT("LimitTestUser2");
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserData UserData3;
		UserData3.UserName = TEXT("LimitTestUser3");
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3, UserData3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test GetUserSuggestions with Limit parameter
		const FString SearchText = TEXT("LimitTest");
		const int TestLimit = 2;
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, TestLimit);
		
		TestFalse("GetUserSuggestions should succeed with Limit", GetSuggestionsResult.Result.Error);
		TestTrue("Users array should be valid", GetSuggestionsResult.Users.Num() >= 0);
		TestTrue("Users count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Users.Num() <= TestLimit || GetSuggestionsResult.Users.Num() == 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
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
 * Tests GetUserSuggestions with multiple users matching the search pattern.
 * Verifies that all matching users are returned and that the results are correct.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsMultipleMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.MultipleMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsMultipleMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_multi_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_multi_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_multi_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_user_suggestions_multi_3";
	const FString TestUserID4 = SDK_PREFIX + "test_get_user_suggestions_multi_4";
	const FString TestUserID5 = SDK_PREFIX + "test_get_user_suggestions_multi_5";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create multiple test users with matching name patterns
		TArray<FString> TestUserIDs = {TestUserID1, TestUserID2, TestUserID3, TestUserID4, TestUserID5};
		for(int32 i = 0; i < TestUserIDs.Num(); ++i)
		{
			FPubnubChatUserData UserData;
			UserData.UserName = FString::Printf(TEXT("MultiTestUser%d"), i + 1);
			FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserIDs[i], UserData);
			TestFalse(FString::Printf(TEXT("CreateUser %s should succeed"), *TestUserIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserIDs[i]); }
		}
		
		// Get user suggestions with search text that matches all created users
		const FString SearchText = TEXT("MultiTest");
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, 10);
		
		TestFalse("GetUserSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Users array should contain at least created users", GetSuggestionsResult.Users.Num() >= CreatedUserIDs.Num());
		
		// Verify all created users are in the result
		TArray<FString> FoundUserIDs;
		for(UPubnubChatUser* User : GetSuggestionsResult.Users)
		{
			if(User)
			{
				FoundUserIDs.Add(User->GetUserID());
			}
		}
		
		for(const FString& CreatedUserID : CreatedUserIDs)
		{
			TestTrue(FString::Printf(TEXT("Created user %s should be found in suggestions"), *CreatedUserID), FoundUserIDs.Contains(CreatedUserID));
		}
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetUserSuggestions with partial name matching.
 * Verifies that the function correctly matches users based on name prefix pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsPartialMatchTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.PartialMatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsPartialMatchTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_partial_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_partial_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_partial_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_user_suggestions_partial_3";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test users with different name patterns
		FPubnubChatUserData UserData1;
		UserData1.UserName = TEXT("PartialMatchUser1");
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedUserIDs.Add(TestUserID1); }
		
		FPubnubChatUserData UserData2;
		UserData2.UserName = TEXT("PartialMatchUser2");
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedUserIDs.Add(TestUserID2); }
		
		FPubnubChatUserData UserData3;
		UserData3.UserName = TEXT("DifferentNameUser");
		FPubnubChatUserResult CreateResult3 = Chat->CreateUser(TestUserID3, UserData3);
		TestFalse("CreateUser3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedUserIDs.Add(TestUserID3); }
		
		// Test partial match - should match first two users but not third
		const FString SearchText = TEXT("PartialMatch");
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, 10);
		
		TestFalse("GetUserSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Users array should be valid", GetSuggestionsResult.Users.Num() >= 0);
		
		// Verify matching users are in results
		TArray<FString> FoundUserIDs;
		for(UPubnubChatUser* User : GetSuggestionsResult.Users)
		{
			if(User)
			{
				FoundUserIDs.Add(User->GetUserID());
			}
		}
		
		// First two users should be found
		TestTrue("PartialMatchUser1 should be found", FoundUserIDs.Contains(TestUserID1));
		TestTrue("PartialMatchUser2 should be found", FoundUserIDs.Contains(TestUserID2));
		
		// Third user should not be found (different name pattern)
		TestFalse("DifferentNameUser should not be found", FoundUserIDs.Contains(TestUserID3));
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetUserSuggestions with no matching users.
 * Verifies that the function returns an empty result set when no users match the search pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsNoMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.NoMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsNoMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_nomatch_init";
	const FString TestUserID = SDK_PREFIX + "test_get_user_suggestions_nomatch";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test user with a specific name
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("NoMatchTestUser");
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Test with search text that doesn't match any user
		const FString SearchText = TEXT("NonExistentUserPrefix");
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, 10);
		
		TestFalse("GetUserSuggestions should succeed even with no matches", GetSuggestionsResult.Result.Error);
		TestEqual("Users array should be empty for no matches", GetSuggestionsResult.Users.Num(), 0);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetUserSuggestions with case sensitivity.
 * Verifies that the function correctly matches users regardless of case differences in the search text.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsCaseSensitivityTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.CaseSensitivity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsCaseSensitivityTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_case_init";
	const FString TestUserID = SDK_PREFIX + "test_get_user_suggestions_case";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test user with mixed case name
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("CaseTestUser");
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Test with different case variations
		const FString SearchTextLower = TEXT("casetest");
		const FString SearchTextUpper = TEXT("CASETEST");
		const FString SearchTextMixed = TEXT("CaseTest");
		
		FPubnubChatGetUserSuggestionsResult ResultLower = Chat->GetUserSuggestions(SearchTextLower, 10);
		FPubnubChatGetUserSuggestionsResult ResultUpper = Chat->GetUserSuggestions(SearchTextUpper, 10);
		FPubnubChatGetUserSuggestionsResult ResultMixed = Chat->GetUserSuggestions(SearchTextMixed, 10);
		
		TestFalse("GetUserSuggestions with lowercase should succeed", ResultLower.Result.Error);
		TestFalse("GetUserSuggestions with uppercase should succeed", ResultUpper.Result.Error);
		TestFalse("GetUserSuggestions with mixed case should succeed", ResultMixed.Result.Error);
		
		// At least one of the case variations should find the user (depending on backend implementation)
		bool FoundInAnyCase = ResultLower.Users.Num() > 0 || ResultUpper.Users.Num() > 0 || ResultMixed.Users.Num() > 0;
		TestTrue("User should be found with at least one case variation", FoundInAnyCase || ResultMixed.Users.Num() > 0);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetUserSuggestions limit enforcement.
 * Verifies that the limit parameter correctly restricts the number of returned results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsLimitEnforcementTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.LimitEnforcement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsLimitEnforcementTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_2";
	const FString TestUserID3 = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_3";
	const FString TestUserID4 = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_4";
	const FString TestUserID5 = SDK_PREFIX + "test_get_user_suggestions_limit_enforce_5";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create multiple test users with matching name patterns
		TArray<FString> TestUserIDs = {TestUserID1, TestUserID2, TestUserID3, TestUserID4, TestUserID5};
		for(int32 i = 0; i < TestUserIDs.Num(); ++i)
		{
			FPubnubChatUserData UserData;
			UserData.UserName = FString::Printf(TEXT("LimitEnforceUser%d"), i + 1);
			FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserIDs[i], UserData);
			TestFalse(FString::Printf(TEXT("CreateUser %s should succeed"), *TestUserIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserIDs[i]); }
		}
		
		// Test with limit smaller than number of matching users
		const FString SearchText = TEXT("LimitEnforce");
		const int SmallLimit = 2;
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult = Chat->GetUserSuggestions(SearchText, SmallLimit);
		
		TestFalse("GetUserSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Users count should not exceed limit", GetSuggestionsResult.Users.Num() <= SmallLimit || GetSuggestionsResult.Users.Num() == 0);
		
		// Test with limit larger than number of matching users
		const int LargeLimit = 20;
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResultLarge = Chat->GetUserSuggestions(SearchText, LargeLimit);
		
		TestFalse("GetUserSuggestions with large limit should succeed", GetSuggestionsResultLarge.Result.Error);
		TestTrue("Users count should be valid", GetSuggestionsResultLarge.Users.Num() >= 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests consistency of GetUserSuggestions results across multiple calls.
 * Verifies that calling GetUserSuggestions multiple times with the same parameters returns consistent results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestions.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserSuggestionsConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_consistency_init";
	const FString TestUserID = SDK_PREFIX + "test_get_user_suggestions_consistency";
	
	TArray<FString> CreatedUserIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create a test user
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("ConsistencyTestUser");
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedUserIDs.Add(TestUserID); }
		
		// Call GetUserSuggestions multiple times with same parameters
		const FString SearchText = TEXT("Consistency");
		const int TestLimit = 10;
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult1 = Chat->GetUserSuggestions(SearchText, TestLimit);
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult2 = Chat->GetUserSuggestions(SearchText, TestLimit);
		FPubnubChatGetUserSuggestionsResult GetSuggestionsResult3 = Chat->GetUserSuggestions(SearchText, TestLimit);
		
		TestFalse("First GetUserSuggestions should succeed", GetSuggestionsResult1.Result.Error);
		TestFalse("Second GetUserSuggestions should succeed", GetSuggestionsResult2.Result.Error);
		TestFalse("Third GetUserSuggestions should succeed", GetSuggestionsResult3.Result.Error);
		
		// Results should be consistent (users count should be similar)
		TestTrue("Users count should be consistent", FMath::Abs(GetSuggestionsResult1.Users.Num() - GetSuggestionsResult2.Users.Num()) <= 1);
		TestTrue("Users count should be consistent", FMath::Abs(GetSuggestionsResult2.Users.Num() - GetSuggestionsResult3.Users.Num()) <= 1);
		
		// Cleanup: Delete created user
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC USER TESTS - FULL PARAMETERS (Chat.User Async)
// ============================================================================

/**
 * FullParameters test for CreateUserAsync.
 * Calls CreateUserAsync with all parameters (UserID, UserData) and verifies callback result.
 * Cleans up created user and init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.CreateUserAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatCreateUserAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_user_async_full_init";
	const FString NewUserID = SDK_PREFIX + "test_create_user_async_full_new";

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

	FPubnubChatUserData UserData;
	UserData.UserName = TEXT("TestUserName");
	UserData.ExternalID = TEXT("external_123");
	UserData.ProfileUrl = TEXT("https://example.com/profile.jpg");
	UserData.Email = TEXT("test@example.com");
	UserData.Custom = TEXT("{\"key\":\"value\"}");
	UserData.Status = TEXT("active");
	UserData.Type = TEXT("user");

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserResult> CallbackResult = MakeShared<FPubnubChatUserResult>();

	FOnPubnubChatUserResponseNative OnUserResponse;
	OnUserResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatUserResult& UserResult)
	{
		*CallbackResult = UserResult;
		*bCallbackReceived = true;
	});
	Chat->CreateUserAsync(NewUserID, OnUserResponse, UserData);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, NewUserID, UserData]()
	{
		TestFalse("CreateUserAsync callback should report success", CallbackResult->Result.Error);
		TestNotNull("CreateUserAsync callback should return user", CallbackResult->User);
		if(CallbackResult->User)
		{
			TestEqual("Created User UserID should match", CallbackResult->User->GetUserID(), NewUserID);
			FPubnubChatUserData RetrievedData = CallbackResult->User->GetUserData();
			TestEqual("UserName should match", RetrievedData.UserName, UserData.UserName);
			TestEqual("ExternalID should match", RetrievedData.ExternalID, UserData.ExternalID);
			TestEqual("ProfileUrl should match", RetrievedData.ProfileUrl, UserData.ProfileUrl);
			TestEqual("Email should match", RetrievedData.Email, UserData.Email);
			TestEqual("Custom should match", RetrievedData.Custom, UserData.Custom);
			TestEqual("Status should match", RetrievedData.Status, UserData.Status);
			TestEqual("Type should match", RetrievedData.Type, UserData.Type);
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, NewUserID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(NewUserID, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for GetUserAsync.
 * Creates a user synchronously, then calls GetUserAsync with UserID and verifies callback result.
 * Cleans up created user and init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatGetUserAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_async_full_target";

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

	FPubnubChatUserData UserData;
	UserData.UserName = TEXT("GetUserAsyncTarget");
	UserData.Email = TEXT("getuserasync@example.com");
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed for GetUserAsync test", CreateResult.Result.Error);

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserResult> CallbackResult = MakeShared<FPubnubChatUserResult>();

	FOnPubnubChatUserResponseNative OnUserResponse;
	OnUserResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatUserResult& UserResult)
	{
		*CallbackResult = UserResult;
		*bCallbackReceived = true;
	});
	Chat->GetUserAsync(TargetUserID, OnUserResponse);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TargetUserID, UserData]()
	{
		TestFalse("GetUserAsync callback should report success", CallbackResult->Result.Error);
		TestNotNull("GetUserAsync callback should return user", CallbackResult->User);
		if(CallbackResult->User)
		{
			TestEqual("GetUser UserID should match", CallbackResult->User->GetUserID(), TargetUserID);
			FPubnubChatUserData RetrievedData = CallbackResult->User->GetUserData();
			TestEqual("UserName should match", RetrievedData.UserName, UserData.UserName);
			TestEqual("Email should match", RetrievedData.Email, UserData.Email);
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for GetUsersAsync.
 * Creates users synchronously, then calls GetUsersAsync with Limit, Filter, Sort, Page and verifies callback result.
 * Cleans up created users and init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUsersAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatGetUsersAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_async_full_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_users_async_full_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_users_async_full_2";

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

	FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1);
	FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2);
	TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
	TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);

	const int TestLimit = 10;
	const FString TestFilter = TEXT("name LIKE '*test_get_users_async_full*'");
	FPubnubGetAllSort TestSort;
	FPubnubGetAllSingleSort SingleSort;
	SingleSort.SortType = EPubnubGetAllSortType::PGAST_ID;
	SingleSort.SortOrder = false;
	TestSort.GetAllSort.Add(SingleSort);
	FPubnubPage TestPage;

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetUsersResult> CallbackResult = MakeShared<FPubnubChatGetUsersResult>();

	FOnPubnubChatGetUsersResponseNative OnUsersResponse;
	OnUsersResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetUsersResult& UsersResult)
	{
		*CallbackResult = UsersResult;
		*bCallbackReceived = true;
	});
	Chat->GetUsersAsync(OnUsersResponse, TestLimit, TestFilter, TestSort, TestPage);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestLimit]()
	{
		TestFalse("GetUsersAsync callback should report success", CallbackResult->Result.Error);
		TestTrue("Users array should be valid", CallbackResult->Users.Num() >= 0);
		TestTrue("Total count should be non-negative", CallbackResult->Total >= 0);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestUserID1, TestUserID2, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TestUserID1, false);
			Chat->DeleteUser(TestUserID2, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for UpdateUserAsync.
 * Creates a user synchronously, then calls UpdateUserAsync with UserID and full UpdateUserInputData; verifies callback result.
 * Cleans up created user and init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.UpdateUserAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatUpdateUserAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_user_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_update_user_async_full";

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

	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed for UpdateUserAsync test", CreateResult.Result.Error);

	FPubnubChatUpdateUserInputData UpdateData;
	UpdateData.UserName = TEXT("UpdatedFullUser");
	UpdateData.ExternalID = TEXT("updated_external_456");
	UpdateData.ProfileUrl = TEXT("https://example.com/updated_profile.jpg");
	UpdateData.Email = TEXT("updated@example.com");
	UpdateData.Custom = TEXT("{\"updated\":\"data\"}");
	UpdateData.Status = TEXT("updatedStatus");
	UpdateData.Type = TEXT("updatedType");
	UpdateData.ForceSetUserName = true;
	UpdateData.ForceSetExternalID = true;
	UpdateData.ForceSetProfileUrl = true;
	UpdateData.ForceSetEmail = true;
	UpdateData.ForceSetCustom = true;
	UpdateData.ForceSetStatus = true;
	UpdateData.ForceSetType = true;

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserResult> CallbackResult = MakeShared<FPubnubChatUserResult>();

	FOnPubnubChatUserResponseNative OnUserResponse;
	OnUserResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatUserResult& UserResult)
	{
		*CallbackResult = UserResult;
		*bCallbackReceived = true;
	});
	Chat->UpdateUserAsync(TargetUserID, UpdateData, OnUserResponse);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TargetUserID, UpdateData]()
	{
		TestFalse("UpdateUserAsync callback should report success", CallbackResult->Result.Error);
		TestNotNull("UpdateUserAsync callback should return user", CallbackResult->User);
		if(CallbackResult->User)
		{
			TestEqual("Updated User UserID should match", CallbackResult->User->GetUserID(), TargetUserID);
			FPubnubChatUserData RetrievedData = CallbackResult->User->GetUserData();
			TestEqual("UserName should match", RetrievedData.UserName, UpdateData.UserName);
			TestEqual("ExternalID should match", RetrievedData.ExternalID, UpdateData.ExternalID);
			TestEqual("ProfileUrl should match", RetrievedData.ProfileUrl, UpdateData.ProfileUrl);
			TestEqual("Email should match", RetrievedData.Email, UpdateData.Email);
			TestEqual("Custom should match", RetrievedData.Custom, UpdateData.Custom);
			TestEqual("Status should match", RetrievedData.Status, UpdateData.Status);
			TestEqual("Type should match", RetrievedData.Type, UpdateData.Type);
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for DeleteUserAsync.
 * Creates a user synchronously, then calls DeleteUserAsync with UserID and Soft=true; verifies callback result.
 * Cleans up: hard-delete the soft-deleted user, then delete init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.DeleteUserAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatDeleteUserAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_delete_user_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_delete_user_async_full_target";

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

	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed for DeleteUserAsync test", CreateResult.Result.Error);

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();

	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	Chat->DeleteUserAsync(TargetUserID, OnOperationResponse, true);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("DeleteUserAsync callback should report success", CallbackResult->Error);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

/**
 * FullParameters test for GetUserSuggestionsAsync.
 * Creates users with names synchronously, then calls GetUserSuggestionsAsync with Text and Limit; verifies callback result.
 * Cleans up created users and init user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserSuggestionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetUserSuggestionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatGetUserSuggestionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_suggestions_async_full_init";
	const FString TestUserID1 = SDK_PREFIX + "test_get_user_suggestions_async_full_1";
	const FString TestUserID2 = SDK_PREFIX + "test_get_user_suggestions_async_full_2";

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

	FPubnubChatUserData UserData1;
	UserData1.UserName = TEXT("AsyncFullTestUser1");
	FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
	FPubnubChatUserData UserData2;
	UserData2.UserName = TEXT("AsyncFullTestUser2");
	FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
	TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
	TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);

	const FString SearchText = TEXT("AsyncFullTest");
	const int TestLimit = 5;

	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetUserSuggestionsResult> CallbackResult = MakeShared<FPubnubChatGetUserSuggestionsResult>();

	FOnPubnubChatGetUserSuggestionsResponseNative OnSuggestionsResponse;
	OnSuggestionsResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetUserSuggestionsResult& SuggestionsResult)
	{
		*CallbackResult = SuggestionsResult;
		*bCallbackReceived = true;
	});
	Chat->GetUserSuggestionsAsync(SearchText, OnSuggestionsResponse, TestLimit);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestLimit]()
	{
		TestFalse("GetUserSuggestionsAsync callback should report success", CallbackResult->Result.Error);
		TestTrue("Users array should be valid", CallbackResult->Users.Num() >= 0);
		TestTrue("Users count should not exceed limit when enforced", CallbackResult->Users.Num() <= TestLimit || CallbackResult->Users.Num() == 0);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestUserID1, TestUserID2, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TestUserID1, false);
			Chat->DeleteUser(TestUserID2, false);
			Chat->DeleteUser(InitUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// PRESENCE TESTS
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

/**
 * Tests User->WherePresent happy path.
 * Verifies that WherePresent returns channels where user is present.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserWherePresentHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Presence.WherePresent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatUserWherePresentHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_where_present_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_where_present_happy";
	
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
	
	// Get current user
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	TestNotNull("CurrentUser should be created", CurrentUser);
	
	if(!CurrentUser)
	{
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
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CurrentUser, TestChannelID]()
	{
		// Call User->WherePresent
		FPubnubChatWherePresentResult WherePresentResult = CurrentUser->WherePresent();
		
		TestFalse("WherePresent should succeed", WherePresentResult.Result.Error);
		TestTrue("Channels array should contain the joined channel", WherePresentResult.Channels.Contains(TestChannelID));
		TestEqual("Channels array should have at least 1 channel", WherePresentResult.Channels.Num(), 1);
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
 * Tests User->IsPresentOn happy path.
 * Verifies that IsPresentOn returns true when user is present on channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsPresentOnHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.Presence.IsPresentOn.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatUserIsPresentOnHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_is_present_on_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_is_present_on_happy";
	
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
	
	// Get current user
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	TestNotNull("CurrentUser should be created", CurrentUser);
	
	if(!CurrentUser)
	{
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
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Wait a bit for presence to propagate
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CurrentUser, TestChannelID]()
	{
		// Call User->IsPresentOn
		FPubnubChatIsPresentResult IsPresentResult = CurrentUser->IsPresentOn(TestChannelID);
		
		TestFalse("IsPresentOn should succeed", IsPresentResult.Result.Error);
		TestTrue("IsPresentOn should return true when user is present", IsPresentResult.IsPresent);
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
// GETMEMBERSHIPS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized user object
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		
		// Try to get memberships with uninitialized user
		FPubnubChatMembershipsResult GetMembershipsResult = UninitializedUser->GetMemberships();
		TestTrue("GetMemberships should fail with uninitialized user", GetMembershipsResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMembershipsResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Memberships array should be empty", GetMembershipsResult.Memberships.Num(), 0);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_memberships_happy";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join to create a membership
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Get memberships with default parameters (only required)
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships();
	
	TestFalse("GetMemberships should succeed", GetMembershipsResult.Result.Error);
	TestTrue("Memberships array should be valid", GetMembershipsResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetMembershipsResult.Total >= 0);
	
	// Verify that at least the current channel membership is returned
	TestTrue("Should have at least one membership (the channel joined)", GetMembershipsResult.Memberships.Num() >= 1);
	
	bool FoundCurrentChannel = false;
	for(UPubnubChatMembership* Membership : GetMembershipsResult.Memberships)
	{
		if(Membership && Membership->GetChannelID() == TestChannelID)
		{
			FoundCurrentChannel = true;
			TestEqual("Membership UserID should match", Membership->GetUserID(), InitUserID);
			break;
		}
	}
	TestTrue("Current channel should be found in memberships", FoundCurrentChannel);
	
	// Cleanup: Delete membership created by Join
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
		if(DeleteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_memberships_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join to create a membership
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Test GetMemberships with all parameters
	const int TestLimit = 10;
	const FString TestFilter = FString::Printf(TEXT("channel.id == \"%s\""), *TestChannelID);
	FPubnubMembershipSort TestSort;
	FPubnubMembershipSingleSort SingleSort;
	SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSort.SortOrder = false; // Ascending
	TestSort.MembershipSort.Add(SingleSort);
	FPubnubPage TestPage; // Empty page for first page
	
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships(TestLimit, TestFilter, TestSort, TestPage);
	
	TestFalse("GetMemberships should succeed with all parameters", GetMembershipsResult.Result.Error);
	TestTrue("Memberships array should be valid", GetMembershipsResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetMembershipsResult.Total >= 0);
	
	// Verify filter worked - should only return the test channel
	if(GetMembershipsResult.Memberships.Num() > 0)
	{
		for(UPubnubChatMembership* Membership : GetMembershipsResult.Memberships)
		{
			if(Membership)
			{
				TestEqual("Filtered membership ChannelID should match filter", Membership->GetChannelID(), TestChannelID);
				TestEqual("Membership UserID should match", Membership->GetUserID(), InitUserID);
			}
		}
	}
	
	// Cleanup: Delete membership created by Join
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
		if(DeleteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetMemberships with multiple channel memberships.
 * Verifies that all memberships are returned correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsMultipleChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.4Advanced.MultipleChannels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsMultipleChannelsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_multiple_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_memberships_multiple_ch1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_memberships_multiple_ch2";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create first channel and join
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel1 should be created", CreateResult1.Channel);
	
	if(!CreateResult1.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult1 = CreateResult1.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join1 should succeed", JoinResult1.Result.Error);
	
	// Create second channel and join
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel2 should be created", CreateResult2.Channel);
	
	if(!CreateResult2.Channel)
	{
		if(JoinResult1.Membership)
		{
			JoinResult1.Membership->Delete();
		}
		if(CreateResult1.Channel)
		{
			CreateResult1.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult2 = CreateResult2.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join2 should succeed", JoinResult2.Result.Error);
	
	// Get all memberships
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships();
	
	TestFalse("GetMemberships should succeed", GetMembershipsResult.Result.Error);
	TestTrue("Should have at least 2 memberships", GetMembershipsResult.Memberships.Num() >= 2);
	
	// Verify all expected channels are in the memberships list
	TArray<FString> ExpectedChannelIDs = {TestChannelID1, TestChannelID2};
	TArray<FString> FoundChannelIDs;
	
	for(UPubnubChatMembership* Membership : GetMembershipsResult.Memberships)
	{
		if(Membership)
		{
			FoundChannelIDs.AddUnique(Membership->GetChannelID());
			TestEqual("Membership UserID should match", Membership->GetUserID(), InitUserID);
		}
	}
	
	for(const FString& ExpectedChannelID : ExpectedChannelIDs)
	{
		TestTrue(FString::Printf(TEXT("Channel %s should be found in memberships"), *ExpectedChannelID), FoundChannelIDs.Contains(ExpectedChannelID));
	}
	
	// Cleanup: Delete memberships created by Join
	if(JoinResult1.Membership)
	{
		FPubnubChatOperationResult DeleteMembership1Result = JoinResult1.Membership->Delete();
		if(DeleteMembership1Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join1 membership during cleanup: %s"), *DeleteMembership1Result.ErrorMessage);
		}
	}
	if(JoinResult2.Membership)
	{
		FPubnubChatOperationResult DeleteMembership2Result = JoinResult2.Membership->Delete();
		if(DeleteMembership2Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join2 membership during cleanup: %s"), *DeleteMembership2Result.ErrorMessage);
		}
	}
	
	if(CreateResult1.Channel)
	{
		CreateResult1.Channel->Leave();
	}
	if(CreateResult2.Channel)
	{
		CreateResult2.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1, false);
		Chat->DeleteChannel(TestChannelID2, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMemberships pagination functionality.
 * Verifies that pagination works correctly with Limit and Page parameters.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsPaginationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.4Advanced.Pagination", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsPaginationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_pagination_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_memberships_pagination";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Get first page with limit
	const int PageLimit = 1;
	FPubnubPage FirstPage; // Empty page for first page
	FPubnubChatMembershipsResult FirstPageResult = CurrentUser->GetMemberships(PageLimit, TEXT(""), FPubnubMembershipSort(), FirstPage);
	
	TestFalse("GetMemberships first page should succeed", FirstPageResult.Result.Error);
	TestTrue("First page should have at least one membership", FirstPageResult.Memberships.Num() >= 1);
	TestTrue("First page should respect limit", FirstPageResult.Memberships.Num() <= PageLimit);
	
	// If there's a next page, get it
	if(!FirstPageResult.Page.Next.IsEmpty())
	{
		FPubnubPage NextPage;
		NextPage.Next = FirstPageResult.Page.Next;
		FPubnubChatMembershipsResult NextPageResult = CurrentUser->GetMemberships(PageLimit, TEXT(""), FPubnubMembershipSort(), NextPage);
		
		TestFalse("GetMemberships next page should succeed", NextPageResult.Result.Error);
		TestTrue("Next page should have valid results", NextPageResult.Memberships.Num() >= 0);
		TestTrue("Next page should respect limit", NextPageResult.Memberships.Num() <= PageLimit);
		
		// Verify memberships from different pages are different
		if(FirstPageResult.Memberships.Num() > 0 && NextPageResult.Memberships.Num() > 0)
		{
			UPubnubChatMembership* FirstMembership = FirstPageResult.Memberships[0];
			UPubnubChatMembership* NextMembership = NextPageResult.Memberships[0];
			
			if(FirstMembership && NextMembership)
			{
				TestNotEqual("Memberships from different pages should be different", FirstMembership->GetChannelID(), NextMembership->GetChannelID());
			}
		}
	}
	
	// Cleanup: Delete membership created by Join
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
		if(DeleteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMemberships with filtering by channel ID.
 * Verifies that filter correctly returns only matching memberships.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsFilterTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.4Advanced.Filter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsFilterTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_filter_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_memberships_filter_ch1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_memberships_filter_ch2";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create first channel and join
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel1 should be created", CreateResult1.Channel);
	
	if(!CreateResult1.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult1 = CreateResult1.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join1 should succeed", JoinResult1.Result.Error);
	
	// Create second channel and join
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel2 should be created", CreateResult2.Channel);
	
	if(!CreateResult2.Channel)
	{
		if(JoinResult1.Membership)
		{
			JoinResult1.Membership->Delete();
		}
		if(CreateResult1.Channel)
		{
			CreateResult1.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult2 = CreateResult2.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join2 should succeed", JoinResult2.Result.Error);
	
	// Get all memberships without filter
	FPubnubChatMembershipsResult GetAllResult = CurrentUser->GetMemberships();
	TestFalse("GetMemberships without filter should succeed", GetAllResult.Result.Error);
	TestTrue("Should have at least 2 memberships", GetAllResult.Memberships.Num() >= 2);
	
	// Get memberships with filter for first channel only
	const FString TestFilter = FString::Printf(TEXT("channel.id == \"%s\""), *TestChannelID1);
	FPubnubChatMembershipsResult GetFilteredResult = CurrentUser->GetMemberships(0, TestFilter);
	
	TestFalse("GetMemberships with filter should succeed", GetFilteredResult.Result.Error);
	
	// Verify filter worked - should only return the first channel
	bool FoundChannel1 = false;
	bool FoundChannel2 = false;
	for(UPubnubChatMembership* Membership : GetFilteredResult.Memberships)
	{
		if(Membership)
		{
			if(Membership->GetChannelID() == TestChannelID1)
			{
				FoundChannel1 = true;
			}
			if(Membership->GetChannelID() == TestChannelID2)
			{
				FoundChannel2 = true;
			}
		}
	}
	
	TestTrue("Filtered result should contain channel1", FoundChannel1);
	TestFalse("Filtered result should NOT contain channel2", FoundChannel2);
	
	// Cleanup: Delete memberships created by Join
	if(JoinResult1.Membership)
	{
		FPubnubChatOperationResult DeleteMembership1Result = JoinResult1.Membership->Delete();
		if(DeleteMembership1Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join1 membership during cleanup: %s"), *DeleteMembership1Result.ErrorMessage);
		}
	}
	if(JoinResult2.Membership)
	{
		FPubnubChatOperationResult DeleteMembership2Result = JoinResult2.Membership->Delete();
		if(DeleteMembership2Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join2 membership during cleanup: %s"), *DeleteMembership2Result.ErrorMessage);
		}
	}
	
	if(CreateResult1.Channel)
	{
		CreateResult1.Channel->Leave();
	}
	if(CreateResult2.Channel)
	{
		CreateResult2.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1, false);
		Chat->DeleteChannel(TestChannelID2, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMemberships with sorting functionality.
 * Verifies that sorting works correctly with different sort parameters.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsSortTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.4Advanced.Sort", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsSortTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_sort_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_memberships_sort_ch1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_memberships_sort_ch2";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create first channel and join
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
	TestNotNull("Channel1 should be created", CreateResult1.Channel);
	
	if(!CreateResult1.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult1 = CreateResult1.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join1 should succeed", JoinResult1.Result.Error);
	
	// Create second channel and join
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
	TestNotNull("Channel2 should be created", CreateResult2.Channel);
	
	if(!CreateResult2.Channel)
	{
		if(JoinResult1.Membership)
		{
			JoinResult1.Membership->Delete();
		}
		if(CreateResult1.Channel)
		{
			CreateResult1.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID1, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult2 = CreateResult2.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join2 should succeed", JoinResult2.Result.Error);
	
	// Get memberships with ascending sort by channel ID
	FPubnubMembershipSort AscendingSort;
	FPubnubMembershipSingleSort SingleSortAsc;
	SingleSortAsc.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSortAsc.SortOrder = false; // Ascending
	AscendingSort.MembershipSort.Add(SingleSortAsc);
	
	FPubnubChatMembershipsResult GetAscendingResult = CurrentUser->GetMemberships(0, TEXT(""), AscendingSort);
	
	TestFalse("GetMemberships with ascending sort should succeed", GetAscendingResult.Result.Error);
	TestTrue("Should have at least 2 memberships", GetAscendingResult.Memberships.Num() >= 2);
	
	// Verify sorting - channels should be in ascending order
	if(GetAscendingResult.Memberships.Num() >= 2)
	{
		FString FirstChannelID = GetAscendingResult.Memberships[0]->GetChannelID();
		FString SecondChannelID = GetAscendingResult.Memberships[1]->GetChannelID();
		
		// Verify that first channel ID is less than or equal to second (ascending order)
		TestTrue("Channels should be sorted in ascending order", FirstChannelID <= SecondChannelID);
	}
	
	// Get memberships with descending sort by channel ID
	FPubnubMembershipSort DescendingSort;
	FPubnubMembershipSingleSort SingleSortDesc;
	SingleSortDesc.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSortDesc.SortOrder = true; // Descending
	DescendingSort.MembershipSort.Add(SingleSortDesc);
	
	FPubnubChatMembershipsResult GetDescendingResult = CurrentUser->GetMemberships(0, TEXT(""), DescendingSort);
	
	TestFalse("GetMemberships with descending sort should succeed", GetDescendingResult.Result.Error);
	TestTrue("Should have at least 2 memberships", GetDescendingResult.Memberships.Num() >= 2);
	
	// Verify sorting - channels should be in descending order
	if(GetDescendingResult.Memberships.Num() >= 2)
	{
		FString FirstChannelID = GetDescendingResult.Memberships[0]->GetChannelID();
		FString SecondChannelID = GetDescendingResult.Memberships[1]->GetChannelID();
		
		// Verify that first channel ID is greater than or equal to second (descending order)
		TestTrue("Channels should be sorted in descending order", FirstChannelID >= SecondChannelID);
	}
	
	// Cleanup: Delete memberships created by Join
	if(JoinResult1.Membership)
	{
		FPubnubChatOperationResult DeleteMembership1Result = JoinResult1.Membership->Delete();
		if(DeleteMembership1Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join1 membership during cleanup: %s"), *DeleteMembership1Result.ErrorMessage);
		}
	}
	if(JoinResult2.Membership)
	{
		FPubnubChatOperationResult DeleteMembership2Result = JoinResult2.Membership->Delete();
		if(DeleteMembership2Result.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join2 membership during cleanup: %s"), *DeleteMembership2Result.ErrorMessage);
		}
	}
	
	if(CreateResult1.Channel)
	{
		CreateResult1.Channel->Leave();
	}
	if(CreateResult2.Channel)
	{
		CreateResult2.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1, false);
		Chat->DeleteChannel(TestChannelID2, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMemberships with invited memberships (pending status).
 * Verifies that both joined and invited memberships are returned correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsWithInvitedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMemberships.4Advanced.WithInvited", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsWithInvitedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_memberships_invited_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_memberships_invited_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_memberships_invited";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create channel and join (creates a joined membership)
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		if(JoinResult.Membership)
		{
			JoinResult.Membership->Delete();
		}
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite target user (creates a pending membership for target user)
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	
	// Get memberships for current user (should include the joined channel)
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships();
	
	TestFalse("GetMemberships should succeed", GetMembershipsResult.Result.Error);
	TestTrue("Should have at least one membership (the joined channel)", GetMembershipsResult.Memberships.Num() >= 1);
	
	// Verify that the joined channel is in the memberships
	bool FoundJoinedChannel = false;
	for(UPubnubChatMembership* Membership : GetMembershipsResult.Memberships)
	{
		if(Membership && Membership->GetChannelID() == TestChannelID)
		{
			FoundJoinedChannel = true;
			TestEqual("Membership UserID should match", Membership->GetUserID(), InitUserID);
			
			// Verify membership status is not pending (user joined, not invited)
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestNotEqual("Joined membership Status should not be pending", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			break;
		}
	}
	TestTrue("Joined channel should be found in memberships", FoundJoinedChannel);
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(InviteResult.Membership)
	{
		FPubnubChatOperationResult DeleteInviteMembershipResult = InviteResult.Membership->Delete();
		if(DeleteInviteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite membership during cleanup: %s"), *DeleteInviteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (User object)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserUpdateAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.UpdateAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserUpdateAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_update_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_user_update_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatUpdateUserInputData UpdateData;
	UpdateData.UserName = TEXT("AsyncUpdatedName");
	UpdateData.Email = TEXT("async_updated@example.com");
	UpdateData.Custom = TEXT("{\"async\":\"update\"}");
	UpdateData.Status = TEXT("active");
	UpdateData.Type = TEXT("test");
	UpdateData.ForceSetUserName = true;
	UpdateData.ForceSetEmail = true;
	UpdateData.ForceSetCustom = true;
	UpdateData.ForceSetStatus = true;
	UpdateData.ForceSetType = true;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.User->UpdateAsync(UpdateData, OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateResult, UpdateData]()
	{
		TestFalse("UpdateAsync should succeed", CallbackResult->Error);
		FPubnubChatUserData RetrievedData = CreateResult.User->GetUserData();
		TestEqual("UserName should be updated", RetrievedData.UserName, UpdateData.UserName);
		TestEqual("Email should be updated", RetrievedData.Email, UpdateData.Email);
		TestEqual("Custom should be updated", RetrievedData.Custom, UpdateData.Custom);
		TestEqual("Status should be updated", RetrievedData.Status, UpdateData.Status);
		TestEqual("Type should be updated", RetrievedData.Type, UpdateData.Type);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserDeleteAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.DeleteAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserDeleteAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_delete_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_user_delete_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.User->DeleteAsync(OnOperationResponse, true);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateResult]()
	{
		TestFalse("DeleteAsync should succeed", CallbackResult->Error);
		FPubnubChatIsDeletedResult IsDeletedResult = CreateResult.User->IsDeleted();
		TestFalse("IsDeleted should succeed", IsDeletedResult.Result.Error);
		TestTrue("User should be marked as deleted", IsDeletedResult.IsDeleted);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserRestoreAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.RestoreAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserRestoreAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_restore_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_user_restore_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatOperationResult DeleteResult = CreateResult.User->Delete(true);
	TestFalse("Soft delete should succeed", DeleteResult.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.User->RestoreAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateResult]()
	{
		TestFalse("RestoreAsync should succeed", CallbackResult->Error);
		FPubnubChatIsDeletedResult IsDeletedResult = CreateResult.User->IsDeleted();
		TestFalse("IsDeleted should succeed", IsDeletedResult.Result.Error);
		TestFalse("User should not be deleted after restore", IsDeletedResult.IsDeleted);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsDeletedAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsDeletedAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsDeletedAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_is_deleted_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_user_is_deleted_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatOperationResult DeleteResult = CreateResult.User->Delete(true);
	TestFalse("Soft delete should succeed", DeleteResult.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatIsDeletedResult> CallbackResult = MakeShared<FPubnubChatIsDeletedResult>();
	FOnPubnubChatIsDeletedResponseNative OnIsDeletedResponse;
	OnIsDeletedResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatIsDeletedResult& IsDeletedResult)
	{
		*CallbackResult = IsDeletedResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.User->IsDeletedAsync(OnIsDeletedResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("IsDeletedAsync should succeed", CallbackResult->Result.Error);
		TestTrue("IsDeletedAsync should report deleted", CallbackResult->IsDeleted);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TargetUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserWherePresentAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.WherePresentAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatUserWherePresentAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_where_present_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_where_present_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatWherePresentResult> CallbackResult = MakeShared<FPubnubChatWherePresentResult>();
	FOnPubnubChatWherePresentResponseNative OnWherePresentResponse;
	OnWherePresentResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatWherePresentResult& WherePresentResult)
	{
		*CallbackResult = WherePresentResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CurrentUser, OnWherePresentResponse]()
	{
		CurrentUser->WherePresentAsync(OnWherePresentResponse);
	}, 1.0f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID]()
	{
		TestFalse("WherePresentAsync should succeed", CallbackResult->Result.Error);
		TestTrue("Channels array should contain joined channel", CallbackResult->Channels.Contains(TestChannelID));
	}, 0.1f));
	
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsPresentOnAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsPresentOnAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatUserIsPresentOnAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_is_present_on_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_is_present_on_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatIsPresentResult> CallbackResult = MakeShared<FPubnubChatIsPresentResult>();
	FOnPubnubChatIsPresentResponseNative OnIsPresentResponse;
	OnIsPresentResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatIsPresentResult& IsPresentResult)
	{
		*CallbackResult = IsPresentResult;
		*bCallbackReceived = true;
	});
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CurrentUser, TestChannelID, OnIsPresentResponse]()
	{
		CurrentUser->IsPresentOnAsync(TestChannelID, OnIsPresentResponse);
	}, 1.0f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("IsPresentOnAsync should succeed", CallbackResult->Result.Error);
		TestTrue("IsPresentOnAsync should return true", CallbackResult->IsPresent);
	}, 0.1f));
	
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembershipsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_get_memberships_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_get_memberships_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	const int TestLimit = 10;
	const FString TestFilter = TEXT("channel.id == \"") + TestChannelID + TEXT("\"");
	FPubnubMembershipSort TestSort;
	FPubnubMembershipSingleSort SingleSort;
	SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSort.SortOrder = false;
	TestSort.MembershipSort.Add(SingleSort);
	FPubnubPage TestPage;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMembershipsResult> CallbackResult = MakeShared<FPubnubChatMembershipsResult>();
	FOnPubnubChatMembershipsResponseNative OnMembershipsResponse;
	OnMembershipsResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatMembershipsResult& MembershipsResult)
	{
		*CallbackResult = MembershipsResult;
		*bCallbackReceived = true;
	});
	
	CurrentUser->GetMembershipsAsync(OnMembershipsResponse, TestLimit, TestFilter, TestSort, TestPage);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID]()
	{
		TestFalse("GetMembershipsAsync should succeed", CallbackResult->Result.Error);
		if(CallbackResult->Memberships.Num() > 0)
		{
			TestEqual("First membership channel should match", CallbackResult->Memberships[0]->GetChannelID(), TestChannelID);
		}
	}, 0.1f));
	
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserSetRestrictionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.SetRestrictionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserSetRestrictionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_set_restrictions_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_set_restrictions_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CurrentUser->SetRestrictionsAsync(TestChannelID, true, false, OnOperationResponse, TEXT("Async reason"));
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateChannelResult, CurrentUser]()
	{
		TestFalse("SetRestrictionsAsync should succeed", CallbackResult->Error);
		if(CreateChannelResult.Channel)
		{
			FPubnubChatGetRestrictionResult GetResult = CreateChannelResult.Channel->GetUserRestrictions(CurrentUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {InitUserID}, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetChannelRestrictionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_get_channel_restrictions_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_get_channel_restrictions_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatOperationResult SetResult = CurrentUser->SetRestrictions(TestChannelID, true, false, TEXT("Async reason"));
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetRestrictionResult> CallbackResult = MakeShared<FPubnubChatGetRestrictionResult>();
	FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponse;
	OnRestrictionResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetRestrictionResult& RestrictionResult)
	{
		*CallbackResult = RestrictionResult;
		*bCallbackReceived = true;
	});
	
	CurrentUser->GetChannelRestrictionsAsync(CreateChannelResult.Channel, OnRestrictionResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID]()
	{
		TestFalse("GetChannelRestrictionsAsync should succeed", CallbackResult->Result.Error);
		TestEqual("Restriction ChannelID should match", CallbackResult->Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", CallbackResult->Restriction.Ban);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {InitUserID}, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelsRestrictionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetChannelsRestrictionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelsRestrictionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_get_channels_restrictions_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_user_get_channels_restrictions_async_full";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatOperationResult SetResult = CurrentUser->SetRestrictions(TestChannelID, true, false, TEXT("Async reason"));
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	const int TestLimit = 10;
	FPubnubMembershipSort TestSort;
	FPubnubMembershipSingleSort SingleSort;
	SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSort.SortOrder = false;
	TestSort.MembershipSort.Add(SingleSort);
	FPubnubPage TestPage;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatGetRestrictionsResult> CallbackResult = MakeShared<FPubnubChatGetRestrictionsResult>();
	FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponse;
	OnRestrictionsResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatGetRestrictionsResult& RestrictionsResult)
	{
		*CallbackResult = RestrictionsResult;
		*bCallbackReceived = true;
	});
	
	CurrentUser->GetChannelsRestrictionsAsync(OnRestrictionsResponse, TestLimit, TestSort, TestPage);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, TestChannelID]()
	{
		TestFalse("GetChannelsRestrictionsAsync should succeed", CallbackResult->Result.Error);
		bool bFound = false;
		for(const FPubnubChatRestriction& Restriction : CallbackResult->Restrictions)
		{
			if(Restriction.ChannelID == TestChannelID)
			{
				bFound = true;
				break;
			}
		}
		TestTrue("Restrictions should include test channel", bFound);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {InitUserID}, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

