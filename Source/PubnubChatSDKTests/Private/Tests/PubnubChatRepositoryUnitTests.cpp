// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK/Private/PubnubChatObjectsRepository.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

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
	FPubnubChatInternalUser* UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should exist after registration", UserData);
	
	if(UserData)
	{
		TestEqual("UserID should match", UserData->UserID, TestUserID);
	}
	
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
	FPubnubChatInternalUser* UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should exist after multiple registrations", UserData);
	
	// Unregister once - data should still exist
	Repository->UnregisterUser(TestUserID);
	UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should still exist after one unregistration", UserData);
	
	// Unregister second time - data should still exist
	Repository->UnregisterUser(TestUserID);
	UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should still exist after two unregistrations", UserData);
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterUser(TestUserID);
	UserData = Repository->GetUserData(TestUserID);
	TestNull("User data should be cleaned up when reference count reaches 0", UserData);
	
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
	FPubnubChatInternalUser* UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should exist", UserData);
	
	if(UserData)
	{
		TestEqual("UserName should match", UserData->UserData.UserName, TestData.UserName);
		TestEqual("Email should match", UserData->UserData.Email, TestData.Email);
		TestEqual("ProfileUrl should match", UserData->UserData.ProfileUrl, TestData.ProfileUrl);
	}
	
	// Register again (simulating second object with same ID)
	Repository->RegisterUser(TestUserID);
	
	// Data should still be there and unchanged
	UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should still exist after second registration", UserData);
	
	if(UserData)
	{
		TestEqual("UserName should still match after second registration", UserData->UserData.UserName, TestData.UserName);
		TestEqual("Email should still match after second registration", UserData->UserData.Email, TestData.Email);
	}
	
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
	
	// Get data reference
	FPubnubChatInternalUser* DataRef1 = Repository->GetUserData(TestUserID);
	TestNotNull("User data has to be valid", DataRef1);
	if (!DataRef1)
	{ return false;}
	TestEqual("First reference should see initial data", DataRef1->UserData.UserName, InitialData.UserName);
	
	// Register again (simulating second object)
	Repository->RegisterUser(TestUserID);
	
	// Update data
	FPubnubChatUserData UpdatedData;
	UpdatedData.UserName = TEXT("UpdatedUser");
	UpdatedData.Email = TEXT("updated@example.com");
	Repository->UpdateUserData(TestUserID, UpdatedData);
	
	// Both references should see updated data
	FPubnubChatInternalUser* DataPtr1 = Repository->GetUserData(TestUserID);
	FPubnubChatInternalUser* DataPtr2 = Repository->GetUserData(TestUserID);
	
	TestNotNull("DataPtr1 should exist", DataPtr1);
	TestNotNull("DataPtr2 should exist", DataPtr2);
	
	if(DataPtr1 && DataPtr2)
	{
		TestEqual("DataPtr1 should see updated UserName", DataPtr1->UserData.UserName, UpdatedData.UserName);
		TestEqual("DataPtr2 should see updated UserName", DataPtr2->UserData.UserName, UpdatedData.UserName);
		TestEqual("DataPtr1 should see updated Email", DataPtr1->UserData.Email, UpdatedData.Email);
		TestEqual("DataPtr2 should see updated Email", DataPtr2->UserData.Email, UpdatedData.Email);
		
		// Verify they point to the same data (same memory address)
		TestEqual("Both pointers should point to same data", DataPtr1, DataPtr2);
	}
	
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
	FPubnubChatInternalUser* UserData = Repository->GetUserData(TestUserID);
	TestNotNull("User data should exist before cleanup", UserData);
	
	// Unregister - should clean up data
	Repository->UnregisterUser(TestUserID);
	
	// Verify data is cleaned up
	UserData = Repository->GetUserData(TestUserID);
	TestNull("User data should be cleaned up after unregistration", UserData);
	
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
	
	// Test 1: Register with empty ID - should handle gracefully
	Repository->RegisterUser(TEXT(""));
	FPubnubChatInternalUser* EmptyData = Repository->GetUserData(TEXT(""));
	TestNull("Empty UserID should not create data", EmptyData);
	
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
	FPubnubChatInternalUser* NonExistentData = Repository->GetUserData(TEXT("non_existent"));
	TestNull("Non-existent user should return nullptr", NonExistentData);
	
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
	FPubnubChatInternalChannel* ChannelData = Repository->GetChannelData(TestChannelID);
	TestNotNull("Channel data should exist after registration", ChannelData);
	
	if(ChannelData)
	{
		TestEqual("ChannelID should match", ChannelData->ChannelID, TestChannelID);
	}
	
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
	FPubnubChatInternalChannel* ChannelData = Repository->GetChannelData(TestChannelID);
	TestNotNull("Channel data should exist after multiple registrations", ChannelData);
	
	// Unregister twice - data should still exist
	Repository->UnregisterChannel(TestChannelID);
	Repository->UnregisterChannel(TestChannelID);
	ChannelData = Repository->GetChannelData(TestChannelID);
	TestNotNull("Channel data should still exist after two unregistrations", ChannelData);
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterChannel(TestChannelID);
	ChannelData = Repository->GetChannelData(TestChannelID);
	TestNull("Channel data should be cleaned up when reference count reaches 0", ChannelData);
	
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
	FPubnubChatInternalMessage* MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should exist after registration", MessageData);
	
	if(MessageData)
	{
		TestEqual("MessageID should match", MessageData->MessageID, CompositeMessageID);
	}
	
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
	FPubnubChatInternalMessage* MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should exist after multiple registrations", MessageData);
	
	// Unregister once - data should still exist
	Repository->UnregisterMessage(CompositeMessageID);
	MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should still exist after one unregistration", MessageData);
	
	// Unregister second time - data should still exist
	Repository->UnregisterMessage(CompositeMessageID);
	MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should still exist after two unregistrations", MessageData);
	
	// Unregister third time - data should be cleaned up
	Repository->UnregisterMessage(CompositeMessageID);
	MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNull("Message data should be cleaned up when reference count reaches 0", MessageData);
	
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
	FPubnubChatInternalMessage* MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should exist", MessageData);
	
	if(MessageData)
	{
		TestEqual("Text should match", MessageData->MessageData.Text, TestData.Text);
		TestEqual("ChannelID should match", MessageData->MessageData.ChannelID, TestData.ChannelID);
		TestEqual("UserID should match", MessageData->MessageData.UserID, TestData.UserID);
		TestEqual("Meta should match", MessageData->MessageData.Meta, TestData.Meta);
	}
	
	// Register again (simulating second object with same composite ID)
	Repository->RegisterMessage(CompositeMessageID);
	
	// Data should still be there and unchanged
	MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should still exist after second registration", MessageData);
	
	if(MessageData)
	{
		TestEqual("Text should still match after second registration", MessageData->MessageData.Text, TestData.Text);
		TestEqual("ChannelID should still match after second registration", MessageData->MessageData.ChannelID, TestData.ChannelID);
	}
	
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
	
	// Get data reference
	FPubnubChatInternalMessage* DataRef1 = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data has to be valid", DataRef1);
	if (!DataRef1)
	{ return false;}
	TestEqual("First reference should see initial data", DataRef1->MessageData.Text, InitialData.Text);
	
	// Register again (simulating second object)
	Repository->RegisterMessage(CompositeMessageID);
	
	// Update data
	FPubnubChatMessageData UpdatedData;
	UpdatedData.Text = TEXT("Updated message");
	UpdatedData.ChannelID = TestChannelID;
	UpdatedData.UserID = TEXT("test_user_updated");
	Repository->UpdateMessageData(CompositeMessageID, UpdatedData);
	
	// Both references should see updated data
	FPubnubChatInternalMessage* DataPtr1 = Repository->GetMessageData(CompositeMessageID);
	FPubnubChatInternalMessage* DataPtr2 = Repository->GetMessageData(CompositeMessageID);
	
	TestNotNull("DataPtr1 should exist", DataPtr1);
	TestNotNull("DataPtr2 should exist", DataPtr2);
	
	if(DataPtr1 && DataPtr2)
	{
		TestEqual("DataPtr1 should see updated Text", DataPtr1->MessageData.Text, UpdatedData.Text);
		TestEqual("DataPtr2 should see updated Text", DataPtr2->MessageData.Text, UpdatedData.Text);
		TestEqual("DataPtr1 should see updated UserID", DataPtr1->MessageData.UserID, UpdatedData.UserID);
		TestEqual("DataPtr2 should see updated UserID", DataPtr2->MessageData.UserID, UpdatedData.UserID);
		
		// Verify they point to the same data (same memory address)
		TestEqual("Both pointers should point to same data", DataPtr1, DataPtr2);
	}
	
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
	FPubnubChatInternalMessage* MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should exist before cleanup", MessageData);
	
	// Unregister - should clean up data
	Repository->UnregisterMessage(CompositeMessageID);
	
	// Verify data is cleaned up
	MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNull("Message data should be cleaned up after unregistration", MessageData);
	
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
	
	// Test 1: Register with empty composite ID - should handle gracefully
	Repository->RegisterMessage(TEXT(""));
	FPubnubChatInternalMessage* EmptyData = Repository->GetMessageData(TEXT(""));
	TestNull("Empty composite MessageID should not create data", EmptyData);
	
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
	FPubnubChatInternalMessage* NonExistentData = Repository->GetMessageData(NonExistentCompositeID);
	TestNull("Non-existent message should return nullptr", NonExistentData);
	
	// Test 6: Test composite ID format with different separators (should use dot)
	const FString ChannelWithDot = TEXT("channel.with.dots");
	const FString Timetoken = TEXT("12345678901234567");
	const FString CompositeIDWithDots = FString::Printf(TEXT("%s.%s"), *ChannelWithDot, *Timetoken);
	Repository->RegisterMessage(CompositeIDWithDots);
	FPubnubChatInternalMessage* DataWithDots = Repository->GetMessageData(CompositeIDWithDots);
	TestNotNull("Message with dots in channel ID should work", DataWithDots);
	if(DataWithDots)
	{
		TestEqual("Composite ID should preserve channel with dots", DataWithDots->MessageID, CompositeIDWithDots);
	}
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
	FPubnubChatInternalMessage* MessageData = Repository->GetMessageData(CompositeMessageID);
	TestNotNull("Message data should exist", MessageData);
	
	if(MessageData)
	{
		TestEqual("MessageActions count should match", MessageData->MessageData.MessageActions.Num(), 2);
		
		if(MessageData->MessageData.MessageActions.Num() >= 2)
		{
			TestEqual("First action Type should match", MessageData->MessageData.MessageActions[0].Type, Action1.Type);
			TestEqual("First action Value should match", MessageData->MessageData.MessageActions[0].Value, Action1.Value);
			TestEqual("First action UserID should match", MessageData->MessageData.MessageActions[0].UserID, Action1.UserID);
			
			TestEqual("Second action Type should match", MessageData->MessageData.MessageActions[1].Type, Action2.Type);
			TestEqual("Second action Value should match", MessageData->MessageData.MessageActions[1].Value, Action2.Value);
			TestEqual("Second action UserID should match", MessageData->MessageData.MessageActions[1].UserID, Action2.UserID);
		}
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
	TestNotNull("User1 data should exist", Repository->GetUserData(TestUserID));
	TestNotNull("User2 data should exist", Repository->GetUserData(TEXT("test_user_2")));
	TestNotNull("Channel1 data should exist", Repository->GetChannelData(TestChannelID));
	TestNotNull("Channel2 data should exist", Repository->GetChannelData(TEXT("test_channel_2")));
	TestNotNull("Message1 data should exist", Repository->GetMessageData(CompositeMessageID1));
	TestNotNull("Message2 data should exist", Repository->GetMessageData(CompositeMessageID2));
	
	// Clear all
	Repository->ClearAll();
	
	// Verify all data is cleared
	TestNull("User1 data should be cleared", Repository->GetUserData(TestUserID));
	TestNull("User2 data should be cleared", Repository->GetUserData(TEXT("test_user_2")));
	TestNull("Channel1 data should be cleared", Repository->GetChannelData(TestChannelID));
	TestNull("Channel2 data should be cleared", Repository->GetChannelData(TEXT("test_channel_2")));
	TestNull("Message1 data should be cleared", Repository->GetMessageData(CompositeMessageID1));
	TestNull("Message2 data should be cleared", Repository->GetMessageData(CompositeMessageID2));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

