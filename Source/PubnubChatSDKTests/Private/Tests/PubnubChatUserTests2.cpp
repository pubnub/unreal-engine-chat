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
			Chat->DeleteUser(TestUserID, false);
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
			Chat->DeleteUser(TestUserID, false);
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
			Chat->DeleteUser(TestUserID, false);
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
			Chat->DeleteUser(TestUserID, false);
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
		// Hard delete user (Soft = false) - this should trigger a delete event
		FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(TestUserID, false);
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
		Chat->DeleteUser(TestUserID, false);
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
			Chat->DeleteUser(TestUserID, false);
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
		Chat->DeleteUser(TestUserID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

