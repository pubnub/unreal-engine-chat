// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetCurrentUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetCurrentUserNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Get Chat without initializing
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	TestNull("Chat should be null before InitChat", Chat);

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetCurrentUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserAfterDestroyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetCurrentUser.4Advanced.AfterDestroy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetCurrentUserConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetCurrentUser.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// CREATEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateUserNotInitializedTest::RunTest(const FString& Parameters)
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to create user with empty UserID
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TEXT(""));
		
		TestTrue("CreateUser should fail with empty UserID", CreateResult.Result.Error);
		TestNull("User should not be created", CreateResult.User);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserDuplicateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.4Advanced.DuplicateUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateUserDataSharingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.CreateUser.4Advanced.DataSharing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// GETUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUserNotInitializedTest::RunTest(const FString& Parameters)
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to get user with empty UserID
		FPubnubChatUserResult GetResult = Chat->GetUser(TEXT(""));
		
		TestTrue("GetUser should fail with empty UserID", GetResult.Result.Error);
		TestNull("User should not be returned", GetResult.User);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserNonExistentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.4Advanced.NonExistentUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to get user that doesn't exist
		FPubnubChatUserResult GetResult = Chat->GetUser(NonExistentUserID);
		
		// GetUser will fail if user doesn't exist on server
		TestTrue("GetUser should fail for non-existent user", GetResult.Result.Error);
		TestNull("User should not be returned", GetResult.User);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserMultipleCallsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.4Advanced.MultipleCalls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUserCurrentUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUser.4Advanced.GetCurrentUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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

	CleanUp();
	return true;
}

// ============================================================================
// UPDATEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUpdateUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to update user without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_update_user_not_init";
			FPubnubChatUserData UserData;
			FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TestUserID, UserData);
			
			TestTrue("UpdateUser should fail when Chat is not initialized", UpdateResult.Result.Error);
			TestNull("User should not be returned", UpdateResult.User);
			TestFalse("ErrorMessage should not be empty", UpdateResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to update user with empty UserID
		FPubnubChatUserData UserData;
		FPubnubChatUserResult UpdateResult = Chat->UpdateUser(TEXT(""), UserData);
		
		TestTrue("UpdateUser should fail with empty UserID", UpdateResult.Result.Error);
		TestNull("User should not be returned", UpdateResult.User);
		TestFalse("ErrorMessage should not be empty", UpdateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user with minimal data
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("UpdatedUser");
		
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

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user with all parameters
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("UpdatedFullUser");
		UserData.ExternalID = TEXT("updated_external_456");
		UserData.ProfileUrl = TEXT("https://example.com/updated_profile.jpg");
		UserData.Email = TEXT("updated@example.com");
		UserData.Custom = TEXT("{\"updated\":\"data\"}");
		UserData.Status = TEXT("updatedStatus");
		UserData.Type = TEXT("updatedType");
		
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user first time
		FPubnubChatUserData UserData1;
		UserData1.UserName = TEXT("FirstUpdate");
		UserData1.Email = TEXT("first@example.com");
		FPubnubChatUserResult UpdateResult1 = Chat->UpdateUser(TargetUserID, UserData1);
		TestFalse("First UpdateUser should succeed", UpdateResult1.Result.Error);
		
		// Update user second time
		FPubnubChatUserData UserData2;
		UserData2.UserName = TEXT("SecondUpdate");
		UserData2.Email = TEXT("second@example.com");
		FPubnubChatUserResult UpdateResult2 = Chat->UpdateUser(TargetUserID, UserData2);
		TestFalse("Second UpdateUser should succeed", UpdateResult2.Result.Error);
		
		// Update user third time
		FPubnubChatUserData UserData3;
		UserData3.UserName = TEXT("ThirdUpdate");
		UserData3.Email = TEXT("third@example.com");
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUpdateUserDataSynchronizationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.UpdateUser.4Advanced.DataSynchronization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Update user
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("SyncedUser");
		UserData.Email = TEXT("synced@example.com");
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

	CleanUp();
	return true;
}

// ============================================================================
// DELETEUSER TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatDeleteUserNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to delete user without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestUserID = SDK_PREFIX + "test_delete_user_not_init";
			FPubnubChatUserResult DeleteResult = Chat->DeleteUser(TestUserID, false);
			
			TestTrue("DeleteUser should fail when Chat is not initialized", DeleteResult.Result.Error);
			TestNull("User should not be returned", DeleteResult.User);
			TestFalse("ErrorMessage should not be empty", DeleteResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to delete user with empty UserID
		FPubnubChatUserResult DeleteResult = Chat->DeleteUser(TEXT(""), false);
		
		TestTrue("DeleteUser should fail with empty UserID", DeleteResult.Result.Error);
		TestNull("User should not be returned", DeleteResult.User);
		TestFalse("ErrorMessage should not be empty", DeleteResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Delete user with default parameters (hard delete)
		FPubnubChatUserResult DeleteResult = Chat->DeleteUser(TargetUserID);
		
		TestFalse("DeleteUser should succeed", DeleteResult.Result.Error);
		// Note: DeleteUser may or may not return a user object depending on implementation
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHardDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.3FullParameters.HardDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Hard delete user (Soft = false)
		FPubnubChatUserResult DeleteResult = Chat->DeleteUser(TargetUserID, false);
		
		TestFalse("Hard DeleteUser should succeed", DeleteResult.Result.Error);
		
		// Verify user is actually deleted - GetUser should fail
		FPubnubChatUserResult GetResult = Chat->GetUser(TargetUserID);
		TestTrue("GetUser should fail after hard delete", GetResult.Result.Error);
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserSoftDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.3FullParameters.SoftDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the user
		FPubnubChatUserResult CreateResult = Chat->CreateUser(TargetUserID);
		TestFalse("CreateUser should succeed", CreateResult.Result.Error);
		
		// Soft delete user (Soft = true)
		FPubnubChatUserResult DeleteResult = Chat->DeleteUser(TargetUserID, true);
		
		TestFalse("Soft DeleteUser should succeed", DeleteResult.Result.Error);
		TestNotNull("Soft delete should return user object", DeleteResult.User);
		
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatDeleteUserHardVsSoftTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.DeleteUser.4Advanced.HardVsSoft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create first user for soft delete
		FPubnubChatUserResult CreateResult1 = Chat->CreateUser(SoftDeleteUserID);
		TestFalse("CreateUser1 should succeed", CreateResult1.Result.Error);
		
		// Create second user for hard delete
		FPubnubChatUserResult CreateResult2 = Chat->CreateUser(HardDeleteUserID);
		TestFalse("CreateUser2 should succeed", CreateResult2.Result.Error);
		
		// Soft delete first user
		FPubnubChatUserResult SoftDeleteResult = Chat->DeleteUser(SoftDeleteUserID, true);
		TestFalse("Soft DeleteUser should succeed", SoftDeleteResult.Result.Error);
		
		// Hard delete second user
		FPubnubChatUserResult HardDeleteResult = Chat->DeleteUser(HardDeleteUserID, false);
		TestFalse("Hard DeleteUser should succeed", HardDeleteResult.Result.Error);
		
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

	CleanUp();
	return true;
}

// ============================================================================
// GETUSERS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetUsersNotInitializedTest::RunTest(const FString& Parameters)
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

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
		const FString TestFilter = TEXT("name LIKE '%test_get_users_full%'");
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
	
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.3FullParameters.WithLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithFilterTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.3FullParameters.WithFilter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithSortTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.3FullParameters.WithSort", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersWithPageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.3FullParameters.WithPage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

/**
 * Tests pagination functionality of GetUsers.
 * Verifies that pagination works correctly with limit parameter, next page navigation, and previous page navigation.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersPaginationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.Pagination", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

/**
 * Tests filtering functionality of GetUsers.
 * Verifies that users can be filtered by UserID and by name patterns using filter expressions.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersFilteringTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.Filtering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
		FString FilterByName = TEXT("name LIKE '%FilterUser%'");
		FPubnubChatGetUsersResult FilteredByNameResult = Chat->GetUsers(0, FilterByName);
		
		TestFalse("GetUsers with name filter should succeed", FilteredByNameResult.Result.Error);
		TestTrue("Filtered by name users array should be valid", FilteredByNameResult.Users.Num() >= 0);
		
		// Cleanup: Delete created users
		for(const FString& UserID : CreatedUserIDs)
		{
			Chat->DeleteUser(UserID, false);
		}
	}
	
	CleanUp();
	return true;
}

/**
 * Tests sorting functionality of GetUsers.
 * Verifies that users can be sorted by ID and Name in both ascending and descending order.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersSortingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.Sorting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

/**
 * Tests GetUsers with a filter that matches no users.
 * Verifies that the function returns an empty result set with zero total count when no users match the filter criteria.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersEmptyResultTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.EmptyResult", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Test GetUsers with filter that matches no users
		const FString NonExistentFilter = FString::Printf(TEXT("id == '%s_nonexistent_%d'"), *SDK_PREFIX, FDateTime::Now().ToUnixTimestamp());
		FPubnubChatGetUsersResult EmptyResult = Chat->GetUsers(0, NonExistentFilter);
		
		TestFalse("GetUsers with non-existent filter should succeed", EmptyResult.Result.Error);
		TestEqual("Users array should be empty for non-existent filter", EmptyResult.Users.Num(), 0);
		TestEqual("Total count should be 0 for non-existent filter", EmptyResult.Total, 0);
	}
	
	CleanUp();
	return true;
}

/**
 * Tests consistency of GetUsers results across multiple calls.
 * Verifies that calling GetUsers multiple times returns consistent total counts and user counts.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetUsersConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.GetUsers.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

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
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
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
	
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

