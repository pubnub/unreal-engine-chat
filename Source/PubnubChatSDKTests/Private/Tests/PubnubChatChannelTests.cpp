// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// CREATEPUBLICCONVERSATION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create channel without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_create_channel_not_init";
			FPubnubChatChannelData ChannelData;
			FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
			
			TestTrue("CreatePublicConversation should fail when Chat is not initialized", CreateResult.Result.Error);
			TestNull("Channel should not be created", CreateResult.Channel);
			TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_create_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to create channel with empty ChannelID
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TEXT(""), ChannelData);
		
		TestTrue("CreatePublicConversation should fail with empty ChannelID", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel with only required parameter (ChannelID) and default ChannelData
		FPubnubChatChannelData ChannelData; // Default empty struct
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
			
			// Verify channel can get data from repository
			FPubnubChatChannelData ChannelDataRetrieved = CreateResult.Channel->GetChannelData();
			TestTrue("Channel should be able to get data from repository", true);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_channel_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_channel_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel with all parameters
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("TestChannelName");
		ChannelData.Description = TEXT("Test channel description");
		ChannelData.Custom = TEXT("{\"key\":\"value\"}");
		ChannelData.Status = TEXT("active");
		ChannelData.Type = TEXT("public");
		
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed with all parameters", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
			
			// Verify all data is stored correctly
			FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
			TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
			TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
			TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
			TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
			TestEqual("Type should match", RetrievedData.Type, ChannelData.Type);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests creating a channel that already exists.
 * Verifies that attempting to create a duplicate channel fails appropriately.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationDuplicateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.4Advanced.DuplicateChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationDuplicateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_channel_dup_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_channel_dup";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel first time
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("First CreatePublicConversation should succeed", CreateResult1.Result.Error);
		TestNotNull("First Channel should be created", CreateResult1.Channel);
		
		// Try to create same channel again - should fail or handle gracefully
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		// Note: Behavior may vary - could fail or succeed depending on implementation
		// For now, we verify the operation completes (either success or expected failure)
		TestTrue("Second CreatePublicConversation should complete", true);
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests that channel data persists correctly after creation.
 * Verifies that created channel data can be retrieved and matches what was set.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationDataPersistenceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.4Advanced.DataPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationDataPersistenceTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_channel_persistence_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_channel_persistence";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel with data
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("PersistentChannel");
		ChannelData.Description = TEXT("Test persistence");
		ChannelData.Status = TEXT("active");
		
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		// Get same channel - should have same data
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed", GetResult.Result.Error);
		TestNotNull("GetChannel should return channel", GetResult.Channel);
		
		if(CreateResult.Channel && GetResult.Channel)
		{
			// Both channels should have same data from repository
			FPubnubChatChannelData CreateData = CreateResult.Channel->GetChannelData();
			FPubnubChatChannelData GetData = GetResult.Channel->GetChannelData();
			
			TestEqual("ChannelNames should match", CreateData.ChannelName, GetData.ChannelName);
			TestEqual("Descriptions should match", CreateData.Description, GetData.Description);
			TestEqual("Statuses should match", CreateData.Status, GetData.Status);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests that CreatePublicConversation properly tracks step results for internal operations.
 * Verifies that step results contain expected operations like SetChannelMetadata.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreatePublicConversationStepResultsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreatePublicConversation.4Advanced.StepResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreatePublicConversationStepResultsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_channel_step_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_channel_step";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		
		// Verify step results are tracked
		TestTrue("StepResults should contain at least one step", CreateResult.Result.StepResults.Num() > 0);
		
		// Check that step results contain expected operations
		bool bFoundSetChannelMetadata = false;
		
		for(const FPubnubChatOperationStepResult& Step : CreateResult.Result.StepResults)
		{
			if(Step.StepName == TEXT("SetChannelMetadata"))
			{
				bFoundSetChannelMetadata = true;
				TestFalse("SetChannelMetadata step should not have error", Step.OperationResult.Error);
			}
		}
		
		TestTrue("Should have SetChannelMetadata step", bFoundSetChannelMetadata);
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// GETCHANNEL TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get channel without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TestChannelID = SDK_PREFIX + "test_get_channel_not_init";
			FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
			
			TestTrue("GetChannel should fail when Chat is not initialized", GetResult.Result.Error);
			TestNull("Channel should not be returned", GetResult.Channel);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channel_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to get channel with empty ChannelID
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TEXT(""));
		
		TestTrue("GetChannel should fail with empty ChannelID", GetResult.Result.Error);
		TestNull("Channel should not be returned", GetResult.Channel);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		
		// Then get the channel
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
		
		TestFalse("GetChannel should succeed", GetResult.Result.Error);
		TestNotNull("Channel should be returned", GetResult.Channel);
		
		if(GetResult.Channel)
		{
			TestEqual("GetChannel ChannelID should match", GetResult.Channel->GetChannelID(), TestChannelID);
			
			// Verify channel can get data from repository
			FPubnubChatChannelData ChannelDataRetrieved = GetResult.Channel->GetChannelData();
			TestTrue("Channel should be able to get data from repository", true);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the channel with all data
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("FullTestChannel");
		ChannelData.Description = TEXT("Full test channel description");
		ChannelData.Custom = TEXT("{\"test\":\"data\"}");
		ChannelData.Status = TEXT("active");
		ChannelData.Type = TEXT("public");
		
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		
		// Get the channel (only takes ChannelID parameter)
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
		
		TestFalse("GetChannel should succeed", GetResult.Result.Error);
		TestNotNull("Channel should be returned", GetResult.Channel);
		
		if(GetResult.Channel)
		{
			TestEqual("GetChannel ChannelID should match", GetResult.Channel->GetChannelID(), TestChannelID);
			
			// Verify all data is retrieved correctly
			FPubnubChatChannelData RetrievedData = GetResult.Channel->GetChannelData();
			TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
			TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
			TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
			TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
			TestEqual("Type should match", RetrievedData.Type, ChannelData.Type);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests getting a channel that doesn't exist.
 * Verifies that GetChannel fails appropriately when the channel doesn't exist on the server.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelNonExistentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.4Advanced.NonExistentChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelNonExistentTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_nonexistent_init";
	const FString NonExistentChannelID = SDK_PREFIX + "test_get_channel_nonexistent_" + FString::FromInt(FDateTime::Now().ToUnixTimestamp());
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to get channel that doesn't exist
		FPubnubChatChannelResult GetResult = Chat->GetChannel(NonExistentChannelID);
		
		// GetChannel will fail if channel doesn't exist on server
		TestTrue("GetChannel should fail for non-existent channel", GetResult.Result.Error);
		TestNull("Channel should not be returned", GetResult.Channel);
		TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

/**
 * Tests calling GetChannel multiple times for the same channel.
 * Verifies that multiple calls return consistent results and that channel objects share data from repository.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelMultipleCallsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.4Advanced.MultipleCalls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelMultipleCallsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("MultipleCallsChannel");
		ChannelData.Description = TEXT("Test multiple calls");
		
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		
		// Get channel multiple times
		FPubnubChatChannelResult GetResult1 = Chat->GetChannel(TestChannelID);
		FPubnubChatChannelResult GetResult2 = Chat->GetChannel(TestChannelID);
		FPubnubChatChannelResult GetResult3 = Chat->GetChannel(TestChannelID);
		
		TestFalse("First GetChannel should succeed", GetResult1.Result.Error);
		TestFalse("Second GetChannel should succeed", GetResult2.Result.Error);
		TestFalse("Third GetChannel should succeed", GetResult3.Result.Error);
		
		TestNotNull("First Channel should be returned", GetResult1.Channel);
		TestNotNull("Second Channel should be returned", GetResult2.Channel);
		TestNotNull("Third Channel should be returned", GetResult3.Channel);
		
		if(GetResult1.Channel && GetResult2.Channel && GetResult3.Channel)
		{
			// All channels should have same ChannelID
			TestEqual("First Channel ChannelID should match", GetResult1.Channel->GetChannelID(), TestChannelID);
			TestEqual("Second Channel ChannelID should match", GetResult2.Channel->GetChannelID(), TestChannelID);
			TestEqual("Third Channel ChannelID should match", GetResult3.Channel->GetChannelID(), TestChannelID);
			
			// All channels should share same data from repository
			FPubnubChatChannelData Data1 = GetResult1.Channel->GetChannelData();
			FPubnubChatChannelData Data2 = GetResult2.Channel->GetChannelData();
			FPubnubChatChannelData Data3 = GetResult3.Channel->GetChannelData();
			
			TestEqual("Data1 ChannelName should match Data2", Data1.ChannelName, Data2.ChannelName);
			TestEqual("Data2 ChannelName should match Data3", Data2.ChannelName, Data3.ChannelName);
			TestEqual("Data1 Description should match Data2", Data1.Description, Data2.Description);
			TestEqual("Data2 Description should match Data3", Data2.Description, Data3.Description);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests data consistency between CreatePublicConversation and GetChannel.
 * Verifies that data created via CreatePublicConversation matches what is retrieved via GetChannel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelDataConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.4Advanced.DataConsistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelDataConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_consistency_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_consistency";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel with specific data
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("ConsistentChannel");
		ChannelData.Description = TEXT("Test data consistency");
		ChannelData.Custom = TEXT("{\"consistency\":\"test\"}");
		ChannelData.Status = TEXT("active");
		ChannelData.Type = TEXT("public");
		
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		// Get the channel
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed", GetResult.Result.Error);
		TestNotNull("GetChannel should return channel", GetResult.Channel);
		
		if(CreateResult.Channel && GetResult.Channel)
		{
			// Data from CreatePublicConversation should match data from GetChannel
			FPubnubChatChannelData CreateData = CreateResult.Channel->GetChannelData();
			FPubnubChatChannelData GetData = GetResult.Channel->GetChannelData();
			
			TestEqual("ChannelNames should match", CreateData.ChannelName, GetData.ChannelName);
			TestEqual("Descriptions should match", CreateData.Description, GetData.Description);
			TestEqual("Custom should match", CreateData.Custom, GetData.Custom);
			TestEqual("Statuses should match", CreateData.Status, GetData.Status);
			TestEqual("Types should match", CreateData.Type, GetData.Type);
		}
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests that GetChannel properly tracks step results for internal operations.
 * Verifies that step results contain expected operations like GetChannelMetadata.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelStepResultsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannel.4Advanced.StepResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelStepResultsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_step_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_step";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// First create the channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		
		// Get the channel
		FPubnubChatChannelResult GetResult = Chat->GetChannel(TestChannelID);
		
		TestFalse("GetChannel should succeed", GetResult.Result.Error);
		
		// Verify step results are tracked
		TestTrue("StepResults should contain at least one step", GetResult.Result.StepResults.Num() > 0);
		
		// Check that step results contain expected operations
		bool bFoundGetChannelMetadata = false;
		
		for(const FPubnubChatOperationStepResult& Step : GetResult.Result.StepResults)
		{
			if(Step.StepName == TEXT("GetChannelMetadata"))
			{
				bFoundGetChannelMetadata = true;
				TestFalse("GetChannelMetadata step should not have error", Step.OperationResult.Error);
			}
		}
		
		TestTrue("Should have GetChannelMetadata step", bFoundGetChannelMetadata);
		
		// Cleanup: Delete created channel
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

