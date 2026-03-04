// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChatMembership.h"
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

	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);

	if(CreateChannelResult.Channel)
	{
		FPubnubChatUserResult MentionedUserResult = SenderChat->GetUser(MentionedUserID);
		TestFalse("Get mentioned user should succeed", MentionedUserResult.Result.Error);
		TestNotNull("Mentioned user object should be valid", MentionedUserResult.User);
		if(MentionedUserResult.User)
		{
			FPubnubChatInviteResult InviteResult = CreateChannelResult.Channel->Invite(MentionedUserResult.User);
			TestFalse("Invite should succeed", InviteResult.Result.Error);
		}
	}

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionCount]()
	{
		TestEqual("Mention callback count should remain 0 for non-mention events", *MentionCount, 0);
	}, 1.0f));

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

// ============================================================================
// STREAMINVITATIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamInvitationsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamInvitations.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamInvitationsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_invitations_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StreamResult = UninitializedUser->StreamInvitations();
		TestTrue("StreamInvitations should fail with uninitialized user", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamInvitationsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamInvitations.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamInvitationsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_invitations_happy_sender";
	const FString InviteeUserID = SDK_PREFIX + "test_stream_invitations_happy_invitee";
	const FString TestChannelID = SDK_PREFIX + "test_stream_invitations_happy_channel";

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

	FPubnubChatUserResult CreateInviteeResult = SenderChat->CreateUser(InviteeUserID, FPubnubChatUserData());
	TestFalse("Create invitee user should succeed", CreateInviteeResult.Result.Error);
	TestNotNull("Invitee user object should be valid", CreateInviteeResult.User);
	if(!CreateInviteeResult.User)
	{
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig InviteeConfig;
	FPubnubChatInitChatResult InviteeInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InviteeUserID, InviteeConfig);
	TestFalse("Invitee InitChat should succeed", InviteeInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> InviteeChat = MakeShared<UPubnubChat*>(InviteeInitResult.Chat);
	if(!*InviteeChat)
	{
		AddError("Invitee chat should be initialized");
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.Type = TEXT("public");
	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		(*InviteeChat)->DeleteUser(InviteeUserID);
		CleanUpCurrentChatUser(*InviteeChat);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<bool> bInviteReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatInviteEvent> ReceivedInviteEvent = MakeShared<FPubnubChatInviteEvent>();
	(*InviteeChat)->GetCurrentUser()->OnInvitedNative.AddLambda([bInviteReceived, ReceivedInviteEvent](const FPubnubChatInviteEvent& InviteEvent)
	{
		*bInviteReceived = true;
		*ReceivedInviteEvent = InviteEvent;
	});

	FPubnubChatOperationResult StreamResult = (*InviteeChat)->GetCurrentUser()->StreamInvitations();
	TestFalse("StreamInvitations should succeed", StreamResult.Error);

	FPubnubChatInviteResult InviteResult = CreateChannelResult.Channel->Invite(CreateInviteeResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Invite should return membership", InviteResult.Membership);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bInviteReceived]() -> bool {
		return *bInviteReceived;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedInviteEvent, SenderUserID, TestChannelID]()
	{
		TestEqual("InvitedByUserID should match sender", ReceivedInviteEvent->InvitedByUserID, SenderUserID);
		TestEqual("ChannelID should match invited channel", ReceivedInviteEvent->ChannelID, TestChannelID);
		TestEqual("ChannelType should match channel type", ReceivedInviteEvent->ChannelType, FString("public"));
		TestFalse("Invite event timetoken should not be empty", ReceivedInviteEvent->Timetoken.IsEmpty());
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, InviteeChat, TestChannelID, SenderUserID, InviteeUserID]()
	{
		if(*InviteeChat)
		{
			(*InviteeChat)->GetCurrentUser()->StopStreamingInvitations();
			(*InviteeChat)->DeleteUser(InviteeUserID);
			CleanUpCurrentChatUser(*InviteeChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(InviteeUserID);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamInvitationsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamInvitations.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamInvitationsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_invitations_idempotent_sender";
	const FString InviteeUserID = SDK_PREFIX + "test_stream_invitations_idempotent_invitee";
	const FString TestChannelID = SDK_PREFIX + "test_stream_invitations_idempotent_channel";

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

	FPubnubChatUserResult CreateInviteeResult = SenderChat->CreateUser(InviteeUserID, FPubnubChatUserData());
	TestFalse("Create invitee user should succeed", CreateInviteeResult.Result.Error);
	if(!CreateInviteeResult.User)
	{
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig InviteeConfig;
	FPubnubChatInitChatResult InviteeInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InviteeUserID, InviteeConfig);
	TestFalse("Invitee InitChat should succeed", InviteeInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> InviteeChat = MakeShared<UPubnubChat*>(InviteeInitResult.Chat);
	if(!*InviteeChat)
	{
		AddError("Invitee chat should be initialized");
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.Type = TEXT("public");
	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		(*InviteeChat)->DeleteUser(InviteeUserID);
		CleanUpCurrentChatUser(*InviteeChat);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> InviteCount = MakeShared<int32>(0);
	(*InviteeChat)->GetCurrentUser()->OnInvitedNative.AddLambda([InviteCount](const FPubnubChatInviteEvent& InviteEvent)
	{
		*InviteCount = *InviteCount + 1;
	});

	FPubnubChatOperationResult FirstStreamResult = (*InviteeChat)->GetCurrentUser()->StreamInvitations();
	FPubnubChatOperationResult SecondStreamResult = (*InviteeChat)->GetCurrentUser()->StreamInvitations();
	TestFalse("First StreamInvitations should succeed", FirstStreamResult.Error);
	TestFalse("Second StreamInvitations should also succeed (idempotent)", SecondStreamResult.Error);

	FPubnubChatInviteResult InviteResult = CreateChannelResult.Channel->Invite(CreateInviteeResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([InviteCount]() -> bool {
		return *InviteCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, InviteCount]()
	{
		TestEqual("Exactly one invite callback should fire for one invite event", *InviteCount, 1);
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, InviteeChat, TestChannelID, SenderUserID, InviteeUserID]()
	{
		if(*InviteeChat)
		{
			(*InviteeChat)->GetCurrentUser()->StopStreamingInvitations();
			(*InviteeChat)->DeleteUser(InviteeUserID);
			CleanUpCurrentChatUser(*InviteeChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(InviteeUserID);
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
 * Verifies StreamInvitations handles multiple invite events emitted via InviteMultiple with correct event data.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamInvitationsInviteMultipleTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamInvitations.4Advanced.InviteMultiple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamInvitationsInviteMultipleTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stream_invitations_multi_sender";
	const FString InviteeUserID = SDK_PREFIX + "test_stream_invitations_multi_invitee";
	const FString OtherUserID = SDK_PREFIX + "test_stream_invitations_multi_other";
	const FString TestChannelID = SDK_PREFIX + "test_stream_invitations_multi_channel";

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

	FPubnubChatUserResult InviteeResult = SenderChat->CreateUser(InviteeUserID, FPubnubChatUserData());
	FPubnubChatUserResult OtherUserResult = SenderChat->CreateUser(OtherUserID, FPubnubChatUserData());
	TestFalse("Create invitee should succeed", InviteeResult.Result.Error);
	TestFalse("Create other user should succeed", OtherUserResult.Result.Error);
	if(!InviteeResult.User || !OtherUserResult.User)
	{
		SenderChat->DeleteUser(OtherUserID);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig InviteeConfig;
	FPubnubChatInitChatResult InviteeInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InviteeUserID, InviteeConfig);
	TestFalse("Invitee InitChat should succeed", InviteeInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> InviteeChat = MakeShared<UPubnubChat*>(InviteeInitResult.Chat);
	if(!*InviteeChat)
	{
		AddError("Invitee chat should be initialized");
		SenderChat->DeleteUser(OtherUserID);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.Type = TEXT("public");
	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		(*InviteeChat)->DeleteUser(InviteeUserID);
		CleanUpCurrentChatUser(*InviteeChat);
		SenderChat->DeleteUser(OtherUserID);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> InviteeEventCount = MakeShared<int32>(0);
	TSharedPtr<FPubnubChatInviteEvent> LastInviteEvent = MakeShared<FPubnubChatInviteEvent>();
	(*InviteeChat)->GetCurrentUser()->OnInvitedNative.AddLambda([InviteeEventCount, LastInviteEvent](const FPubnubChatInviteEvent& InviteEvent)
	{
		*InviteeEventCount = *InviteeEventCount + 1;
		*LastInviteEvent = InviteEvent;
	});

	TestFalse("StreamInvitations should succeed", (*InviteeChat)->GetCurrentUser()->StreamInvitations().Error);

	FPubnubChatInviteMultipleResult InviteMultipleResult = CreateChannelResult.Channel->InviteMultiple({InviteeResult.User, OtherUserResult.User});
	TestFalse("InviteMultiple should succeed", InviteMultipleResult.Result.Error);
	TestTrue("InviteMultiple should create two memberships", InviteMultipleResult.Memberships.Num() >= 2);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([InviteeEventCount]() -> bool {
		return *InviteeEventCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, InviteeEventCount, LastInviteEvent, SenderUserID, TestChannelID]()
	{
		TestEqual("Invitee should receive exactly one invite event for own invitation", *InviteeEventCount, 1);
		TestEqual("InvitedByUserID should match sender", LastInviteEvent->InvitedByUserID, SenderUserID);
		TestEqual("ChannelID should match invited channel", LastInviteEvent->ChannelID, TestChannelID);
		TestEqual("ChannelType should match channel type", LastInviteEvent->ChannelType, FString("public"));
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, InviteeChat, TestChannelID, SenderUserID, InviteeUserID, OtherUserID]()
	{
		if(*InviteeChat)
		{
			(*InviteeChat)->GetCurrentUser()->StopStreamingInvitations();
			(*InviteeChat)->DeleteUser(InviteeUserID);
			CleanUpCurrentChatUser(*InviteeChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(TestChannelID);
			SenderChat->DeleteUser(OtherUserID);
			SenderChat->DeleteUser(InviteeUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// STOPSTREAMINGINVITATIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingInvitationsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingInvitations.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingInvitationsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_invitations_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StopResult = UninitializedUser->StopStreamingInvitations();
		TestTrue("StopStreamingInvitations should fail with uninitialized user", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingInvitationsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingInvitations.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingInvitationsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_invitations_happy";

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

	FPubnubChatOperationResult StreamResult = Chat->GetCurrentUser()->StreamInvitations();
	TestFalse("StreamInvitations should succeed", StreamResult.Error);

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingInvitations();
	TestFalse("StopStreamingInvitations should succeed", StopResult.Error);

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
	TestTrue("StopStreamingInvitations should include Unsubscribe step", bFoundUnsubscribeStep);

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingInvitationsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingInvitations.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingInvitationsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_invitations_noop";

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

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingInvitations();
	TestFalse("StopStreamingInvitations should succeed when not streaming (no-op)", StopResult.Error);
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
 * Verifies StopStreamingInvitations prevents receiving subsequent invitation events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingInvitationsPreventsEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingInvitations.4Advanced.PreventsEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingInvitationsPreventsEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_stop_invitations_prevent_sender";
	const FString InviteeUserID = SDK_PREFIX + "test_stop_invitations_prevent_invitee";
	const FString FirstChannelID = SDK_PREFIX + "test_stop_invitations_prevent_channel_1";
	const FString SecondChannelID = SDK_PREFIX + "test_stop_invitations_prevent_channel_2";

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

	FPubnubChatUserResult CreateInviteeResult = SenderChat->CreateUser(InviteeUserID, FPubnubChatUserData());
	TestFalse("Create invitee user should succeed", CreateInviteeResult.Result.Error);
	if(!CreateInviteeResult.User)
	{
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig InviteeConfig;
	FPubnubChatInitChatResult InviteeInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InviteeUserID, InviteeConfig);
	TestFalse("Invitee InitChat should succeed", InviteeInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> InviteeChat = MakeShared<UPubnubChat*>(InviteeInitResult.Chat);
	if(!*InviteeChat)
	{
		AddError("Invitee chat should be initialized");
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	ChannelData.Type = TEXT("public");
	FPubnubChatChannelResult FirstCreateChannelResult = SenderChat->CreatePublicConversation(FirstChannelID, ChannelData);
	FPubnubChatChannelResult SecondCreateChannelResult = SenderChat->CreatePublicConversation(SecondChannelID, ChannelData);
	TestFalse("First CreatePublicConversation should succeed", FirstCreateChannelResult.Result.Error);
	TestFalse("Second CreatePublicConversation should succeed", SecondCreateChannelResult.Result.Error);
	if(!FirstCreateChannelResult.Channel || !SecondCreateChannelResult.Channel)
	{
		(*InviteeChat)->DeleteUser(InviteeUserID);
		CleanUpCurrentChatUser(*InviteeChat);
		SenderChat->DeleteUser(InviteeUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> InviteCount = MakeShared<int32>(0);
	(*InviteeChat)->GetCurrentUser()->OnInvitedNative.AddLambda([InviteCount](const FPubnubChatInviteEvent& InviteEvent)
	{
		*InviteCount = *InviteCount + 1;
	});

	TestFalse("StreamInvitations should succeed", (*InviteeChat)->GetCurrentUser()->StreamInvitations().Error);
	TestFalse("First Invite should succeed", FirstCreateChannelResult.Channel->Invite(CreateInviteeResult.User).Result.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([InviteCount]() -> bool {
		return *InviteCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, InviteeChat]()
	{
		TestFalse("StopStreamingInvitations should succeed", (*InviteeChat)->GetCurrentUser()->StopStreamingInvitations().Error);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondCreateChannelResult, CreateInviteeResult]()
	{
		TestFalse("Second Invite should succeed", SecondCreateChannelResult.Channel->Invite(CreateInviteeResult.User).Result.Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, InviteCount]()
	{
		TestEqual("Invite count should stay unchanged after stop", *InviteCount, 1);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, InviteeChat, FirstChannelID, SecondChannelID, SenderUserID, InviteeUserID]()
	{
		if(*InviteeChat)
		{
			(*InviteeChat)->DeleteUser(InviteeUserID);
			CleanUpCurrentChatUser(*InviteeChat);
		}
		if(SenderChat)
		{
			SenderChat->DeleteChannel(FirstChannelID);
			SenderChat->DeleteChannel(SecondChannelID);
			SenderChat->DeleteUser(InviteeUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// STREAMRESTRICTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_restrictions_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StreamResult = UninitializedUser->StreamRestrictions();
		TestTrue("StreamRestrictions should fail with uninitialized user", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString ModeratorUserID = SDK_PREFIX + "test_stream_restrictions_happy_moderator";
	const FString RestrictedUserID = SDK_PREFIX + "test_stream_restrictions_happy_restricted";
	const FString TestChannelID = SDK_PREFIX + "test_stream_restrictions_happy_channel";
	const FString RestrictionReason = TEXT("happy-path-ban");

	FPubnubChatConfig ModeratorConfig;
	FPubnubChatInitChatResult ModeratorInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, ModeratorUserID, ModeratorConfig);
	TestFalse("Moderator InitChat should succeed", ModeratorInitResult.Result.Error);
	UPubnubChat* ModeratorChat = ModeratorInitResult.Chat;
	if(!ModeratorChat)
	{
		AddError("Moderator chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateRestrictedUserResult = ModeratorChat->CreateUser(RestrictedUserID, FPubnubChatUserData());
	TestFalse("Create restricted user should succeed", CreateRestrictedUserResult.Result.Error);

	FPubnubChatChannelResult CreateChannelResult = ModeratorChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig RestrictedConfig;
	FPubnubChatInitChatResult RestrictedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, RestrictedUserID, RestrictedConfig);
	TestFalse("Restricted InitChat should succeed", RestrictedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> RestrictedChat = MakeShared<UPubnubChat*>(RestrictedInitResult.Chat);
	if(!*RestrictedChat)
	{
		AddError("Restricted chat should be initialized");
		ModeratorChat->DeleteChannel(TestChannelID);
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<bool> bRestrictionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatRestriction> ReceivedRestriction = MakeShared<FPubnubChatRestriction>();
	(*RestrictedChat)->GetCurrentUser()->OnRestrictionChangedNative.AddLambda([bRestrictionReceived, ReceivedRestriction](const FPubnubChatRestriction& Restriction)
	{
		*bRestrictionReceived = true;
		*ReceivedRestriction = Restriction;
	});

	TestFalse("StreamRestrictions should succeed", (*RestrictedChat)->GetCurrentUser()->StreamRestrictions().Error);

	FPubnubChatRestriction Restriction;
	Restriction.UserID = RestrictedUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	Restriction.Reason = RestrictionReason;
	TestFalse("Chat->SetRestrictions should succeed", ModeratorChat->SetRestrictions(Restriction).Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bRestrictionReceived]() -> bool {
		return *bRestrictionReceived;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedRestriction, RestrictedUserID, TestChannelID, RestrictionReason]()
	{
		TestEqual("Restriction UserID should match restricted user", ReceivedRestriction->UserID, RestrictedUserID);
		TestEqual("Restriction ChannelID should match channel", ReceivedRestriction->ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", ReceivedRestriction->Ban);
		TestFalse("Restriction Mute should be false", ReceivedRestriction->Mute);
		TestEqual("Restriction Reason should match", ReceivedRestriction->Reason, RestrictionReason);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeratorChat, RestrictedChat, TestChannelID, ModeratorUserID, RestrictedUserID]()
	{
		if(*RestrictedChat)
		{
			(*RestrictedChat)->GetCurrentUser()->StopStreamingRestrictions();
			(*RestrictedChat)->DeleteUser(RestrictedUserID);
			CleanUpCurrentChatUser(*RestrictedChat);
		}
		if(ModeratorChat)
		{
			ModeratorChat->DeleteChannel(TestChannelID);
			ModeratorChat->DeleteUser(RestrictedUserID);
			ModeratorChat->DeleteUser(ModeratorUserID);
		}
		CleanUpCurrentChatUser(ModeratorChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamRestrictionsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamRestrictions.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamRestrictionsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString ModeratorUserID = SDK_PREFIX + "test_stream_restrictions_idempotent_moderator";
	const FString RestrictedUserID = SDK_PREFIX + "test_stream_restrictions_idempotent_restricted";
	const FString TestChannelID = SDK_PREFIX + "test_stream_restrictions_idempotent_channel";

	FPubnubChatConfig ModeratorConfig;
	FPubnubChatInitChatResult ModeratorInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, ModeratorUserID, ModeratorConfig);
	TestFalse("Moderator InitChat should succeed", ModeratorInitResult.Result.Error);
	UPubnubChat* ModeratorChat = ModeratorInitResult.Chat;
	if(!ModeratorChat)
	{
		AddError("Moderator chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create restricted user should succeed", ModeratorChat->CreateUser(RestrictedUserID, FPubnubChatUserData()).Result.Error);
	FPubnubChatChannelResult CreateChannelResult = ModeratorChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig RestrictedConfig;
	FPubnubChatInitChatResult RestrictedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, RestrictedUserID, RestrictedConfig);
	TestFalse("Restricted InitChat should succeed", RestrictedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> RestrictedChat = MakeShared<UPubnubChat*>(RestrictedInitResult.Chat);
	if(!*RestrictedChat)
	{
		AddError("Restricted chat should be initialized");
		ModeratorChat->DeleteChannel(TestChannelID);
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> RestrictionCount = MakeShared<int32>(0);
	(*RestrictedChat)->GetCurrentUser()->OnRestrictionChangedNative.AddLambda([RestrictionCount](const FPubnubChatRestriction& Restriction)
	{
		*RestrictionCount = *RestrictionCount + 1;
	});

	FPubnubChatOperationResult FirstStreamResult = (*RestrictedChat)->GetCurrentUser()->StreamRestrictions();
	FPubnubChatOperationResult SecondStreamResult = (*RestrictedChat)->GetCurrentUser()->StreamRestrictions();
	TestFalse("First StreamRestrictions should succeed", FirstStreamResult.Error);
	TestFalse("Second StreamRestrictions should also succeed (idempotent)", SecondStreamResult.Error);

	TestFalse("Channel->SetRestrictions should succeed", CreateChannelResult.Channel->SetRestrictions(RestrictedUserID, false, true, TEXT("idempotent-mute")).Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([RestrictionCount]() -> bool {
		return *RestrictionCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, RestrictionCount]()
	{
		TestEqual("Exactly one callback should fire for one restriction event", *RestrictionCount, 1);
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeratorChat, RestrictedChat, TestChannelID, ModeratorUserID, RestrictedUserID]()
	{
		if(*RestrictedChat)
		{
			(*RestrictedChat)->GetCurrentUser()->StopStreamingRestrictions();
			(*RestrictedChat)->DeleteUser(RestrictedUserID);
			CleanUpCurrentChatUser(*RestrictedChat);
		}
		if(ModeratorChat)
		{
			ModeratorChat->DeleteChannel(TestChannelID);
			ModeratorChat->DeleteUser(RestrictedUserID);
			ModeratorChat->DeleteUser(ModeratorUserID);
		}
		CleanUpCurrentChatUser(ModeratorChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Verifies restriction stream receives correct data for all supported emitters:
 * Chat->SetRestrictions, Channel->SetRestrictions, and User->SetRestrictions.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStreamRestrictionsAllEmittersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StreamRestrictions.4Advanced.AllEmitters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStreamRestrictionsAllEmittersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString ModeratorUserID = SDK_PREFIX + "test_stream_restrictions_emitters_moderator";
	const FString RestrictedUserID = SDK_PREFIX + "test_stream_restrictions_emitters_restricted";
	const FString TestChannelID = SDK_PREFIX + "test_stream_restrictions_emitters_channel";

	const FString ChatReason = TEXT("set-by-chat");
	const FString ChannelReason = TEXT("set-by-channel");
	const FString UserReason = TEXT("set-by-user");

	FPubnubChatConfig ModeratorConfig;
	FPubnubChatInitChatResult ModeratorInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, ModeratorUserID, ModeratorConfig);
	TestFalse("Moderator InitChat should succeed", ModeratorInitResult.Result.Error);
	UPubnubChat* ModeratorChat = ModeratorInitResult.Chat;
	if(!ModeratorChat)
	{
		AddError("Moderator chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult RestrictedUserResult = ModeratorChat->CreateUser(RestrictedUserID, FPubnubChatUserData());
	TestFalse("Create restricted user should succeed", RestrictedUserResult.Result.Error);
	TestNotNull("Restricted user should be created", RestrictedUserResult.User);
	if(!RestrictedUserResult.User)
	{
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateChannelResult = ModeratorChat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig RestrictedConfig;
	FPubnubChatInitChatResult RestrictedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, RestrictedUserID, RestrictedConfig);
	TestFalse("Restricted InitChat should succeed", RestrictedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> RestrictedChat = MakeShared<UPubnubChat*>(RestrictedInitResult.Chat);
	if(!*RestrictedChat)
	{
		AddError("Restricted chat should be initialized");
		ModeratorChat->DeleteChannel(TestChannelID);
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<TArray<FPubnubChatRestriction>> ReceivedRestrictions = MakeShared<TArray<FPubnubChatRestriction>>();
	(*RestrictedChat)->GetCurrentUser()->OnRestrictionChangedNative.AddLambda([ReceivedRestrictions](const FPubnubChatRestriction& Restriction)
	{
		ReceivedRestrictions->Add(Restriction);
	});

	TestFalse("StreamRestrictions should succeed", (*RestrictedChat)->GetCurrentUser()->StreamRestrictions().Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeratorChat, CreateChannelResult, RestrictedUserResult, RestrictedUserID, TestChannelID, ChatReason, ChannelReason, UserReason]()
	{
		FPubnubChatRestriction ChatRestriction;
		ChatRestriction.UserID = RestrictedUserID;
		ChatRestriction.ChannelID = TestChannelID;
		ChatRestriction.Ban = true;
		ChatRestriction.Mute = false;
		ChatRestriction.Reason = ChatReason;
		TestFalse("Chat->SetRestrictions should succeed", ModeratorChat->SetRestrictions(ChatRestriction).Error);

		TestFalse("Channel->SetRestrictions should succeed", CreateChannelResult.Channel->SetRestrictions(RestrictedUserID, false, true, ChannelReason).Error);

		TestFalse("User->SetRestrictions should succeed", RestrictedUserResult.User->SetRestrictions(TestChannelID, false, false, UserReason).Error);
	}, 0.6f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedRestrictions]() -> bool {
		return ReceivedRestrictions->Num() >= 3;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedRestrictions, RestrictedUserID, TestChannelID, ChatReason, ChannelReason, UserReason]()
	{
		const auto FindByReason = [ReceivedRestrictions](const FString& Reason) -> const FPubnubChatRestriction*
		{
			for (const FPubnubChatRestriction& Restriction : *ReceivedRestrictions)
			{
				if (Restriction.Reason == Reason)
				{
					return &Restriction;
				}
			}
			return nullptr;
		};

		const FPubnubChatRestriction* ChatRestriction = FindByReason(ChatReason);
		const FPubnubChatRestriction* ChannelRestriction = FindByReason(ChannelReason);
		const FPubnubChatRestriction* UserRestriction = FindByReason(UserReason);

		TestNotNull("Chat emitted restriction should be received", ChatRestriction);
		TestNotNull("Channel emitted restriction should be received", ChannelRestriction);
		TestNotNull("User emitted restriction should be received", UserRestriction);

		if (ChatRestriction)
		{
			TestEqual("Chat restriction UserID should match", ChatRestriction->UserID, RestrictedUserID);
			TestEqual("Chat restriction ChannelID should match", ChatRestriction->ChannelID, TestChannelID);
			TestTrue("Chat restriction Ban should be true", ChatRestriction->Ban);
			TestFalse("Chat restriction Mute should be false", ChatRestriction->Mute);
		}
		if (ChannelRestriction)
		{
			TestEqual("Channel restriction UserID should match", ChannelRestriction->UserID, RestrictedUserID);
			TestEqual("Channel restriction ChannelID should match", ChannelRestriction->ChannelID, TestChannelID);
			TestFalse("Channel restriction Ban should be false", ChannelRestriction->Ban);
			TestTrue("Channel restriction Mute should be true", ChannelRestriction->Mute);
		}
		if (UserRestriction)
		{
			TestEqual("User restriction UserID should match", UserRestriction->UserID, RestrictedUserID);
			TestEqual("User restriction ChannelID should match", UserRestriction->ChannelID, TestChannelID);
			TestFalse("User restriction Ban should be false after lift", UserRestriction->Ban);
			TestFalse("User restriction Mute should be false after lift", UserRestriction->Mute);
		}
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeratorChat, RestrictedChat, TestChannelID, ModeratorUserID, RestrictedUserID]()
	{
		if(*RestrictedChat)
		{
			(*RestrictedChat)->GetCurrentUser()->StopStreamingRestrictions();
			(*RestrictedChat)->DeleteUser(RestrictedUserID);
			CleanUpCurrentChatUser(*RestrictedChat);
		}
		if(ModeratorChat)
		{
			ModeratorChat->DeleteChannel(TestChannelID);
			ModeratorChat->DeleteUser(RestrictedUserID);
			ModeratorChat->DeleteUser(ModeratorUserID);
		}
		CleanUpCurrentChatUser(ModeratorChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// STOPSTREAMINGRESTRICTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_restrictions_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatOperationResult StopResult = UninitializedUser->StopStreamingRestrictions();
		TestTrue("StopStreamingRestrictions should fail with uninitialized user", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_restrictions_happy";

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

	FPubnubChatOperationResult StreamResult = Chat->GetCurrentUser()->StreamRestrictions();
	TestFalse("StreamRestrictions should succeed", StreamResult.Error);

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingRestrictions();
	TestFalse("StopStreamingRestrictions should succeed", StopResult.Error);

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
	TestTrue("StopStreamingRestrictions should include Unsubscribe step", bFoundUnsubscribeStep);

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingRestrictionsNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingRestrictions.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingRestrictionsNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_restrictions_noop";

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

	FPubnubChatOperationResult StopResult = Chat->GetCurrentUser()->StopStreamingRestrictions();
	TestFalse("StopStreamingRestrictions should succeed when not streaming (no-op)", StopResult.Error);
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
 * Verifies StopStreamingRestrictions prevents receiving subsequent restriction events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserStopStreamingRestrictionsPreventsEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.StopStreamingRestrictions.4Advanced.PreventsEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserStopStreamingRestrictionsPreventsEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString ModeratorUserID = SDK_PREFIX + "test_stop_restrictions_prevent_moderator";
	const FString RestrictedUserID = SDK_PREFIX + "test_stop_restrictions_prevent_restricted";
	const FString FirstChannelID = SDK_PREFIX + "test_stop_restrictions_prevent_channel_1";
	const FString SecondChannelID = SDK_PREFIX + "test_stop_restrictions_prevent_channel_2";

	FPubnubChatConfig ModeratorConfig;
	FPubnubChatInitChatResult ModeratorInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, ModeratorUserID, ModeratorConfig);
	TestFalse("Moderator InitChat should succeed", ModeratorInitResult.Result.Error);
	UPubnubChat* ModeratorChat = ModeratorInitResult.Chat;
	if(!ModeratorChat)
	{
		AddError("Moderator chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create restricted user should succeed", ModeratorChat->CreateUser(RestrictedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatChannelResult FirstCreateChannelResult = ModeratorChat->CreatePublicConversation(FirstChannelID, FPubnubChatChannelData());
	FPubnubChatChannelResult SecondCreateChannelResult = ModeratorChat->CreatePublicConversation(SecondChannelID, FPubnubChatChannelData());
	TestFalse("First CreatePublicConversation should succeed", FirstCreateChannelResult.Result.Error);
	TestFalse("Second CreatePublicConversation should succeed", SecondCreateChannelResult.Result.Error);
	if(!FirstCreateChannelResult.Channel || !SecondCreateChannelResult.Channel)
	{
		ModeratorChat->DeleteChannel(FirstChannelID);
		ModeratorChat->DeleteChannel(SecondChannelID);
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	FPubnubChatConfig RestrictedConfig;
	FPubnubChatInitChatResult RestrictedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, RestrictedUserID, RestrictedConfig);
	TestFalse("Restricted InitChat should succeed", RestrictedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> RestrictedChat = MakeShared<UPubnubChat*>(RestrictedInitResult.Chat);
	if(!*RestrictedChat)
	{
		AddError("Restricted chat should be initialized");
		ModeratorChat->DeleteChannel(FirstChannelID);
		ModeratorChat->DeleteChannel(SecondChannelID);
		ModeratorChat->DeleteUser(RestrictedUserID);
		ModeratorChat->DeleteUser(ModeratorUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> RestrictionCount = MakeShared<int32>(0);
	(*RestrictedChat)->GetCurrentUser()->OnRestrictionChangedNative.AddLambda([RestrictionCount](const FPubnubChatRestriction& Restriction)
	{
		*RestrictionCount = *RestrictionCount + 1;
	});

	TestFalse("StreamRestrictions should succeed", (*RestrictedChat)->GetCurrentUser()->StreamRestrictions().Error);
	TestFalse("First Channel->SetRestrictions should succeed", FirstCreateChannelResult.Channel->SetRestrictions(RestrictedUserID, true, false, TEXT("first-ban")).Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([RestrictionCount]() -> bool {
		return *RestrictionCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, RestrictedChat]()
	{
		TestFalse("StopStreamingRestrictions should succeed", (*RestrictedChat)->GetCurrentUser()->StopStreamingRestrictions().Error);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SecondCreateChannelResult, RestrictedUserID]()
	{
		TestFalse("Second Channel->SetRestrictions should succeed", SecondCreateChannelResult.Channel->SetRestrictions(RestrictedUserID, false, true, TEXT("second-mute")).Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, RestrictionCount]()
	{
		TestEqual("Restriction callback count should stay unchanged after stop", *RestrictionCount, 1);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeratorChat, RestrictedChat, FirstChannelID, SecondChannelID, ModeratorUserID, RestrictedUserID]()
	{
		if(*RestrictedChat)
		{
			(*RestrictedChat)->DeleteUser(RestrictedUserID);
			CleanUpCurrentChatUser(*RestrictedChat);
		}
		if(ModeratorChat)
		{
			ModeratorChat->DeleteChannel(FirstChannelID);
			ModeratorChat->DeleteChannel(SecondChannelID);
			ModeratorChat->DeleteUser(RestrictedUserID);
			ModeratorChat->DeleteUser(ModeratorUserID);
		}
		CleanUpCurrentChatUser(ModeratorChat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// GETMEMBERSHIP TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembership.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_membership_not_init";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;

	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatMembershipResult GetMembershipResult = UninitializedUser->GetMembership(TEXT("channel"));
		TestTrue("GetMembership should fail on uninitialized user", GetMembershipResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMembershipResult.Result.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembership.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_membership_empty_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatMembershipResult GetMembershipResult = Chat->GetCurrentUser()->GetMembership(TEXT(""));
	TestTrue("GetMembership should fail for empty ChannelID", GetMembershipResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", GetMembershipResult.Result.ErrorMessage.IsEmpty());

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembership.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_membership_happy";
	const FString TestChannelID = SDK_PREFIX + "test_get_membership_happy_channel";

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

	FPubnubChatMembershipResult GetMembershipResult = Chat->GetCurrentUser()->GetMembership(TestChannelID);
	TestFalse("GetMembership should succeed", GetMembershipResult.Result.Error);
	TestNotNull("Membership should be returned", GetMembershipResult.Membership);

	if(GetMembershipResult.Membership)
	{
		TestEqual("Membership UserID should match", GetMembershipResult.Membership->GetUserID(), InitUserID);
		TestEqual("Membership ChannelID should match", GetMembershipResult.Membership->GetChannelID(), TestChannelID);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembership.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_membership_no_optional";
	const FString TestChannelID = SDK_PREFIX + "test_get_membership_no_optional_channel";

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

	FPubnubChatMembershipResult GetMembershipResult = Chat->GetCurrentUser()->GetMembership(TestChannelID);
	TestFalse("GetMembership should succeed with required parameters only", GetMembershipResult.Result.Error);
	TestNotNull("Membership should be returned", GetMembershipResult.Membership);

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
 * Verifies GetMembership returns membership data populated by Join (status/type/custom).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetMembershipReturnsMembershipDataTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.GetMembership.4Advanced.ReturnsMembershipData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetMembershipReturnsMembershipDataTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_membership_data";
	const FString TestChannelID = SDK_PREFIX + "test_get_membership_data_channel";

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

	FPubnubChatMembershipData JoinMembershipData;
	JoinMembershipData.Status = TEXT("active");
	JoinMembershipData.Type = TEXT("member");
	JoinMembershipData.Custom = TEXT("{\"role\":\"owner\"}");

	FPubnubChatJoinResult JoinResult;
	if(CreateResult.Channel)
	{
		JoinResult = CreateResult.Channel->Join(JoinMembershipData);
		TestFalse("Join should succeed", JoinResult.Result.Error);
	}

	FPubnubChatMembershipResult GetMembershipResult = Chat->GetCurrentUser()->GetMembership(TestChannelID);
	TestFalse("GetMembership should succeed", GetMembershipResult.Result.Error);
	TestNotNull("Membership should be returned", GetMembershipResult.Membership);

	if(GetMembershipResult.Membership)
	{
		FPubnubChatMembershipData MembershipData = GetMembershipResult.Membership->GetMembershipData();
		TestEqual("Membership Status should match", MembershipData.Status, JoinMembershipData.Status);
		TestEqual("Membership Type should match", MembershipData.Type, JoinMembershipData.Type);
		TestTrue("Membership Custom should contain role owner", MembershipData.Custom.Contains(TEXT("\"role\":\"owner\"")));
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
// ISMEMBERON TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsMemberOnNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsMemberOn.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsMemberOnNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_member_on_not_init";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;

	if(Chat)
	{
		UPubnubChatUser* UninitializedUser = NewObject<UPubnubChatUser>(Chat);
		FPubnubChatIsMemberOnResult IsMemberOnResult = UninitializedUser->IsMemberOn(TEXT("channel"));
		TestTrue("IsMemberOn should fail on uninitialized user", IsMemberOnResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", IsMemberOnResult.Result.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsMemberOnEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsMemberOn.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsMemberOnEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_member_on_empty_channel";

	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, FPubnubChatConfig());
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatIsMemberOnResult IsMemberOnResult = Chat->GetCurrentUser()->IsMemberOn(TEXT(""));
	TestTrue("IsMemberOn should fail for empty ChannelID", IsMemberOnResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", IsMemberOnResult.Result.ErrorMessage.IsEmpty());

	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsMemberOnHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsMemberOn.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsMemberOnHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_member_on_happy";
	const FString TestChannelID = SDK_PREFIX + "test_is_member_on_happy_channel";

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

	FPubnubChatIsMemberOnResult IsMemberOnResult = Chat->GetCurrentUser()->IsMemberOn(TestChannelID);
	TestFalse("IsMemberOn should succeed", IsMemberOnResult.Result.Error);
	TestTrue("IsMemberOn should be true for joined channel", IsMemberOnResult.IsMemberOn);

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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsMemberOnNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsMemberOn.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsMemberOnNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_member_on_no_optional";
	const FString TestChannelID = SDK_PREFIX + "test_is_member_on_no_optional_channel";

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

	FPubnubChatIsMemberOnResult IsMemberOnResult = Chat->GetCurrentUser()->IsMemberOn(TestChannelID);
	TestFalse("IsMemberOn should succeed with required parameters only", IsMemberOnResult.Result.Error);
	TestTrue("IsMemberOn should be true for joined channel", IsMemberOnResult.IsMemberOn);

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
 * Verifies IsMemberOn returns false (without error) when channel exists but user is not a member.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserIsMemberOnNonMemberChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.User.IsMemberOn.4Advanced.NonMemberChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserIsMemberOnNonMemberChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_is_member_on_non_member";
	const FString TestChannelID = SDK_PREFIX + "test_is_member_on_non_member_channel";

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

	FPubnubChatIsMemberOnResult IsMemberOnResult = Chat->GetCurrentUser()->IsMemberOn(TestChannelID);
	TestFalse("IsMemberOn should succeed for non-member channel query", IsMemberOnResult.Result.Error);
	TestFalse("IsMemberOn should be false when user did not join", IsMemberOnResult.IsMemberOn);

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

