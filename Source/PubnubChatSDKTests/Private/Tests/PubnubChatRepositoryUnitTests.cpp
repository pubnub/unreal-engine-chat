// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK/Private/PubnubChatObjectsRepository.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
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
	FPubnubChatInternalUser& DataRef1 = Repository->GetOrCreateUserData(TestUserID);
	TestEqual("First reference should see initial data", DataRef1.UserData.UserName, InitialData.UserName);
	
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
	
	// Register users and channels
	Repository->RegisterUser(TestUserID);
	Repository->RegisterUser(TEXT("test_user_2"));
	Repository->RegisterChannel(TestChannelID);
	Repository->RegisterChannel(TEXT("test_channel_2"));
	
	// Verify data exists
	TestNotNull("User1 data should exist", Repository->GetUserData(TestUserID));
	TestNotNull("User2 data should exist", Repository->GetUserData(TEXT("test_user_2")));
	TestNotNull("Channel1 data should exist", Repository->GetChannelData(TestChannelID));
	TestNotNull("Channel2 data should exist", Repository->GetChannelData(TEXT("test_channel_2")));
	
	// Clear all
	Repository->ClearAll();
	
	// Verify all data is cleared
	TestNull("User1 data should be cleared", Repository->GetUserData(TestUserID));
	TestNull("User2 data should be cleared", Repository->GetUserData(TEXT("test_user_2")));
	TestNull("Channel1 data should be cleared", Repository->GetChannelData(TestChannelID));
	TestNull("Channel2 data should be cleared", Repository->GetChannelData(TEXT("test_channel_2")));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

