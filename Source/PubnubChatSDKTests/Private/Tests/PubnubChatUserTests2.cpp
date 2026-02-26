// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// STREAMUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesNotInitializedTest::RunTest(const FString& Parameters)
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
		// Create uninitialized user object
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		
		// Try to stream updates with uninitialized user
		FPubnubChatOperationResult StreamUpdatesResult = UninitializedUser->StreamUpdates();
		TestTrue("StreamUpdates should fail with uninitialized user", StreamUpdatesResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamUpdatesResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_happy_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_happy";
	
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
	FPubnubChatUserData InitialUserData;
	InitialUserData.UserName = TEXT("InitialUserName");
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, InitialUserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for update reception
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FString> ReceivedUserID = MakeShared<FString>(TEXT(""));
	TSharedPtr<FPubnubChatUserData> ReceivedUserData = MakeShared<FPubnubChatUserData>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedUserID, ReceivedUserData](FString UserID, const FPubnubChatUserData& UserData)
	{
		*bUpdateReceived = true;
		*ReceivedUserID = UserID;
		*ReceivedUserData = UserData;
	};
	CreateResult.User->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates (no parameters required)
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Update user to trigger update event
		FPubnubChatUpdateUserInputData UpdatedUserData;
		UpdatedUserData.UserName = TEXT("UpdatedUserName");
		UpdatedUserData.ForceSetUserName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(UpdatedUserData);
		TestFalse("Update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify update was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedUserID, ReceivedUserData, TestUserID, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		TestEqual("Received UserID should match", *ReceivedUserID, TestUserID);
		TestEqual("Received UserName should be updated", ReceivedUserData->UserName, TEXT("UpdatedUserName"));
		
		// Verify user data was updated in repository
		FPubnubChatUserData RetrievedData = CreateResult.User->GetUserData();
		TestEqual("Retrieved UserName should be updated", RetrievedData.UserName, TEXT("UpdatedUserName"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestUserID]()
	{
		if(CreateResult.User)
		{
			CreateResult.User->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
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
 * Tests StreamUpdates with partial field updates.
 * Verifies that only updated fields are changed and other fields remain unchanged.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesPartialUpdateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdates.4Advanced.PartialUpdate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesPartialUpdateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_partial_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_partial";
	
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
	
	// Create user with initial data
	FPubnubChatUserData InitialUserData;
	InitialUserData.UserName = TEXT("InitialName");
	InitialUserData.Email = TEXT("initial@example.com");
	InitialUserData.Status = TEXT("active");
	InitialUserData.Type = TEXT("premium");
	InitialUserData.Custom = TEXT("{\"key\":\"initial\"}");
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, InitialUserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for update reception
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserData> ReceivedUserData = MakeShared<FPubnubChatUserData>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedUserData](FString UserID, const FPubnubChatUserData& UserData)
	{
		*bUpdateReceived = true;
		*ReceivedUserData = UserData;
	};
	CreateResult.User->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription, then update only some fields
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Update only UserName and Email, leave others unchanged
		FPubnubChatUpdateUserInputData PartialUpdateData;
		PartialUpdateData.UserName = TEXT("UpdatedName");
		PartialUpdateData.Email = TEXT("updated@example.com");
		PartialUpdateData.ForceSetUserName = true;
		PartialUpdateData.ForceSetEmail = true;
		// Status, Type, and Custom are empty, so they should remain unchanged
		
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(PartialUpdateData);
		TestFalse("Partial update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify partial update was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedUserData, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		
		// Verify updated fields
		TestEqual("UserName should be updated", ReceivedUserData->UserName, TEXT("UpdatedName"));
		TestEqual("Email should be updated", ReceivedUserData->Email, TEXT("updated@example.com"));
		
		// Verify unchanged fields remain the same
		TestEqual("Status should remain unchanged", ReceivedUserData->Status, TEXT("active"));
		TestEqual("Type should remain unchanged", ReceivedUserData->Type, TEXT("premium"));
		TestEqual("Custom should remain unchanged", ReceivedUserData->Custom, TEXT("{\"key\":\"initial\"}"));
		
		// Verify user data in repository matches
		FPubnubChatUserData RetrievedData = CreateResult.User->GetUserData();
		TestEqual("Retrieved UserName should match", RetrievedData.UserName, TEXT("UpdatedName"));
		TestEqual("Retrieved Email should match", RetrievedData.Email, TEXT("updated@example.com"));
		TestEqual("Retrieved Status should match", RetrievedData.Status, TEXT("active"));
		TestEqual("Retrieved Type should match", RetrievedData.Type, TEXT("premium"));
		TestEqual("Retrieved Custom should match", RetrievedData.Custom, TEXT("{\"key\":\"initial\"}"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestUserID]()
	{
		if(CreateResult.User)
		{
			CreateResult.User->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests StreamUpdates with multiple sequential updates.
 * Verifies that multiple updates are received correctly in sequence.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesMultipleUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdates.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesMultipleUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_multiple_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_multiple";
	
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
	FPubnubChatUserData InitialUserData;
	InitialUserData.UserName = TEXT("InitialName");
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, InitialUserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Track received updates
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatUserData>> ReceivedUpdates = MakeShared<TArray<FPubnubChatUserData>>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, UpdateCount, ReceivedUpdates](FString UserID, const FPubnubChatUserData& UserData)
	{
		*UpdateCount = *UpdateCount + 1;
		ReceivedUpdates->Add(UserData);
	};
	CreateResult.User->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription, then send first update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateUserInputData FirstUpdate;
		FirstUpdate.UserName = TEXT("FirstUpdate");
		FirstUpdate.ForceSetUserName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(FirstUpdate);
		TestFalse("First update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait for first update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Send second update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateUserInputData SecondUpdate;
		SecondUpdate.UserName = TEXT("SecondUpdate");
		SecondUpdate.ForceSetUserName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(SecondUpdate);
		TestFalse("Second update should succeed", UpdateResult.Error);
	}, 0.1f));
	
	// Wait for second update to be received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify both updates were received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedUpdates]()
	{
		TestTrue("Both updates should have been received", *UpdateCount >= 2);
		if(*UpdateCount >= 2)
		{
			TestEqual("First update UserName should match", (*ReceivedUpdates)[0].UserName, TEXT("FirstUpdate"));
			TestEqual("Second update UserName should match", (*ReceivedUpdates)[1].UserName, TEXT("SecondUpdate"));
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestUserID]()
	{
		if(CreateResult.User)
		{
			CreateResult.User->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests StreamUpdates with user delete event.
 * Verifies that delete events are received correctly and user is removed from repository.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesDeleteEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdates.4Advanced.DeleteEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesDeleteEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_delete_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_delete";
	
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
	FPubnubChatUserData InitialUserData;
	InitialUserData.UserName = TEXT("UserToDelete");
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, InitialUserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for deletion reception
	TSharedPtr<bool> bDeleteReceived = MakeShared<bool>(false);
	
	// Set up delegate to receive user removal
	auto DeletedLambda = [this, bDeleteReceived]()
	{
		*bDeleteReceived = true;
	};
	CreateResult.User->OnDeletedNative.AddLambda(DeletedLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription to be ready, then delete user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestUserID]()
	{
		// Delete user - this should trigger a delete event
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TestUserID);
		TestFalse("DeleteUser should succeed", DeleteResult.Error);
	}, 0.5f));
	
	// Wait until delete event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bDeleteReceived]() -> bool {
		return *bDeleteReceived;
	}, MAX_WAIT_TIME));
	
	// Verify delete event was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bDeleteReceived]()
	{
		TestTrue("Delete event should have been received", *bDeleteReceived);
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
// STREAMUPDATESON TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesOnEmptyArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesOn.1Validation.EmptyArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesOnEmptyArrayTest::RunTest(const FString& Parameters)
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
	TArray<UPubnubChatUser*> EmptyUsersArray;
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatUser::StreamUpdatesOn(EmptyUsersArray);
	
	// Should succeed but do nothing (no users to process)
	TestFalse("StreamUpdatesOn with empty array should succeed", StreamUpdatesOnResult.Error);
	TestEqual("StepResults should be empty for empty array", StreamUpdatesOnResult.StepResults.Num(), 0);
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesOnUninitializedUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesOn.1Validation.UninitializedUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesOnUninitializedUsersTest::RunTest(const FString& Parameters)
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
	
	// Create uninitialized user objects
	UPubnubChatUser* UninitializedUser1 = NewObject<UPubnubChatUser>(Chat);
	UPubnubChatUser* UninitializedUser2 = NewObject<UPubnubChatUser>(Chat);
	
	// Call StreamUpdatesOn with uninitialized users
	TArray<UPubnubChatUser*> UninitializedUsersArray;
	UninitializedUsersArray.Add(UninitializedUser1);
	UninitializedUsersArray.Add(UninitializedUser2);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatUser::StreamUpdatesOn(UninitializedUsersArray);
	
	// Should fail because all users are uninitialized
	TestTrue("StreamUpdatesOn should fail with uninitialized users", StreamUpdatesOnResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesOnHappyPathSingleUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesOn.2HappyPath.SingleUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesOnHappyPathSingleUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_single_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_on_single";
	
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
	FPubnubChatUserData UserData;
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Call StreamUpdatesOn with single user
	TArray<UPubnubChatUser*> UsersArray;
	UsersArray.Add(CreateResult.User);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatUser::StreamUpdatesOn(UsersArray);
	
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
	
	// Cleanup: Stop streaming updates and delete user
	if(CreateResult.User)
	{
		CreateResult.User->StopStreamingUpdates();
	}
	if(Chat)
	{
		Chat->DeleteUser(TestUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesOnHappyPathMultipleUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesOn.2HappyPath.MultipleUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesOnHappyPathMultipleUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_multi_init";
	const FString TestUserID1 = SDK_PREFIX + "test_stream_updates_on_multi_1";
	const FString TestUserID2 = SDK_PREFIX + "test_stream_updates_on_multi_2";
	
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
	
	// Create users
	FPubnubChatUserData UserData1;
	FPubnubChatUserResult CreateResult1 = Chat->CreateUser(TestUserID1, UserData1);
	TestFalse("CreateUser 1 should succeed", CreateResult1.Result.Error);
	TestNotNull("User 1 should be created", CreateResult1.User);
	
	FPubnubChatUserData UserData2;
	FPubnubChatUserResult CreateResult2 = Chat->CreateUser(TestUserID2, UserData2);
	TestFalse("CreateUser 2 should succeed", CreateResult2.Result.Error);
	TestNotNull("User 2 should be created", CreateResult2.User);
	
	if(!CreateResult1.User || !CreateResult2.User)
	{
		if(CreateResult1.User && Chat) Chat->DeleteUser(TestUserID1);
		if(CreateResult2.User && Chat) Chat->DeleteUser(TestUserID2);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Call StreamUpdatesOn with both users
	TArray<UPubnubChatUser*> UsersArray;
	UsersArray.Add(CreateResult1.User);
	UsersArray.Add(CreateResult2.User);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatUser::StreamUpdatesOn(UsersArray);
	
	TestFalse("StreamUpdatesOn should succeed", StreamUpdatesOnResult.Error);
	TestTrue("Should have at least two step results (one per user)", StreamUpdatesOnResult.StepResults.Num() >= 2);
	
	// Verify that StreamUpdates was called successfully for both users
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
	
	// Cleanup: Stop streaming updates and delete users
	if(CreateResult1.User)
	{
		CreateResult1.User->StopStreamingUpdates();
	}
	if(CreateResult2.User)
	{
		CreateResult2.User->StopStreamingUpdates();
	}
	if(Chat)
	{
		Chat->DeleteUser(TestUserID1);
		Chat->DeleteUser(TestUserID2);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesOnMixedUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesOn.4Advanced.MixedUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesOnMixedUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_on_mixed_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_on_mixed";
	
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
	
	// Create one initialized user
	FPubnubChatUserData UserData;
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create uninitialized user
	UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
	
	// Call StreamUpdatesOn with mixed users (initialized + uninitialized)
	TArray<UPubnubChatUser*> UsersArray;
	UsersArray.Add(CreateResult.User);
	UsersArray.Add(UninitializedUser);
	
	FPubnubChatOperationResult StreamUpdatesOnResult = UPubnubChatUser::StreamUpdatesOn(UsersArray);
	
	// Should fail because one user is uninitialized
	TestTrue("StreamUpdatesOn should fail with mixed users", StreamUpdatesOnResult.Error);
	TestFalse("ErrorMessage should not be empty", StreamUpdatesOnResult.ErrorMessage.IsEmpty());
	
	// Verify that at least one Subscribe step succeeded (from initialized user)
	// and errors from uninitialized user are merged
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
	
	// Note: The initialized user might succeed, but overall result should be error due to uninitialized user
	// Check both StepResults errors and overall Error flag (uninitialized user returns early without steps)
	TestTrue("Should have error from uninitialized user", bFoundError || StreamUpdatesOnResult.Error);
	
	// Cleanup: Stop streaming updates if it was started
	if(CreateResult.User)
	{
		CreateResult.User->StopStreamingUpdates();
	}
	if(Chat)
	{
		Chat->DeleteUser(TestUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// STOPSTREAMINGUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingUpdatesNotInitializedTest::RunTest(const FString& Parameters)
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
		// Create uninitialized user object
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		
		// Try to stop streaming updates with uninitialized user
		FPubnubChatOperationResult StopResult = UninitializedUser->StopStreamingUpdates();
		TestTrue("StopStreamingUpdates should fail with uninitialized user", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_happy_init";
	const FString TestUserID = SDK_PREFIX + "test_stop_streaming_happy";
	
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
	FPubnubChatUserData UserData;
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Stop streaming updates (no parameters required)
	FPubnubChatOperationResult StopResult = CreateResult.User->StopStreamingUpdates();
	TestFalse("StopStreamingUpdates should succeed", StopResult.Error);
	
	// Verify step results contain Unsubscribe step
	bool bFoundUnsubscribeStep = false;
	for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
	{
		if(Step.StepName == TEXT("Unsubscribe"))
		{
			bFoundUnsubscribeStep = true;
			TestFalse("Unsubscribe step should not have error", Step.OperationResult.Error);
			break;
		}
	}
	TestTrue("Should have Unsubscribe step", bFoundUnsubscribeStep);
	
	// Cleanup: Delete user
	if(Chat)
	{
		Chat->DeleteUser(TestUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that StopStreamingUpdates prevents further updates from being received.
 * Verifies that after stopping, user updates no longer trigger the delegate.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingUpdatesPreventsUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingUpdates.4Advanced.PreventsUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingUpdatesPreventsUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_prevents_init";
	const FString TestUserID = SDK_PREFIX + "test_stop_streaming_prevents";
	
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
	FPubnubChatUserData InitialUserData;
	InitialUserData.UserName = TEXT("InitialName");
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, InitialUserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Track received updates
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, UpdateCount](FString UserID, const FPubnubChatUserData& UserData)
	{
		*UpdateCount = *UpdateCount + 1;
	};
	CreateResult.User->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription, then send an update that should be received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateUserInputData FirstUpdate;
		FirstUpdate.UserName = TEXT("FirstUpdate");
		FirstUpdate.ForceSetUserName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(FirstUpdate);
		TestFalse("First update should succeed", UpdateResult.Error);
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, UpdateCount]()
	{
		FPubnubChatOperationResult StopResult = CreateResult.User->StopStreamingUpdates();
		TestFalse("StopStreamingUpdates should succeed", StopResult.Error);
		
		// Record update count before second update
		int32 CountBeforeStop = *UpdateCount;
		
		// Send second update after stopping
		FPubnubChatUpdateUserInputData SecondUpdate;
		SecondUpdate.UserName = TEXT("SecondUpdate");
		SecondUpdate.ForceSetUserName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.User->Update(SecondUpdate);
		TestFalse("Second update should succeed", UpdateResult.Error);
		
		// Wait a bit to ensure update would have been received if streaming was active
	}, 0.1f));
	
	// Wait and verify second update was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount]()
	{
		// Update count should still be 1 (only first update received)
		TestEqual("Update count should still be 1 after stopping", *UpdateCount, 1);
	}, 1.0f));
	
	// Cleanup: Delete user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that StopStreamingUpdates can be called multiple times safely.
 * Verifies that calling StopStreamingUpdates multiple times doesn't cause errors.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingUpdatesMultipleStopsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingUpdates.4Advanced.MultipleStops", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingUpdatesMultipleStopsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_multiple_init";
	const FString TestUserID = SDK_PREFIX + "test_stop_streaming_multiple";
	
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
	FPubnubChatUserData UserData;
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID, UserData);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	TestNotNull("User should be created", CreateResult.User);
	
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Stop streaming updates first time
	FPubnubChatOperationResult StopResult1 = CreateResult.User->StopStreamingUpdates();
	TestFalse("First StopStreamingUpdates should succeed", StopResult1.Error);
	
	// Stop streaming updates second time (should still succeed)
	FPubnubChatOperationResult StopResult2 = CreateResult.User->StopStreamingUpdates();
	TestFalse("Second StopStreamingUpdates should succeed", StopResult2.Error);
	
	// Stop streaming updates third time (should still succeed)
	FPubnubChatOperationResult StopResult3 = CreateResult.User->StopStreamingUpdates();
	TestFalse("Third StopStreamingUpdates should succeed", StopResult3.Error);
	
	// Cleanup: Delete user
	if(Chat)
	{
		Chat->DeleteUser(TestUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Stream Updates)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamUpdatesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamUpdatesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamUpdatesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_async_full_init";
	const FString TestUserID = SDK_PREFIX + "test_stream_updates_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID);
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
	
	CreateResult.User->StreamUpdatesAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("StreamUpdatesAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestUserID]()
	{
		if(CreateResult.User)
		{
			CreateResult.User->StopStreamingUpdates();
		}
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingUpdatesAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingUpdatesAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingUpdatesAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_async_full_init";
	const FString TestUserID = SDK_PREFIX + "test_stop_streaming_async_full";
	
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
	
	FPubnubChatUserResult CreateResult = Chat->CreateUser(TestUserID);
	TestFalse("CreateUser should succeed", CreateResult.Result.Error);
	if(!CreateResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatOperationResult StreamResult = CreateResult.User->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamResult.Error);
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	CreateResult.User->StopStreamingUpdatesAsync(OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("StopStreamingUpdatesAsync should succeed", CallbackResult->Error);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestUserID]()
	{
		if(Chat)
		{
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// STREAMMENTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamMentionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamMentions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamMentionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_mentions_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StreamResult = UninitializedUser->StreamMentions();
		TestTrue("StreamMentions should fail with uninitialized user", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamMentionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamMentions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamMentionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_mentions_happy_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_stream_mentions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_stream_mentions_happy_channel";
	const FString MentionText = TEXT("happy mention");
	const FString MessagePayload = TEXT("{\"value\":\"message for mention stream\"}");

	FPubnubChatConfig SenderConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, SenderConfig);
	TestFalse("Sender InitChat should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		AddError("Sender chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create mentioned user should succeed", SenderChat->CreateUser(MentionedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatConfig MentionedConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedConfig);
	TestFalse("Mentioned InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUpCurrentChatUser(*MentionedChat);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult PublishResult = SenderChat->GetPubnubClient()->PublishMessage(TestChannelID, MessagePayload);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	const FString MentionedMessageTimetoken = PublishResult.PublishedMessage.Timetoken;
	TestFalse("Published timetoken should not be empty", MentionedMessageTimetoken.IsEmpty());

	TSharedPtr<bool> bMentionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserMention> ReceivedMention = MakeShared<FPubnubChatUserMention>();
	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([bMentionReceived, ReceivedMention](const FPubnubChatUserMention& UserMention)
	{
		*bMentionReceived = true;
		*ReceivedMention = UserMention;
	});

	FPubnubChatOperationResult StreamResult = (*MentionedChat)->GetCurrentUser()->StreamMentions();
	TestFalse("StreamMentions should succeed", StreamResult.Error);

	FPubnubChatOperationResult EmitResult = CreateChannelResult.Channel->EmitUserMention(MentionedUserID, MentionedMessageTimetoken, MentionText);
	TestFalse("EmitUserMention should succeed", EmitResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMentionReceived]() -> bool {
		return *bMentionReceived;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMention, SenderUserID, TestChannelID, MentionedMessageTimetoken]()
	{
		TestEqual("Received mention MessageTimetoken should match", ReceivedMention->MessageTimetoken, MentionedMessageTimetoken);
		TestEqual("Received mention ChannelID should match", ReceivedMention->ChannelID, TestChannelID);
		TestEqual("Received mention MentionedByUserID should match sender", ReceivedMention->MentionedByUserID, SenderUserID);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, TestChannelID, SenderUserID, MentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(MentionedUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamMentionsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamMentions.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamMentionsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_mentions_idempotent_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_stream_mentions_idempotent_target";
	const FString TestChannelID = SDK_PREFIX + "test_stream_mentions_idempotent_channel";
	const FString MentionText = TEXT("idempotent mention");
	const FString MessagePayload = TEXT("{\"value\":\"message for idempotent mention\"}");

	FPubnubChatConfig SenderConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, SenderConfig);
	TestFalse("Sender InitChat should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		AddError("Sender chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create mentioned user should succeed", SenderChat->CreateUser(MentionedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatConfig MentionedConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedConfig);
	TestFalse("Mentioned InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUpCurrentChatUser(*MentionedChat);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult PublishResult = SenderChat->GetPubnubClient()->PublishMessage(TestChannelID, MessagePayload);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	const FString MentionedMessageTimetoken = PublishResult.PublishedMessage.Timetoken;

	TSharedPtr<int32> MentionCount = MakeShared<int32>(0);
	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([MentionCount](const FPubnubChatUserMention& UserMention)
	{
		*MentionCount = *MentionCount + 1;
	});

	FPubnubChatOperationResult FirstStreamResult = (*MentionedChat)->GetCurrentUser()->StreamMentions();
	FPubnubChatOperationResult SecondStreamResult = (*MentionedChat)->GetCurrentUser()->StreamMentions();
	TestFalse("First StreamMentions should succeed", FirstStreamResult.Error);
	TestFalse("Second StreamMentions should also succeed (idempotent)", SecondStreamResult.Error);

	TestFalse("EmitUserMention should succeed", CreateChannelResult.Channel->EmitUserMention(MentionedUserID, MentionedMessageTimetoken, MentionText).Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([MentionCount]() -> bool {
		return *MentionCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionCount]()
	{
		TestEqual("Only one mention callback should be fired for one event", *MentionCount, 1);
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, TestChannelID, SenderUserID, MentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(MentionedUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Verifies mention stream filters only mention events and ignores other chat event types.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamMentionsIgnoresOtherEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamMentions.4Advanced.IgnoresOtherEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamMentionsIgnoresOtherEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_mentions_filter_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_stream_mentions_filter_target";
	const FString TestChannelID = SDK_PREFIX + "test_stream_mentions_filter_channel";

	FPubnubChatConfig SenderConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, SenderConfig);
	TestFalse("Sender InitChat should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		AddError("Sender chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create mentioned user should succeed", SenderChat->CreateUser(MentionedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatConfig MentionedConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedConfig);
	TestFalse("Mentioned InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> MentionCount = MakeShared<int32>(0);
	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([MentionCount](const FPubnubChatUserMention& UserMention)
	{
		*MentionCount = *MentionCount + 1;
	});

	TestFalse("StreamMentions should succeed", (*MentionedChat)->GetCurrentUser()->StreamMentions().Error);

	FPubnubChatOperationResult EmitOtherEventResult = SenderChat->EmitChatEvent(EPubnubChatEventType::PCET_Report, MentionedUserID, TEXT("{\"text\":\"r\",\"reason\":\"x\",\"channelId\":\"c\",\"userId\":\"u\",\"timetoken\":\"1\"}"));
	TestFalse("EmitChatEvent report should succeed", EmitOtherEventResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionCount]()
	{
		TestEqual("Mention callback count should remain 0 for non-mention events", *MentionCount, 0);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, SenderUserID, MentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteUser(MentionedUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// STOPSTREAMINGMENTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingMentionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingMentions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingMentionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_mentions_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StopResult = UninitializedUser->StopStreamingMentions();
		TestTrue("StopStreamingMentions should fail with uninitialized user", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingMentionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingMentions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingMentionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_mentions_happy";

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

	FPubnubChatOperationResult StreamResult = Chat->GetCurrentUser()->StreamMentions();
	TestFalse("StreamMentions should succeed", StreamResult.Error);

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingMentions();
	TestFalse("StopStreamingMentions should succeed", StopResult.Error);

	bool bFoundUnsubscribeStep = false;
	for(const FPubnubChatOperationStepResult& Step : StopResult.StepResults)
	{
		if(Step.StepName == TEXT("Unsubscribe"))
		{
			bFoundUnsubscribeStep = true;
			TestFalse("Unsubscribe step should not have error", Step.OperationResult.Error);
			break;
		}
	}
	TestTrue("StopStreamingMentions should include Unsubscribe step", bFoundUnsubscribeStep);

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingMentionsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingMentions.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingMentionsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_mentions_noop";

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

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingMentions();
	TestFalse("StopStreamingMentions should succeed when not streaming (no-op)", StopResult.Error);
	TestEqual("No-op stop should not have step results", StopResult.StepResults.Num(), 0);

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Verifies StopStreamingMentions prevents receiving subsequent mention events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingMentionsPreventsEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingMentions.4Advanced.PreventsEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingMentionsPreventsEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stop_mentions_prevent_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_stop_mentions_prevent_target";
	const FString TestChannelID = SDK_PREFIX + "test_stop_mentions_prevent_channel";
	const FString MessagePayload = TEXT("{\"value\":\"message for stop mention test\"}");

	FPubnubChatConfig SenderConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, SenderConfig);
	TestFalse("Sender InitChat should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		AddError("Sender chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create mentioned user should succeed", SenderChat->CreateUser(MentionedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatConfig MentionedConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedConfig);
	TestFalse("Mentioned InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUpCurrentChatUser(*MentionedChat);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult FirstPublishResult = SenderChat->GetPubnubClient()->PublishMessage(TestChannelID, MessagePayload);
	TestFalse("First PublishMessage should succeed", FirstPublishResult.Result.Error);
	const FString FirstTimetoken = FirstPublishResult.PublishedMessage.Timetoken;

	FPubnubPublishMessageResult SecondPublishResult = SenderChat->GetPubnubClient()->PublishMessage(TestChannelID, MessagePayload);
	TestFalse("Second PublishMessage should succeed", SecondPublishResult.Result.Error);
	const FString SecondTimetoken = SecondPublishResult.PublishedMessage.Timetoken;

	TSharedPtr<int32> MentionCount = MakeShared<int32>(0);
	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([MentionCount](const FPubnubChatUserMention& UserMention)
	{
		*MentionCount = *MentionCount + 1;
	});

	TestFalse("StreamMentions should succeed", (*MentionedChat)->GetCurrentUser()->StreamMentions().Error);
	TestFalse("First EmitUserMention should succeed", CreateChannelResult.Channel->EmitUserMention(MentionedUserID, FirstTimetoken, TEXT("first")).Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([MentionCount]() -> bool {
		return *MentionCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionedChat]()
	{
		TestFalse("StopStreamingMentions should succeed", (*MentionedChat)->GetCurrentUser()->StopStreamingMentions().Error);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, MentionedUserID, SecondTimetoken]()
	{
		TestFalse("Second EmitUserMention should succeed", CreateChannelResult.Channel->EmitUserMention(MentionedUserID, SecondTimetoken, TEXT("second")).Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionCount]()
	{
		TestEqual("Mention count should stay unchanged after stop", *MentionCount, 1);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, TestChannelID, SenderUserID, MentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(MentionedUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

