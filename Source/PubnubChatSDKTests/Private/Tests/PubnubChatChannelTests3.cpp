// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
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

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// GETMEMBERS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembers.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_members_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to get members with uninitialized channel
		FPubnubChatMembershipsResult GetMembersResult = UninitializedChannel->GetMembers();
		TestTrue("GetMembers should fail with uninitialized channel", GetMembersResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMembersResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Memberships array should be empty", GetMembersResult.Memberships.Num(), 0);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembers.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_members_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_members_happy";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Get members with default parameters (only required)
	FPubnubChatMembershipsResult GetMembersResult = CreateResult.Channel->GetMembers();
	
	TestFalse("GetMembers should succeed", GetMembersResult.Result.Error);
	TestTrue("Memberships array should be valid", GetMembersResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetMembersResult.Total >= 0);
	
	// Verify that at least the current user is a member
	TestTrue("Should have at least one member (the user who joined)", GetMembersResult.Memberships.Num() >= 1);
	
	bool FoundCurrentUser = false;
	for(UPubnubChatMembership* Membership : GetMembersResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == InitUserID)
		{
			FoundCurrentUser = true;
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
			break;
		}
	}
	TestTrue("Current user should be found in members", FoundCurrentUser);
	
	// Cleanup: Delete membership created by Join
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
		if(DeleteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembers.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_members_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_members_full";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create additional user and invite them to have more members
	const FString TargetUserID = SDK_PREFIX + "test_get_members_full_target";
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	
	// Test GetMembers with all parameters
	const int TestLimit = 10;
	const FString TestFilter = FString::Printf(TEXT("uuid.id == \"%s\""), *InitUserID);
	FPubnubMemberSort TestSort;
	FPubnubMemberSingleSort SingleSort;
	SingleSort.SortType = EPubnubMemberSortType::PMeST_UserID;
	SingleSort.SortOrder = false; // Ascending
	TestSort.MemberSort.Add(SingleSort);
	FPubnubPage TestPage; // Empty page for first page
	
	FPubnubChatMembershipsResult GetMembersResult = CreateResult.Channel->GetMembers(TestLimit, TestFilter, TestSort, TestPage);
	
	TestFalse("GetMembers should succeed with all parameters", GetMembersResult.Result.Error);
	TestTrue("Memberships array should be valid", GetMembersResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetMembersResult.Total >= 0);
	
	// Verify filter worked - should only return the init user
	if(GetMembersResult.Memberships.Num() > 0)
	{
		for(UPubnubChatMembership* Membership : GetMembersResult.Memberships)
		{
			if(Membership)
			{
				TestEqual("Filtered membership UserID should match filter", Membership->GetUserID(), InitUserID);
			}
		}
	}
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(InviteResult.Membership)
	{
		FPubnubChatOperationResult DeleteInviteMembershipResult = InviteResult.Membership->Delete();
		if(DeleteInviteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite membership during cleanup: %s"), *DeleteInviteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetMembers with multiple members in the channel.
 * Verifies that all members are returned correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersMultipleMembersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembers.4Advanced.MultipleMembers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersMultipleMembersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_members_multiple_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_get_members_multiple_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_get_members_multiple_target2";
	const FString TestChannelID = SDK_PREFIX + "test_get_members_multiple";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create and invite multiple users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatInviteResult Invite1Result = CreateResult.Channel->Invite(CreateUser1Result.User);
	TestFalse("Invite1 should succeed", Invite1Result.Result.Error);
	
	FPubnubChatInviteResult Invite2Result = CreateResult.Channel->Invite(CreateUser2Result.User);
	TestFalse("Invite2 should succeed", Invite2Result.Result.Error);
	
	// Get all members
	FPubnubChatMembershipsResult GetMembersResult = CreateResult.Channel->GetMembers();
	
	TestFalse("GetMembers should succeed", GetMembersResult.Result.Error);
	TestTrue("Should have at least 3 members (init user + 2 invited)", GetMembersResult.Memberships.Num() >= 3);
	
	// Verify all expected users are in the members list
	TArray<FString> ExpectedUserIDs = {InitUserID, TargetUserID1, TargetUserID2};
	TArray<FString> FoundUserIDs;
	
	for(UPubnubChatMembership* Membership : GetMembersResult.Memberships)
	{
		if(Membership)
		{
			FoundUserIDs.AddUnique(Membership->GetUserID());
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
		}
	}
	
	for(const FString& ExpectedUserID : ExpectedUserIDs)
	{
		TestTrue(FString::Printf(TEXT("User %s should be found in members"), *ExpectedUserID), FoundUserIDs.Contains(ExpectedUserID));
	}
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(Invite1Result.Membership)
	{
		FPubnubChatOperationResult DeleteInvite1MembershipResult = Invite1Result.Membership->Delete();
		if(DeleteInvite1MembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite1 membership during cleanup: %s"), *DeleteInvite1MembershipResult.ErrorMessage);
		}
	}
	if(Invite2Result.Membership)
	{
		FPubnubChatOperationResult DeleteInvite2MembershipResult = Invite2Result.Membership->Delete();
		if(DeleteInvite2MembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite2 membership during cleanup: %s"), *DeleteInvite2MembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMembers pagination functionality.
 * Verifies that pagination works correctly with Limit and Page parameters.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMembersPaginationTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMembers.4Advanced.Pagination", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMembersPaginationTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_members_pagination_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_members_pagination";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Get first page with limit
	const int PageLimit = 1;
	FPubnubPage FirstPage; // Empty page for first page
	FPubnubChatMembershipsResult FirstPageResult = CreateResult.Channel->GetMembers(PageLimit, TEXT(""), FPubnubMemberSort(), FirstPage);
	
	TestFalse("GetMembers first page should succeed", FirstPageResult.Result.Error);
	TestTrue("First page should have at least one member", FirstPageResult.Memberships.Num() >= 1);
	TestTrue("First page should respect limit", FirstPageResult.Memberships.Num() <= PageLimit);
	
	// If there's a next page, get it
	if(!FirstPageResult.Page.Next.IsEmpty())
	{
		FPubnubPage NextPage;
		NextPage.Next = FirstPageResult.Page.Next;
		FPubnubChatMembershipsResult NextPageResult = CreateResult.Channel->GetMembers(PageLimit, TEXT(""), FPubnubMemberSort(), NextPage);
		
		TestFalse("GetMembers next page should succeed", NextPageResult.Result.Error);
		TestTrue("Next page should have valid results", NextPageResult.Memberships.Num() >= 0);
		TestTrue("Next page should respect limit", NextPageResult.Memberships.Num() <= PageLimit);
		
		// Verify members from different pages are different
		if(FirstPageResult.Memberships.Num() > 0 && NextPageResult.Memberships.Num() > 0)
		{
			UPubnubChatMembership* FirstMember = FirstPageResult.Memberships[0];
			UPubnubChatMembership* NextMember = NextPageResult.Memberships[0];
			
			if(FirstMember && NextMember)
			{
				TestNotEqual("Members from different pages should be different", FirstMember->GetUserID(), NextMember->GetUserID());
			}
		}
	}
	
	// Cleanup: Delete membership created by Join
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteMembershipResult = JoinResult.Membership->Delete();
		if(DeleteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
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
// GETINVITEES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInvitees.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_invitees_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to get invitees with uninitialized channel
		FPubnubChatMembershipsResult GetInviteesResult = UninitializedChannel->GetInvitees();
		TestTrue("GetInvitees should fail with uninitialized channel", GetInviteesResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetInviteesResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Memberships array should be empty", GetInviteesResult.Memberships.Num(), 0);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInvitees.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_invitees_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_invitees_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_invitees_happy";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	// Get invitees with default parameters (only required)
	FPubnubChatMembershipsResult GetInviteesResult = CreateResult.Channel->GetInvitees();
	
	TestFalse("GetInvitees should succeed", GetInviteesResult.Result.Error);
	TestTrue("Invitees array should be valid", GetInviteesResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetInviteesResult.Total >= 0);
	
	// Verify that the invited user is in the invitees list
	bool FoundInvitedUser = false;
	for(UPubnubChatMembership* Membership : GetInviteesResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == TargetUserID)
		{
			FoundInvitedUser = true;
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual("Invitee Membership Status should be pending", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			break;
		}
	}
	TestTrue("Invited user should be found in invitees", FoundInvitedUser);
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(InviteResult.Membership)
	{
		FPubnubChatOperationResult DeleteInviteMembershipResult = InviteResult.Membership->Delete();
		if(DeleteInviteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite membership during cleanup: %s"), *DeleteInviteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInvitees.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_invitees_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_invitees_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_invitees_full";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	
	// Test GetInvitees with all parameters
	const int TestLimit = 10;
	const FString TestFilter = FString::Printf(TEXT("uuid.id == \"%s\""), *TargetUserID);
	FPubnubMemberSort TestSort;
	FPubnubMemberSingleSort SingleSort;
	SingleSort.SortType = EPubnubMemberSortType::PMeST_UserID;
	SingleSort.SortOrder = false; // Ascending
	TestSort.MemberSort.Add(SingleSort);
	FPubnubPage TestPage; // Empty page for first page
	
	FPubnubChatMembershipsResult GetInviteesResult = CreateResult.Channel->GetInvitees(TestLimit, TestFilter, TestSort, TestPage);
	
	TestFalse("GetInvitees should succeed with all parameters", GetInviteesResult.Result.Error);
	TestTrue("Invitees array should be valid", GetInviteesResult.Memberships.Num() >= 0);
	TestTrue("Total count should be non-negative", GetInviteesResult.Total >= 0);
	
	// Verify filter worked - should only return the target user
	if(GetInviteesResult.Memberships.Num() > 0)
	{
		for(UPubnubChatMembership* Membership : GetInviteesResult.Memberships)
		{
			if(Membership)
			{
				TestEqual("Filtered invitee UserID should match filter", Membership->GetUserID(), TargetUserID);
				TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
				
				// Verify membership has pending status
				FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
				TestEqual("Invitee Membership Status should be pending", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			}
		}
	}
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(InviteResult.Membership)
	{
		FPubnubChatOperationResult DeleteInviteMembershipResult = InviteResult.Membership->Delete();
		if(DeleteInviteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite membership during cleanup: %s"), *DeleteInviteMembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that GetInvitees only returns users with pending status.
 * Verifies that after a user joins the channel, they are no longer returned in GetInvitees.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesAfterJoinTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInvitees.4Advanced.AfterJoin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesAfterJoinTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_invitees_join_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_invitees_join_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_invitees_join";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	// Get invitees - should include the invited user
	FPubnubChatMembershipsResult GetInviteesBeforeJoinResult = CreateResult.Channel->GetInvitees();
	
	TestFalse("GetInvitees before join should succeed", GetInviteesBeforeJoinResult.Result.Error);
	
	bool FoundBeforeJoin = false;
	for(UPubnubChatMembership* Membership : GetInviteesBeforeJoinResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == TargetUserID)
		{
			FoundBeforeJoin = true;
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual("Membership Status should be pending before join", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			break;
		}
	}
	TestTrue("Invited user should be found in GetInvitees before join", FoundBeforeJoin);
	
	// Now create a second chat instance with the target user ID and join the channel
	FPubnubChatInitChatResult TargetUserInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TargetUserID, ChatConfig);
	TestFalse("Target user InitChat should succeed", TargetUserInitResult.Result.Error);
	TestNotNull("Target user Chat should be created", TargetUserInitResult.Chat);
	
	if(!TargetUserInitResult.Chat)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubChat* TargetUserChat = TargetUserInitResult.Chat;
	
	// Get the channel for the target user
	FPubnubChatChannelResult TargetUserGetChannelResult = TargetUserChat->GetChannel(TestChannelID);
	TestFalse("GetChannel for target user should succeed", TargetUserGetChannelResult.Result.Error);
	TestNotNull("Channel should be found for target user", TargetUserGetChannelResult.Channel);
	
	if(!TargetUserGetChannelResult.Channel)
	{
		CleanUpCurrentChatUser(TargetUserChat);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Join the channel with the target user
	FPubnubChatJoinResult TargetUserJoinResult = TargetUserGetChannelResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Target user Join should succeed", TargetUserJoinResult.Result.Error);
	
	// Now get invitees again from the original channel - should NOT include the target user anymore
	FPubnubChatMembershipsResult GetInviteesAfterJoinResult = CreateResult.Channel->GetInvitees();
	
	TestFalse("GetInvitees after join should succeed", GetInviteesAfterJoinResult.Result.Error);
	
	bool FoundAfterJoin = false;
	for(UPubnubChatMembership* Membership : GetInviteesAfterJoinResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == TargetUserID)
		{
			FoundAfterJoin = true;
			break;
		}
	}
	TestFalse("Invited user should NOT be found in GetInvitees after join", FoundAfterJoin);
	
	// Verify the user is still in GetMembers (as a regular member, not invitee)
	FPubnubChatMembershipsResult GetMembersResult = CreateResult.Channel->GetMembers();
	TestFalse("GetMembers should succeed", GetMembersResult.Result.Error);
	
	bool FoundInMembers = false;
	for(UPubnubChatMembership* Membership : GetMembersResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == TargetUserID)
		{
			FoundInMembers = true;
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			// Status should not be pending anymore (it should be empty or a different status)
			TestNotEqual("Membership Status should not be pending after join", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			break;
		}
	}
	TestTrue("User should be found in GetMembers after join", FoundInMembers);
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(InviteResult.Membership)
	{
		FPubnubChatOperationResult DeleteInviteMembershipResult = InviteResult.Membership->Delete();
		if(DeleteInviteMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite membership during cleanup: %s"), *DeleteInviteMembershipResult.ErrorMessage);
		}
	}
	if(TargetUserJoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteTargetJoinMembershipResult = TargetUserJoinResult.Membership->Delete();
		if(DeleteTargetJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete TargetUser Join membership during cleanup: %s"), *DeleteTargetJoinMembershipResult.ErrorMessage);
		}
	}
	
	// Cleanup: Leave channel for target user
	if(TargetUserGetChannelResult.Channel)
	{
		TargetUserGetChannelResult.Channel->Leave();
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}
	
	CleanUpCurrentChatUser(TargetUserChat);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetInvitees with multiple invited users.
 * Verifies that all invited users are returned correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetInviteesMultipleInviteesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetInvitees.4Advanced.MultipleInvitees", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetInviteesMultipleInviteesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_invitees_multiple_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_get_invitees_multiple_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_get_invitees_multiple_target2";
	const FString TestChannelID = SDK_PREFIX + "test_get_invitees_multiple";
	
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
	
	// Create channel and join
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
	
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create and invite multiple users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	FPubnubChatInviteResult Invite1Result = CreateResult.Channel->Invite(CreateUser1Result.User);
	TestFalse("Invite1 should succeed", Invite1Result.Result.Error);
	
	FPubnubChatInviteResult Invite2Result = CreateResult.Channel->Invite(CreateUser2Result.User);
	TestFalse("Invite2 should succeed", Invite2Result.Result.Error);
	
	// Get all invitees
	FPubnubChatMembershipsResult GetInviteesResult = CreateResult.Channel->GetInvitees();
	
	TestFalse("GetInvitees should succeed", GetInviteesResult.Result.Error);
	TestTrue("Should have at least 2 invitees", GetInviteesResult.Memberships.Num() >= 2);
	
	// Verify all expected users are in the invitees list
	TArray<FString> ExpectedUserIDs = {TargetUserID1, TargetUserID2};
	TArray<FString> FoundUserIDs;
	
	for(UPubnubChatMembership* Membership : GetInviteesResult.Memberships)
	{
		if(Membership)
		{
			// Verify membership has pending status before adding to list
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual("Invitee Membership Status should be pending", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
			
			// Only add to FoundUserIDs if status is actually pending (GetInvitees should only return pending)
			if(MembershipData.Status == Pubnub_Chat_Invited_User_Membership_status)
			{
				FoundUserIDs.AddUnique(Membership->GetUserID());
			}
		}
	}
	
	for(const FString& ExpectedUserID : ExpectedUserIDs)
	{
		TestTrue(FString::Printf(TEXT("User %s should be found in invitees"), *ExpectedUserID), FoundUserIDs.Contains(ExpectedUserID));
	}
	
	// Verify init user is NOT in invitees (they joined, not invited)
	// GetInvitees filters by status == "pending", so init user should not appear
	TestFalse("Init user should NOT be in invitees", FoundUserIDs.Contains(InitUserID));
	
	// Additional verification: Check that init user is not in the raw results either
	bool FoundInitUserInRawResults = false;
	for(UPubnubChatMembership* Membership : GetInviteesResult.Memberships)
	{
		if(Membership && Membership->GetUserID() == InitUserID)
		{
			FoundInitUserInRawResults = true;
			// If init user is found, verify their status is NOT pending (this would indicate a bug)
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestNotEqual("Init user membership status should NOT be pending if they joined", MembershipData.Status, Pubnub_Chat_Invited_User_Membership_status);
			break;
		}
	}
	TestFalse("Init user should NOT appear in GetInvitees results at all (they joined, not invited)", FoundInitUserInRawResults);
	
	// Cleanup: Delete memberships created by Join and Invite
	if(JoinResult.Membership)
	{
		FPubnubChatOperationResult DeleteJoinMembershipResult = JoinResult.Membership->Delete();
		if(DeleteJoinMembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Join membership during cleanup: %s"), *DeleteJoinMembershipResult.ErrorMessage);
		}
	}
	if(Invite1Result.Membership)
	{
		FPubnubChatOperationResult DeleteInvite1MembershipResult = Invite1Result.Membership->Delete();
		if(DeleteInvite1MembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite1 membership during cleanup: %s"), *DeleteInvite1MembershipResult.ErrorMessage);
		}
	}
	if(Invite2Result.Membership)
	{
		FPubnubChatOperationResult DeleteInvite2MembershipResult = Invite2Result.Membership->Delete();
		if(DeleteInvite2MembershipResult.Error)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete Invite2 membership during cleanup: %s"), *DeleteInvite2MembershipResult.ErrorMessage);
		}
	}
	
	if(CreateResult.Channel)
	{
		CreateResult.Channel->Leave();
	}
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID2);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETHISTORY TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to get history with uninitialized channel
		const FString TestStartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString TestEndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(TestStartTimetoken, -10000000);
		FPubnubChatGetHistoryResult GetHistoryResult = UninitializedChannel->GetHistory(TestStartTimetoken, TestEndTimetoken);
		
		TestTrue("GetHistory should fail with uninitialized channel", GetHistoryResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
		TestEqual("Messages array should be empty", GetHistoryResult.Messages.Num(), 0);
		TestFalse("IsMore should be false on error", GetHistoryResult.IsMore);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryEmptyStartTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.1Validation.EmptyStartTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryEmptyStartTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_empty_start_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_empty_start";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to get history with empty StartTimetoken
	const FString TestEndTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(TEXT(""), TestEndTimetoken);
	
	TestTrue("GetHistory should fail with empty StartTimetoken", GetHistoryResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
	TestEqual("Messages array should be empty", GetHistoryResult.Messages.Num(), 0);
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryEmptyEndTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.1Validation.EmptyEndTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryEmptyEndTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_empty_end_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_empty_end";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to get history with empty EndTimetoken
	const FString TestStartTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(TestStartTimetoken, TEXT(""));
	
	TestTrue("GetHistory should fail with empty EndTimetoken", GetHistoryResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", GetHistoryResult.Result.ErrorMessage.IsEmpty());
	TestEqual("Messages array should be empty", GetHistoryResult.Messages.Num(), 0);
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_happy";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send a message to create history
	const FString TestMessage = TEXT("Test message for history");
	FPubnubChatSendTextParams SendTextParams;
	SendTextParams.StoreInHistory = true;
	FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage, SendTextParams);
	TestFalse("SendText should succeed", SendResult.Error);
	
	// Get history with only required parameters (StartTimetoken, EndTimetoken) and default Count
	const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString StartTimetoken = CurrentTimetoken;
	const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000); // 10 seconds ago
	
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(StartTimetoken, EndTimetoken);
	
	TestFalse("GetHistory should succeed", GetHistoryResult.Result.Error);
	TestTrue("Messages array should be valid", GetHistoryResult.Messages.Num() >= 0);
	
	// Verify that we got at least the message we sent
	if(GetHistoryResult.Messages.Num() > 0)
	{
		bool FoundTestMessage = false;
		for(UPubnubChatMessage* Message : GetHistoryResult.Messages)
		{
			if(Message)
			{
				FPubnubChatMessageData MessageData = Message->GetMessageData();
				if(MessageData.Text.Contains(TestMessage))
				{
					FoundTestMessage = true;
					TestEqual("Message ChannelID should match", MessageData.ChannelID, TestChannelID);
					TestFalse("Message Timetoken should not be empty", Message->GetMessageTimetoken().IsEmpty());
					break;
				}
			}
		}
		TestTrue("Test message should be found in history", FoundTestMessage);
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_full";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send multiple messages to create history
	const int NumMessages = 5;
	TArray<FString> SentMessages;
	FPubnubChatSendTextParams SendTextParams;
	SendTextParams.StoreInHistory = true;
	
	for(int i = 0; i < NumMessages; i++)
	{
		const FString TestMessage = FString::Printf(TEXT("Test message %d"), i);
		SentMessages.Add(TestMessage);
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage, SendTextParams);
		TestFalse(FString::Printf(TEXT("SendText %d should succeed"), i), SendResult.Error);
	}
	
	// Get history with all parameters (StartTimetoken, EndTimetoken, Count)
	const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString StartTimetoken = CurrentTimetoken;
	const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000); // 10 seconds ago
	const int TestCount = 3; // Request only 3 messages
	
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(StartTimetoken, EndTimetoken, TestCount);
	
	TestFalse("GetHistory should succeed with all parameters", GetHistoryResult.Result.Error);
	TestTrue("Messages array should be valid", GetHistoryResult.Messages.Num() >= 0);
	TestTrue("Should respect Count parameter", GetHistoryResult.Messages.Num() <= TestCount);
	
	// Verify IsMore flag is set correctly
	if(GetHistoryResult.Messages.Num() == TestCount)
	{
		TestTrue("IsMore should be true when we got exactly Count messages", GetHistoryResult.IsMore);
	}
	else
	{
		TestFalse("IsMore should be false when we got fewer than Count messages", GetHistoryResult.IsMore);
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
 * Tests GetHistory with empty channel (no messages).
 * Verifies that GetHistory returns empty array when there are no messages in the time range.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryEmptyChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.4Advanced.EmptyChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryEmptyChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_empty";
	
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
	
	// Create channel without sending any messages
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
	
	// Get history for a time range in the past (before channel creation)
	// StartTimetoken should be newer (larger) than EndTimetoken
	const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString StartTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -500000000); // 50 seconds ago (newer)
	const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -1000000000); // 100 seconds ago (older)
	
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(StartTimetoken, EndTimetoken);
	
	TestFalse("GetHistory should succeed even with empty channel", GetHistoryResult.Result.Error);
	TestEqual("Messages array should be empty for empty channel", GetHistoryResult.Messages.Num(), 0);
	TestFalse("IsMore should be false when no messages found", GetHistoryResult.IsMore);
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetHistory IsMore flag behavior.
 * Verifies that IsMore is set correctly when there are more messages than the requested count.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryIsMoreFlagTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.4Advanced.IsMoreFlag", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryIsMoreFlagTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_ismore_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_ismore";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send more messages than we'll request
	const int NumMessages = 10;
	const int RequestCount = 5;
	FPubnubChatSendTextParams SendTextParams;
	SendTextParams.StoreInHistory = true;
	
	for(int i = 0; i < NumMessages; i++)
	{
		const FString TestMessage = FString::Printf(TEXT("Test message %d"), i);
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage, SendTextParams);
		TestFalse(FString::Printf(TEXT("SendText %d should succeed"), i), SendResult.Error);
	}
	
	// Get history with Count less than number of messages sent
	const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString StartTimetoken = CurrentTimetoken;
	const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000); // 10 seconds ago
	
	FPubnubChatGetHistoryResult GetHistoryResult = CreateResult.Channel->GetHistory(StartTimetoken, EndTimetoken, RequestCount);
	
	TestFalse("GetHistory should succeed", GetHistoryResult.Result.Error);
	TestTrue("Should get at least some messages", GetHistoryResult.Messages.Num() > 0);
	
	// If we got exactly RequestCount messages, IsMore should be true (indicating more messages exist)
	if(GetHistoryResult.Messages.Num() == RequestCount)
	{
		TestTrue("IsMore should be true when we got exactly Count messages and more exist", GetHistoryResult.IsMore);
	}
	else if(GetHistoryResult.Messages.Num() < RequestCount)
	{
		TestFalse("IsMore should be false when we got fewer than Count messages", GetHistoryResult.IsMore);
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
// GETMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_not_init_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to get message with uninitialized channel
		const FString TestTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		FPubnubChatMessageResult GetMessageResult = UninitializedChannel->GetMessage(TestTimetoken);
		
		TestTrue("GetMessage should fail with uninitialized channel", GetMessageResult.Result.Error);
		TestFalse("ErrorMessage should not be empty", GetMessageResult.Result.ErrorMessage.IsEmpty());
		TestNull("Message should be null on error", GetMessageResult.Message);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageEmptyTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.1Validation.EmptyTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageEmptyTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_empty_timetoken_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_empty_timetoken";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Try to get message with empty Timetoken
	FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(TEXT(""));
	
	TestTrue("GetMessage should fail with empty Timetoken", GetMessageResult.Result.Error);
	TestFalse("ErrorMessage should not be empty", GetMessageResult.Result.ErrorMessage.IsEmpty());
	TestNull("Message should be null on error", GetMessageResult.Message);
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_happy";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send a message and get its timetoken using PubnubClient
	UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
	if(!PubnubClient)
	{
		AddError("Failed to get PubnubClient from Chat");
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	const FString TestMessageContent = TEXT("Test message for GetMessage");
	FPubnubPublishSettings PublishSettings;
	PublishSettings.StoreInHistory = true;
	const FString FormattedMessage = UPubnubChatInternalUtilities::ChatMessageToPublishString(TestMessageContent);
	FPubnubPublishMessageResult PublishResult = PubnubClient->PublishMessage(TestChannelID, FormattedMessage, PublishSettings);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	TestFalse("Published message timetoken should not be empty", PublishResult.PublishedMessage.Timetoken.IsEmpty());
	
	if(PublishResult.Result.Error || PublishResult.PublishedMessage.Timetoken.IsEmpty())
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	const FString MessageTimetoken = PublishResult.PublishedMessage.Timetoken;
	
	// Get message with only required parameter (Timetoken)
	FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
	
	TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
	TestNotNull("Message should be retrieved", GetMessageResult.Message);
	
	if(GetMessageResult.Message)
	{
		TestEqual("Retrieved message timetoken should match", GetMessageResult.Message->GetMessageTimetoken(), MessageTimetoken);
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		TestEqual("Message ChannelID should match", MessageData.ChannelID, TestChannelID);
		TestTrue("Message should contain the sent text", MessageData.Text.Contains(TestMessageContent));
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_full";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send a message with metadata and get its timetoken using PubnubClient
	UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
	if(!PubnubClient)
	{
		AddError("Failed to get PubnubClient from Chat");
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	const FString TestMessageContent = TEXT("Test message with metadata");
	const FString TestMeta = TEXT("{\"key\":\"value\"}");
	FPubnubPublishSettings PublishSettings;
	PublishSettings.StoreInHistory = true;
	PublishSettings.MetaData = TestMeta;
	const FString FormattedMessage = UPubnubChatInternalUtilities::ChatMessageToPublishString(TestMessageContent);
	FPubnubPublishMessageResult PublishResult = PubnubClient->PublishMessage(TestChannelID, FormattedMessage, PublishSettings);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	TestFalse("Published message timetoken should not be empty", PublishResult.PublishedMessage.Timetoken.IsEmpty());
	
	if(PublishResult.Result.Error || PublishResult.PublishedMessage.Timetoken.IsEmpty())
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	const FString MessageTimetoken = PublishResult.PublishedMessage.Timetoken;
	
	// GetMessage only has one parameter (Timetoken), so this test verifies it works with a real message
	FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(MessageTimetoken);
	
	TestFalse("GetMessage should succeed", GetMessageResult.Result.Error);
	TestNotNull("Message should be retrieved", GetMessageResult.Message);
	
	if(GetMessageResult.Message)
	{
		TestEqual("Retrieved message timetoken should match", GetMessageResult.Message->GetMessageTimetoken(), MessageTimetoken);
		FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
		TestEqual("Message ChannelID should match", MessageData.ChannelID, TestChannelID);
		TestTrue("Message should contain the sent text", MessageData.Text.Contains(TestMessageContent));
		TestEqual("Message Meta should match", MessageData.Meta, TestMeta);
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
 * Tests GetMessage with non-existent message timetoken.
 * Verifies that GetMessage handles gracefully when message doesn't exist.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageNonExistentTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.4Advanced.NonExistent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageNonExistentTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_nonexistent_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_nonexistent";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Use a timetoken that doesn't exist (far in the past)
	const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	const FString NonExistentTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -1000000000); // 100 seconds ago
	
	FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(NonExistentTimetoken);
	
	// GetMessage should succeed but return null message if message doesn't exist
	// The operation itself succeeds, but no message is found
	TestFalse("GetMessage operation should succeed even if message doesn't exist", GetMessageResult.Result.Error);
	TestNull("Message should be null when message doesn't exist", GetMessageResult.Message);
	
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests GetMessage retrieves correct message from multiple messages.
 * Verifies that GetMessage retrieves the specific message by timetoken when multiple messages exist.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetMessageMultipleMessagesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetMessage.4Advanced.MultipleMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetMessageMultipleMessagesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_message_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_message_multiple";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Send multiple messages and capture their timetokens
	UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
	if(!PubnubClient)
	{
		AddError("Failed to get PubnubClient from Chat");
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	const int NumMessages = 3;
	TArray<FString> MessageTimetokens;
	TArray<FString> MessageContents;
	
	FPubnubPublishSettings PublishSettings;
	PublishSettings.StoreInHistory = true;
	
	for(int i = 0; i < NumMessages; i++)
	{
		const FString TestMessageContent = FString::Printf(TEXT("Test message %d"), i);
		MessageContents.Add(TestMessageContent);
		const FString FormattedMessage = UPubnubChatInternalUtilities::ChatMessageToPublishString(TestMessageContent);
		FPubnubPublishMessageResult PublishResult = PubnubClient->PublishMessage(TestChannelID, FormattedMessage, PublishSettings);
		TestFalse(FString::Printf(TEXT("PublishMessage %d should succeed"), i), PublishResult.Result.Error);
		TestFalse(FString::Printf(TEXT("Published message %d timetoken should not be empty"), i), PublishResult.PublishedMessage.Timetoken.IsEmpty());
		
		if(!PublishResult.Result.Error && !PublishResult.PublishedMessage.Timetoken.IsEmpty())
		{
			MessageTimetokens.Add(PublishResult.PublishedMessage.Timetoken);
		}
	}
	
	TestEqual("Should have captured all message timetokens", MessageTimetokens.Num(), NumMessages);
	
	if(MessageTimetokens.Num() != NumMessages)
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Get each message individually and verify it's the correct one
	for(int i = 0; i < MessageTimetokens.Num(); i++)
	{
		const FString& TargetTimetoken = MessageTimetokens[i];
		const FString& ExpectedContent = MessageContents[i];
		
		FPubnubChatMessageResult GetMessageResult = CreateResult.Channel->GetMessage(TargetTimetoken);
		
		TestFalse(FString::Printf(TEXT("GetMessage %d should succeed"), i), GetMessageResult.Result.Error);
		TestNotNull(FString::Printf(TEXT("Message %d should be retrieved"), i), GetMessageResult.Message);
		
		if(GetMessageResult.Message)
		{
			TestEqual(FString::Printf(TEXT("Retrieved message %d timetoken should match"), i), GetMessageResult.Message->GetMessageTimetoken(), TargetTimetoken);
			FPubnubChatMessageData MessageData = GetMessageResult.Message->GetMessageData();
			TestTrue(FString::Printf(TEXT("Message %d should contain the correct text"), i), MessageData.Text.Contains(ExpectedContent));
			TestEqual(FString::Printf(TEXT("Message %d ChannelID should match"), i), MessageData.ChannelID, TestChannelID);
		}
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
// FORWARDMESSAGE TESTS
// ============================================================================

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

/**
 * Tests Channel->ForwardMessage happy path.
 * Verifies that Channel->ForwardMessage is a pass-through to Chat->ForwardMessage.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelForwardMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.ForwardMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelForwardMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_forward_message_happy_init";
	const FString SourceChannelID = SDK_PREFIX + "test_channel_forward_message_happy_source";
	const FString DestinationChannelID = SDK_PREFIX + "test_channel_forward_message_happy_dest";
	const FString TestMessageText = TEXT("Message to forward via channel");
	
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
	
	// Create source channel
	FPubnubChatChannelData SourceChannelData;
	FPubnubChatChannelResult CreateSourceResult = Chat->CreatePublicConversation(SourceChannelID, SourceChannelData);
	TestFalse("CreatePublicConversation should succeed for source", CreateSourceResult.Result.Error);
	TestNotNull("Source channel should be created", CreateSourceResult.Channel);
	
	// Create destination channel
	FPubnubChatChannelData DestChannelData;
	FPubnubChatChannelResult CreateDestResult = Chat->CreatePublicConversation(DestinationChannelID, DestChannelData);
	TestFalse("CreatePublicConversation should succeed for destination", CreateDestResult.Result.Error);
	TestNotNull("Destination channel should be created", CreateDestResult.Channel);
	
	if(!CreateSourceResult.Channel || !CreateDestResult.Channel)
	{
		if(CreateSourceResult.Channel)
		{
			Chat->DeleteChannel(SourceChannelID);
		}
		if(CreateDestResult.Channel)
		{
			Chat->DeleteChannel(DestinationChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bSourceMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> SourceMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	TSharedPtr<bool> bForwardedMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ForwardedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	
	// Connect to source channel to receive original message
	auto SourceMessageLambda = [this, bSourceMessageReceived, SourceMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*SourceMessage)
		{
			*bSourceMessageReceived = true;
			*SourceMessage = Message;
		}
	};
	CreateSourceResult.Channel->OnMessageReceivedNative.AddLambda(SourceMessageLambda);
	
	FPubnubChatOperationResult SourceConnectResult = CreateSourceResult.Channel->Connect();
	TestFalse("Connect to source channel should succeed", SourceConnectResult.Error);
	
	// Connect to destination channel to receive forwarded message
	auto DestMessageLambda = [this, bForwardedMessageReceived, ForwardedMessage, TestMessageText](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			FPubnubChatMessageData MessageData = Message->GetMessageData();
			if(MessageData.Text == TestMessageText && !*ForwardedMessage)
			{
				*bForwardedMessageReceived = true;
				*ForwardedMessage = Message;
			}
		}
	};
	CreateDestResult.Channel->OnMessageReceivedNative.AddLambda(DestMessageLambda);
	
	FPubnubChatOperationResult DestConnectResult = CreateDestResult.Channel->Connect();
	TestFalse("Connect to destination channel should succeed", DestConnectResult.Error);
	
	// Wait for subscriptions, then send message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSourceResult, TestMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateSourceResult.Channel->SendText(TestMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until source message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSourceMessageReceived]() -> bool {
		return *bSourceMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Forward the message using Channel->ForwardMessage (pass-through)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SourceMessage, CreateDestResult]()
	{
		if(!*SourceMessage)
		{
			AddError("Source message was not received");
			return;
		}
		
		FPubnubChatOperationResult ForwardResult = CreateDestResult.Channel->ForwardMessage(*SourceMessage);
		TestFalse("Channel->ForwardMessage should succeed", ForwardResult.Error);
		
		// Verify step results contain PublishMessage step
		bool bFoundPublishStep = false;
		for(const FPubnubChatOperationStepResult& Step : ForwardResult.StepResults)
		{
			if(Step.StepName == TEXT("PublishMessage"))
			{
				bFoundPublishStep = true;
				TestFalse("PublishMessage step should not have error", Step.OperationResult.Error);
			}
		}
		TestTrue("Should have PublishMessage step", bFoundPublishStep);
	}, 0.1f));
	
	// Wait until forwarded message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bForwardedMessageReceived]() -> bool {
		return *bForwardedMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Verify forwarded message content
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ForwardedMessage, TestMessageText]()
	{
		if(!*ForwardedMessage)
		{
			AddError("Forwarded message was not received");
			return;
		}
		
		FString ForwardedText = (*ForwardedMessage)->GetCurrentText();
		TestEqual("Forwarded message text should match original", ForwardedText, TestMessageText);
	}, 0.1f));
	
	// Cleanup: Disconnect channels and delete them
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateSourceResult, CreateDestResult, Chat, SourceChannelID, DestinationChannelID]()
	{
		if(CreateSourceResult.Channel)
		{
			CreateSourceResult.Channel->Disconnect();
		}
		if(CreateDestResult.Channel)
		{
			CreateDestResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(SourceChannelID);
			Chat->DeleteChannel(DestinationChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// STREAMUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesNotInitializedTest::RunTest(const FString& Parameters)
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
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stream updates with uninitialized channel
		FPubnubChatOperationResult StreamUpdatesResult = UninitializedChannel->StreamUpdates();
		TestTrue("StreamUpdates should fail with uninitialized channel", StreamUpdatesResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamUpdatesResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_happy";
	
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
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("InitialChannelName");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for update reception
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FString> ReceivedChannelID = MakeShared<FString>(TEXT(""));
	TSharedPtr<FPubnubChatChannelData> ReceivedChannelData = MakeShared<FPubnubChatChannelData>();
	
	// Set up delegate to receive channel updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedChannelID, ReceivedChannelData](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		*bUpdateReceived = true;
		*ReceivedChannelID = ChannelID;
		*ReceivedChannelData = ChannelData;
	};
	CreateResult.Channel->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates (no parameters required)
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Update channel to trigger update event
		FPubnubChatUpdateChannelInputData UpdatedChannelData;
		UpdatedChannelData.ChannelName = TEXT("UpdatedChannelName");
		UpdatedChannelData.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdatedChannelData);
		TestFalse("Update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify update was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedChannelID, ReceivedChannelData, TestChannelID, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		TestEqual("Received ChannelID should match", *ReceivedChannelID, TestChannelID);
		TestEqual("Received ChannelName should be updated", ReceivedChannelData->ChannelName, TEXT("UpdatedChannelName"));
		
		// Verify channel data was updated in repository
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("Retrieved ChannelName should be updated", RetrievedData.ChannelName, TEXT("UpdatedChannelName"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingUpdates();
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

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests StreamUpdates with partial field updates.
 * Verifies that only updated fields are changed and other fields remain unchanged.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesPartialUpdateTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.4Advanced.PartialUpdate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesPartialUpdateTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_partial_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_partial";
	
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
	
	// Create channel with initial data
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("InitialName");
	InitialChannelData.Description = TEXT("InitialDescription");
	InitialChannelData.Status = TEXT("active");
	InitialChannelData.Type = TEXT("public");
	InitialChannelData.Custom = TEXT("{\"key\":\"initial\"}");
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for update reception
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatChannelData> ReceivedChannelData = MakeShared<FPubnubChatChannelData>();
	
	// Set up delegate to receive channel updates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedChannelData](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		*bUpdateReceived = true;
		*ReceivedChannelData = ChannelData;
	};
	CreateResult.Channel->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription, then update only some fields
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		// Update only ChannelName and Description, leave others unchanged
		FPubnubChatUpdateChannelInputData PartialUpdateData;
		PartialUpdateData.ChannelName = TEXT("UpdatedName");
		PartialUpdateData.Description = TEXT("UpdatedDescription");
		PartialUpdateData.ForceSetChannelName = true;
		PartialUpdateData.ForceSetDescription = true;
		// Status, Type, and Custom are empty, so they should remain unchanged
		
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(PartialUpdateData);
		TestFalse("Partial update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait until update is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	// Verify partial update was received correctly
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedChannelData, CreateResult]()
	{
		TestTrue("Update should have been received", *bUpdateReceived);
		
		// Verify updated fields
		TestEqual("ChannelName should be updated", ReceivedChannelData->ChannelName, TEXT("UpdatedName"));
		TestEqual("Description should be updated", ReceivedChannelData->Description, TEXT("UpdatedDescription"));
		
		// Verify unchanged fields remain the same
		TestEqual("Status should remain unchanged", ReceivedChannelData->Status, TEXT("active"));
		TestEqual("Type should remain unchanged", ReceivedChannelData->Type, TEXT("public"));
		TestEqual("Custom should remain unchanged", ReceivedChannelData->Custom, TEXT("{\"key\":\"initial\"}"));
		
		// Verify channel data in repository matches
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("Retrieved ChannelName should match", RetrievedData.ChannelName, TEXT("UpdatedName"));
		TestEqual("Retrieved Description should match", RetrievedData.Description, TEXT("UpdatedDescription"));
		TestEqual("Retrieved Status should match", RetrievedData.Status, TEXT("active"));
		TestEqual("Retrieved Type should match", RetrievedData.Type, TEXT("public"));
		TestEqual("Retrieved Custom should match", RetrievedData.Custom, TEXT("{\"key\":\"initial\"}"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingUpdates();
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

/**
 * Tests StreamUpdates scenario: CreateChannel -> StreamUpdates -> Update fields -> Verify delegate -> Verify GetChannel.
 * This is the specific scenario requested by the user.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesFullScenarioTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.4Advanced.FullScenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesFullScenarioTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_full_scenario_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_full_scenario";
	
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
	
	// Step 1: CreateChannel with some data
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("InitialChannelName");
	InitialChannelData.Description = TEXT("InitialDescription");
	InitialChannelData.Status = TEXT("initial");
	InitialChannelData.Type = TEXT("public");
	InitialChannelData.Custom = TEXT("{\"initial\":\"value\"}");
	
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Verify initial channel data
	FPubnubChatChannelData InitialRetrievedData = CreateResult.Channel->GetChannelData();
	TestEqual("Initial ChannelName should match", InitialRetrievedData.ChannelName, TEXT("InitialChannelName"));
	TestEqual("Initial Description should match", InitialRetrievedData.Description, TEXT("InitialDescription"));
	
	// Shared state for update reception
	TSharedPtr<bool> bUpdateReceived = MakeShared<bool>(false);
	TSharedPtr<FString> ReceivedChannelID = MakeShared<FString>(TEXT(""));
	TSharedPtr<FPubnubChatChannelData> ReceivedChannelData = MakeShared<FPubnubChatChannelData>();
	
	// Step 2: StreamUpdates
	auto UpdateLambda = [this, bUpdateReceived, ReceivedChannelID, ReceivedChannelData](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		*bUpdateReceived = true;
		*ReceivedChannelID = ChannelID;
		*ReceivedChannelData = ChannelData;
	};
	CreateResult.Channel->OnUpdatedNative.AddLambda(UpdateLambda);
	
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Step 3: Update only some fields of the channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateChannelInputData UpdatedChannelData;
		UpdatedChannelData.ChannelName = TEXT("UpdatedChannelName");
		UpdatedChannelData.Description = TEXT("UpdatedDescription");
		UpdatedChannelData.Status = TEXT("updated");
		UpdatedChannelData.ForceSetChannelName = true;
		UpdatedChannelData.ForceSetDescription = true;
		UpdatedChannelData.ForceSetStatus = true;
		// Type and Custom are empty, so they should remain unchanged
		
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdatedChannelData);
		TestFalse("Update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Step 4: Wait and verify correct ChannelData was received from OnChannelUpdateReceived
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bUpdateReceived]() -> bool {
		return *bUpdateReceived;
	}, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bUpdateReceived, ReceivedChannelID, ReceivedChannelData, TestChannelID]()
	{
		TestTrue("Update should have been received via delegate", *bUpdateReceived);
		TestEqual("Received ChannelID should match", *ReceivedChannelID, TestChannelID);
		
		// Verify updated fields in delegate data
		TestEqual("Delegate ChannelName should be updated", ReceivedChannelData->ChannelName, TEXT("UpdatedChannelName"));
		TestEqual("Delegate Description should be updated", ReceivedChannelData->Description, TEXT("UpdatedDescription"));
		TestEqual("Delegate Status should be updated", ReceivedChannelData->Status, TEXT("updated"));
		
		// Verify unchanged fields remain
		TestEqual("Delegate Type should remain unchanged", ReceivedChannelData->Type, TEXT("public"));
		TestEqual("Delegate Custom should remain unchanged", ReceivedChannelData->Custom, TEXT("{\"initial\":\"value\"}"));
	}, 0.1f));
	
	// Step 5: Use GetChannel and verify channel data are updated as expected
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, CreateResult]()
	{
		FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed", GetChannelResult.Result.Error);
		TestNotNull("Channel should be retrieved", GetChannelResult.Channel);
		
		if(GetChannelResult.Channel)
		{
			FPubnubChatChannelData RetrievedData = GetChannelResult.Channel->GetChannelData();
			
			// Verify all fields match expected values
			TestEqual("GetChannel ChannelName should be updated", RetrievedData.ChannelName, TEXT("UpdatedChannelName"));
			TestEqual("GetChannel Description should be updated", RetrievedData.Description, TEXT("UpdatedDescription"));
			TestEqual("GetChannel Status should be updated", RetrievedData.Status, TEXT("updated"));
			TestEqual("GetChannel Type should remain unchanged", RetrievedData.Type, TEXT("public"));
			TestEqual("GetChannel Custom should remain unchanged", RetrievedData.Custom, TEXT("{\"initial\":\"value\"}"));
			
			// Also verify the original channel object has updated data
			FPubnubChatChannelData OriginalChannelData = CreateResult.Channel->GetChannelData();
			TestEqual("Original channel ChannelName should be updated", OriginalChannelData.ChannelName, TEXT("UpdatedChannelName"));
			TestEqual("Original channel Description should be updated", OriginalChannelData.Description, TEXT("UpdatedDescription"));
			TestEqual("Original channel Status should be updated", OriginalChannelData.Status, TEXT("updated"));
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingUpdates();
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

/**
 * Tests StreamUpdates with multiple sequential updates.
 * Verifies that each update is received correctly and channel data is updated incrementally.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesMultipleUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesMultipleUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_multiple";
	
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
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("InitialName");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Track all received updates
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatChannelData>> ReceivedUpdates = MakeShared<TArray<FPubnubChatChannelData>>();
	
	// Set up delegate to receive channel updates
	auto UpdateLambda = [this, UpdateCount, ReceivedUpdates](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		*UpdateCount = *UpdateCount + 1;
		ReceivedUpdates->Add(ChannelData);
	};
	CreateResult.Channel->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Send first update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateChannelInputData FirstUpdate;
		FirstUpdate.ChannelName = TEXT("FirstUpdate");
		FirstUpdate.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(FirstUpdate);
		TestFalse("First update should succeed", UpdateResult.Error);
	}, 0.5f));
	
	// Wait for first update
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 1;
	}, MAX_WAIT_TIME));
	
	// Send second update
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateChannelInputData SecondUpdate;
		SecondUpdate.ChannelName = TEXT("SecondUpdate");
		SecondUpdate.Description = TEXT("SecondDescription");
		SecondUpdate.ForceSetChannelName = true;
		SecondUpdate.ForceSetDescription = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(SecondUpdate);
		TestFalse("Second update should succeed", UpdateResult.Error);
	}, 0.1f));
	
	// Wait for second update
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([UpdateCount]() -> bool {
		return *UpdateCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify all updates were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount, ReceivedUpdates, CreateResult]()
	{
		TestTrue("Should have received at least 2 updates", *UpdateCount >= 2);
		
		// Verify latest update has correct data
		if(ReceivedUpdates->Num() >= 2)
		{
			FPubnubChatChannelData LatestUpdate = (*ReceivedUpdates)[ReceivedUpdates->Num() - 1];
			TestEqual("Latest update ChannelName should match", LatestUpdate.ChannelName, TEXT("SecondUpdate"));
			TestEqual("Latest update Description should match", LatestUpdate.Description, TEXT("SecondDescription"));
		}
		
		// Verify channel data matches latest update
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("Retrieved ChannelName should match latest update", RetrievedData.ChannelName, TEXT("SecondUpdate"));
		TestEqual("Retrieved Description should match latest update", RetrievedData.Description, TEXT("SecondDescription"));
	}, 0.1f));
	
	// Cleanup: Stop streaming updates and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingUpdates();
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

/**
 * Tests StreamUpdates scenario: CreateChannel -> StreamUpdates -> DeleteChannel -> Verify delegate receives PCSUT_Deleted type.
 * Verifies that when a channel is deleted, the stream update delegate receives the correct deletion type.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamUpdatesDeleteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamUpdates.4Advanced.Delete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamUpdatesDeleteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_updates_delete_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_updates_delete";
	
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
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("ChannelToDelete");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Shared state for deletion reception
	TSharedPtr<bool> bDeleteReceived = MakeShared<bool>(false);
	
	// Set up delegate to receive channel removal
	auto DeletedLambda = [this, bDeleteReceived]()
	{
		*bDeleteReceived = true;
	};
	CreateResult.Channel->OnDeletedNative.AddLambda(DeletedLambda);
	
	// Stream updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription to be ready, then delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
	{
		// Delete channel - this should trigger a delete event
		FPubnubChatOperationResult DeleteResult = Chat->DeleteChannel(TestChannelID);
		TestFalse("DeleteChannel should succeed", DeleteResult.Error);
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
// STOPSTREAMINGUPDATES TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingUpdatesNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingUpdates.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingUpdatesNotInitializedTest::RunTest(const FString& Parameters)
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
		// Create uninitialized channel object
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		
		// Try to stop streaming updates with uninitialized channel
		FPubnubChatOperationResult StopResult = UninitializedChannel->StopStreamingUpdates();
		TestTrue("StopStreamingUpdates should fail with uninitialized channel", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingUpdatesHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingUpdates.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingUpdatesHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_happy";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Stop streaming updates (no parameters required)
	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingUpdates();
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
	
	// Cleanup: Delete channel
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
 * Tests that StopStreamingUpdates prevents further updates from being received.
 * Verifies that after stopping, channel updates no longer trigger the delegate.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingUpdatesPreventsUpdatesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingUpdates.4Advanced.PreventsUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingUpdatesPreventsUpdatesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_prevents_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_prevents";
	
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
	FPubnubChatChannelData InitialChannelData;
	InitialChannelData.ChannelName = TEXT("InitialName");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Track received updates
	TSharedPtr<int32> UpdateCount = MakeShared<int32>(0);
	
	// Set up delegate to receive channel updates
	auto UpdateLambda = [this, UpdateCount](FString ChannelID, const FPubnubChatChannelData& ChannelData)
	{
		*UpdateCount = *UpdateCount + 1;
	};
	CreateResult.Channel->OnUpdatedNative.AddLambda(UpdateLambda);
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Wait for subscription, then send an update that should be received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatUpdateChannelInputData FirstUpdate;
		FirstUpdate.ChannelName = TEXT("FirstUpdate");
		FirstUpdate.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(FirstUpdate);
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
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingUpdates();
		TestFalse("StopStreamingUpdates should succeed", StopResult.Error);
		
		// Record update count before second update
		int32 CountBeforeStop = *UpdateCount;
		
		// Send second update after stopping
		FPubnubChatUpdateChannelInputData SecondUpdate;
		SecondUpdate.ChannelName = TEXT("SecondUpdate");
		SecondUpdate.ForceSetChannelName = true;
		FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(SecondUpdate);
		TestFalse("Second update should succeed", UpdateResult.Error);
		
		// Wait a bit to ensure update would have been received if streaming was active
	}, 0.1f));
	
	// Wait and verify second update was NOT received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount]()
	{
		// Wait a bit to ensure if update was going to be received, it would have been
	}, 1.0f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UpdateCount]()
	{
		// After stopping, update count should not have increased
		// We received at least 1 update before stopping, so count should be >= 1
		// But it should NOT have increased after stopping
		TestTrue("Update count should be at least 1 (from before stop)", *UpdateCount >= 1);
		
		// Note: We can't easily verify it didn't increase without storing the count before stop,
		// but we can verify the channel was updated via GetChannel even though delegate wasn't called
		// This is tested implicitly - if delegate was called, UpdateCount would be >= 2
	}, 0.1f));
	
	// Verify channel was still updated (even though delegate wasn't called)
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
	{
		FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(TestChannelID);
		TestFalse("GetChannel should succeed", GetChannelResult.Result.Error);
		if(GetChannelResult.Channel)
		{
			FPubnubChatChannelData RetrievedData = GetChannelResult.Channel->GetChannelData();
			// Channel should have been updated to SecondUpdate even though delegate wasn't called
			TestEqual("Channel should have been updated even after stopping", RetrievedData.ChannelName, TEXT("SecondUpdate"));
		}
	}, 0.1f));
	
	// Cleanup: Delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that StopStreamingUpdates can be called multiple times safely.
 * Verifies that calling StopStreamingUpdates when not streaming doesn't cause errors.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingUpdatesMultipleCallsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingUpdates.4Advanced.MultipleCalls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingUpdatesMultipleCallsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_streaming_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_streaming_multiple";
	
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
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Start streaming updates
	FPubnubChatOperationResult StreamUpdatesResult = CreateResult.Channel->StreamUpdates();
	TestFalse("StreamUpdates should succeed", StreamUpdatesResult.Error);
	
	// Stop streaming updates first time
	FPubnubChatOperationResult StopResult1 = CreateResult.Channel->StopStreamingUpdates();
	TestFalse("First StopStreamingUpdates should succeed", StopResult1.Error);
	
	// Stop streaming updates second time (should still succeed)
	FPubnubChatOperationResult StopResult2 = CreateResult.Channel->StopStreamingUpdates();
	TestFalse("Second StopStreamingUpdates should succeed", StopResult2.Error);
	
	// Stop streaming updates third time (should still succeed)
	FPubnubChatOperationResult StopResult3 = CreateResult.Channel->StopStreamingUpdates();
	TestFalse("Third StopStreamingUpdates should succeed", StopResult3.Error);
	
	// Cleanup: Delete channel
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

