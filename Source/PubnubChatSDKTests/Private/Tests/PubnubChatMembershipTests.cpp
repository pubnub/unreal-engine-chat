// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMembership.h"
#include "PubnubChatMessage.h"
#include "PubnubChatCallbackStop.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// UPDATE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipUpdateNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.Update.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipUpdateNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create membership without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMembership* Membership = NewObject<UPubnubChatMembership>(Chat);
			if(Membership)
			{
				FPubnubChatMembershipData MembershipData;
				FPubnubChatOperationResult UpdateResult = Membership->Update(MembershipData);
				
				TestTrue("Update should fail when Membership is not initialized", UpdateResult.Error);
				TestFalse("ErrorMessage should not be empty", UpdateResult.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipUpdateHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.Update.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipUpdateHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_update_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_update_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Update with default MembershipData (required parameter)
				FPubnubChatMembershipData UpdateMembershipData; // Default empty struct
				FPubnubChatOperationResult UpdateResult = JoinResult.Membership->Update(UpdateMembershipData);
				
				TestFalse("Update should succeed", UpdateResult.Error);
				
				// Verify step results contain SetMemberships step
				bool bFoundSetMemberships = false;
				for(const FPubnubChatOperationStepResult& Step : UpdateResult.StepResults)
				{
					if(Step.StepName == TEXT("SetMemberships"))
					{
						bFoundSetMemberships = true;
						TestFalse("SetMemberships step should not have error", Step.OperationResult.Error);
					}
				}
				TestTrue("Should have SetMemberships step", bFoundSetMemberships);
				
				// Verify membership data was updated in repository
				FPubnubChatMembershipData RetrievedData = JoinResult.Membership->GetMembershipData();
				TestEqual("Retrieved Custom should match updated Custom", RetrievedData.Custom, UpdateMembershipData.Custom);
				TestEqual("Retrieved Status should match updated Status", RetrievedData.Status, UpdateMembershipData.Status);
				TestEqual("Retrieved Type should match updated Type", RetrievedData.Type, UpdateMembershipData.Type);
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipUpdateFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.Update.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipUpdateFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_update_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_update_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Update with all MembershipData parameters set
				FPubnubChatMembershipData UpdateMembershipData;
				UpdateMembershipData.Custom = TEXT("{\"role\":\"admin\",\"level\":5}");
				UpdateMembershipData.Status = TEXT("active");
				UpdateMembershipData.Type = TEXT("member");
				
				FPubnubChatOperationResult UpdateResult = JoinResult.Membership->Update(UpdateMembershipData);
				
				TestFalse("Update should succeed with all parameters", UpdateResult.Error);
				
				// Verify step results contain SetMemberships step
				bool bFoundSetMemberships = false;
				for(const FPubnubChatOperationStepResult& Step : UpdateResult.StepResults)
				{
					if(Step.StepName == TEXT("SetMemberships"))
					{
						bFoundSetMemberships = true;
						TestFalse("SetMemberships step should not have error", Step.OperationResult.Error);
					}
				}
				TestTrue("Should have SetMemberships step", bFoundSetMemberships);
				
				// Verify all membership data was updated correctly in repository
				FPubnubChatMembershipData RetrievedData = JoinResult.Membership->GetMembershipData();
				TestEqual("Retrieved Custom should match updated Custom", RetrievedData.Custom, UpdateMembershipData.Custom);
				TestEqual("Retrieved Status should match updated Status", RetrievedData.Status, UpdateMembershipData.Status);
				TestEqual("Retrieved Type should match updated Type", RetrievedData.Type, UpdateMembershipData.Type);
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
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
 * Tests updating membership multiple times.
 * Verifies that multiple updates can be called and each updates the repository correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipUpdateMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.Update.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipUpdateMultipleTimesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_update_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_update_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// First update
				FPubnubChatMembershipData UpdateData1;
				UpdateData1.Custom = TEXT("{\"step\":1}");
				UpdateData1.Status = TEXT("active");
				UpdateData1.Type = TEXT("member");
				
				FPubnubChatOperationResult UpdateResult1 = JoinResult.Membership->Update(UpdateData1);
				TestFalse("First Update should succeed", UpdateResult1.Error);
				
				// Second update
				FPubnubChatMembershipData UpdateData2;
				UpdateData2.Custom = TEXT("{\"step\":2}");
				UpdateData2.Status = TEXT("inactive");
				UpdateData2.Type = TEXT("moderator");
				
				FPubnubChatOperationResult UpdateResult2 = JoinResult.Membership->Update(UpdateData2);
				TestFalse("Second Update should succeed", UpdateResult2.Error);
				
				// Verify final state matches second update
				FPubnubChatMembershipData FinalData = JoinResult.Membership->GetMembershipData();
				TestEqual("Final Custom should match second update", FinalData.Custom, UpdateData2.Custom);
				TestEqual("Final Status should match second update", FinalData.Status, UpdateData2.Status);
				TestEqual("Final Type should match second update", FinalData.Type, UpdateData2.Type);
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// SETLASTREADMESSAGETIMETOKEN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create membership without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMembership* Membership = NewObject<UPubnubChatMembership>(Chat);
			if(Membership)
			{
				const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
				FPubnubChatOperationResult SetResult = Membership->SetLastReadMessageTimetoken(TestTimetoken);
				
				TestTrue("SetLastReadMessageTimetoken should fail when Membership is not initialized", SetResult.Error);
				TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenEmptyTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.1Validation.EmptyTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenEmptyTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Try to set empty timetoken
				FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TEXT(""));
				
				TestTrue("SetLastReadMessageTimetoken should fail with empty Timetoken", SetResult.Error);
				TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Set last read message timetoken with required parameter (Timetoken)
				const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
				FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
				
				TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
				
				// Verify step results contain SetMemberships step (from Update call)
				bool bFoundSetMemberships = false;
				for(const FPubnubChatOperationStepResult& Step : SetResult.StepResults)
				{
					if(Step.StepName == TEXT("SetMemberships"))
					{
						bFoundSetMemberships = true;
						TestFalse("SetMemberships step should not have error", Step.OperationResult.Error);
					}
				}
				TestTrue("Should have SetMemberships step", bFoundSetMemberships);
				
				// For public channels, no receipt event should be emitted
				// Verify membership data was updated in repository (timetoken should be in custom data)
				FPubnubChatMembershipData RetrievedData = JoinResult.Membership->GetMembershipData();
				TestFalse("Retrieved Custom should not be empty (should contain timetoken)", RetrievedData.Custom.IsEmpty());
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership with initial data
			FOnPubnubChatChannelMessageReceived MessageCallback;
			FPubnubChatMembershipData InitialMembershipData;
			InitialMembershipData.Custom = TEXT("{\"role\":\"user\"}");
			InitialMembershipData.Status = TEXT("active");
			InitialMembershipData.Type = TEXT("member");
			
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Set last read message timetoken (only takes Timetoken parameter)
				const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
				FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
				
				TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
				
				// Verify step results contain SetMemberships step
				bool bFoundSetMemberships = false;
				for(const FPubnubChatOperationStepResult& Step : SetResult.StepResults)
				{
					if(Step.StepName == TEXT("SetMemberships"))
					{
						bFoundSetMemberships = true;
						TestFalse("SetMemberships step should not have error", Step.OperationResult.Error);
					}
				}
				TestTrue("Should have SetMemberships step", bFoundSetMemberships);
				
				// Verify membership data was updated (timetoken should be added to custom data)
				FPubnubChatMembershipData RetrievedData = JoinResult.Membership->GetMembershipData();
				TestFalse("Retrieved Custom should not be empty (should contain timetoken)", RetrievedData.Custom.IsEmpty());
				// Original custom data should be preserved along with timetoken
				TestTrue("Retrieved Custom should contain original role", RetrievedData.Custom.Contains(TEXT("role")));
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
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
 * Tests setting last read message timetoken multiple times.
 * Verifies that multiple calls update the timetoken correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.4Advanced.MultipleCalls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenMultipleTimesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Set timetoken first time
				const FString Timetoken1 = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
				FPubnubChatOperationResult SetResult1 = JoinResult.Membership->SetLastReadMessageTimetoken(Timetoken1);
				TestFalse("First SetLastReadMessageTimetoken should succeed", SetResult1.Error);
				
				// Wait a bit to ensure different timetoken
				FPlatformProcess::Sleep(0.1f);
				
				// Set timetoken second time
				const FString Timetoken2 = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
				FPubnubChatOperationResult SetResult2 = JoinResult.Membership->SetLastReadMessageTimetoken(Timetoken2);
				TestFalse("Second SetLastReadMessageTimetoken should succeed", SetResult2.Error);
				
				// Verify final state contains the second timetoken
				FPubnubChatMembershipData FinalData = JoinResult.Membership->GetMembershipData();
				TestFalse("Final Custom should not be empty", FinalData.Custom.IsEmpty());
				TestTrue("Final Custom should contain second timetoken", FinalData.Custom.Contains(Timetoken2));
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that SetLastReadMessageTimetoken properly updates membership data in repository.
 * Verifies that the timetoken is correctly stored and can be retrieved.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenRepositoryUpdateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.4Advanced.RepositoryUpdate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMembershipSetLastReadMessageTimetokenRepositoryUpdateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_repo_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_repo";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel first
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Join to create membership
			FPubnubChatMembershipData InitialMembershipData;
			InitialMembershipData.Custom = TEXT("{\"test\":\"data\"}");
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
			TestFalse("Join should succeed", JoinResult.Result.Error);
			TestNotNull("Membership should be created", JoinResult.Membership);
			
			if(JoinResult.Membership)
			{
				// Get initial membership data
				FPubnubChatMembershipData InitialData = JoinResult.Membership->GetMembershipData();
				
				// Set last read message timetoken
				const FString TestTimetoken = TEXT("12345678901234567"); // Test timetoken
				FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
				TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
				
				// Verify membership data was updated in repository
				FPubnubChatMembershipData UpdatedData = JoinResult.Membership->GetMembershipData();
				TestFalse("Updated Custom should not be empty", UpdatedData.Custom.IsEmpty());
				TestTrue("Updated Custom should contain timetoken", UpdatedData.Custom.Contains(TestTimetoken));
				// Original custom data should be preserved
				TestTrue("Updated Custom should contain original test data", UpdatedData.Custom.Contains(TEXT("test")));
			}
			
			// Cleanup: Leave and delete channel
			if(CreateResult.Channel)
			{
				CreateResult.Channel->Leave();
			}
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that SetLastReadMessageTimetoken does NOT emit Receipt events for public channels.
 * Verifies that public channels skip event emission as per specification.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenPublicChannelNoEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.4Advanced.PublicChannelNoEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMembershipSetLastReadMessageTimetokenPublicChannelNoEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_public_no_event_init";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_public_no_event";
	
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
	
	// Create public channel
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
	
	// Verify channel is public
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be public", CreatedChannelData.Type, TEXT("public"));
	
	// Join to create membership
	FPubnubChatMembershipData InitialMembershipData;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(InitialMembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	TestNotNull("Membership should be created", JoinResult.Membership);
	
	if(!JoinResult.Membership)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	
	// Listen for Receipt events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		AddError("Receipt event should NOT be received for public channels");
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, EPubnubChatEventType::PCET_Receipt, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, JoinResult]()
	{
		const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatOperationResult SetResult = JoinResult.Membership->SetLastReadMessageTimetoken(TestTimetoken);
		TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
		
		// Verify no EmitChatEvent step for public channels
		bool bFoundEmitChatEvent = false;
		for(const FPubnubChatOperationStepResult& Step : SetResult.StepResults)
		{
			if(Step.StepName == TEXT("EmitChatEvent"))
			{
				bFoundEmitChatEvent = true;
			}
		}
		TestFalse("Should NOT have EmitChatEvent step for public channels", bFoundEmitChatEvent);
	}, 0.5f));
	
	// Wait a bit to ensure event would have been received if it was sent
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([]() -> bool {
		return false; // Never complete, just wait for timeout
	}, 2.0f));
	
	// Verify event was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived]()
	{
		if(*bEventReceived)
		{
			AddError("Receipt event should NOT have been received for public channel");
		}
		else
		{
			TestTrue("Receipt event was correctly NOT received for public channel", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, leave and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, CreateResult, Chat, TestChannelID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
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
	}, 0.1f));
	
	return true;
}

/**
 * Tests that SetLastReadMessageTimetoken DOES emit Receipt events for non-public channels and they can be received.
 * Verifies that group/direct channels emit Receipt events when SetLastReadMessageTimetoken is called.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMembershipSetLastReadMessageTimetokenNonPublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Membership.SetLastReadMessageTimetoken.4Advanced.NonPublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMembershipSetLastReadMessageTimetokenNonPublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_membership_set_lrmt_nonpublic_event_init";
	const FString SecondUserID = SDK_PREFIX + "test_membership_set_lrmt_nonpublic_event_user2";
	const FString TestChannelID = SDK_PREFIX + "test_membership_set_lrmt_nonpublic_event";
	
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
	
	// Create second user for direct conversation
	FPubnubChatUserData SecondUserData;
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(SecondUserID, SecondUserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("Second user should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create direct conversation (non-public channel)
	FPubnubChatChannelData ChannelData;
	FPubnubChatMembershipData HostMembershipData;
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, HostMembershipData);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("Host membership should be created", CreateResult.HostMembership);
	
	if(!CreateResult.Channel || !CreateResult.HostMembership)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify channel is direct (non-public)
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be direct", CreatedChannelData.Type, TEXT("direct"));
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	// Listen for Receipt events
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, TestChannelID, TestTimetoken](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received event type should be Receipt", Event.Type, EPubnubChatEventType::PCET_Receipt);
		TestEqual("Received event ChannelID should match", Event.ChannelID, TestChannelID);
		TestFalse("Received event Payload should not be empty", Event.Payload.IsEmpty());
		TestTrue("Received event Payload should contain timetoken", Event.Payload.Contains(TestTimetoken));
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TestChannelID, EPubnubChatEventType::PCET_Receipt, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestTimetoken]()
	{
		// Set last read message timetoken - should emit Receipt event for non-public channel
		FPubnubChatOperationResult SetResult = CreateResult.HostMembership->SetLastReadMessageTimetoken(TestTimetoken);
		TestFalse("SetLastReadMessageTimetoken should succeed", SetResult.Error);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestTimetoken]()
	{
		if(!*bEventReceived)
		{
			AddError("Receipt event was not received for non-public channel");
		}
		else
		{
			TestEqual("Received event type should be Receipt", ReceivedEvent->Type, EPubnubChatEventType::PCET_Receipt);
			TestTrue("Received event Payload should contain timetoken", ReceivedEvent->Payload.Contains(TestTimetoken));
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, delete users and channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, Chat, TestChannelID, InitUserID, SecondUserID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(InitUserID, false);
			Chat->DeleteUser(SecondUserID, false);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
