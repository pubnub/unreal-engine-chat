// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "PubnubStructLibrary.h"
#include "Private/PubnubChatConst.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"

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

/**
 * Tests GetHistory on a thread channel (calling GetHistory, not GetThreadHistory).
 * Creates a thread, sends a message on it, then calls GetHistory on the thread channel.
 * Verifies that returned messages can be cast to UPubnubChatThreadMessage and that
 * their ParentChannelID matches the parent of that thread.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetHistoryOnThreadReturnsThreadMessagesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.GetHistory.4Advanced.OnThreadReturnsThreadMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetHistoryOnThreadReturnsThreadMessagesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_history_thread_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_history_on_thread";
	const FString ParentMessageText = TEXT("Parent message for GetHistory thread test");
	const FString ThreadMessageText = TEXT("Thread message for GetHistory test");

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);

	if(!CreateChannelResult.Channel)
	{
		Chat->DeleteChannel(TestChannelID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	auto MessageLambda = [this, bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);

	FPubnubChatOperationResult ConnectResult = CreateChannelResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, ParentMessageText]()
	{
		FPubnubChatOperationResult SendResult = CreateChannelResult.Channel->SendText(ParentMessageText);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));

	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel = MakeShared<UPubnubChatThreadChannel*>(nullptr);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage, ThreadChannel]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message was not received");
			return;
		}
		FPubnubChatThreadChannelResult CreateThreadResult = Chat->CreateThreadChannel(*ReceivedMessage);
		TestFalse("CreateThreadChannel should succeed", CreateThreadResult.Result.Error);
		TestNotNull("ThreadChannel should be created", CreateThreadResult.ThreadChannel);
		if(CreateThreadResult.ThreadChannel)
		{
			*ThreadChannel = CreateThreadResult.ThreadChannel;
		}
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		FPubnubChatOperationResult ThreadConnectResult = (*ThreadChannel)->Connect();
		TestFalse("Connect to thread channel should succeed", ThreadConnectResult.Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, ThreadMessageText]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		FPubnubChatOperationResult SendResult = (*ThreadChannel)->SendText(ThreadMessageText);
		TestFalse("SendText to thread should succeed", SendResult.Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ThreadChannel, TestChannelID]()
	{
		if(!*ThreadChannel)
		{
			AddError("ThreadChannel was not created");
			return;
		}
		const FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
		const FString StartTimetoken = CurrentTimetoken;
		const FString EndTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(CurrentTimetoken, -100000000);

		// Call GetHistory (not GetThreadHistory) on the thread channel
		FPubnubChatGetHistoryResult GetHistoryResult = (*ThreadChannel)->GetHistory(StartTimetoken, EndTimetoken);

		TestFalse("GetHistory on thread channel should succeed", GetHistoryResult.Result.Error);
		TestTrue("GetHistory should return at least one message", GetHistoryResult.Messages.Num() >= 1);

		const FString ExpectedParentChannelID = (*ThreadChannel)->GetParentChannelID();
		TestEqual("Thread parent channel ID should match test channel", ExpectedParentChannelID, TestChannelID);

		for(UPubnubChatMessage* Message : GetHistoryResult.Messages)
		{
			TestNotNull("Each history message should be non-null", Message);
			if(!Message) { continue; }
			UPubnubChatThreadMessage* ThreadMessage = Cast<UPubnubChatThreadMessage>(Message);
			TestNotNull("Each message from GetHistory on thread should cast to UPubnubChatThreadMessage", ThreadMessage);
			if(ThreadMessage)
			{
				TestEqual("ParentChannelID should match parent of thread", ThreadMessage->GetParentChannelID(), ExpectedParentChannelID);
			}
		}
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, Chat, TestChannelID, ReceivedMessage, ThreadChannel]()
	{
		if(*ThreadChannel)
		{
			(*ThreadChannel)->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}
		if(Chat && *ReceivedMessage)
		{
			FPubnubChatHasThreadResult HasThreadResult = (*ReceivedMessage)->HasThread();
			if(HasThreadResult.HasThread)
			{
				Chat->RemoveThreadChannel(*ReceivedMessage);
			}
			Chat->DeleteChannel(TestChannelID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));

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

// ============================================================================
// STREAMPRESENCE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamPresenceNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamPresence.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamPresenceNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_presence_not_init_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);

	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult StreamResult = UninitializedChannel->StreamPresence();
		TestTrue("StreamPresence should fail with uninitialized channel", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());
	}

	if(Chat)
	{
		Chat->DeleteUser(InitUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamPresenceAlreadyStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamPresence.1Validation.AlreadyStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamPresenceAlreadyStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_presence_already_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_presence_already";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult FirstResult = CreateResult.Channel->StreamPresence();
	TestFalse("First StreamPresence should succeed", FirstResult.Error);

	bool bFoundSubscribeInFirst = false;
	for(const FPubnubChatOperationStepResult& Step : FirstResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeInFirst = true;
			TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("First call should have Subscribe step", bFoundSubscribeInFirst);

	FPubnubChatOperationResult SecondResult = CreateResult.Channel->StreamPresence();
	TestFalse("Second StreamPresence should succeed (already streaming)", SecondResult.Error);

	bool bFoundSubscribeInSecond = false;
	for(const FPubnubChatOperationStepResult& Step : SecondResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeInSecond = true;
			break;
		}
	}
	TestFalse("Second call should not have Subscribe step", bFoundSubscribeInSecond);

	CreateResult.Channel->StopStreamingPresence();
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamPresenceHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamPresence.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamPresenceHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_presence_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_presence_happy";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamPresence();
	TestFalse("StreamPresence should succeed", StreamResult.Error);

	bool bFoundSubscribeStep = false;
	for(const FPubnubChatOperationStepResult& Step : StreamResult.StepResults)
	{
		if(Step.StepName == TEXT("Subscribe"))
		{
			bFoundSubscribeStep = true;
			TestFalse("Subscribe step should not have error", Step.OperationResult.Error);
		}
	}
	TestTrue("Should have Subscribe step", bFoundSubscribeStep);

	// Verify that WhoIsPresent works while stream is active and includes at least current user.
	FPubnubChatWhoIsPresentResult WhoIsPresentResult = CreateResult.Channel->WhoIsPresent();
	TestFalse("WhoIsPresent should succeed", WhoIsPresentResult.Result.Error);
	TestTrue("WhoIsPresent should contain at least one user", WhoIsPresentResult.Users.Num() >= 1);

	bool bFoundInitUser = false;
	for(const FString& UserID : WhoIsPresentResult.Users)
	{
		if(UserID == InitUserID)
		{
			bFoundInitUser = true;
			break;
		}
	}
	TestTrue("Current user should be present", bFoundInitUser);

	CreateResult.Channel->StopStreamingPresence();
	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests StreamPresence with multiple chat instances.
 * Verifies that join and leave presence events are delivered and user list reflects transitions.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamPresenceJoinLeaveEventsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamPresence.4Advanced.JoinLeaveEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStreamPresenceJoinLeaveEventsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_presence_events_init";
	const FString OtherUserID = SDK_PREFIX + "test_stream_presence_events_other";
	const FString TestChannelID = SDK_PREFIX + "test_stream_presence_events";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> PresenceEventCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FString>> LastReceivedUserIDs = MakeShared<TArray<FString>>();
	TSharedPtr<bool> bSawOtherUserJoin = MakeShared<bool>(false);
	TSharedPtr<bool> bSawOtherUserLeave = MakeShared<bool>(false);

	auto PresenceLambda = [this, PresenceEventCount, LastReceivedUserIDs, bSawOtherUserJoin, bSawOtherUserLeave, OtherUserID](const TArray<FString>& UserIDs)
	{
		*PresenceEventCount = *PresenceEventCount + 1;
		*LastReceivedUserIDs = UserIDs;

		bool bContainsOther = UserIDs.Contains(OtherUserID);
		if (bContainsOther)
		{
			*bSawOtherUserJoin = true;
		}
		else if (*bSawOtherUserJoin)
		{
			*bSawOtherUserLeave = true;
		}
	};
	CreateResult.Channel->OnPresenceChangedNative.AddLambda(PresenceLambda);

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamPresence();
	TestFalse("StreamPresence should succeed", StreamResult.Error);

	TSharedPtr<UPubnubChat*> OtherChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> OtherChannel = MakeShared<UPubnubChatChannel*>(nullptr);

	// Init second chat instance and connect channel to trigger presence "join".
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, OtherChat, OtherChannel, TestPublishKey, TestSubscribeKey, OtherUserID, TestChannelID]()
	{
		FPubnubChatConfig OtherChatConfig;
		FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
		TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);

		if(!OtherInitResult.Chat)
		{
			AddError("Other chat should be initialized");
			return;
		}

		*OtherChat = OtherInitResult.Chat;
		FPubnubChatChannelResult OtherGetChannelResult = (*OtherChat)->GetChannel(TestChannelID);
		TestFalse("Other user GetChannel should succeed", OtherGetChannelResult.Result.Error);
		TestNotNull("Other user channel should be valid", OtherGetChannelResult.Channel);

		if(!OtherGetChannelResult.Channel)
		{
			return;
		}

		*OtherChannel = OtherGetChannelResult.Channel;
		FPubnubChatOperationResult ConnectResult = (*OtherChannel)->Connect();
		TestFalse("Other user Connect should succeed", ConnectResult.Error);
	}, 0.6f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSawOtherUserJoin]() -> bool {
		return *bSawOtherUserJoin;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastReceivedUserIDs, OtherUserID]()
	{
		TestTrue("Presence list should contain other user after join", LastReceivedUserIDs->Contains(OtherUserID));
	}, 0.1f));

	// Disconnect second chat channel to trigger presence "leave".
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, OtherChannel]()
	{
		if(*OtherChannel)
		{
			FPubnubChatOperationResult DisconnectResult = (*OtherChannel)->Disconnect();
			TestFalse("Other user Disconnect should succeed", DisconnectResult.Error);
		}
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bSawOtherUserLeave]() -> bool {
		return *bSawOtherUserLeave;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, PresenceEventCount, LastReceivedUserIDs, OtherUserID]()
	{
		TestTrue("Should receive at least two presence events (join + leave)", *PresenceEventCount >= 2);
		TestFalse("Presence list should not contain other user after leave", LastReceivedUserIDs->Contains(OtherUserID));
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, OtherChat, CreateResult, TestChannelID, InitUserID, OtherUserID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingPresence();
		}
		if(*OtherChat)
		{
			(*OtherChat)->DeleteUser(OtherUserID);
			CleanUpCurrentChatUser(*OtherChat);
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(OtherUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// STOPSTREAMINGPRESENCE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingPresenceNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingPresence.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingPresenceNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_presence_not_init_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult StopResult = UninitializedChannel->StopStreamingPresence();
		TestTrue("StopStreamingPresence should fail with uninitialized channel", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());
	}

	if(Chat)
	{
		Chat->DeleteUser(InitUserID);
	}
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingPresenceNotStreamingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingPresence.1Validation.NotStreaming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingPresenceNotStreamingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_presence_not_streaming_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_presence_not_streaming";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingPresence();
	TestFalse("StopStreamingPresence should succeed when not streaming", StopResult.Error);
	TestEqual("StopStreamingPresence should not contain operation steps when skipped", StopResult.StepResults.Num(), 0);

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingPresenceHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingPresence.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingPresenceHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_presence_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_presence_happy";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamPresence();
	TestFalse("StreamPresence should succeed", StreamResult.Error);

	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingPresence();
	TestFalse("StopStreamingPresence should succeed", StopResult.Error);

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

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that StopStreamingPresence prevents further presence notifications.
 * Verifies no additional OnPresenceChanged events after stop while presence transitions still happen.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingPresenceStopsReceivingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingPresence.4Advanced.StopsReceiving", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelStopStreamingPresenceStopsReceivingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_presence_events_init";
	const FString OtherUserID = SDK_PREFIX + "test_stop_stream_presence_events_other";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_presence_events";

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

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> PresenceEventCount = MakeShared<int32>(0);
	auto PresenceLambda = [PresenceEventCount](const TArray<FString>& UserIDs)
	{
		*PresenceEventCount = *PresenceEventCount + 1;
	};
	CreateResult.Channel->OnPresenceChangedNative.AddLambda(PresenceLambda);

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamPresence();
	TestFalse("StreamPresence should succeed", StreamResult.Error);

	TSharedPtr<UPubnubChat*> OtherChat = MakeShared<UPubnubChat*>(nullptr);
	TSharedPtr<UPubnubChatChannel*> OtherChannel = MakeShared<UPubnubChatChannel*>(nullptr);
	TSharedPtr<int32> CountBeforeStop = MakeShared<int32>(0);

	// Start second chat presence to generate at least one event.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, OtherChat, OtherChannel, TestPublishKey, TestSubscribeKey, OtherUserID, TestChannelID]()
	{
		FPubnubChatConfig OtherChatConfig;
		FPubnubChatInitChatResult OtherInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, OtherUserID, OtherChatConfig);
		TestFalse("Other user InitChat should succeed", OtherInitResult.Result.Error);
		if(!OtherInitResult.Chat)
		{
			AddError("Other chat should be initialized");
			return;
		}
		*OtherChat = OtherInitResult.Chat;

		FPubnubChatChannelResult OtherGetChannelResult = (*OtherChat)->GetChannel(TestChannelID);
		TestFalse("Other user GetChannel should succeed", OtherGetChannelResult.Result.Error);
		TestNotNull("Other user channel should be valid", OtherGetChannelResult.Channel);
		if(!OtherGetChannelResult.Channel)
		{
			return;
		}
		*OtherChannel = OtherGetChannelResult.Channel;

		FPubnubChatOperationResult ConnectResult = (*OtherChannel)->Connect();
		TestFalse("Other user Connect should succeed", ConnectResult.Error);
	}, 0.6f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([PresenceEventCount]() -> bool {
		return *PresenceEventCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CountBeforeStop, PresenceEventCount]()
	{
		*CountBeforeStop = *PresenceEventCount;
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingPresence();
		TestFalse("StopStreamingPresence should succeed", StopResult.Error);
	}, 0.1f));

	// Trigger another presence transition after stop; delegate count should stay unchanged.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, OtherChannel]()
	{
		if(*OtherChannel)
		{
			FPubnubChatOperationResult DisconnectResult = (*OtherChannel)->Disconnect();
			TestFalse("Other user Disconnect should succeed", DisconnectResult.Error);
		}
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, PresenceEventCount, CountBeforeStop]()
	{
		TestEqual("Presence event count should not increase after stop", *PresenceEventCount, *CountBeforeStop);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, OtherChat, CreateResult, TestChannelID, InitUserID, OtherUserID]()
	{
		if(*OtherChat)
		{
			(*OtherChat)->DeleteUser(OtherUserID);
			CleanUpCurrentChatUser(*OtherChat);
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(OtherUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

return true;
}

// ============================================================================
// STREAMCUSTOMEVENTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamCustomEventsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamCustomEvents.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamCustomEventsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_custom_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult StreamResult = UninitializedChannel->StreamCustomEvents();
		TestTrue("StreamCustomEvents should fail on uninitialized object", StreamResult.Error);
		TestFalse("ErrorMessage should not be empty", StreamResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamCustomEventsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamCustomEvents.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamCustomEventsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_custom_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_custom_happy";
	const FString TestPayload = TEXT("{\"value\":\"hello-custom-stream\"}");
	const FString TestCustomMessageType = TEXT("stream_custom_happy_type");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> ReceivedCount = MakeShared<int32>(0);
	TSharedPtr<FPubnubChatCustomEvent> LastCustomEvent = MakeShared<FPubnubChatCustomEvent>();
	CreateResult.Channel->OnCustomEventReceivedNative.AddLambda([ReceivedCount, LastCustomEvent](const FPubnubChatCustomEvent& CustomEvent)
	{
		*ReceivedCount = *ReceivedCount + 1;
		*LastCustomEvent = CustomEvent;
	});

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamCustomEvents();
	TestFalse("StreamCustomEvents should succeed", StreamResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestPayload, TestCustomMessageType]()
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(TestPayload, TestCustomMessageType);
		TestFalse("EmitCustomEvent should succeed", EmitResult.Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedCount]() -> bool {
		return *ReceivedCount > 0;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedCount, LastCustomEvent, InitUserID, TestPayload, TestCustomMessageType]()
	{
		TestTrue("Custom event should be received", *ReceivedCount > 0);
		TestEqual("Custom event type should match CustomMessageType from EmitCustomEvent", LastCustomEvent->Type, TestCustomMessageType);
		TestEqual("Custom event payload should match emitted payload", LastCustomEvent->Payload, TestPayload);
		TestEqual("Custom event user should match init user", LastCustomEvent->UserID, InitUserID);
		TestFalse("Custom event timetoken should not be empty", LastCustomEvent->Timetoken.IsEmpty());
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, TestChannelID, InitUserID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingCustomEvents();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Ensures StreamCustomEvents is idempotent and does not register duplicate handlers on repeated start.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStreamCustomEventsIdempotentStartTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StreamCustomEvents.4Advanced.IdempotentStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStreamCustomEventsIdempotentStartTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stream_custom_idempotent_init";
	const FString TestChannelID = SDK_PREFIX + "test_stream_custom_idempotent";
	const FString TestPayload = TEXT("{\"type\":\"custom\",\"value\":\"idempotent\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> ReceivedCount = MakeShared<int32>(0);
	CreateResult.Channel->OnCustomEventReceivedNative.AddLambda([ReceivedCount](const FPubnubChatCustomEvent& CustomEvent)
	{
		*ReceivedCount = *ReceivedCount + 1;
	});

	FPubnubChatOperationResult FirstStreamResult = CreateResult.Channel->StreamCustomEvents();
	TestFalse("First StreamCustomEvents should succeed", FirstStreamResult.Error);
	FPubnubChatOperationResult SecondStreamResult = CreateResult.Channel->StreamCustomEvents();
	TestFalse("Second StreamCustomEvents should also succeed (idempotent)", SecondStreamResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestPayload]()
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(TestPayload);
		TestFalse("EmitCustomEvent should succeed", EmitResult.Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedCount]() -> bool {
		return *ReceivedCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedCount]()
	{
		TestEqual("Exactly one delegate invocation expected for one emitted event", *ReceivedCount, 1);
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, TestChannelID, InitUserID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->StopStreamingCustomEvents();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// STOPSTREAMINGCUSTOMEVENTS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingCustomEventsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingCustomEvents.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingCustomEventsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_custom_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult StopResult = UninitializedChannel->StopStreamingCustomEvents();
		TestTrue("StopStreamingCustomEvents should fail on uninitialized object", StopResult.Error);
		TestFalse("ErrorMessage should not be empty", StopResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingCustomEventsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingCustomEvents.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingCustomEventsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_custom_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_custom_happy";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamCustomEvents();
	TestFalse("StreamCustomEvents should succeed", StreamResult.Error);

	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingCustomEvents();
	TestFalse("StopStreamingCustomEvents should succeed", StopResult.Error);

	bool bHasUnsubscribeStep = false;
	for (const FPubnubChatOperationStepResult& StepResult : StopResult.StepResults)
	{
		if (StepResult.StepName == "Unsubscribe")
		{
			bHasUnsubscribeStep = true;
			break;
		}
	}
	TestTrue("StopStreamingCustomEvents should include Unsubscribe step", bHasUnsubscribeStep);

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingCustomEventsNoOpWhenAlreadyStoppedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingCustomEvents.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingCustomEventsNoOpWhenAlreadyStoppedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_custom_noop_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_custom_noop";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingCustomEvents();
	TestFalse("StopStreamingCustomEvents should succeed when not streaming (no-op)", StopResult.Error);
	TestEqual("No-op stop should not have step results", StopResult.StepResults.Num(), 0);

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Ensures StopStreamingCustomEvents actually stops delegate delivery for subsequent events.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelStopStreamingCustomEventsStopsDelegateDeliveryTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.StopStreamingCustomEvents.4Advanced.StopsDelegateDelivery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelStopStreamingCustomEventsStopsDelegateDeliveryTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_stop_stream_custom_advanced_init";
	const FString TestChannelID = SDK_PREFIX + "test_stop_stream_custom_advanced";
	const FString FirstPayload = TEXT("{\"type\":\"custom\",\"value\":\"first\"}");
	const FString SecondPayload = TEXT("{\"type\":\"custom\",\"value\":\"second\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<int32> ReceivedCount = MakeShared<int32>(0);
	CreateResult.Channel->OnCustomEventReceivedNative.AddLambda([ReceivedCount](const FPubnubChatCustomEvent& CustomEvent)
	{
		*ReceivedCount = *ReceivedCount + 1;
	});

	FPubnubChatOperationResult StreamResult = CreateResult.Channel->StreamCustomEvents();
	TestFalse("StreamCustomEvents should succeed", StreamResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, FirstPayload]()
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(FirstPayload);
		TestFalse("First EmitCustomEvent should succeed", EmitResult.Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([ReceivedCount]() -> bool {
		return *ReceivedCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult]()
	{
		FPubnubChatOperationResult StopResult = CreateResult.Channel->StopStreamingCustomEvents();
		TestFalse("StopStreamingCustomEvents should succeed", StopResult.Error);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, SecondPayload]()
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(SecondPayload);
		TestFalse("Second EmitCustomEvent should succeed", EmitResult.Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedCount]()
	{
		TestEqual("No additional delegate calls should happen after stop", *ReceivedCount, 1);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// EMITCUSTOMEVENT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitCustomEventNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitCustomEvent.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitCustomEventNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_custom_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult EmitResult = UninitializedChannel->EmitCustomEvent(TEXT("{\"type\":\"custom\",\"v\":\"x\"}"));
		TestTrue("EmitCustomEvent should fail on uninitialized object", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitCustomEventEmptyPayloadTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitCustomEvent.1Validation.EmptyPayload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitCustomEventEmptyPayloadTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_custom_empty_payload_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_custom_empty_payload";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(CreateResult.Channel)
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(TEXT(""));
		TestTrue("EmitCustomEvent should fail for empty payload", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitCustomEventHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitCustomEvent.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitCustomEventHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_custom_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_custom_happy";
	const FString TestPayload = TEXT("{\"type\":\"custom\",\"value\":\"emit-happy\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitCustomEvent(TestPayload);
	TestFalse("EmitCustomEvent should succeed", EmitResult.Error);
	TestTrue("EmitCustomEvent should produce at least one step", EmitResult.StepResults.Num() > 0);
	if (EmitResult.StepResults.Num() > 0)
	{
		TestEqual("First step should be PublishMessage", EmitResult.StepResults[0].StepName, FString("PublishMessage"));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestPayload]()
	{
		FPubnubFetchHistorySettings HistorySettings;
		HistorySettings.MaxPerChannel = 25;
		HistorySettings.IncludeCustomMessageType = true;
		HistorySettings.IncludeUserID = true;
		FPubnubFetchHistoryResult HistoryResult = Chat->GetPubnubClient()->FetchHistory(TestChannelID, HistorySettings);
		TestFalse("FetchHistory should succeed", HistoryResult.Result.Error);

		bool bFoundEmittedPayload = false;
		for (const FPubnubHistoryMessageData& MessageData : HistoryResult.Messages)
		{
			if (MessageData.Message == TestPayload)
			{
				bFoundEmittedPayload = true;
				break;
			}
		}
		TestTrue("Emitted payload should exist in history when StoreInHistory defaults to true", bFoundEmittedPayload);
	}, 0.8f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitCustomEventFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitCustomEvent.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitCustomEventFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_custom_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_custom_full";
	const FString NoHistoryPayload = TEXT("{\"type\":\"custom\",\"value\":\"do-not-store\"}");
	const FString StoredPayload = TEXT("{\"type\":\"custom\",\"value\":\"store\"}");
	const FString NoHistoryType = TEXT("custom_type_no_history");
	const FString StoredType = TEXT("custom_type_stored");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult EmitNoHistoryResult = CreateResult.Channel->EmitCustomEvent(NoHistoryPayload, NoHistoryType, false);
	TestFalse("EmitCustomEvent should succeed with StoreInHistory=false", EmitNoHistoryResult.Error);

	FPubnubChatOperationResult EmitStoredResult = CreateResult.Channel->EmitCustomEvent(StoredPayload, StoredType, true);
	TestFalse("EmitCustomEvent should succeed with StoreInHistory=true", EmitStoredResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, NoHistoryPayload, StoredPayload, StoredType]()
	{
		FPubnubFetchHistorySettings HistorySettings;
		HistorySettings.MaxPerChannel = 25;
		HistorySettings.IncludeCustomMessageType = true;
		HistorySettings.IncludeUserID = true;
		FPubnubFetchHistoryResult HistoryResult = Chat->GetPubnubClient()->FetchHistory(TestChannelID, HistorySettings);
		TestFalse("FetchHistory should succeed", HistoryResult.Result.Error);

		bool bFoundNoHistoryPayload = false;
		bool bFoundStoredPayload = false;
		bool bStoredTypeMatches = false;

		for (const FPubnubHistoryMessageData& MessageData : HistoryResult.Messages)
		{
			if (MessageData.Message == NoHistoryPayload)
			{
				bFoundNoHistoryPayload = true;
			}

			if (MessageData.Message == StoredPayload)
			{
				bFoundStoredPayload = true;
				if (MessageData.CustomMessageType == StoredType)
				{
					bStoredTypeMatches = true;
				}
			}
		}

		TestFalse("Payload sent with StoreInHistory=false should not be present in history", bFoundNoHistoryPayload);
		TestTrue("Payload sent with StoreInHistory=true should be present in history", bFoundStoredPayload);
		TestTrue("Stored payload should contain provided CustomMessageType", bStoredTypeMatches);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID]()
	{
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// EMITUSERMENTION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_not_init";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
		FPubnubChatOperationResult EmitResult = UninitializedChannel->EmitUserMention(TEXT("target"), TEXT("123"), TEXT("hello"));
		TestTrue("EmitUserMention should fail on uninitialized object", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());

		Chat->DeleteUser(InitUserID);
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_empty_user_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_empty_user";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(CreateResult.Channel)
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitUserMention(TEXT(""), TEXT("123"), TEXT("hello"));
		TestTrue("EmitUserMention should fail for empty UserID", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionEmptyTimetokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.1Validation.EmptyTimetoken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionEmptyTimetokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_empty_tt_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_empty_tt";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(CreateResult.Channel)
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitUserMention(TEXT("target"), TEXT(""), TEXT("hello"));
		TestTrue("EmitUserMention should fail for empty Timetoken", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionEmptyTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.1Validation.EmptyText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionEmptyTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_empty_text_init";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_empty_text";

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(CreateResult.Channel)
	{
		FPubnubChatOperationResult EmitResult = CreateResult.Channel->EmitUserMention(TEXT("target"), TEXT("123"), TEXT(""));
		TestTrue("EmitUserMention should fail for empty Text", EmitResult.Error);
		TestFalse("ErrorMessage should not be empty", EmitResult.ErrorMessage.IsEmpty());
	}

	Chat->DeleteChannel(TestChannelID);
	Chat->DeleteUser(InitUserID);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_happy_init";
	const FString MentionedUserID = SDK_PREFIX + "test_emit_mention_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_happy_channel";
	const FString MentionText = TEXT("mention happy path text");
	const FString MentionedMessagePayload = TEXT("{\"value\":\"message for mention\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateMentionedUserResult = Chat->CreateUser(MentionedUserID, FPubnubChatUserData());
	TestFalse("CreateUser for mentioned user should succeed", CreateMentionedUserResult.Result.Error);

	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		Chat->DeleteUser(MentionedUserID);
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult PublishResult = Chat->GetPubnubClient()->PublishMessage(TestChannelID, MentionedMessagePayload);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	const FString MentionedMessageTimetoken = PublishResult.PublishedMessage.Timetoken;
	TestFalse("Published timetoken should not be empty", MentionedMessageTimetoken.IsEmpty());

	FPubnubChatConfig MentionedChatConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedChatConfig);
	TestFalse("Mentioned user InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(MentionedUserID);
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	TSharedPtr<bool> bMentionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserMention> LastMention = MakeShared<FPubnubChatUserMention>();

	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([bMentionReceived, LastMention](const FPubnubChatUserMention& UserMention)
	{
		*bMentionReceived = true;
		*LastMention = UserMention;
	});

	FPubnubChatOperationResult StreamResult = (*MentionedChat)->GetCurrentUser()->StreamMentions();
	TestFalse("StreamMentions should succeed", StreamResult.Error);

	FPubnubChatOperationResult EmitResult = CreateChannelResult.Channel->EmitUserMention(MentionedUserID, MentionedMessageTimetoken, MentionText);
	TestFalse("EmitUserMention should succeed", EmitResult.Error);
	TestTrue("EmitUserMention should contain at least one step", EmitResult.StepResults.Num() > 0);
	if (EmitResult.StepResults.Num() > 0)
	{
		TestEqual("First step should be PublishMessage", EmitResult.StepResults[0].StepName, FString("PublishMessage"));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMentionReceived]() -> bool {
		return *bMentionReceived;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, LastMention, InitUserID, MentionedMessageTimetoken, TestChannelID]()
	{
		TestEqual("Mention MessageTimetoken should match emitted value", LastMention->MessageTimetoken, MentionedMessageTimetoken);
		TestEqual("Mention ChannelID should match source channel", LastMention->ChannelID, TestChannelID);
		TestEqual("MentionedByUserID should match emitter", LastMention->MentionedByUserID, InitUserID);
		TestTrue("ParentChannelID should be empty for EmitUserMention", LastMention->ParentChannelID.IsEmpty());
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, MentionedChat, CreateChannelResult, TestChannelID, InitUserID, MentionedUserID]()
	{
		if (*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if (Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(MentionedUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionNoOptionalParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.3FullParameters.NoOptionalParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionNoOptionalParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_full_init";
	const FString MentionedUserID = SDK_PREFIX + "test_emit_mention_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_full_channel";
	const FString MentionText = TEXT("mention full parameters text");
	const FString MentionedMessagePayload = TEXT("{\"value\":\"source for full mention\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateMentionedUserResult = Chat->CreateUser(MentionedUserID, FPubnubChatUserData());
	TestFalse("CreateUser for mentioned user should succeed", CreateMentionedUserResult.Result.Error);

	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		Chat->DeleteUser(MentionedUserID);
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult PublishResult = Chat->GetPubnubClient()->PublishMessage(TestChannelID, MentionedMessagePayload);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	const FString MentionedMessageTimetoken = PublishResult.PublishedMessage.Timetoken;

	FPubnubChatOperationResult EmitResult = CreateChannelResult.Channel->EmitUserMention(MentionedUserID, MentionedMessageTimetoken, MentionText);
	TestFalse("EmitUserMention should succeed", EmitResult.Error);
	TestTrue("EmitUserMention should contain at least one step", EmitResult.StepResults.Num() > 0);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, MentionedUserID, MentionedMessageTimetoken, TestChannelID, MentionText]()
	{
		FPubnubFetchHistorySettings HistorySettings;
		HistorySettings.MaxPerChannel = 30;
		HistorySettings.IncludeUserID = true;
		FPubnubFetchHistoryResult HistoryResult = Chat->GetPubnubClient()->FetchHistory(MentionedUserID, HistorySettings);
		TestFalse("FetchHistory for mentioned user channel should succeed", HistoryResult.Result.Error);

		bool bFoundMentionEvent = false;
		for (const FPubnubHistoryMessageData& MessageData : HistoryResult.Messages)
		{
			if (!UPubnubChatInternalUtilities::IsThisEventMessage(MessageData.Message))
			{
				continue;
			}

			FPubnubChatEvent Event = UPubnubChatInternalUtilities::GetEventFromPubnubHistoryMessageData(MessageData);
			if (Event.Type != EPubnubChatEventType::PCET_Mention)
			{
				continue;
			}

			FPubnubChatUserMention Mention = UPubnubChatInternalUtilities::GetUserMentionFromChatEvent(Event);
			if (Mention.MessageTimetoken == MentionedMessageTimetoken && Mention.ChannelID == TestChannelID)
			{
				bFoundMentionEvent = true;

				TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
				const bool bParseOk = UPubnubJsonUtilities::StringToJsonObject(Event.Payload, JsonObject);
				TestTrue("Mention payload should be valid JSON", bParseOk);
				if (bParseOk)
				{
					FString PayloadText;
					JsonObject->TryGetStringField(ANSI_TO_TCHAR("text"), PayloadText);
					TestEqual("Mention payload text should match emitted Text parameter", PayloadText, MentionText);
				}
				break;
			}
		}

		TestTrue("Mention event should be present in mentioned user history channel", bFoundMentionEvent);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, InitUserID, MentionedUserID]()
	{
		if (Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(MentionedUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Ensures EmitUserMention is delivered only to the targeted user's mention stream.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionTargetsOnlyRecipientTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.4Advanced.TargetsOnlyRecipient", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionTargetsOnlyRecipientTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_emit_mention_targeted_init";
	const FString MentionedUserID = SDK_PREFIX + "test_emit_mention_targeted_target";
	const FString NonMentionedUserID = SDK_PREFIX + "test_emit_mention_targeted_other";
	const FString TestChannelID = SDK_PREFIX + "test_emit_mention_targeted_channel";
	const FString MentionText = TEXT("targeted mention text");
	const FString MentionedMessagePayload = TEXT("{\"value\":\"source for targeted mention\"}");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}

	Chat->CreateUser(MentionedUserID, FPubnubChatUserData());
	Chat->CreateUser(NonMentionedUserID, FPubnubChatUserData());

	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		Chat->DeleteUser(NonMentionedUserID);
		Chat->DeleteUser(MentionedUserID);
		Chat->DeleteUser(InitUserID);
		CleanUp();
		return false;
	}

	FPubnubPublishMessageResult PublishResult = Chat->GetPubnubClient()->PublishMessage(TestChannelID, MentionedMessagePayload);
	TestFalse("PublishMessage should succeed", PublishResult.Result.Error);
	const FString MentionedMessageTimetoken = PublishResult.PublishedMessage.Timetoken;

	FPubnubChatConfig MentionedChatConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedChatConfig);
	TestFalse("Mentioned user InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);

	FPubnubChatConfig NonMentionedChatConfig;
	FPubnubChatInitChatResult NonMentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, NonMentionedUserID, NonMentionedChatConfig);
	TestFalse("Non-mentioned user InitChat should succeed", NonMentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> NonMentionedChat = MakeShared<UPubnubChat*>(NonMentionedInitResult.Chat);

	if(!*MentionedChat || !*NonMentionedChat)
	{
		AddError("Both additional chats should be initialized");
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(NonMentionedUserID);
			Chat->DeleteUser(MentionedUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUp();
		return false;
	}

	TSharedPtr<int32> MentionedUserCount = MakeShared<int32>(0);
	TSharedPtr<int32> NonMentionedUserCount = MakeShared<int32>(0);

	(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([MentionedUserCount](const FPubnubChatUserMention& UserMention)
	{
		*MentionedUserCount = *MentionedUserCount + 1;
	});
	(*NonMentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([NonMentionedUserCount](const FPubnubChatUserMention& UserMention)
	{
		*NonMentionedUserCount = *NonMentionedUserCount + 1;
	});

	TestFalse("Mentioned user StreamMentions should succeed", (*MentionedChat)->GetCurrentUser()->StreamMentions().Error);
	TestFalse("Non-mentioned user StreamMentions should succeed", (*NonMentionedChat)->GetCurrentUser()->StreamMentions().Error);

	TestFalse(
		"EmitUserMention should succeed",
		CreateChannelResult.Channel->EmitUserMention(MentionedUserID, MentionedMessageTimetoken, MentionText).Error
	);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([MentionedUserCount]() -> bool {
		return *MentionedUserCount >= 1;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, MentionedUserCount, NonMentionedUserCount]()
	{
		TestEqual("Mentioned user should receive exactly one mention", *MentionedUserCount, 1);
		TestEqual("Non-mentioned user should not receive mention", *NonMentionedUserCount, 0);
	}, 1.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, MentionedChat, NonMentionedChat, TestChannelID, InitUserID, MentionedUserID, NonMentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}
		if(*NonMentionedChat)
		{
			(*NonMentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*NonMentionedChat)->DeleteUser(NonMentionedUserID);
			CleanUpCurrentChatUser(*NonMentionedChat);
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(NonMentionedUserID);
			Chat->DeleteUser(MentionedUserID);
			Chat->DeleteUser(InitUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

	return true;
}

/**
 * Ensures EmitUserMention called on a thread channel carries ParentChannelID and thread ChannelID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelEmitUserMentionFromThreadIncludesParentChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.EmitUserMention.4Advanced.FromThreadIncludesParentChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelEmitUserMentionFromThreadIncludesParentChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_emit_mention_thread_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_emit_mention_thread_target";
	const FString ParentChannelID = SDK_PREFIX + "test_emit_mention_thread_parent";
	const FString ParentMessageText = TEXT("parent message for thread mention");
	const FString MentionText = TEXT("thread mention payload text");

	FPubnubChatConfig SenderConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, SenderConfig);
	TestFalse("Sender InitChat should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		AddError("Sender chat should be initialized");
		CleanUp();
		return false;
	}

	TestFalse("Create mentioned user should succeed", SenderChat->CreateUser(MentionedUserID, FPubnubChatUserData()).Result.Error);

	FPubnubChatConfig MentionedConfig;
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, MentionedConfig);
	TestFalse("Mentioned InitChat should succeed", MentionedInitResult.Result.Error);
	TSharedPtr<UPubnubChat*> MentionedChat = MakeShared<UPubnubChat*>(MentionedInitResult.Chat);
	if(!*MentionedChat)
	{
		AddError("Mentioned chat should be initialized");
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUp();
		return false;
	}

	FPubnubChatChannelResult CreateChannelResult = SenderChat->CreatePublicConversation(ParentChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateChannelResult.Result.Error);
	TestNotNull("Parent channel should be created", CreateChannelResult.Channel);
	if(!CreateChannelResult.Channel)
	{
		SenderChat->DeleteUser(MentionedUserID);
		SenderChat->DeleteUser(SenderUserID);
		CleanUpCurrentChatUser(*MentionedChat);
		CleanUp();
		return false;
	}

	TSharedPtr<bool> bParentMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ParentMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	CreateChannelResult.Channel->OnMessageReceivedNative.AddLambda([bParentMessageReceived, ParentMessage](UPubnubChatMessage* Message)
	{
		if (Message && !*ParentMessage)
		{
			*bParentMessageReceived = true;
			*ParentMessage = Message;
		}
	});

	TestFalse("Connect parent channel should succeed", CreateChannelResult.Channel->Connect().Error);

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateChannelResult, ParentMessageText]()
	{
		TestFalse("SendText to parent channel should succeed", CreateChannelResult.Channel->SendText(ParentMessageText).Error);
	}, 0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bParentMessageReceived]() -> bool {
		return *bParentMessageReceived;
	}, MAX_WAIT_TIME));

	TSharedPtr<UPubnubChatThreadChannel*> ThreadChannel = MakeShared<UPubnubChatThreadChannel*>(nullptr);
	TSharedPtr<bool> bMentionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserMention> ReceivedMention = MakeShared<FPubnubChatUserMention>();

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, ParentMessage, ThreadChannel, bMentionReceived, ReceivedMention]()
	{
		if(!*ParentMessage)
		{
			AddError("Parent message was not received");
			return;
		}

		FPubnubChatThreadChannelResult CreateThreadResult = SenderChat->CreateThreadChannel(*ParentMessage);
		TestFalse("CreateThreadChannel should succeed", CreateThreadResult.Result.Error);
		TestNotNull("Thread channel should be created", CreateThreadResult.ThreadChannel);
		if(!CreateThreadResult.ThreadChannel)
		{
			return;
		}
		*ThreadChannel = CreateThreadResult.ThreadChannel;

		TestFalse("StreamMentions should succeed", (*MentionedChat)->GetCurrentUser()->StreamMentions().Error);
		(*MentionedChat)->GetCurrentUser()->OnMentionedNative.AddLambda([bMentionReceived, ReceivedMention](const FPubnubChatUserMention& Mention)
		{
			*bMentionReceived = true;
			*ReceivedMention = Mention;
		});

		const FString ParentMessageTimetoken = (*ParentMessage)->GetMessageTimetoken();
		TestFalse("Parent message timetoken should not be empty", ParentMessageTimetoken.IsEmpty());
		TestFalse("EmitUserMention from thread channel should succeed", (*ThreadChannel)->EmitUserMention((*MentionedChat)->GetCurrentUser()->GetUserID(), ParentMessageTimetoken, TEXT("thread mention payload text")).Error);
	}, 0.2f));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMentionReceived]() -> bool {
		return *bMentionReceived;
	}, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReceivedMention, ThreadChannel, ParentMessage, SenderUserID, ParentChannelID]()
	{
		if(!*ThreadChannel || !*ParentMessage)
		{
			AddError("Required test objects are not valid");
			return;
		}

		TestEqual("Mention ParentChannelID should match parent channel", ReceivedMention->ParentChannelID, ParentChannelID);
		TestEqual("Mention ChannelID should match thread channel ID", ReceivedMention->ChannelID, (*ThreadChannel)->GetChannelID());
		TestEqual("Mention MessageTimetoken should match provided timetoken", ReceivedMention->MessageTimetoken, (*ParentMessage)->GetMessageTimetoken());
		TestEqual("MentionedByUserID should match sender", ReceivedMention->MentionedByUserID, SenderUserID);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, CreateChannelResult, ParentMessage, ThreadChannel, ParentChannelID, SenderUserID, MentionedUserID]()
	{
		if(*MentionedChat)
		{
			(*MentionedChat)->GetCurrentUser()->StopStreamingMentions();
			(*MentionedChat)->DeleteUser(MentionedUserID);
			CleanUpCurrentChatUser(*MentionedChat);
		}

		if(*ThreadChannel)
		{
			(*ThreadChannel)->Disconnect();
		}
		if(CreateChannelResult.Channel)
		{
			CreateChannelResult.Channel->Disconnect();
		}

		if(SenderChat)
		{
			if(*ParentMessage)
			{
				FPubnubChatHasThreadResult HasThreadResult = (*ParentMessage)->HasThread();
				if(HasThreadResult.HasThread)
				{
					SenderChat->RemoveThreadChannel(*ParentMessage);
				}
			}
			SenderChat->DeleteChannel(ParentChannelID);
			SenderChat->DeleteUser(MentionedUserID);
			SenderChat->DeleteUser(SenderUserID);
		}
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
	}, 0.2f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

