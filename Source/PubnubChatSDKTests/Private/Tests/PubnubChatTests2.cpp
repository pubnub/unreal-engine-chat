// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "PubnubStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "Misc/Guid.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// MARKALLMESSAGESASREAD TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>(ChatSubsystem);
	
	if(Chat)
	{
		// Try to mark all messages as read without initialized chat
		FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
		
		TestTrue("MarkAllMessagesAsRead should fail when Chat is not initialized", MarkResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", MarkResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Memberships array should be empty", MarkResult.Memberships.Num(), 0);
		TestEqual("Total should be 0", MarkResult.Total, 0);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString UniqueSuffix = TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_happy_init" + UniqueSuffix;
	const FString TestChannelID = SDK_PREFIX + "test_mark_all_read_happy" + UniqueSuffix;
	
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
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join channel to create membership
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Get current timetoken before marking as read
	FString TimetokenBefore = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	// Call MarkAllMessagesAsRead with only required parameters (all defaults)
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
	
	TestFalse("MarkAllMessagesAsRead should succeed", MarkResult.Result.Error);
	TestTrue("Memberships array should be valid", MarkResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", MarkResult.Total >= 0);
	
	// Verify that returned memberships have correctly set lastReadMessageTimetoken
	if(MarkResult.Memberships.Num() > 0)
	{
		FString TimetokenAfter = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		
		for(UPubnubChatMembership* Membership : MarkResult.Memberships)
		{
			if(Membership)
			{
				FString LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
				TestFalse("LastReadMessageTimetoken should not be empty", LastReadTimetoken.IsEmpty());
				
				// Verify timetoken is within reasonable range (between before and after)
				// Convert to int64 for comparison
				if(!TimetokenBefore.IsEmpty() && TimetokenBefore.IsNumeric() &&
				   !TimetokenAfter.IsEmpty() && TimetokenAfter.IsNumeric() &&
				   !LastReadTimetoken.IsEmpty() && LastReadTimetoken.IsNumeric())
				{
					int64 TimetokenBeforeInt = 0;
					int64 TimetokenAfterInt = 0;
					int64 LastReadTimetokenInt = 0;
					
					LexFromString(TimetokenBeforeInt, *TimetokenBefore);
					LexFromString(TimetokenAfterInt, *TimetokenAfter);
					LexFromString(LastReadTimetokenInt, *LastReadTimetoken);
					
					TestTrue("LastReadMessageTimetoken should be >= TimetokenBefore", LastReadTimetokenInt >= TimetokenBeforeInt);
					TestTrue("LastReadMessageTimetoken should be <= TimetokenAfter", LastReadTimetokenInt <= TimetokenAfterInt);
				}
				
				// Verify membership data contains the timetoken in Custom field
				FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
				TestTrue("Custom field should contain lastReadMessageTimetoken", MembershipData.Custom.Contains(Pubnub_Chat_LRMT_Property_Name));
			}
		}
	}
	
	// Cleanup: Leave channel, delete channel
	if(CreateChannelResult.Channel)
	{
		CreateChannelResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_mark_all_read_full";
	
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
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join channel to create membership
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Test MarkAllMessagesAsRead with all parameters
	const int TestLimit = 10;
	const FString TestFilter = TEXT("channel.id == \"") + TestChannelID + TEXT("\"");
	FPubnubMembershipSort TestSort;
	FPubnubMembershipSingleSort SingleSort;
	SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSort.SortOrder = false; // Ascending
	TestSort.MembershipSort.Add(SingleSort);
	FPubnubPage TestPage;
	
	// Get current timetoken before marking as read
	FString TimetokenBefore = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead(TestLimit, TestFilter, TestSort, TestPage);
	
	TestFalse("MarkAllMessagesAsRead should succeed with all parameters", MarkResult.Result.Error);
	TestTrue("Memberships array should be valid", MarkResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", MarkResult.Total >= 0);
	
	// Verify results match filter criteria and have correct lastReadMessageTimetoken
	if(MarkResult.Memberships.Num() > 0)
	{
		FString TimetokenAfter = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		
		for(UPubnubChatMembership* Membership : MarkResult.Memberships)
		{
			if(Membership)
			{
				// Verify filter worked - should only return the test channel
				TestEqual("Filtered membership ChannelID should match filter", Membership->GetChannelID(), TestChannelID);
				TestEqual("Membership UserID should match", Membership->GetUserID(), InitUserID);
				
				// Verify lastReadMessageTimetoken is set correctly
				FString LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
				TestFalse("LastReadMessageTimetoken should not be empty", LastReadTimetoken.IsEmpty());
				
				// Verify timetoken is within reasonable range
				if(!TimetokenBefore.IsEmpty() && TimetokenBefore.IsNumeric() &&
				   !TimetokenAfter.IsEmpty() && TimetokenAfter.IsNumeric() &&
				   !LastReadTimetoken.IsEmpty() && LastReadTimetoken.IsNumeric())
				{
					int64 TimetokenBeforeInt = 0;
					int64 TimetokenAfterInt = 0;
					int64 LastReadTimetokenInt = 0;
					
					LexFromString(TimetokenBeforeInt, *TimetokenBefore);
					LexFromString(TimetokenAfterInt, *TimetokenAfter);
					LexFromString(LastReadTimetokenInt, *LastReadTimetoken);
					
					TestTrue("LastReadMessageTimetoken should be >= TimetokenBefore", LastReadTimetokenInt >= TimetokenBeforeInt);
					TestTrue("LastReadMessageTimetoken should be <= TimetokenAfter", LastReadTimetokenInt <= TimetokenAfterInt);
				}
				
				// Verify membership data contains the timetoken in Custom field
				FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
				TestTrue("Custom field should contain lastReadMessageTimetoken", MembershipData.Custom.Contains(Pubnub_Chat_LRMT_Property_Name));
			}
		}
	}
	
	// Cleanup: Leave channel, delete channel
	if(CreateChannelResult.Channel)
	{
		CreateChannelResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests MarkAllMessagesAsRead with multiple channels.
 * Verifies that all memberships have correctly set lastReadMessageTimetoken.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadMultipleChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.4Advanced.MultipleChannels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadMultipleChannelsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_multi_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_mark_all_read_multi_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_mark_all_read_multi_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_mark_all_read_multi_3";
	
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
	
	// Create first channel
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateChannel1Result = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation should succeed for channel 1", CreateChannel1Result.Result.Error);
	TestNotNull("Channel 1 should be created", CreateChannel1Result.Channel);
	
	// Create second channel
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateChannel2Result = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation should succeed for channel 2", CreateChannel2Result.Result.Error);
	TestNotNull("Channel 2 should be created", CreateChannel2Result.Channel);
	
	// Create third channel
	FPubnubChatChannelData ChannelData3;
	FPubnubChatChannelResult CreateChannel3Result = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
	TestFalse("CreatePublicConversation should succeed for channel 3", CreateChannel3Result.Result.Error);
	TestNotNull("Channel 3 should be created", CreateChannel3Result.Channel);
	
	if(!CreateChannel1Result.Channel || !CreateChannel2Result.Channel || !CreateChannel3Result.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join all channels to create memberships
	FPubnubChatJoinResult Join1Result = CreateChannel1Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 1", Join1Result.Result.Error);
	
	FPubnubChatJoinResult Join2Result = CreateChannel2Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 2", Join2Result.Result.Error);
	
	FPubnubChatJoinResult Join3Result = CreateChannel3Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 3", Join3Result.Result.Error);
	
	// Get current timetoken before marking as read
	FString TimetokenBefore = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	
	// Mark all messages as read
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
	
	TestFalse("MarkAllMessagesAsRead should succeed", MarkResult.Result.Error);
	TestTrue("Should have at least 3 memberships", MarkResult.Memberships.Num() >= 3);
	
	// Verify all memberships have correctly set lastReadMessageTimetoken
	FString TimetokenAfter = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	TArray<FString> FoundChannelIDs;
	
	for(UPubnubChatMembership* Membership : MarkResult.Memberships)
	{
		if(Membership)
		{
			FString ChannelID = Membership->GetChannelID();
			FoundChannelIDs.Add(ChannelID);
			
			// Verify lastReadMessageTimetoken is set correctly
			FString LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
			TestFalse("LastReadMessageTimetoken should not be empty", LastReadTimetoken.IsEmpty());
			
			// Verify timetoken is within reasonable range
			if(!TimetokenBefore.IsEmpty() && TimetokenBefore.IsNumeric() &&
			   !TimetokenAfter.IsEmpty() && TimetokenAfter.IsNumeric() &&
			   !LastReadTimetoken.IsEmpty() && LastReadTimetoken.IsNumeric())
			{
				int64 TimetokenBeforeInt = 0;
				int64 TimetokenAfterInt = 0;
				int64 LastReadTimetokenInt = 0;
				
				LexFromString(TimetokenBeforeInt, *TimetokenBefore);
				LexFromString(TimetokenAfterInt, *TimetokenAfter);
				LexFromString(LastReadTimetokenInt, *LastReadTimetoken);
				
				TestTrue("LastReadMessageTimetoken should be >= TimetokenBefore", LastReadTimetokenInt >= TimetokenBeforeInt);
				TestTrue("LastReadMessageTimetoken should be <= TimetokenAfter", LastReadTimetokenInt <= TimetokenAfterInt);
			}
			
			// Verify membership data contains the timetoken in Custom field
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestTrue("Custom field should contain lastReadMessageTimetoken", MembershipData.Custom.Contains(Pubnub_Chat_LRMT_Property_Name));
			
			// Verify all memberships have the same timetoken (they should all be marked at the same time)
			if(FoundChannelIDs.Num() > 1)
			{
				// Get timetoken from first membership
				UPubnubChatMembership* FirstMembership = MarkResult.Memberships[0];
				if(FirstMembership)
				{
					FString FirstTimetoken = FirstMembership->GetLastReadMessageTimetoken();
					// All timetokens should be the same (or very close) since they're set in the same operation
					TestEqual("All memberships should have the same lastReadMessageTimetoken", LastReadTimetoken, FirstTimetoken);
				}
			}
		}
	}
	
	// Verify all three channels are present
	TestTrue("Should find channel 1", FoundChannelIDs.Contains(TestChannelID1));
	TestTrue("Should find channel 2", FoundChannelIDs.Contains(TestChannelID2));
	TestTrue("Should find channel 3", FoundChannelIDs.Contains(TestChannelID3));
	
	// Cleanup: Leave all channels, delete all channels
	if(CreateChannel1Result.Channel)
	{
		CreateChannel1Result.Channel->Leave();
	}
	if(CreateChannel2Result.Channel)
	{
		CreateChannel2Result.Channel->Leave();
	}
	if(CreateChannel3Result.Channel)
	{
		CreateChannel3Result.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests MarkAllMessagesAsRead with no memberships (empty result).
 * Verifies that function handles empty memberships gracefully.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadNoMembershipsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.4Advanced.NoMemberships", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadNoMembershipsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_no_memberships_init";
	
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
	
	// Don't create any channels or memberships - user has no memberships
	
	// Mark all messages as read (should handle empty memberships gracefully)
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
	
	TestFalse("MarkAllMessagesAsRead should succeed even with no memberships", MarkResult.Result.Error);
	TestEqual("Memberships array should be empty", MarkResult.Memberships.Num(), 0);
	TestEqual("Total should be 0", MarkResult.Total, 0);
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests MarkAllMessagesAsRead preserves existing custom data in memberships.
 * Verifies that custom data is not lost when setting lastReadMessageTimetoken.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadPreservesCustomDataTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.4Advanced.PreservesCustomData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadPreservesCustomDataTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_custom_init";
	const FString TestChannelID = SDK_PREFIX + "test_mark_all_read_custom";
	const FString TestCustomKey = TEXT("testKey");
	const FString TestCustomValue = TEXT("testValue");
	
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
	
	// Create channel
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join channel with custom membership data
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = FString::Printf(TEXT("{\"%s\":\"%s\"}"), *TestCustomKey, *TestCustomValue);
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(MembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Verify initial custom data
	if(JoinResult.Membership)
	{
		FPubnubChatMembershipData InitialData = JoinResult.Membership->GetMembershipData();
		TestTrue("Initial custom data should contain test key", InitialData.Custom.Contains(TestCustomKey));
		TestTrue("Initial custom data should contain test value", InitialData.Custom.Contains(TestCustomValue));
	}
	
	// Mark all messages as read
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
	
	TestFalse("MarkAllMessagesAsRead should succeed", MarkResult.Result.Error);
	TestTrue("Should have at least one membership", MarkResult.Memberships.Num() >= 1);
	
	// Verify custom data is preserved and lastReadMessageTimetoken is added
	if(MarkResult.Memberships.Num() > 0)
	{
		for(UPubnubChatMembership* Membership : MarkResult.Memberships)
		{
			if(Membership && Membership->GetChannelID() == TestChannelID)
			{
				FPubnubChatMembershipData UpdatedData = Membership->GetMembershipData();
				
				// Verify original custom data is preserved
				TestTrue("Custom data should still contain test key", UpdatedData.Custom.Contains(TestCustomKey));
				TestTrue("Custom data should still contain test value", UpdatedData.Custom.Contains(TestCustomValue));
				
				// Verify lastReadMessageTimetoken is added
				TestTrue("Custom data should contain lastReadMessageTimetoken", UpdatedData.Custom.Contains(Pubnub_Chat_LRMT_Property_Name));
				
				// Verify lastReadMessageTimetoken can be retrieved
				FString LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
				TestFalse("LastReadMessageTimetoken should not be empty", LastReadTimetoken.IsEmpty());
			}
		}
	}
	
	// Cleanup: Leave channel, delete channel
	if(CreateChannelResult.Channel)
	{
		CreateChannelResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests MarkAllMessagesAsRead local membership data matches server data.
 * Verifies that membership data returned from MarkAllMessagesAsRead matches what's stored on the server.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadLocalMatchesServerTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsRead.4Advanced.LocalMatchesServer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadLocalMatchesServerTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_local_server_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_mark_all_read_local_server_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_mark_all_read_local_server_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_mark_all_read_local_server_3";
	
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
	
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if(!CurrentUser)
	{
		AddError("CurrentUser should be available");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Create first channel
	FPubnubChatChannelData ChannelData1;
	FPubnubChatChannelResult CreateChannel1Result = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
	TestFalse("CreatePublicConversation should succeed for channel 1", CreateChannel1Result.Result.Error);
	TestNotNull("Channel 1 should be created", CreateChannel1Result.Channel);
	
	// Create second channel
	FPubnubChatChannelData ChannelData2;
	FPubnubChatChannelResult CreateChannel2Result = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
	TestFalse("CreatePublicConversation should succeed for channel 2", CreateChannel2Result.Result.Error);
	TestNotNull("Channel 2 should be created", CreateChannel2Result.Channel);
	
	// Create third channel
	FPubnubChatChannelData ChannelData3;
	FPubnubChatChannelResult CreateChannel3Result = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
	TestFalse("CreatePublicConversation should succeed for channel 3", CreateChannel3Result.Result.Error);
	TestNotNull("Channel 3 should be created", CreateChannel3Result.Channel);
	
	if(!CreateChannel1Result.Channel || !CreateChannel2Result.Channel || !CreateChannel3Result.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join all channels to create memberships
	FPubnubChatJoinResult Join1Result = CreateChannel1Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 1", Join1Result.Result.Error);
	
	FPubnubChatJoinResult Join2Result = CreateChannel2Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 2", Join2Result.Result.Error);
	
	FPubnubChatJoinResult Join3Result = CreateChannel3Result.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed for channel 3", Join3Result.Result.Error);
	
	// Mark all messages as read - get local membership data
	FPubnubChatMarkAllMessagesAsReadResult MarkResult = Chat->MarkAllMessagesAsRead();
	TestFalse("MarkAllMessagesAsRead should succeed", MarkResult.Result.Error);
	TestTrue("Should have at least 3 memberships", MarkResult.Memberships.Num() >= 3);
	
	// Get memberships from server
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships();
	TestFalse("GetMemberships should succeed", GetMembershipsResult.Result.Error);
	TestTrue("Should have at least 3 memberships from server", GetMembershipsResult.Memberships.Num() >= 3);
	
	// Create maps for easy lookup
	TMap<FString, UPubnubChatMembership*> LocalMembershipsMap;
	TMap<FString, UPubnubChatMembership*> ServerMembershipsMap;
	
	for(UPubnubChatMembership* Membership : MarkResult.Memberships)
	{
		if(Membership)
		{
			FString ChannelID = Membership->GetChannelID();
			if(ChannelID == TestChannelID1 || ChannelID == TestChannelID2 || ChannelID == TestChannelID3)
			{
				LocalMembershipsMap.Add(ChannelID, Membership);
			}
		}
	}
	
	for(UPubnubChatMembership* Membership : GetMembershipsResult.Memberships)
	{
		if(Membership)
		{
			FString ChannelID = Membership->GetChannelID();
			if(ChannelID == TestChannelID1 || ChannelID == TestChannelID2 || ChannelID == TestChannelID3)
			{
				ServerMembershipsMap.Add(ChannelID, Membership);
			}
		}
	}
	
	// Verify all test channels are present in both local and server data
	TestTrue("Should have channel 1 in local data", LocalMembershipsMap.Contains(TestChannelID1));
	TestTrue("Should have channel 2 in local data", LocalMembershipsMap.Contains(TestChannelID2));
	TestTrue("Should have channel 3 in local data", LocalMembershipsMap.Contains(TestChannelID3));
	TestTrue("Should have channel 1 in server data", ServerMembershipsMap.Contains(TestChannelID1));
	TestTrue("Should have channel 2 in server data", ServerMembershipsMap.Contains(TestChannelID2));
	TestTrue("Should have channel 3 in server data", ServerMembershipsMap.Contains(TestChannelID3));
	
	// Compare local and server membership data for each channel
	TArray<FString> TestChannelIDs = {TestChannelID1, TestChannelID2, TestChannelID3};
	for(const FString& ChannelID : TestChannelIDs)
	{
		UPubnubChatMembership** LocalMembershipPtr = LocalMembershipsMap.Find(ChannelID);
		UPubnubChatMembership** ServerMembershipPtr = ServerMembershipsMap.Find(ChannelID);
		
		if(!LocalMembershipPtr || !*LocalMembershipPtr)
		{
			AddError(FString::Printf(TEXT("Local membership not found for channel %s"), *ChannelID));
			continue;
		}
		
		if(!ServerMembershipPtr || !*ServerMembershipPtr)
		{
			AddError(FString::Printf(TEXT("Server membership not found for channel %s"), *ChannelID));
			continue;
		}
		
		UPubnubChatMembership* LocalMembership = *LocalMembershipPtr;
		UPubnubChatMembership* ServerMembership = *ServerMembershipPtr;
		
		// Get membership data from both local and server
		FPubnubChatMembershipData LocalMembershipData = LocalMembership->GetMembershipData();
		FPubnubChatMembershipData ServerMembershipData = ServerMembership->GetMembershipData();
		
		// Extract lastReadMessageTimetoken from both
		FString LocalLastReadTimetoken = UPubnubChatInternalUtilities::GetLastReadMessageTimetokenFromMembershipData(LocalMembershipData);
		FString ServerLastReadTimetoken = UPubnubChatInternalUtilities::GetLastReadMessageTimetokenFromMembershipData(ServerMembershipData);
		
		// Verify lastReadMessageTimetoken matches between local and server
		TestEqual(FString::Printf(TEXT("lastReadMessageTimetoken should match for channel %s"), *ChannelID), LocalLastReadTimetoken, ServerLastReadTimetoken);
		TestFalse(FString::Printf(TEXT("lastReadMessageTimetoken should not be empty for channel %s"), *ChannelID), LocalLastReadTimetoken.IsEmpty());
		TestFalse(FString::Printf(TEXT("lastReadMessageTimetoken should not be empty for channel %s"), *ChannelID), ServerLastReadTimetoken.IsEmpty());
		
		// Verify other membership fields match
		TestEqual(FString::Printf(TEXT("Status should match for channel %s"), *ChannelID), LocalMembershipData.Status, ServerMembershipData.Status);
		TestEqual(FString::Printf(TEXT("Type should match for channel %s"), *ChannelID), LocalMembershipData.Type, ServerMembershipData.Type);
		
		// Verify UserID and ChannelID match
		TestEqual(FString::Printf(TEXT("UserID should match for channel %s"), *ChannelID), LocalMembership->GetUserID(), ServerMembership->GetUserID());
		TestEqual(FString::Printf(TEXT("ChannelID should match for channel %s"), *ChannelID), LocalMembership->GetChannelID(), ServerMembership->GetChannelID());
	}
	
	// Cleanup: Leave all channels, delete all channels
	if(CreateChannel1Result.Channel)
	{
		CreateChannel1Result.Channel->Leave();
	}
	if(CreateChannel2Result.Channel)
	{
		CreateChannel2Result.Channel->Leave();
	}
	if(CreateChannel3Result.Channel)
	{
		CreateChannel3Result.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Messages)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMarkAllMessagesAsReadAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Messages.MarkAllMessagesAsReadAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMarkAllMessagesAsReadAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_mark_all_read_async_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_mark_all_read_async_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUp();
		return false;
	}
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	if(!CreateChannelResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatJoinResult JoinResult = CreateChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	const int TestLimit = 10;
	const FString TestFilter = TEXT("channel.id == \"") + TestChannelID + TEXT("\"");
	FPubnubMembershipSort TestSort;
	FPubnubMembershipSingleSort SingleSort;
	SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
	SingleSort.SortOrder = false;
	TestSort.MembershipSort.Add(SingleSort);
	FPubnubPage TestPage;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatMarkAllMessagesAsReadResult> CallbackResult = MakeShared<FPubnubChatMarkAllMessagesAsReadResult>();
	FOnPubnubChatMarkAllMessagesAsReadResponseNative OnMarkAllResponse;
	OnMarkAllResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatMarkAllMessagesAsReadResult& MarkAllResult)
	{
		*CallbackResult = MarkAllResult;
		*bCallbackReceived = true;
	});
	
	Chat->MarkAllMessagesAsReadAsync(OnMarkAllResponse, TestLimit, TestFilter, TestSort, TestPage);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult]()
	{
		TestFalse("MarkAllMessagesAsReadAsync should succeed", CallbackResult->Result.Error);
		TestTrue("Memberships array should be valid", CallbackResult->Memberships.Num() >= 0);
		TestTrue("Total should be non-negative", CallbackResult->Total >= 0);
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID]()
	{
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Leave();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

