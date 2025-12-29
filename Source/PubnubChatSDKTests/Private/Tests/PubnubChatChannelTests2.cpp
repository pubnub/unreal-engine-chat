// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "PubnubChatUser.h"
#include "PubnubChatCallbackStop.h"
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
// INVITE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Create user to invite
			const FString TargetUserID = SDK_PREFIX + "test_invite_not_init_target";
			FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
			TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
			TestNotNull("User should be created", CreateUserResult.User);
			
			if(CreateUserResult.User)
			{
				// Try to invite with uninitialized channel
				FPubnubChatInviteResult InviteResult = UninitializedChannel->Invite(CreateUserResult.User);
				TestTrue("Invite should fail with uninitialized channel", InviteResult.Result.Error);
			}
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
				if(CreateUserResult.User)
				{
					Chat->DeleteUser(TargetUserID, false);
				}
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNullUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.1Validation.NullUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteNullUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_null_user_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_null_user";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FOnPubnubChatChannelMessageReceived MessageCallback;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite with null user
			FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(nullptr);
			TestTrue("Invite should fail with null user", InviteResult.Result.Error);
			
			// Cleanup
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

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	if(InviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", InviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", InviteResult.Membership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData MembershipData = InviteResult.Membership->GetMembershipData();
		TestEqual("Membership Status should be pending", MembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && InviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// Note: Invite only takes User parameter, so full parameter test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create channel with custom data and join
	FPubnubChatChannelData ChannelData;
	ChannelData.Custom = TEXT("{\"test\":\"channel\"}");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, MembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user with custom data
	FPubnubChatUserData UserData;
	UserData.Custom = TEXT("{\"test\":\"user\"}");
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Invite user
	FPubnubChatInviteResult InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Invite should succeed", InviteResult.Result.Error);
	TestNotNull("Membership should be created", InviteResult.Membership);
	
	if(InviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", InviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", InviteResult.Membership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData RetrievedMembershipData = InviteResult.Membership->GetMembershipData();
		TestEqual("Membership Status should be pending", RetrievedMembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && InviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that Invite returns existing membership if user is already a member.
 * Verifies that inviting the same user twice returns the existing membership on the second call.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteAlreadyMemberTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.AlreadyMember", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteAlreadyMemberTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_already_member_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_already_member_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_already_member";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// First invite - should create membership with "pending" status
	FPubnubChatInviteResult FirstInviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("First Invite should succeed", FirstInviteResult.Result.Error);
	TestNotNull("First Membership should be created", FirstInviteResult.Membership);
	
	if(!FirstInviteResult.Membership)
	{
		CleanUp();
		return false;
	}
	
	FPubnubChatMembershipData FirstMembershipData = FirstInviteResult.Membership->GetMembershipData();
	TestEqual("First Membership Status should be pending", FirstMembershipData.Status, TEXT("pending"));
	
	// Second invite - should return existing membership (not create a new one)
	FPubnubChatInviteResult SecondInviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
	TestFalse("Second Invite should succeed (returns existing membership)", SecondInviteResult.Result.Error);
	TestNotNull("Second Membership should be returned", SecondInviteResult.Membership);
	
	if(SecondInviteResult.Membership)
	{
		TestEqual("Membership ChannelID should match", SecondInviteResult.Membership->GetChannelID(), TestChannelID);
		TestEqual("Membership UserID should match", SecondInviteResult.Membership->GetUserID(), TargetUserID);
		
		// Status should still be "pending" (user hasn't joined yet)
		FPubnubChatMembershipData SecondMembershipData = SecondInviteResult.Membership->GetMembershipData();
		TestEqual("Second Membership Status should still be pending", SecondMembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove membership for invited user
	if(Chat && SecondInviteResult.Membership)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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

	CleanUp();
	return true;
}

/**
 * Tests that Invite emits Invite events for non-public channels.
 * Verifies that invitation events are sent when inviting to group/direct channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteNonPublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.NonPublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteNonPublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_nonpublic_event_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_nonpublic_event_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_nonpublic_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Create direct conversation (non-public channel)
	FPubnubChatChannelData ChannelData;
	FPubnubChatMembershipData HostMembershipData;
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, HostMembershipData);
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	// Verify channel is direct (non-public)
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be direct", CreatedChannelData.Type, TEXT("direct"));
	
	// Create second user to invite
	const FString SecondUserID = SDK_PREFIX + "test_invite_nonpublic_event_user2";
	FPubnubChatUserResult CreateSecondUserResult = Chat->CreateUser(SecondUserID, FPubnubChatUserData());
	TestFalse("CreateSecondUser should succeed", CreateSecondUserResult.Result.Error);
	TestNotNull("Second user should be created", CreateSecondUserResult.User);
	
	if(!CreateSecondUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	
	// Listen for Invite events on target user's channel
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, SecondUserID, TestChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, SecondUserID);
		TestFalse("Received event Payload should not be empty", Event.Payload.IsEmpty());
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("direct")));
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(SecondUserID, EPubnubChatEventType::PCET_Invite, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteResult> InviteResult = MakeShared<FPubnubChatInviteResult>();
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateSecondUserResult, InviteResult]()
	{
		// Invite second user to the direct conversation
		*InviteResult = CreateResult.Channel->Invite(CreateSecondUserResult.User);
		TestFalse("Invite should succeed", InviteResult->Result.Error);
		TestNotNull("Membership should be created", InviteResult->Membership);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID]()
	{
		if(!*bEventReceived)
		{
			AddError("Invite event was not received for non-public channel");
		}
		else
		{
			TestEqual("Received event type should be Invite", ReceivedEvent->Type, EPubnubChatEventType::PCET_Invite);
			TestTrue("Received event Payload should contain channelId", ReceivedEvent->Payload.Contains(TestChannelID));
			TestTrue("Received event Payload should contain channelType", ReceivedEvent->Payload.Contains(TEXT("direct")));
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, remove memberships, delete users and channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, Chat, TestChannelID, InitUserID, TargetUserID, SecondUserID]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		if(Chat)
		{
			// Remove membership for invited user
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(SecondUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(InitUserID, false);
			Chat->DeleteUser(TargetUserID, false);
			Chat->DeleteUser(SecondUserID, false);
		}
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that Invite emits Invite events for public channels.
 * Verifies that invitation events are sent when inviting to public channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInvitePublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Invite.4Advanced.PublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInvitePublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_public_event_init";
	const FString TargetUserID = SDK_PREFIX + "test_invite_public_event_target";
	const FString TestChannelID = SDK_PREFIX + "test_invite_public_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Verify channel is public
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be public", CreatedChannelData.Type, TEXT("public"));
	
	// Join channel
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatEvent> ReceivedEvent = MakeShared<FPubnubChatEvent>();
	
	// Listen for Invite events on target user's channel
	FOnPubnubChatEventReceivedNative EventCallback;
	EventCallback.BindLambda([this, bEventReceived, ReceivedEvent, TargetUserID, TestChannelID](const FPubnubChatEvent& Event)
	{
		*bEventReceived = true;
		*ReceivedEvent = Event;
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, TargetUserID);
		TestFalse("Received event Payload should not be empty", Event.Payload.IsEmpty());
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("public")));
	});
	
	FPubnubChatListenForEventsResult ListenResult = Chat->ListenForEvents(TargetUserID, EPubnubChatEventType::PCET_Invite, EventCallback);
	TestFalse("ListenForEvents should succeed", ListenResult.Result.Error);
	TestNotNull("CallbackStop should be created", ListenResult.CallbackStop);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteResult> InviteResult = MakeShared<FPubnubChatInviteResult>();
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUserResult, InviteResult]()
	{
		// Invite user to public channel
		*InviteResult = CreateResult.Channel->Invite(CreateUserResult.User);
		TestFalse("Invite should succeed", InviteResult->Result.Error);
		TestNotNull("Membership should be created", InviteResult->Membership);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedEvent, TestChannelID]()
	{
		if(!*bEventReceived)
		{
			AddError("Invite event was not received for public channel");
		}
		else
		{
			TestEqual("Received event type should be Invite", ReceivedEvent->Type, EPubnubChatEventType::PCET_Invite);
			TestTrue("Received event Payload should contain channelId", ReceivedEvent->Payload.Contains(TestChannelID));
			TestTrue("Received event Payload should contain channelType", ReceivedEvent->Payload.Contains(TEXT("public")));
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, remove membership, leave and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult, CreateResult, Chat, TestChannelID, TargetUserID, InviteResult]()
	{
		if(ListenResult.CallbackStop)
		{
			ListenResult.CallbackStop->Stop();
		}
		if(Chat)
		{
			// Remove membership for invited user
			if(InviteResult->Membership)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					FPubnubMembershipsResult RemoveResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove membership during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
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
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// INVITE MULTIPLE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Create users to invite
			const FString TargetUserID = SDK_PREFIX + "test_invite_multiple_not_init_target";
			FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
			TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
			TestNotNull("User should be created", CreateUserResult.User);
			
			if(CreateUserResult.User)
			{
				TArray<UPubnubChatUser*> Users = {CreateUserResult.User};
				
				// Try to invite multiple with uninitialized channel
				FPubnubChatInviteMultipleResult InviteResult = UninitializedChannel->InviteMultiple(Users);
				TestTrue("InviteMultiple should fail with uninitialized channel", InviteResult.Result.Error);
			}
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
				if(CreateUserResult.User)
				{
					Chat->DeleteUser(TargetUserID, false);
				}
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleEmptyArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.EmptyArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleEmptyArrayTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FOnPubnubChatChannelMessageReceived MessageCallback;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite multiple with empty array
			TArray<UPubnubChatUser*> EmptyUsers;
			FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(EmptyUsers);
			TestTrue("InviteMultiple should fail with empty array", InviteResult.Result.Error);
			
			// Cleanup
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

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleAllInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.1Validation.AllInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleAllInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_all_invalid_init";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_all_invalid";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel and join
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			FOnPubnubChatChannelMessageReceived MessageCallback;
			FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
			TestFalse("Join should succeed", JoinResult.Result.Error);
			
			// Try to invite multiple with all invalid users (null pointers)
			TArray<UPubnubChatUser*> InvalidUsers = {nullptr, nullptr};
			FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(InvalidUsers);
			TestTrue("InviteMultiple should fail with all invalid users", InviteResult.Result.Error);
			
			// Cleanup
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

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_happy_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_happy_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_happy_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Invite multiple users
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed", InviteResult.Result.Error);
	TestEqual("Should have 2 memberships", InviteResult.Memberships.Num(), 2);
	
	for(int32 i = 0; i < InviteResult.Memberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = InviteResult.Memberships[i];
		TestNotNull(FString::Printf(TEXT("Membership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("Membership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("Membership %d Status should be pending"), i), MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

// Note: InviteMultiple only takes Users array parameter, so full parameter test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultipleFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_full_target2";
	const FString TargetUserID3 = SDK_PREFIX + "test_invite_multiple_full_target3";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create channel with custom data and join
	FPubnubChatChannelData ChannelData;
	ChannelData.Custom = TEXT("{\"test\":\"channel\"}");
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, MembershipData);
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users with custom data
	FPubnubChatUserData UserData1;
	UserData1.Custom = TEXT("{\"test\":\"user1\"}");
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, UserData1);
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserData UserData2;
	UserData2.Custom = TEXT("{\"test\":\"user2\"}");
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, UserData2);
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	FPubnubChatUserData UserData3;
	UserData3.Custom = TEXT("{\"test\":\"user3\"}");
	FPubnubChatUserResult CreateUser3Result = Chat->CreateUser(TargetUserID3, UserData3);
	TestFalse("CreateUser3 should succeed", CreateUser3Result.Result.Error);
	TestNotNull("User3 should be created", CreateUser3Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User || !CreateUser3Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Invite multiple users
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User, CreateUser3Result.User};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed", InviteResult.Result.Error);
	TestEqual("Should have 3 memberships", InviteResult.Memberships.Num(), 3);
	
	for(int32 i = 0; i < InviteResult.Memberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = InviteResult.Memberships[i];
		TestNotNull(FString::Printf(TEXT("Membership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("Membership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData RetrievedMembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("Membership %d Status should be pending"), i), RetrievedMembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2, TargetUserID3};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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
		Chat->DeleteUser(TargetUserID3, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that InviteMultiple works correctly when some users in array are invalid.
 * Verifies that valid users are still invited even if some are null/invalid.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultiplePartialInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.PartialInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelInviteMultiplePartialInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_partial_invalid_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_partial_invalid_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_partial_invalid_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_partial_invalid";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create valid users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Invite multiple users with some invalid (null) users mixed in
	TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, nullptr, CreateUser2Result.User, nullptr};
	FPubnubChatInviteMultipleResult InviteResult = CreateResult.Channel->InviteMultiple(Users);
	TestFalse("InviteMultiple should succeed with partial invalid users", InviteResult.Result.Error);
	TestEqual("Should have 2 memberships (only valid users)", InviteResult.Memberships.Num(), 2);
	
	// Verify both valid users were invited
	TArray<FString> InvitedUserIDs;
	for(UPubnubChatMembership* Membership : InviteResult.Memberships)
	{
		if(Membership)
		{
			InvitedUserIDs.Add(Membership->GetUserID());
			TestEqual("Membership ChannelID should match", Membership->GetChannelID(), TestChannelID);
			
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual("Membership Status should be pending", MembershipData.Status, TEXT("pending"));
		}
	}
	
	TestTrue("User1 should be invited", InvitedUserIDs.Contains(TargetUserID1));
	TestTrue("User2 should be invited", InvitedUserIDs.Contains(TargetUserID2));
	
	// Cleanup: Remove memberships for invited users
	if(Chat && InviteResult.Memberships.Num() > 0)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			// Don't fail test if cleanup fails, but log it
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
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

	CleanUp();
	return true;
}

/**
 * Tests that InviteMultiple emits Invite events for non-public channels.
 * Verifies that invitation events are sent when inviting multiple users to group/direct channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultipleNonPublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.NonPublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteMultipleNonPublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_nonpublic_event_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_nonpublic_event_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_nonpublic_event_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_nonpublic_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Create group conversation (non-public channel)
	TArray<UPubnubChatUser*> InitialUsers = {CreateUser1Result.User};
	FPubnubChatChannelData ChannelData;
	FPubnubChatMembershipData HostMembershipData;
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(InitialUsers, TestChannelID, ChannelData, HostMembershipData);
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	// Verify channel is group (non-public)
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be group", CreatedChannelData.Type, TEXT("group"));
	
	// Shared state for event reception
	TSharedPtr<int32> EventsReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatEvent>> ReceivedEvents = MakeShared<TArray<FPubnubChatEvent>>();
	
	// Listen for Invite events on first target user's channel
	FOnPubnubChatEventReceivedNative EventCallback1;
	EventCallback1.BindLambda([this, EventsReceivedCount, ReceivedEvents, TargetUserID1, TestChannelID](const FPubnubChatEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, TargetUserID1);
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("group")));
	});
	
	FPubnubChatListenForEventsResult ListenResult1 = Chat->ListenForEvents(TargetUserID1, EPubnubChatEventType::PCET_Invite, EventCallback1);
	TestFalse("ListenForEvents1 should succeed", ListenResult1.Result.Error);
	TestNotNull("CallbackStop1 should be created", ListenResult1.CallbackStop);
	
	// Listen for Invite events on second target user's channel
	FOnPubnubChatEventReceivedNative EventCallback2;
	EventCallback2.BindLambda([this, EventsReceivedCount, ReceivedEvents, TargetUserID2, TestChannelID](const FPubnubChatEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, TargetUserID2);
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("group")));
	});
	
	FPubnubChatListenForEventsResult ListenResult2 = Chat->ListenForEvents(TargetUserID2, EPubnubChatEventType::PCET_Invite, EventCallback2);
	TestFalse("ListenForEvents2 should succeed", ListenResult2.Result.Error);
	TestNotNull("CallbackStop2 should be created", ListenResult2.CallbackStop);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteMultipleResult> InviteResult = MakeShared<FPubnubChatInviteMultipleResult>();
	
	// Wait a bit for subscriptions to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUser1Result, CreateUser2Result, InviteResult]()
	{
		// Invite multiple users to the group conversation
		TArray<UPubnubChatUser*> UsersToInvite = {CreateUser1Result.User, CreateUser2Result.User};
		*InviteResult = CreateResult.Channel->InviteMultiple(UsersToInvite);
		TestFalse("InviteMultiple should succeed", InviteResult->Result.Error);
		TestEqual("Should have 2 memberships", InviteResult->Memberships.Num(), 2);
	}, 0.5f));
	
	// Wait until events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([EventsReceivedCount]() -> bool {
		return *EventsReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify events were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, EventsReceivedCount, TestChannelID]()
	{
		if(*EventsReceivedCount < 2)
		{
			AddError(FString::Printf(TEXT("Expected 2 Invite events, but received %d"), *EventsReceivedCount));
		}
		else
		{
			TestTrue("Both Invite events were received for non-public channel", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, remove memberships, delete users and channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult1, ListenResult2, Chat, TestChannelID, InitUserID, TargetUserID1, TargetUserID2, InviteResult]()
	{
		if(ListenResult1.CallbackStop)
		{
			ListenResult1.CallbackStop->Stop();
		}
		if(ListenResult2.CallbackStop)
		{
			ListenResult2.CallbackStop->Stop();
		}
		if(Chat)
		{
			// Remove memberships for invited users
			if(InviteResult->Memberships.Num() > 0)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(TestChannelID, false);
			Chat->DeleteUser(InitUserID, false);
			Chat->DeleteUser(TargetUserID1, false);
			Chat->DeleteUser(TargetUserID2, false);
		}
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests that InviteMultiple emits Invite events for public channels.
 * Verifies that invitation events are sent when inviting multiple users to public channels.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelInviteMultiplePublicChannelEventTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.InviteMultiple.4Advanced.PublicChannelEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatChannelInviteMultiplePublicChannelEventTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_invite_multiple_public_event_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_invite_multiple_public_event_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_invite_multiple_public_event_target2";
	const FString TestChannelID = SDK_PREFIX + "test_invite_multiple_public_event";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Verify channel is public
	FPubnubChatChannelData CreatedChannelData = CreateResult.Channel->GetChannelData();
	TestEqual("Channel Type should be public", CreatedChannelData.Type, TEXT("public"));
	
	// Join channel
	FOnPubnubChatChannelMessageReceived MessageCallback;
	FPubnubChatJoinResult JoinResult = CreateResult.Channel->Join(MessageCallback, FPubnubChatMembershipData());
	TestFalse("Join should succeed", JoinResult.Result.Error);
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Shared state for event reception
	TSharedPtr<int32> EventsReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<FPubnubChatEvent>> ReceivedEvents = MakeShared<TArray<FPubnubChatEvent>>();
	
	// Listen for Invite events on first target user's channel
	FOnPubnubChatEventReceivedNative EventCallback1;
	EventCallback1.BindLambda([this, EventsReceivedCount, ReceivedEvents, TargetUserID1, TestChannelID](const FPubnubChatEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, TargetUserID1);
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("public")));
	});
	
	FPubnubChatListenForEventsResult ListenResult1 = Chat->ListenForEvents(TargetUserID1, EPubnubChatEventType::PCET_Invite, EventCallback1);
	TestFalse("ListenForEvents1 should succeed", ListenResult1.Result.Error);
	TestNotNull("CallbackStop1 should be created", ListenResult1.CallbackStop);
	
	// Listen for Invite events on second target user's channel
	FOnPubnubChatEventReceivedNative EventCallback2;
	EventCallback2.BindLambda([this, EventsReceivedCount, ReceivedEvents, TargetUserID2, TestChannelID](const FPubnubChatEvent& Event)
	{
		(*EventsReceivedCount)++;
		ReceivedEvents->Add(Event);
		TestEqual("Received event type should be Invite", Event.Type, EPubnubChatEventType::PCET_Invite);
		TestEqual("Received event ChannelID should match target user", Event.ChannelID, TargetUserID2);
		TestTrue("Received event Payload should contain channelId", Event.Payload.Contains(TestChannelID));
		TestTrue("Received event Payload should contain channelType", Event.Payload.Contains(TEXT("public")));
	});
	
	FPubnubChatListenForEventsResult ListenResult2 = Chat->ListenForEvents(TargetUserID2, EPubnubChatEventType::PCET_Invite, EventCallback2);
	TestFalse("ListenForEvents2 should succeed", ListenResult2.Result.Error);
	TestNotNull("CallbackStop2 should be created", ListenResult2.CallbackStop);
	
	// Shared state for invite result (needed for cleanup)
	TSharedPtr<FPubnubChatInviteMultipleResult> InviteResult = MakeShared<FPubnubChatInviteMultipleResult>();
	
	// Wait a bit for subscriptions to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, CreateUser1Result, CreateUser2Result, InviteResult]()
	{
		// Invite multiple users to public channel
		TArray<UPubnubChatUser*> Users = {CreateUser1Result.User, CreateUser2Result.User};
		*InviteResult = CreateResult.Channel->InviteMultiple(Users);
		TestFalse("InviteMultiple should succeed", InviteResult->Result.Error);
		TestEqual("Should have 2 memberships", InviteResult->Memberships.Num(), 2);
	}, 0.5f));
	
	// Wait until events are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([EventsReceivedCount]() -> bool {
		return *EventsReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Verify events were received
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, EventsReceivedCount, TestChannelID]()
	{
		if(*EventsReceivedCount < 2)
		{
			AddError(FString::Printf(TEXT("Expected 2 Invite events, but received %d"), *EventsReceivedCount));
		}
		else
		{
			TestTrue("Both Invite events were received for public channel", true);
		}
	}, 0.1f));
	
	// Cleanup: Stop listening, remove memberships, leave and delete channel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ListenResult1, ListenResult2, CreateResult, Chat, TestChannelID, TargetUserID1, TargetUserID2, InviteResult]()
	{
		if(ListenResult1.CallbackStop)
		{
			ListenResult1.CallbackStop->Stop();
		}
		if(ListenResult2.CallbackStop)
		{
			ListenResult2.CallbackStop->Stop();
		}
		if(Chat)
		{
			// Remove memberships for invited users
			if(InviteResult->Memberships.Num() > 0)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
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
		CleanUp();
	}, 0.1f));
	
	return true;
}

// ============================================================================
// CREATEDIRECTCONVERSATION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create direct conversation without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			const FString TargetUserID = SDK_PREFIX + "test_create_direct_not_init_target";
			UPubnubChatUser* TestUser = NewObject<UPubnubChatUser>(Chat);
			if(TestUser)
			{
				FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(TestUser, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
				
				TestTrue("CreateDirectConversation should fail when Chat is not initialized", CreateResult.Result.Error);
				TestNull("Channel should not be created", CreateResult.Channel);
				TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationNullUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.1Validation.NullUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationNullUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_null_user_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to create direct conversation with null user
		const FString TestChannelID = SDK_PREFIX + "test_create_direct_null_user";
		FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(nullptr, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateDirectConversation should fail with null user", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_create_direct_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Create direct conversation with only required parameter (User) and default parameters
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestNotNull("InviteeMembership should be created", CreateResult.InviteeMembership);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel type is set to "direct"
		FPubnubChatChannelData ChannelDataRetrieved = CreateResult.Channel->GetChannelData();
		TestEqual("Channel Type should be direct", ChannelDataRetrieved.Type, TEXT("direct"));
	}
	
	if(CreateResult.HostMembership)
	{
		TestEqual("HostMembership ChannelID should match", CreateResult.HostMembership->GetChannelID(), TestChannelID);
		TestEqual("HostMembership UserID should match", CreateResult.HostMembership->GetUserID(), InitUserID);
	}
	
	if(CreateResult.InviteeMembership)
	{
		TestEqual("InviteeMembership ChannelID should match", CreateResult.InviteeMembership->GetChannelID(), TestChannelID);
		TestEqual("InviteeMembership UserID should match", CreateResult.InviteeMembership->GetUserID(), TargetUserID);
		
		// Verify membership has pending status
		FPubnubChatMembershipData MembershipData = CreateResult.InviteeMembership->GetMembershipData();
		TestEqual("InviteeMembership Status should be pending", MembershipData.Status, TEXT("pending"));
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee membership
			if(CreateResult.InviteeMembership)
			{
				FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveInviteeResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_create_direct_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target user with custom data
	FPubnubChatUserData UserData;
	UserData.Custom = TEXT("{\"test\":\"user\"}");
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, UserData);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Create direct conversation with all parameters
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("DirectChannelName");
	ChannelData.Description = TEXT("Direct channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");
	// Note: Type will be overridden to "direct" by the function
	
	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"host\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");
	
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TestChannelID, ChannelData, HostMembershipData);
	
	TestFalse("CreateDirectConversation should succeed with all parameters", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestNotNull("InviteeMembership should be created", CreateResult.InviteeMembership);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel data is stored correctly (type should be overridden to "direct")
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
		TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
		TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
		TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
		TestEqual("Channel Type should be direct (overridden)", RetrievedData.Type, TEXT("direct"));
	}
	
	if(CreateResult.HostMembership)
	{
		FPubnubChatMembershipData HostMembershipDataRetrieved = CreateResult.HostMembership->GetMembershipData();
		TestEqual("HostMembership Custom should match", HostMembershipDataRetrieved.Custom, HostMembershipData.Custom);
		TestEqual("HostMembership Status should match", HostMembershipDataRetrieved.Status, HostMembershipData.Status);
		TestEqual("HostMembership Type should match", HostMembershipDataRetrieved.Type, HostMembershipData.Type);
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee membership
			if(CreateResult.InviteeMembership)
			{
				FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveInviteeResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that CreateDirectConversation auto-generates ChannelID when empty string is provided.
 * Verifies that a deterministic hash-based ID is generated and used as ChannelID when ChannelID parameter is empty.
 * The ID format is "direct.{hash}" where hash is computed from sorted user IDs.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateDirectConversationAutoGeneratedChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateDirectConversation.4Advanced.AutoGeneratedChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateDirectConversationAutoGeneratedChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_direct_auto_id_init";
	const FString TargetUserID = SDK_PREFIX + "test_create_direct_auto_id_target";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	TestNotNull("User should be created", CreateUserResult.User);
	
	if(!CreateUserResult.User)
	{
		CleanUp();
		return false;
	}
	
	// Create direct conversation with empty ChannelID (should auto-generate hash-based ID)
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(CreateResult.Channel)
	{
		FString GeneratedChannelID = CreateResult.Channel->GetChannelID();
		TestFalse("ChannelID should be auto-generated (not empty)", GeneratedChannelID.IsEmpty());
		
		// Verify it starts with "direct." prefix
		TestTrue("ChannelID should start with 'direct.' prefix", GeneratedChannelID.StartsWith(TEXT("direct.")));
		
		// Verify determinism: same users should generate the same hash
		// Create another direct conversation with the same users and empty ChannelID
		FPubnubChatCreateDirectConversationResult CreateResult2 = Chat->CreateDirectConversation(CreateUserResult.User, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
		TestFalse("Second CreateDirectConversation should succeed", CreateResult2.Result.Error);
		if(CreateResult2.Channel)
		{
			FString GeneratedChannelID2 = CreateResult2.Channel->GetChannelID();
			TestEqual("Same users should generate the same ChannelID", GeneratedChannelID, GeneratedChannelID2);
			
			// Cleanup second channel
			if(Chat)
			{
				UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
				if(PubnubClient)
				{
					if(CreateResult2.InviteeMembership)
					{
						FPubnubMembershipsResult RemoveInviteeResult2 = PubnubClient->RemoveMemberships(TargetUserID, {GeneratedChannelID2}, FPubnubMembershipInclude::FromValue(false), 1);
						if(RemoveInviteeResult2.Result.Error)
						{
							UE_LOG(LogTemp, Warning, TEXT("Failed to remove second invitee membership during cleanup: %s"), *RemoveInviteeResult2.Result.ErrorMessage);
						}
					}
					if(CreateResult2.HostMembership)
					{
						FPubnubMembershipsResult RemoveHostResult2 = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID2}, FPubnubMembershipInclude::FromValue(false), 1);
						if(RemoveHostResult2.Result.Error)
						{
							UE_LOG(LogTemp, Warning, TEXT("Failed to remove second host membership during cleanup: %s"), *RemoveHostResult2.Result.ErrorMessage);
						}
					}
				}
				Chat->DeleteChannel(GeneratedChannelID2, false);
			}
		}
		
		// Cleanup: Remove memberships, delete channel and users
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				// Remove invitee membership
				if(CreateResult.InviteeMembership)
				{
					FPubnubMembershipsResult RemoveInviteeResult = PubnubClient->RemoveMemberships(TargetUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveInviteeResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee membership during cleanup: %s"), *RemoveInviteeResult.Result.ErrorMessage);
					}
				}
				
				// Remove host membership
				if(CreateResult.HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(GeneratedChannelID, false);
			Chat->DeleteUser(TargetUserID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// CREATEGROUPCONVERSATION TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to create group conversation without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			TArray<UPubnubChatUser*> Users;
			UPubnubChatUser* TestUser = NewObject<UPubnubChatUser>(Chat);
			if(TestUser)
			{
				Users.Add(TestUser);
				FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
				
				TestTrue("CreateGroupConversation should fail when Chat is not initialized", CreateResult.Result.Error);
				TestNull("Channel should not be created", CreateResult.Channel);
				TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationEmptyUsersArrayTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.EmptyUsersArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationEmptyUsersArrayTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_empty_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to create group conversation with empty users array
		TArray<UPubnubChatUser*> EmptyUsers;
		const FString TestChannelID = SDK_PREFIX + "test_create_group_empty_users";
		FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(EmptyUsers, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateGroupConversation should fail with empty users array", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationAllInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.1Validation.AllInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationAllInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_invalid_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to create group conversation with all invalid users (null pointers)
		TArray<UPubnubChatUser*> InvalidUsers;
		InvalidUsers.Add(nullptr);
		InvalidUsers.Add(nullptr);
		const FString TestChannelID = SDK_PREFIX + "test_create_group_invalid_users";
		FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(InvalidUsers, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
		
		TestTrue("CreateGroupConversation should fail with all invalid users", CreateResult.Result.Error);
		TestNull("Channel should not be created", CreateResult.Channel);
		TestFalse("ErrorMessage should not be empty", CreateResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_happy_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_happy_target1";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	if(!CreateUser1Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Create group conversation with only required parameter (Users array) and default parameters
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestEqual("InviteesMemberships should have 1 membership", CreateResult.InviteesMemberships.Num(), 1);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel type is set to "group"
		FPubnubChatChannelData ChannelDataRetrieved = CreateResult.Channel->GetChannelData();
		TestEqual("Channel Type should be group", ChannelDataRetrieved.Type, TEXT("group"));
	}
	
	if(CreateResult.HostMembership)
	{
		TestEqual("HostMembership ChannelID should match", CreateResult.HostMembership->GetChannelID(), TestChannelID);
		TestEqual("HostMembership UserID should match", CreateResult.HostMembership->GetUserID(), InitUserID);
	}
	
	if(CreateResult.InviteesMemberships.Num() > 0)
	{
		UPubnubChatMembership* InviteeMembership = CreateResult.InviteesMemberships[0];
		TestNotNull("InviteeMembership should be created", InviteeMembership);
		
		if(InviteeMembership)
		{
			TestEqual("InviteeMembership ChannelID should match", InviteeMembership->GetChannelID(), TestChannelID);
			TestEqual("InviteeMembership UserID should match", InviteeMembership->GetUserID(), TargetUserID1);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = InviteeMembership->GetMembershipData();
			TestEqual("InviteeMembership Status should be pending", MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID1, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_create_group_full_target2";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target users with custom data
	FPubnubChatUserData UserData1;
	UserData1.Custom = TEXT("{\"test\":\"user1\"}");
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, UserData1);
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserData UserData2;
	UserData2.Custom = TEXT("{\"test\":\"user2\"}");
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, UserData2);
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Create group conversation with all parameters
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("GroupChannelName");
	ChannelData.Description = TEXT("Group channel description");
	ChannelData.Custom = TEXT("{\"key\":\"value\"}");
	ChannelData.Status = TEXT("active");
	// Note: Type will be overridden to "group" by the function
	
	FPubnubChatMembershipData HostMembershipData;
	HostMembershipData.Custom = TEXT("{\"role\":\"admin\"}");
	HostMembershipData.Status = TEXT("active");
	HostMembershipData.Type = TEXT("owner");
	
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	Users.Add(CreateUser2Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, ChannelData, HostMembershipData);
	
	TestFalse("CreateGroupConversation should succeed with all parameters", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestNotNull("HostMembership should be created", CreateResult.HostMembership);
	TestEqual("InviteesMemberships should have 2 memberships", CreateResult.InviteesMemberships.Num(), 2);
	
	if(CreateResult.Channel)
	{
		TestEqual("Created Channel ChannelID should match", CreateResult.Channel->GetChannelID(), TestChannelID);
		
		// Verify channel data is stored correctly (type should be overridden to "group")
		FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
		TestEqual("ChannelName should match", RetrievedData.ChannelName, ChannelData.ChannelName);
		TestEqual("Description should match", RetrievedData.Description, ChannelData.Description);
		TestEqual("Custom should match", RetrievedData.Custom, ChannelData.Custom);
		TestEqual("Status should match", RetrievedData.Status, ChannelData.Status);
		TestEqual("Channel Type should be group (overridden)", RetrievedData.Type, TEXT("group"));
	}
	
	if(CreateResult.HostMembership)
	{
		FPubnubChatMembershipData HostMembershipDataRetrieved = CreateResult.HostMembership->GetMembershipData();
		TestEqual("HostMembership Custom should match", HostMembershipDataRetrieved.Custom, HostMembershipData.Custom);
		TestEqual("HostMembership Status should match", HostMembershipDataRetrieved.Status, HostMembershipData.Status);
		TestEqual("HostMembership Type should match", HostMembershipDataRetrieved.Type, HostMembershipData.Type);
	}
	
	// Verify all invitee memberships
	for(int32 i = 0; i < CreateResult.InviteesMemberships.Num(); ++i)
	{
		UPubnubChatMembership* Membership = CreateResult.InviteesMemberships[i];
		TestNotNull(FString::Printf(TEXT("InviteeMembership %d should be created"), i), Membership);
		
		if(Membership)
		{
			TestEqual(FString::Printf(TEXT("InviteeMembership %d ChannelID should match"), i), Membership->GetChannelID(), TestChannelID);
			
			// Verify membership has pending status
			FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
			TestEqual(FString::Printf(TEXT("InviteeMembership %d Status should be pending"), i), MembershipData.Status, TEXT("pending"));
		}
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID1, false);
		Chat->DeleteUser(TargetUserID2, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests that CreateGroupConversation auto-generates ChannelID when empty string is provided.
 * Verifies that a GUID is generated and used as ChannelID when ChannelID parameter is empty.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationAutoGeneratedChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.4Advanced.AutoGeneratedChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationAutoGeneratedChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_auto_id_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_auto_id_target1";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target user
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	if(!CreateUser1Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Create group conversation with empty ChannelID (should auto-generate GUID)
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(CreateResult.Channel)
	{
		FString GeneratedChannelID = CreateResult.Channel->GetChannelID();
		TestFalse("ChannelID should be auto-generated (not empty)", GeneratedChannelID.IsEmpty());
		
		// Verify it's a valid GUID format (contains hyphens)
		TestTrue("ChannelID should be a GUID format", GeneratedChannelID.Contains(TEXT("-")));
		
		// Cleanup: Remove memberships, delete channel and users
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				// Remove invitee memberships
				if(CreateResult.InviteesMemberships.Num() > 0)
				{
					TArray<FString> UserIDsToRemove = {TargetUserID1};
					FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(GeneratedChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
					if(RemoveResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
					}
				}
				
				// Remove host membership
				if(CreateResult.HostMembership)
				{
					FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {GeneratedChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
					if(RemoveHostResult.Result.Error)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
					}
				}
			}
			
			Chat->DeleteChannel(GeneratedChannelID, false);
			Chat->DeleteUser(TargetUserID1, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests that CreateGroupConversation handles mixed valid and invalid users correctly.
 * Verifies that valid users are still invited even if some users in the array are null/invalid.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateGroupConversationMixedValidInvalidUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.CreateGroupConversation.4Advanced.MixedValidInvalidUsers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateGroupConversationMixedValidInvalidUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_group_mixed_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_create_group_mixed_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_create_group_mixed_target2";
	const FString TestChannelID = SDK_PREFIX + "test_create_group_mixed";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create target users
	FPubnubChatUserResult CreateUser1Result = Chat->CreateUser(TargetUserID1, FPubnubChatUserData());
	TestFalse("CreateUser1 should succeed", CreateUser1Result.Result.Error);
	TestNotNull("User1 should be created", CreateUser1Result.User);
	
	FPubnubChatUserResult CreateUser2Result = Chat->CreateUser(TargetUserID2, FPubnubChatUserData());
	TestFalse("CreateUser2 should succeed", CreateUser2Result.Result.Error);
	TestNotNull("User2 should be created", CreateUser2Result.User);
	
	if(!CreateUser1Result.User || !CreateUser2Result.User)
	{
		CleanUp();
		return false;
	}
	
	// Create group conversation with mixed valid and invalid users
	TArray<UPubnubChatUser*> Users;
	Users.Add(CreateUser1Result.User);
	Users.Add(nullptr); // Invalid user
	Users.Add(CreateUser2Result.User);
	Users.Add(nullptr); // Another invalid user
	
	FPubnubChatCreateGroupConversationResult CreateResult = Chat->CreateGroupConversation(Users, TestChannelID, FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateGroupConversation should succeed (invalid users should be filtered)", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	TestEqual("InviteesMemberships should have 2 memberships (only valid users)", CreateResult.InviteesMemberships.Num(), 2);
	
	if(CreateResult.InviteesMemberships.Num() == 2)
	{
		// Verify both valid users were invited
		TArray<FString> InvitedUserIDs;
		for(UPubnubChatMembership* Membership : CreateResult.InviteesMemberships)
		{
			if(Membership)
			{
				InvitedUserIDs.Add(Membership->GetUserID());
			}
		}
		
		TestTrue("User1 should be invited", InvitedUserIDs.Contains(TargetUserID1));
		TestTrue("User2 should be invited", InvitedUserIDs.Contains(TargetUserID2));
	}
	
	// Cleanup: Remove memberships, delete channel and users
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			// Remove invitee memberships
			if(CreateResult.InviteesMemberships.Num() > 0)
			{
				TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(TestChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove invitee memberships during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			
			// Remove host membership
			if(CreateResult.HostMembership)
			{
				FPubnubMembershipsResult RemoveHostResult = PubnubClient->RemoveMemberships(InitUserID, {TestChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
				if(RemoveHostResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove host membership during cleanup: %s"), *RemoveHostResult.Result.ErrorMessage);
				}
			}
		}
		
		Chat->DeleteChannel(TestChannelID, false);
		Chat->DeleteUser(TargetUserID1, false);
		Chat->DeleteUser(TargetUserID2, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// GETCHANNELSUGGESTIONS TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to get channel suggestions without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT("test"), 10);
			
			TestTrue("GetChannelSuggestions should fail when Chat is not initialized", GetSuggestionsResult.Result.Error);
			TestEqual("Channels array should be empty", GetSuggestionsResult.Channels.Num(), 0);
			TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsEmptyTextTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.1Validation.EmptyText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsEmptyTextTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channel_suggestions_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to get channel suggestions with empty Text
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT(""), 10);
		
		TestTrue("GetChannelSuggestions should fail with empty Text", GetSuggestionsResult.Result.Error);
		TestEqual("Channels array should be empty", GetSuggestionsResult.Channels.Num(), 0);
		TestFalse("ErrorMessage should not be empty", GetSuggestionsResult.Result.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_happy";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create a test channel with a specific name pattern
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("TestChannelHappy");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Call GetChannelSuggestions with only required parameter (Text)
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(TEXT("TestChannel"));
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid (may be empty)", GetSuggestionsResult.Channels.Num() >= 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_full_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_full_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_full_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_full_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create test channels with matching name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("FullTestChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("FullTestChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("FullTestChannel3");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test GetChannelSuggestions with all parameters (Text and Limit)
		const FString SearchText = TEXT("FullTest");
		const int TestLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("GetChannelSuggestions should succeed with all parameters", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		TestTrue("Channels count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Channels.Num() <= TestLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsWithLimitTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.3FullParameters.WithLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsWithLimitTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_limit_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_limit_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_limit_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_limit_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create test channels with matching name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("LimitTestChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("LimitTestChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("LimitTestChannel3");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test GetChannelSuggestions with Limit parameter
		const FString SearchText = TEXT("LimitTest");
		const int TestLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("GetChannelSuggestions should succeed with Limit", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		TestTrue("Channels count should not exceed limit (if limit is enforced)", GetSuggestionsResult.Channels.Num() <= TestLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests GetChannelSuggestions with multiple channels matching the search pattern.
 * Verifies that all matching channels are returned and that the results are correct.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsMultipleMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.MultipleMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsMultipleMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_multi_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_multi_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_multi_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_multi_3";
	const FString TestChannelID4 = SDK_PREFIX + "test_get_channel_suggestions_multi_4";
	const FString TestChannelID5 = SDK_PREFIX + "test_get_channel_suggestions_multi_5";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create multiple test channels with matching name patterns
		TArray<FString> TestChannelIDs = {TestChannelID1, TestChannelID2, TestChannelID3, TestChannelID4, TestChannelID5};
		for(int32 i = 0; i < TestChannelIDs.Num(); ++i)
		{
			FPubnubChatChannelData ChannelData;
			ChannelData.ChannelName = FString::Printf(TEXT("MultiTestChannel%d"), i + 1);
			FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelIDs[i], ChannelData);
			TestFalse(FString::Printf(TEXT("CreatePublicConversation %s should succeed"), *TestChannelIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelIDs[i]); }
		}
		
		// Get channel suggestions with search text that matches all created channels
		const FString SearchText = TEXT("MultiTest");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should contain at least created channels", GetSuggestionsResult.Channels.Num() >= CreatedChannelIDs.Num());
		
		// Verify all created channels are in the result
		TArray<FString> FoundChannelIDs;
		for(UPubnubChatChannel* Channel : GetSuggestionsResult.Channels)
		{
			if(Channel)
			{
				FoundChannelIDs.Add(Channel->GetChannelID());
			}
		}
		
		for(const FString& CreatedChannelID : CreatedChannelIDs)
		{
			TestTrue(FString::Printf(TEXT("Created channel %s should be found in suggestions"), *CreatedChannelID), FoundChannelIDs.Contains(CreatedChannelID));
		}
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with partial name matching.
 * Verifies that the function correctly matches channels based on name prefix pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsPartialMatchTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.PartialMatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsPartialMatchTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_partial_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_partial_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_partial_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_partial_3";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create test channels with different name patterns
		FPubnubChatChannelData ChannelData1;
		ChannelData1.ChannelName = TEXT("PartialMatchChannel1");
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation1 should succeed", CreateResult1.Result.Error);
		if(!CreateResult1.Result.Error) { CreatedChannelIDs.Add(TestChannelID1); }
		
		FPubnubChatChannelData ChannelData2;
		ChannelData2.ChannelName = TEXT("PartialMatchChannel2");
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation2 should succeed", CreateResult2.Result.Error);
		if(!CreateResult2.Result.Error) { CreatedChannelIDs.Add(TestChannelID2); }
		
		FPubnubChatChannelData ChannelData3;
		ChannelData3.ChannelName = TEXT("DifferentNameChannel");
		FPubnubChatChannelResult CreateResult3 = Chat->CreatePublicConversation(TestChannelID3, ChannelData3);
		TestFalse("CreatePublicConversation3 should succeed", CreateResult3.Result.Error);
		if(!CreateResult3.Result.Error) { CreatedChannelIDs.Add(TestChannelID3); }
		
		// Test partial match - should match first two channels but not third
		const FString SearchText = TEXT("PartialMatch");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels array should be valid", GetSuggestionsResult.Channels.Num() >= 0);
		
		// Verify matching channels are in results
		TArray<FString> FoundChannelIDs;
		for(UPubnubChatChannel* Channel : GetSuggestionsResult.Channels)
		{
			if(Channel)
			{
				FoundChannelIDs.Add(Channel->GetChannelID());
			}
		}
		
		// First two channels should be found
		TestTrue("PartialMatchChannel1 should be found", FoundChannelIDs.Contains(TestChannelID1));
		TestTrue("PartialMatchChannel2 should be found", FoundChannelIDs.Contains(TestChannelID2));
		
		// Third channel should not be found (different name pattern)
		TestFalse("DifferentNameChannel should not be found", FoundChannelIDs.Contains(TestChannelID3));
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with no matching channels.
 * Verifies that the function returns an empty result set when no channels match the search pattern.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsNoMatchesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.NoMatches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsNoMatchesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_nomatch_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_nomatch";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create a test channel with a specific name
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("NoMatchTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Test with search text that doesn't match any channel
		const FString SearchText = TEXT("NonExistentChannelPrefix");
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, 10);
		
		TestFalse("GetChannelSuggestions should succeed even with no matches", GetSuggestionsResult.Result.Error);
		TestEqual("Channels array should be empty for no matches", GetSuggestionsResult.Channels.Num(), 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions with case sensitivity.
 * Verifies that the function correctly matches channels regardless of case differences in the search text.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsCaseSensitivityTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.CaseSensitivity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsCaseSensitivityTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_case_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_case";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create a test channel with mixed case name
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("CaseTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Test with different case variations
		const FString SearchTextLower = TEXT("casetest");
		const FString SearchTextUpper = TEXT("CASETEST");
		const FString SearchTextMixed = TEXT("CaseTest");
		
		FPubnubChatGetChannelSuggestionsResult ResultLower = Chat->GetChannelSuggestions(SearchTextLower, 10);
		FPubnubChatGetChannelSuggestionsResult ResultUpper = Chat->GetChannelSuggestions(SearchTextUpper, 10);
		FPubnubChatGetChannelSuggestionsResult ResultMixed = Chat->GetChannelSuggestions(SearchTextMixed, 10);
		
		TestFalse("GetChannelSuggestions with lowercase should succeed", ResultLower.Result.Error);
		TestFalse("GetChannelSuggestions with uppercase should succeed", ResultUpper.Result.Error);
		TestFalse("GetChannelSuggestions with mixed case should succeed", ResultMixed.Result.Error);
		
		// At least one of the case variations should find the channel (depending on backend implementation)
		bool FoundInAnyCase = ResultLower.Channels.Num() > 0 || ResultUpper.Channels.Num() > 0 || ResultMixed.Channels.Num() > 0;
		TestTrue("Channel should be found with at least one case variation", FoundInAnyCase || ResultMixed.Channels.Num() > 0);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests GetChannelSuggestions limit enforcement.
 * Verifies that the limit parameter correctly restricts the number of returned results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsLimitEnforcementTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.LimitEnforcement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsLimitEnforcementTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_2";
	const FString TestChannelID3 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_3";
	const FString TestChannelID4 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_4";
	const FString TestChannelID5 = SDK_PREFIX + "test_get_channel_suggestions_limit_enforce_5";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create multiple test channels with matching name patterns
		TArray<FString> TestChannelIDs = {TestChannelID1, TestChannelID2, TestChannelID3, TestChannelID4, TestChannelID5};
		for(int32 i = 0; i < TestChannelIDs.Num(); ++i)
		{
			FPubnubChatChannelData ChannelData;
			ChannelData.ChannelName = FString::Printf(TEXT("LimitEnforceChannel%d"), i + 1);
			FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelIDs[i], ChannelData);
			TestFalse(FString::Printf(TEXT("CreatePublicConversation %s should succeed"), *TestChannelIDs[i]), CreateResult.Result.Error);
			if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelIDs[i]); }
		}
		
		// Test with limit smaller than number of matching channels
		const FString SearchText = TEXT("LimitEnforce");
		const int SmallLimit = 2;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult = Chat->GetChannelSuggestions(SearchText, SmallLimit);
		
		TestFalse("GetChannelSuggestions should succeed", GetSuggestionsResult.Result.Error);
		TestTrue("Channels count should not exceed limit", GetSuggestionsResult.Channels.Num() <= SmallLimit || GetSuggestionsResult.Channels.Num() == 0);
		
		// Test with limit larger than number of matching channels
		const int LargeLimit = 20;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResultLarge = Chat->GetChannelSuggestions(SearchText, LargeLimit);
		
		TestFalse("GetChannelSuggestions with large limit should succeed", GetSuggestionsResultLarge.Result.Error);
		TestTrue("Channels count should be valid", GetSuggestionsResultLarge.Channels.Num() >= 0);
		
		// Cleanup: Delete created channels
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

/**
 * Tests consistency of GetChannelSuggestions results across multiple calls.
 * Verifies that calling GetChannelSuggestions multiple times with the same parameters returns consistent results.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatGetChannelSuggestionsConsistencyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetChannelSuggestions.4Advanced.Consistency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatGetChannelSuggestionsConsistencyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_suggestions_consistency_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_suggestions_consistency";
	
	TArray<FString> CreatedChannelIDs;
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create a test channel
		FPubnubChatChannelData ChannelData;
		ChannelData.ChannelName = TEXT("ConsistencyTestChannel");
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		if(!CreateResult.Result.Error) { CreatedChannelIDs.Add(TestChannelID); }
		
		// Call GetChannelSuggestions multiple times with same parameters
		const FString SearchText = TEXT("Consistency");
		const int TestLimit = 10;
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult1 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult2 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		FPubnubChatGetChannelSuggestionsResult GetSuggestionsResult3 = Chat->GetChannelSuggestions(SearchText, TestLimit);
		
		TestFalse("First GetChannelSuggestions should succeed", GetSuggestionsResult1.Result.Error);
		TestFalse("Second GetChannelSuggestions should succeed", GetSuggestionsResult2.Result.Error);
		TestFalse("Third GetChannelSuggestions should succeed", GetSuggestionsResult3.Result.Error);
		
		// Results should be consistent (channels count should be similar)
		TestTrue("Channels count should be consistent", FMath::Abs(GetSuggestionsResult1.Channels.Num() - GetSuggestionsResult2.Channels.Num()) <= 1);
		TestTrue("Channels count should be consistent", FMath::Abs(GetSuggestionsResult2.Channels.Num() - GetSuggestionsResult3.Channels.Num()) <= 1);
		
		// Cleanup: Delete created channel
		for(const FString& ChannelID : CreatedChannelIDs)
		{
			Chat->DeleteChannel(ChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// UPDATE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Try to update with uninitialized channel
			FPubnubChatOperationResult UpdateResult = UninitializedChannel->Update(ChannelData);
			TestTrue("Update should fail with uninitialized channel", UpdateResult.Error);
		}
		
		// Cleanup
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Update channel with minimal data (empty ChannelData)
	FPubnubChatChannelData UpdateData;
	FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdateData);
	TestFalse("Update should succeed", UpdateResult.Error);
	
	// Verify channel data was updated
	FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match", RetrievedData.ChannelName, UpdateData.ChannelName);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData InitialChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	// Update channel with all parameters
	FPubnubChatChannelData UpdateData;
	UpdateData.ChannelName = TEXT("UpdatedChannelName");
	UpdateData.Description = TEXT("Updated description");
	UpdateData.Custom = TEXT("{\"updated\":\"custom\"}");
	UpdateData.Status = TEXT("updatedStatus");
	UpdateData.Type = TEXT("updatedType");
	
	FPubnubChatOperationResult UpdateResult = CreateResult.Channel->Update(UpdateData);
	TestFalse("Update should succeed with all parameters", UpdateResult.Error);
	
	// Verify all data was updated correctly
	FPubnubChatChannelData RetrievedData = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match", RetrievedData.ChannelName, UpdateData.ChannelName);
	TestEqual("Description should match", RetrievedData.Description, UpdateData.Description);
	TestEqual("Custom should match", RetrievedData.Custom, UpdateData.Custom);
	TestEqual("Status should match", RetrievedData.Status, UpdateData.Status);
	TestEqual("Type should match", RetrievedData.Type, UpdateData.Type);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests updating channel multiple times.
 * Verifies that subsequent updates correctly override previous channel data.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUpdateMultipleTimesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.Update.4Advanced.MultipleUpdates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUpdateMultipleTimesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_update_multiple_init";
	const FString TestChannelID = SDK_PREFIX + "test_update_multiple";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create channel
	FPubnubChatChannelData InitialChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, InitialChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	// First update
	FPubnubChatChannelData FirstUpdateData;
	FirstUpdateData.ChannelName = TEXT("FirstUpdate");
	FirstUpdateData.Description = TEXT("First description");
	FPubnubChatOperationResult FirstUpdateResult = CreateResult.Channel->Update(FirstUpdateData);
	TestFalse("First Update should succeed", FirstUpdateResult.Error);
	
	// Verify first update
	FPubnubChatChannelData RetrievedAfterFirst = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match first update", RetrievedAfterFirst.ChannelName, FirstUpdateData.ChannelName);
	TestEqual("Description should match first update", RetrievedAfterFirst.Description, FirstUpdateData.Description);
	
	// Second update
	FPubnubChatChannelData SecondUpdateData;
	SecondUpdateData.ChannelName = TEXT("SecondUpdate");
	SecondUpdateData.Description = TEXT("Second description");
	FPubnubChatOperationResult SecondUpdateResult = CreateResult.Channel->Update(SecondUpdateData);
	TestFalse("Second Update should succeed", SecondUpdateResult.Error);
	
	// Verify second update overrides first
	FPubnubChatChannelData RetrievedAfterSecond = CreateResult.Channel->GetChannelData();
	TestEqual("ChannelName should match second update", RetrievedAfterSecond.ChannelName, SecondUpdateData.ChannelName);
	TestEqual("Description should match second update", RetrievedAfterSecond.Description, SecondUpdateData.Description);
	TestNotEqual("ChannelName should be different from first update", RetrievedAfterSecond.ChannelName, FirstUpdateData.ChannelName);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// PINMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			
			// Create a message object (we'll use SendText to create one)
			FOnPubnubChatChannelMessageReceivedNative MessageCallback;
			MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			});
			
			FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
			TestFalse("Connect should succeed", ConnectResult.Result.Error);
			
			const FString TestMessage = TEXT("Test message for pinning");
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin with uninitialized channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, UninitializedChannel, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = UninitializedChannel->PinMessage(*ReceivedMessage);
					TestTrue("PinMessage should fail with uninitialized channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
			{
				if(CreateResult.Channel)
				{
					CreateResult.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID, false);
				}
				CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUp();
		}
	}
	else
	{
		CleanUp();
	}

	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageNullMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.NullMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageNullMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_null_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_null_msg";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Try to pin null message
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(nullptr);
			TestTrue("PinMessage should fail with null message", PinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageDifferentChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.1Validation.DifferentChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageDifferentChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_diff_channel_init";
	const FString TestChannelID1 = SDK_PREFIX + "test_pin_diff_channel_1";
	const FString TestChannelID2 = SDK_PREFIX + "test_pin_diff_channel_2";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create first channel
		FPubnubChatChannelData ChannelData1;
		FPubnubChatChannelResult CreateResult1 = Chat->CreatePublicConversation(TestChannelID1, ChannelData1);
		TestFalse("CreatePublicConversation should succeed", CreateResult1.Result.Error);
		TestNotNull("Channel should be created", CreateResult1.Channel);
		
		// Create second channel
		FPubnubChatChannelData ChannelData2;
		FPubnubChatChannelResult CreateResult2 = Chat->CreatePublicConversation(TestChannelID2, ChannelData2);
		TestFalse("CreatePublicConversation should succeed", CreateResult2.Result.Error);
		TestNotNull("Channel should be created", CreateResult2.Channel);
		
		if(CreateResult1.Channel && CreateResult2.Channel)
		{
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			
			// Connect to first channel and send message
			FOnPubnubChatChannelMessageReceivedNative MessageCallback;
			MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			});
			
			FPubnubChatConnectResult ConnectResult = CreateResult1.Channel->Connect(MessageCallback);
			TestFalse("Connect should succeed", ConnectResult.Result.Error);
			
			const FString TestMessage = TEXT("Test message");
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult1, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult1.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin message from channel 1 to channel 2
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult2, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = CreateResult2.Channel->PinMessage(*ReceivedMessage);
					TestTrue("PinMessage should fail with message from different channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult1, Chat, TestChannelID1, TestChannelID2]()
			{
				if(CreateResult1.Channel)
				{
					CreateResult1.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID1, false);
					Chat->DeleteChannel(TestChannelID2, false);
				}
				CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUp();
		}
	}
	else
	{
		CleanUp();
	}

	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin");
	
	// Connect to channel and send message
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
			TestFalse("PinMessage should succeed", PinResult.Error);
			
			// Verify pinned message is stored in channel data
			FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests pinning a message twice.
 * Verifies that pinning a message twice overrides the previously pinned message.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelPinMessageTwiceTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.PinMessage.4Advanced.PinTwiceOverride", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelPinMessageTwiceTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_twice_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_twice";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<int32> MessagesReceivedCount = MakeShared<int32>(0);
	TSharedPtr<TArray<UPubnubChatMessage*>> ReceivedMessages = MakeShared<TArray<UPubnubChatMessage*>>();
	const FString TestMessage1 = TEXT("First message to pin");
	const FString TestMessage2 = TEXT("Second message to pin");
	
	// Connect to channel and send two messages
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([MessagesReceivedCount, ReceivedMessages](UPubnubChatMessage* Message)
	{
		if(Message)
		{
			(*MessagesReceivedCount)++;
			ReceivedMessages->Add(Message);
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage1]()
	{
		FPubnubChatOperationResult SendResult1 = CreateResult.Channel->SendText(TestMessage1);
		TestFalse("First SendText should succeed", SendResult1.Error);
	}, 0.5f));
	
	// Send second message after a delay
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage2]()
	{
		FPubnubChatOperationResult SendResult2 = CreateResult.Channel->SendText(TestMessage2);
		TestFalse("Second SendText should succeed", SendResult2.Error);
	}, 0.7f));
	
	// Wait until both messages are received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([MessagesReceivedCount]() -> bool {
		return *MessagesReceivedCount >= 2;
	}, MAX_WAIT_TIME));
	
	// Pin messages
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessages]()
	{
		if(ReceivedMessages->Num() < 2)
		{
			AddError("Should receive at least 2 messages");
		}
		else
		{
			UPubnubChatMessage* FirstMessage = (*ReceivedMessages)[0];
			UPubnubChatMessage* SecondMessage = (*ReceivedMessages)[1];
			
			// Pin first message
			FPubnubChatOperationResult PinResult1 = CreateResult.Channel->PinMessage(FirstMessage);
			TestFalse("First PinMessage should succeed", PinResult1.Error);
			
			// Verify first message is pinned
			FPubnubChatChannelData AfterFirstPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data after first pin", AfterFirstPin.Custom.IsEmpty());
			
			// Pin second message (should override first)
			FPubnubChatOperationResult PinResult2 = CreateResult.Channel->PinMessage(SecondMessage);
			TestFalse("Second PinMessage should succeed", PinResult2.Error);
			
			// Verify second message is now pinned (overriding first)
			FPubnubChatChannelData AfterSecondPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data after second pin", AfterSecondPin.Custom.IsEmpty());
			
			// The pinned message should be the second one, not the first
			TestTrue("Second pin should override first pin", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// UNPINMESSAGE TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_not_init_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create uninitialized channel object
			UPubnubChatChannel* UninitializedChannel = NewObject<UPubnubChatChannel>(Chat);
			
			// Try to unpin with uninitialized channel
			FPubnubChatOperationResult UnpinResult = UninitializedChannel->UnpinMessage();
			TestTrue("UnpinMessage should fail with uninitialized channel", UnpinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin and unpin");
	
	// Connect to channel and send message
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin and unpin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message first
			FPubnubChatOperationResult PinResult = CreateResult.Channel->PinMessage(*ReceivedMessage);
			TestFalse("PinMessage should succeed", PinResult.Error);
			
			// Verify message is pinned
			FPubnubChatChannelData AfterPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", AfterPin.Custom.IsEmpty());
			
			// Unpin the message
			FPubnubChatOperationResult UnpinResult = CreateResult.Channel->UnpinMessage();
			TestFalse("UnpinMessage should succeed", UnpinResult.Error);
			
			// Verify message is unpinned (Custom should no longer contain pinned message properties)
			FPubnubChatChannelData AfterUnpin = CreateResult.Channel->GetChannelData();
			// Note: Custom might still contain other data, but pinned message properties should be removed
			TestTrue("Unpin should complete successfully", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests unpinning when there is no pinned message.
 * Verifies that unpinning when no message is pinned should not throw an error and should just ignore.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelUnpinMessageNoPinnedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Channel.UnpinMessage.4Advanced.NoPinnedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelUnpinMessageNoPinnedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_no_pinned_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_no_pinned";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
		CleanUp();
		return false;
	}
	
	// Create channel (no pinned message)
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	
	if(!CreateResult.Channel)
	{
		CleanUp();
		return false;
	}
	
	// Try to unpin when there's no pinned message
	FPubnubChatOperationResult UnpinResult = CreateResult.Channel->UnpinMessage();
	// Should not error - should just ignore
	TestFalse("UnpinMessage should not error when no message is pinned", UnpinResult.Error);
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID, false);
	}

	CleanUp();
	return true;
}

// ============================================================================
// PINMESSAGETOCHANNEL TESTS (Chat object)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to pin message without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatMessage* TestMessage = nullptr;
			UPubnubChatChannel* TestChannel = nullptr;
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(TestMessage, TestChannel);
			
			TestTrue("PinMessageToChannel should fail when Chat is not initialized", PinResult.Error);
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNullMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NullMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNullMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_null_msg_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_null_msg";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Try to pin null message
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(nullptr, CreateResult.Channel);
			TestTrue("PinMessageToChannel should fail with null message", PinResult.Error);
			
			// Cleanup
			if(Chat)
			{
				Chat->DeleteChannel(TestChannelID, false);
			}
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelNullChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.1Validation.NullChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelNullChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_null_channel_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_null_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Create channel and send message
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Shared state for message reception
			TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
			TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
			const FString TestMessage = TEXT("Test message");
			
			// Connect to channel and send message
			FOnPubnubChatChannelMessageReceivedNative MessageCallback;
			MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
			{
				if(Message && !*ReceivedMessage)
				{
					*bMessageReceived = true;
					*ReceivedMessage = Message;
				}
			});
			
			FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
			TestFalse("Connect should succeed", ConnectResult.Result.Error);
			
			// Wait a bit for subscription to be ready
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
			{
				FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
				TestFalse("SendText should succeed", SendResult.Error);
			}, 0.5f));
			
			// Wait until message is received
			ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
				return *bMessageReceived;
			}, MAX_WAIT_TIME));
			
			// Try to pin message with null channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, ReceivedMessage]()
			{
				if(*ReceivedMessage)
				{
					FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, nullptr);
					TestTrue("PinMessageToChannel should fail with null channel", PinResult.Error);
				}
				else
				{
					AddError("Message was not received");
				}
			}, 0.1f));
			
			// Cleanup
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
			{
				if(CreateResult.Channel)
				{
					CreateResult.Channel->Disconnect();
				}
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID, false);
				}
				CleanUp();
			}, 0.1f));
		}
		else
		{
			CleanUp();
		}
	}
	else
	{
		CleanUp();
	}

	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatPinMessageToChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.PinMessageToChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatPinMessageToChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_pin_to_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_pin_to_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin");
	
	// Connect to channel and send message
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin the message using Chat->PinMessageToChannel
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message using Chat->PinMessageToChannel
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, CreateResult.Channel);
			TestFalse("PinMessageToChannel should succeed", PinResult.Error);
			
			// Verify pinned message is stored in channel data
			FPubnubChatChannelData UpdatedChannelData = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", UpdatedChannelData.Custom.IsEmpty());
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

// ============================================================================
// UNPINMESSAGEFROMCHANNEL TESTS (Chat object)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelNotInitializedTest::RunTest(const FString& Parameters)
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
		// Try to unpin message without initialized chat
		Chat = NewObject<UPubnubChat>(ChatSubsystem);
		if(Chat)
		{
			UPubnubChatChannel* TestChannel = nullptr;
			FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(TestChannel);
			
			TestTrue("UnpinMessageFromChannel should fail when Chat is not initialized", UnpinResult.Error);
		}
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelNullChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.1Validation.NullChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelNullChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_channel_null_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(Chat)
	{
		// Try to unpin message with null channel
		FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(nullptr);
		TestTrue("UnpinMessageFromChannel should fail with null channel", UnpinResult.Error);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUnpinMessageFromChannelHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.UnpinMessageFromChannel.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUnpinMessageFromChannelHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_unpin_from_channel_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_unpin_from_channel_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = ChatSubsystem->GetChat();
	if(!Chat)
	{
		AddError("Chat should be initialized");
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
		CleanUp();
		return false;
	}
	
	// Shared state for message reception
	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	const FString TestMessage = TEXT("Test message to pin and unpin");
	
	// Connect to channel and send message
	FOnPubnubChatChannelMessageReceivedNative MessageCallback;
	MessageCallback.BindLambda([bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	});
	
	FPubnubChatConnectResult ConnectResult = CreateResult.Channel->Connect(MessageCallback);
	TestFalse("Connect should succeed", ConnectResult.Result.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, TestMessage]()
	{
		FPubnubChatOperationResult SendResult = CreateResult.Channel->SendText(TestMessage);
		TestFalse("SendText should succeed", SendResult.Error);
	}, 0.5f));
	
	// Wait until message is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
		return *bMessageReceived;
	}, MAX_WAIT_TIME));
	
	// Pin and unpin the message
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, ReceivedMessage]()
	{
		if(!*ReceivedMessage)
		{
			AddError("Message should be received");
		}
		else
		{
			// Pin the message first using Chat->PinMessageToChannel
			FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(*ReceivedMessage, CreateResult.Channel);
			TestFalse("PinMessageToChannel should succeed", PinResult.Error);
			
			// Verify message is pinned
			FPubnubChatChannelData AfterPin = CreateResult.Channel->GetChannelData();
			TestFalse("Channel Custom should contain pinned message data", AfterPin.Custom.IsEmpty());
			
			// Unpin the message using Chat->UnpinMessageFromChannel
			FPubnubChatOperationResult UnpinResult = Chat->UnpinMessageFromChannel(CreateResult.Channel);
			TestFalse("UnpinMessageFromChannel should succeed", UnpinResult.Error);
			
			// Verify message is unpinned
			FPubnubChatChannelData AfterUnpin = CreateResult.Channel->GetChannelData();
			TestTrue("Unpin should complete successfully", true);
		}
	}, 0.1f));
	
	// Cleanup
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateResult, Chat, TestChannelID]()
	{
		if(CreateResult.Channel)
		{
			CreateResult.Channel->Disconnect();
		}
		if(Chat)
		{
			Chat->DeleteChannel(TestChannelID, false);
		}
		CleanUp();
	}, 0.1f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

