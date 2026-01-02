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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID1, false);
		Chat->DeleteUser(TargetUserID2, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
	FOnPubnubChatChannelMessageReceived TargetUserMessageCallback;
	FPubnubChatJoinResult TargetUserJoinResult = TargetUserGetChannelResult.Channel->Join(TargetUserMessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
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
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
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
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID1, false);
		Chat->DeleteUser(TargetUserID2, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
			Chat->DeleteChannel(TestChannelID, false);
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
		Chat->DeleteChannel(TestChannelID, false);
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

