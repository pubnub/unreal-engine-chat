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
 * Verifies that a GUID is generated and used as ChannelID when ChannelID parameter is empty.
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
	
	// Create direct conversation with empty ChannelID (should auto-generate GUID)
	FPubnubChatCreateDirectConversationResult CreateResult = Chat->CreateDirectConversation(CreateUserResult.User, TEXT(""), FPubnubChatChannelData(), FPubnubChatMembershipData());
	
	TestFalse("CreateDirectConversation should succeed", CreateResult.Result.Error);
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

#endif // WITH_DEV_AUTOMATION_TESTS

