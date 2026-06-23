// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK/Private/PubnubChatObjectsRepository.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

// ============================================================================
// REPOSITORY UNIT TESTS - Direct Repository Testing (No API Calls)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserRegistrationTest, "PubnubChat.Unit.Repository.User.Registration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_1");
	
	// Create repository directly (no API calls)
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GetTransientPackage());
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register user - should create data entry
	Repository->RegisterUser(TestUserID);
	
	// Verify data exists after registration
	FPubnubChatUserData UserData;
	TestTrue("User data should exist after registration", Repository->TryGetUserData(TestUserID, UserData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserMultipleRegistrationTest, "PubnubChat.Unit.Repository.User.MultipleRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserMultipleRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_multiple");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register same user multiple times (simulating multiple objects with same ID)
	Repository->RegisterUser(TestUserID);
	Repository->RegisterUser(TestUserID);
	Repository->RegisterUser(TestUserID);
	
	// Data should still exist (not cleaned up)
	FPubnubChatUserData UserData;
	TestTrue("User data should exist after multiple registrations", Repository->TryGetUserData(TestUserID, UserData));
	
	// Unregister once - data should still exist
	Repository->UnregisterUser(TestUserID);
	TestTrue("User data should still exist after one unregistration", Repository->TryGetUserData(TestUserID, UserData));
	
	// Unregister second time - data should still exist
	Repository->UnregisterUser(TestUserID);
	TestTrue("User data should still exist after two unregistrations", Repository->TryGetUserData(TestUserID, UserData));
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterUser(TestUserID);
	TestFalse("User data should be cleaned up when reference count reaches 0", Repository->TryGetUserData(TestUserID, UserData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserDataPersistenceTest, "PubnubChat.Unit.Repository.User.DataPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserDataPersistenceTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_persistence");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register and update data
	Repository->RegisterUser(TestUserID);
	
	FPubnubChatUserData TestData;
	TestData.UserName = TEXT("TestUser");
	TestData.Email = TEXT("test@example.com");
	TestData.ProfileUrl = TEXT("https://example.com/avatar.jpg");
	
	Repository->UpdateUserData(TestUserID, TestData);
	
	// Verify data is stored
	FPubnubChatUserData UserData;
	TestTrue("User data should exist", Repository->TryGetUserData(TestUserID, UserData));
	TestEqual("UserName should match", UserData.UserName, TestData.UserName);
	TestEqual("Email should match", UserData.Email, TestData.Email);
	TestEqual("ProfileUrl should match", UserData.ProfileUrl, TestData.ProfileUrl);
	
	// Register again (simulating second object with same ID)
	Repository->RegisterUser(TestUserID);
	
	// Data should still be there and unchanged
	TestTrue("User data should still exist after second registration", Repository->TryGetUserData(TestUserID, UserData));
	TestEqual("UserName should still match after second registration", UserData.UserName, TestData.UserName);
	TestEqual("Email should still match after second registration", UserData.Email, TestData.Email);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserDataSharingTest, "PubnubChat.Unit.Repository.User.DataSharing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserDataSharingTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_sharing");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register user and set initial data
	Repository->RegisterUser(TestUserID);
	
	FPubnubChatUserData InitialData;
	InitialData.UserName = TEXT("InitialUser");
	Repository->UpdateUserData(TestUserID, InitialData);
	
	// Get data copy
	FPubnubChatUserData DataRef1;
	TestTrue("User data has to be valid", Repository->TryGetUserData(TestUserID, DataRef1));
	TestEqual("First reference should see initial data", DataRef1.UserName, InitialData.UserName);
	
	// Register again (simulating second object)
	Repository->RegisterUser(TestUserID);
	
	// Update data
	FPubnubChatUserData UpdatedData;
	UpdatedData.UserName = TEXT("UpdatedUser");
	UpdatedData.Email = TEXT("updated@example.com");
	Repository->UpdateUserData(TestUserID, UpdatedData);
	
	// Both reads should see updated data
	FPubnubChatUserData DataPtr1;
	FPubnubChatUserData DataPtr2;
	TestTrue("DataPtr1 should exist", Repository->TryGetUserData(TestUserID, DataPtr1));
	TestTrue("DataPtr2 should exist", Repository->TryGetUserData(TestUserID, DataPtr2));
	TestEqual("DataPtr1 should see updated UserName", DataPtr1.UserName, UpdatedData.UserName);
	TestEqual("DataPtr2 should see updated UserName", DataPtr2.UserName, UpdatedData.UserName);
	TestEqual("DataPtr1 should see updated Email", DataPtr1.Email, UpdatedData.Email);
	TestEqual("DataPtr2 should see updated Email", DataPtr2.Email, UpdatedData.Email);
	TestEqual("Both reads should return same UserName", DataPtr1.UserName, DataPtr2.UserName);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserCleanupTest, "PubnubChat.Unit.Repository.User.Cleanup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserCleanupTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_cleanup");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register user and add data
	Repository->RegisterUser(TestUserID);
	FPubnubChatUserData TestData;
	TestData.UserName = TEXT("TestUser");
	Repository->UpdateUserData(TestUserID, TestData);
	
	// Verify data exists
	FPubnubChatUserData UserData;
	TestTrue("User data should exist before cleanup", Repository->TryGetUserData(TestUserID, UserData));
	
	// Unregister - should clean up data
	Repository->UnregisterUser(TestUserID);
	
	// Verify data is cleaned up
	TestFalse("User data should be cleaned up after unregistration", Repository->TryGetUserData(TestUserID, UserData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryUserEdgeCasesTest, "PubnubChat.Unit.Repository.User.EdgeCases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryUserEdgeCasesTest::RunTest(const FString& Parameters)
{
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	FPubnubChatUserData UserData;
	
	// Test 1: Register with empty ID - should handle gracefully
	Repository->RegisterUser(TEXT(""));
	TestFalse("Empty UserID should not create data", Repository->TryGetUserData(TEXT(""), UserData));
	
	// Test 2: Unregister non-existent user - should handle gracefully
	Repository->UnregisterUser(TEXT("non_existent_user"));
	// Should not crash
	
	// Test 3: Unregister empty ID - should handle gracefully
	Repository->UnregisterUser(TEXT(""));
	// Should not crash
	
	// Test 4: Multiple unregistrations without registration - should handle gracefully
	const FString TestUserID = TEXT("test_user_edge");
	Repository->UnregisterUser(TestUserID);
	Repository->UnregisterUser(TestUserID);
	Repository->UnregisterUser(TestUserID);
	// Should not crash
	
	// Test 5: Get data for non-existent user
	TestFalse("Non-existent user should not be found", Repository->TryGetUserData(TEXT("non_existent"), UserData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryChannelRegistrationTest, "PubnubChat.Unit.Repository.Channel.Registration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryChannelRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_1");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register channel - should create data entry
	Repository->RegisterChannel(TestChannelID);
	
	// Verify data exists after registration
	FPubnubChatChannelData ChannelData;
	TestTrue("Channel data should exist after registration", Repository->TryGetChannelData(TestChannelID, ChannelData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryChannelMultipleRegistrationTest, "PubnubChat.Unit.Repository.Channel.MultipleRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryChannelMultipleRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_multiple");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register same channel multiple times
	Repository->RegisterChannel(TestChannelID);
	Repository->RegisterChannel(TestChannelID);
	Repository->RegisterChannel(TestChannelID);
	
	// Data should still exist
	FPubnubChatChannelData ChannelData;
	TestTrue("Channel data should exist after multiple registrations", Repository->TryGetChannelData(TestChannelID, ChannelData));
	
	// Unregister twice - data should still exist
	Repository->UnregisterChannel(TestChannelID);
	Repository->UnregisterChannel(TestChannelID);
	TestTrue("Channel data should still exist after two unregistrations", Repository->TryGetChannelData(TestChannelID, ChannelData));
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterChannel(TestChannelID);
	TestFalse("Channel data should be cleaned up when reference count reaches 0", Repository->TryGetChannelData(TestChannelID, ChannelData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageRegistrationTest, "PubnubChat.Unit.Repository.Message.Registration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_1");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	// Create repository directly (no API calls)
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GetTransientPackage());
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register message - should create data entry
	Repository->RegisterMessage(CompositeMessageID);
	
	// Verify data exists after registration
	FPubnubChatMessageData MessageData;
	TestTrue("Message data should exist after registration", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageMultipleRegistrationTest, "PubnubChat.Unit.Repository.Message.MultipleRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageMultipleRegistrationTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_multiple");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register same message multiple times (simulating multiple objects with same composite ID)
	Repository->RegisterMessage(CompositeMessageID);
	Repository->RegisterMessage(CompositeMessageID);
	Repository->RegisterMessage(CompositeMessageID);
	
	// Data should still exist (not cleaned up)
	FPubnubChatMessageData MessageData;
	TestTrue("Message data should exist after multiple registrations", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	// Unregister once - data should still exist
	Repository->UnregisterMessage(CompositeMessageID);
	TestTrue("Message data should still exist after one unregistration", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	// Unregister second time - data should still exist
	Repository->UnregisterMessage(CompositeMessageID);
	TestTrue("Message data should still exist after two unregistrations", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterMessage(CompositeMessageID);
	TestFalse("Message data should be cleaned up when reference count reaches 0", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageDataPersistenceTest, "PubnubChat.Unit.Repository.Message.DataPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageDataPersistenceTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_persistence");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register and update data
	Repository->RegisterMessage(CompositeMessageID);
	
	FPubnubChatMessageData TestData;
	TestData.Text = TEXT("Test message text");
	TestData.ChannelID = TestChannelID;
	TestData.UserID = TEXT("test_user_1");
	TestData.Meta = TEXT("{\"key\":\"value\"}");
	
	Repository->UpdateMessageData(CompositeMessageID, TestData);
	
	// Verify data is stored
	FPubnubChatMessageData MessageData;
	TestTrue("Message data should exist", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	TestEqual("Text should match", MessageData.Text, TestData.Text);
	TestEqual("ChannelID should match", MessageData.ChannelID, TestData.ChannelID);
	TestEqual("UserID should match", MessageData.UserID, TestData.UserID);
	TestEqual("Meta should match", MessageData.Meta, TestData.Meta);
	
	// Register again (simulating second object with same composite ID)
	Repository->RegisterMessage(CompositeMessageID);
	
	// Data should still be there and unchanged
	TestTrue("Message data should still exist after second registration", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	TestEqual("Text should still match after second registration", MessageData.Text, TestData.Text);
	TestEqual("ChannelID should still match after second registration", MessageData.ChannelID, TestData.ChannelID);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageDataSharingTest, "PubnubChat.Unit.Repository.Message.DataSharing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageDataSharingTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_sharing");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register message and set initial data
	Repository->RegisterMessage(CompositeMessageID);
	
	FPubnubChatMessageData InitialData;
	InitialData.Text = TEXT("Initial message");
	InitialData.ChannelID = TestChannelID;
	Repository->UpdateMessageData(CompositeMessageID, InitialData);
	
	// Get data copy
	FPubnubChatMessageData DataRef1;
	TestTrue("Message data has to be valid", Repository->TryGetMessageData(CompositeMessageID, DataRef1));
	TestEqual("First reference should see initial data", DataRef1.Text, InitialData.Text);
	
	// Register again (simulating second object)
	Repository->RegisterMessage(CompositeMessageID);
	
	// Update data
	FPubnubChatMessageData UpdatedData;
	UpdatedData.Text = TEXT("Updated message");
	UpdatedData.ChannelID = TestChannelID;
	UpdatedData.UserID = TEXT("test_user_updated");
	Repository->UpdateMessageData(CompositeMessageID, UpdatedData);
	
	// Both reads should see updated data
	FPubnubChatMessageData DataPtr1;
	FPubnubChatMessageData DataPtr2;
	TestTrue("DataPtr1 should exist", Repository->TryGetMessageData(CompositeMessageID, DataPtr1));
	TestTrue("DataPtr2 should exist", Repository->TryGetMessageData(CompositeMessageID, DataPtr2));
	TestEqual("DataPtr1 should see updated Text", DataPtr1.Text, UpdatedData.Text);
	TestEqual("DataPtr2 should see updated Text", DataPtr2.Text, UpdatedData.Text);
	TestEqual("DataPtr1 should see updated UserID", DataPtr1.UserID, UpdatedData.UserID);
	TestEqual("DataPtr2 should see updated UserID", DataPtr2.UserID, UpdatedData.UserID);
	TestEqual("Both reads should return same Text", DataPtr1.Text, DataPtr2.Text);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageCleanupTest, "PubnubChat.Unit.Repository.Message.Cleanup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageCleanupTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_cleanup");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register message and add data
	Repository->RegisterMessage(CompositeMessageID);
	FPubnubChatMessageData TestData;
	TestData.Text = TEXT("Test message");
	TestData.ChannelID = TestChannelID;
	Repository->UpdateMessageData(CompositeMessageID, TestData);
	
	// Verify data exists
	FPubnubChatMessageData MessageData;
	TestTrue("Message data should exist before cleanup", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	// Unregister - should clean up data
	Repository->UnregisterMessage(CompositeMessageID);
	
	// Verify data is cleaned up
	TestFalse("Message data should be cleaned up after unregistration", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageEdgeCasesTest, "PubnubChat.Unit.Repository.Message.EdgeCases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageEdgeCasesTest::RunTest(const FString& Parameters)
{
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	FPubnubChatMessageData MessageData;
	
	// Test 1: Register with empty composite ID - should handle gracefully
	Repository->RegisterMessage(TEXT(""));
	TestFalse("Empty composite MessageID should not create data", Repository->TryGetMessageData(TEXT(""), MessageData));
	
	// Test 2: Unregister non-existent message - should handle gracefully
	const FString NonExistentChannelID = TEXT("non_existent_channel");
	const FString NonExistentTimetoken = TEXT("99999999999999999");
	const FString NonExistentCompositeID = FString::Printf(TEXT("%s.%s"), *NonExistentChannelID, *NonExistentTimetoken);
	Repository->UnregisterMessage(NonExistentCompositeID);
	// Should not crash
	
	// Test 3: Unregister empty composite ID - should handle gracefully
	Repository->UnregisterMessage(TEXT(""));
	// Should not crash
	
	// Test 4: Multiple unregistrations without registration - should handle gracefully
	const FString TestChannelID = TEXT("test_channel_edge");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	Repository->UnregisterMessage(CompositeMessageID);
	Repository->UnregisterMessage(CompositeMessageID);
	Repository->UnregisterMessage(CompositeMessageID);
	// Should not crash
	
	// Test 5: Get data for non-existent message
	TestFalse("Non-existent message should not be found", Repository->TryGetMessageData(NonExistentCompositeID, MessageData));
	
	// Test 6: Test composite ID format with different separators (should use dot)
	const FString ChannelWithDot = TEXT("channel.with.dots");
	const FString Timetoken = TEXT("12345678901234567");
	const FString CompositeIDWithDots = FString::Printf(TEXT("%s.%s"), *ChannelWithDot, *Timetoken);
	Repository->RegisterMessage(CompositeIDWithDots);
	TestTrue("Message with dots in channel ID should work", Repository->TryGetMessageData(CompositeIDWithDots, MessageData));
	Repository->UnregisterMessage(CompositeIDWithDots);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryMessageActionsTest, "PubnubChat.Unit.Repository.Message.MessageActions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryMessageActionsTest::RunTest(const FString& Parameters)
{
	const FString TestChannelID = TEXT("test_channel_actions");
	const FString TestTimetoken = TEXT("12345678901234567");
	const FString CompositeMessageID = FString::Printf(TEXT("%s.%s"), *TestChannelID, *TestTimetoken);
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register message
	Repository->RegisterMessage(CompositeMessageID);
	
	// Create message data with actions
	FPubnubChatMessageData TestData;
	TestData.Text = TEXT("Message with actions");
	TestData.ChannelID = TestChannelID;
	TestData.UserID = TEXT("test_user_1");
	
	// Add message actions
	FPubnubChatMessageAction Action1;
	Action1.Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	Action1.Value = TEXT("👍");
	Action1.UserID = TEXT("user1");
	Action1.Timetoken = TEXT("11111111111111111");
	
	FPubnubChatMessageAction Action2;
	Action2.Type = EPubnubChatMessageActionType::PCMAT_Custom;
	Action2.Value = TEXT("custom_value");
	Action2.UserID = TEXT("user2");
	Action2.Timetoken = TEXT("22222222222222222");
	
	TestData.MessageActions.Add(Action1);
	TestData.MessageActions.Add(Action2);
	
	Repository->UpdateMessageData(CompositeMessageID, TestData);
	
	// Verify actions are stored
	FPubnubChatMessageData MessageData;
	TestTrue("Message data should exist", Repository->TryGetMessageData(CompositeMessageID, MessageData));
	TestEqual("MessageActions count should match", MessageData.MessageActions.Num(), 2);
	
	if(MessageData.MessageActions.Num() >= 2)
	{
		TestEqual("First action Type should match", MessageData.MessageActions[0].Type, Action1.Type);
		TestEqual("First action Value should match", MessageData.MessageActions[0].Value, Action1.Value);
		TestEqual("First action UserID should match", MessageData.MessageActions[0].UserID, Action1.UserID);
		
		TestEqual("Second action Type should match", MessageData.MessageActions[1].Type, Action2.Type);
		TestEqual("Second action Value should match", MessageData.MessageActions[1].Value, Action2.Value);
		TestEqual("Second action UserID should match", MessageData.MessageActions[1].UserID, Action2.UserID);
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatRepositoryClearAllTest, "PubnubChat.Unit.Repository.ClearAll", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatRepositoryClearAllTest::RunTest(const FString& Parameters)
{
	const FString TestUserID = TEXT("test_user_clear");
	const FString TestChannelID = TEXT("test_channel_clear");
	
	UPubnubChatObjectsRepository* Repository = NewObject<UPubnubChatObjectsRepository>(GEngine);
	TestNotNull("Repository should be created", Repository);
	
	if(!Repository)
	{
		return false;
	}
	
	// Register users, channels, and messages
	Repository->RegisterUser(TestUserID);
	Repository->RegisterUser(TEXT("test_user_2"));
	Repository->RegisterChannel(TestChannelID);
	Repository->RegisterChannel(TEXT("test_channel_2"));
	
	const FString TestMessageChannelID = TEXT("test_channel_message");
	const FString TestTimetoken1 = TEXT("11111111111111111");
	const FString TestTimetoken2 = TEXT("22222222222222222");
	const FString CompositeMessageID1 = FString::Printf(TEXT("%s.%s"), *TestMessageChannelID, *TestTimetoken1);
	const FString CompositeMessageID2 = FString::Printf(TEXT("%s.%s"), *TestMessageChannelID, *TestTimetoken2);
	
	Repository->RegisterMessage(CompositeMessageID1);
	Repository->RegisterMessage(CompositeMessageID2);
	
	// Verify data exists
	FPubnubChatUserData UserData;
	FPubnubChatChannelData ChannelData;
	FPubnubChatMessageData MessageData;
	TestTrue("User1 data should exist", Repository->TryGetUserData(TestUserID, UserData));
	TestTrue("User2 data should exist", Repository->TryGetUserData(TEXT("test_user_2"), UserData));
	TestTrue("Channel1 data should exist", Repository->TryGetChannelData(TestChannelID, ChannelData));
	TestTrue("Channel2 data should exist", Repository->TryGetChannelData(TEXT("test_channel_2"), ChannelData));
	TestTrue("Message1 data should exist", Repository->TryGetMessageData(CompositeMessageID1, MessageData));
	TestTrue("Message2 data should exist", Repository->TryGetMessageData(CompositeMessageID2, MessageData));
	
	// Clear all
	Repository->ClearAll();
	
	// Verify all data is cleared
	TestFalse("User1 data should be cleared", Repository->TryGetUserData(TestUserID, UserData));
	TestFalse("User2 data should be cleared", Repository->TryGetUserData(TEXT("test_user_2"), UserData));
	TestFalse("Channel1 data should be cleared", Repository->TryGetChannelData(TestChannelID, ChannelData));
	TestFalse("Channel2 data should be cleared", Repository->TryGetChannelData(TEXT("test_channel_2"), ChannelData));
	TestFalse("Message1 data should be cleared", Repository->TryGetMessageData(CompositeMessageID1, MessageData));
	TestFalse("Message2 data should be cleared", Repository->TryGetMessageData(CompositeMessageID2, MessageData));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
