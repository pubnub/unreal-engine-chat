// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
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
	TSharedPtr<EPubnubChatStreamedUpdateType> ReceivedUpdateType = MakeShared<EPubnubChatStreamedUpdateType>();
	TSharedPtr<FString> ReceivedUserID = MakeShared<FString>(TEXT(""));
	TSharedPtr<FPubnubChatUserData> ReceivedUserData = MakeShared<FPubnubChatUserData>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedUpdateType, ReceivedUserID, ReceivedUserData](EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
	{
		*bUpdateReceived = true;
		*ReceivedUpdateType = UpdateType;
		*ReceivedUserID = UserID;
		*ReceivedUserData = UserData;
	};
	CreateResult.User->OnUserUpdateReceivedNative.AddLambda(UpdateLambda);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedUpdateType, ReceivedUserID, ReceivedUserData, TestUserID, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		TestEqual("Received UpdateType should be Updated", *ReceivedUpdateType, EPubnubChatStreamedUpdateType::PCSUT_Updated);
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
	TSharedPtr<EPubnubChatStreamedUpdateType> ReceivedUpdateType = MakeShared<EPubnubChatStreamedUpdateType>();
	TSharedPtr<FPubnubChatUserData> ReceivedUserData = MakeShared<FPubnubChatUserData>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedUpdateType, ReceivedUserData](EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
	{
		*bUpdateReceived = true;
		*ReceivedUpdateType = UpdateType;
		*ReceivedUserData = UserData;
	};
	CreateResult.User->OnUserUpdateReceivedNative.AddLambda(UpdateLambda);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedUpdateType, ReceivedUserData, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		TestEqual("Received UpdateType should be Updated", *ReceivedUpdateType, EPubnubChatStreamedUpdateType::PCSUT_Updated);
		
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
	TSharedPtr<TArray<EPubnubChatStreamedUpdateType>> ReceivedUpdateTypes = MakeShared<TArray<EPubnubChatStreamedUpdateType>>();
	TSharedPtr<TArray<FPubnubChatUserData>> ReceivedUpdates = MakeShared<TArray<FPubnubChatUserData>>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, UpdateCount, ReceivedUpdateTypes, ReceivedUpdates](EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
	{
		*UpdateCount = *UpdateCount + 1;
		ReceivedUpdateTypes->Add(UpdateType);
		ReceivedUpdates->Add(UserData);
	};
	CreateResult.User->OnUserUpdateReceivedNative.AddLambda(UpdateLambda);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedUpdateTypes, ReceivedUpdates]()
	{
		TestTrue("Both updates should have been received", *UpdateCount >= 2);
		if(*UpdateCount >= 2)
		{
			TestEqual("First update type should be Updated", (*ReceivedUpdateTypes)[0], EPubnubChatStreamedUpdateType::PCSUT_Updated);
			TestEqual("Second update type should be Updated", (*ReceivedUpdateTypes)[1], EPubnubChatStreamedUpdateType::PCSUT_Updated);
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
	TSharedPtr<EPubnubChatStreamedUpdateType> ReceivedUpdateType = MakeShared<EPubnubChatStreamedUpdateType>();
	TSharedPtr<FString> ReceivedUserID = MakeShared<FString>(TEXT(""));
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, bDeleteReceived, ReceivedUpdateType, ReceivedUserID](EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
	{
		*bDeleteReceived = true;
		*ReceivedUpdateType = UpdateType;
		*ReceivedUserID = UserID;
	};
	CreateResult.User->OnUserUpdateReceivedNative.AddLambda(UpdateLambda);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bDeleteReceived, ReceivedUpdateType, ReceivedUserID, TestUserID]()
	{
		TestTrue("Delete event should have been received", *bDeleteReceived);
		TestEqual("Received UpdateType should be Deleted", *ReceivedUpdateType, EPubnubChatStreamedUpdateType::PCSUT_Deleted);
		TestEqual("Received UserID should match", *ReceivedUserID, TestUserID);
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
	TSharedPtr<EPubnubChatStreamedUpdateType> ReceivedUpdateType = MakeShared<EPubnubChatStreamedUpdateType>();
	
	// Set up delegate to receive user updates
	auto UpdateLambda = [this, UpdateCount, ReceivedUpdateType](EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
	{
		*UpdateCount = *UpdateCount + 1;
		*ReceivedUpdateType = UpdateType;
	};
	CreateResult.User->OnUserUpdateReceivedNative.AddLambda(UpdateLambda);
	
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
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedUpdateType]()
	{
		TestTrue("First update should have been received", *UpdateCount >= 1);
		TestEqual("First update type should be Updated", *ReceivedUpdateType, EPubnubChatStreamedUpdateType::PCSUT_Updated);
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

#endif // WITH_DEV_AUTOMATION_TESTS

