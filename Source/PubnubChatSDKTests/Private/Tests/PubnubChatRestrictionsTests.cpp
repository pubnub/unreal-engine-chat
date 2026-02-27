// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "Dom/JsonObject.h"
#include "PubnubChatChannel.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatMembership.h"
#include "PubnubClient.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// SETRESTRICTIONS TESTS (UPubnubChat::SetRestrictions)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();
	if(Chat)
	{
		FPubnubChatRestriction Restriction;
		Restriction.UserID = SDK_PREFIX + "test_user";
		Restriction.ChannelID = SDK_PREFIX + "test_channel";
		Restriction.Ban = true;
		
		FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
		
		TestTrue("SetRestrictions should fail when Chat is not initialized", SetResult.Error);
		TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.1Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_set_restrictions_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		FPubnubChatRestriction Restriction;
		Restriction.UserID = TEXT(""); // Empty UserID
		Restriction.ChannelID = SDK_PREFIX + "test_channel";
		Restriction.Ban = true;
		
		FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
		
		TestTrue("SetRestrictions should fail with empty UserID", SetResult.Error);
		TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.1Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_set_restrictions_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		FPubnubChatRestriction Restriction;
		Restriction.UserID = SDK_PREFIX + "test_user";
		Restriction.ChannelID = TEXT(""); // Empty ChannelID
		Restriction.Ban = true;
		
		FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
		
		TestTrue("SetRestrictions should fail with empty ChannelID", SetResult.Error);
		TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsHappyPathBanTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.2HappyPath.Ban", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsHappyPathBanTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_ban_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_ban_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_ban_channel";
	
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
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restriction with only required parameters (Ban = true, Mute = false, Reason = "")
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	Restriction.Reason = TEXT("");
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Verify restriction was set by getting it back
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
			TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
			TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
			TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		}
	}
	
	// Cleanup: Remove restriction, delete channel and user
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsHappyPathMuteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.2HappyPath.Mute", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsHappyPathMuteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_mute_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_mute_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_mute_channel";
	
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
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restriction with Mute = true
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = false;
	Restriction.Mute = true;
	Restriction.Reason = TEXT("");
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Verify restriction was set
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestFalse("Restriction Ban should be false", GetResult.Restriction.Ban);
			TestTrue("Restriction Mute should be true", GetResult.Restriction.Mute);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_full_channel";
	const FString TestReason = TEXT("Violation of community guidelines");
	
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
	
	// Create target user
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	// Create channel
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restriction with all parameters including Reason
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	Restriction.Reason = TestReason;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Verify restriction was set with reason
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
			TestEqual("Restriction Reason should match", GetResult.Restriction.Reason, TestReason);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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
 * Tests lifting restrictions by setting both Ban and Mute to false.
 * Verifies that restrictions are properly removed from the moderation channel.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsLiftRestrictionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.4Advanced.LiftRestrictions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsLiftRestrictionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_lift_restrictions_init";
	const FString TargetUserID = SDK_PREFIX + "test_lift_restrictions_target";
	const FString TestChannelID = SDK_PREFIX + "test_lift_restrictions_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// First, set a restriction
	FPubnubChatRestriction SetRestriction;
	SetRestriction.UserID = TargetUserID;
	SetRestriction.ChannelID = TestChannelID;
	SetRestriction.Ban = true;
	SetRestriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(SetRestriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Verify restriction exists
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true before lifting", GetResult.Restriction.Ban);
			
			// Now lift the restriction
			FPubnubChatRestriction LiftRestriction;
			LiftRestriction.UserID = TargetUserID;
			LiftRestriction.ChannelID = TestChannelID;
			LiftRestriction.Ban = false;
			LiftRestriction.Mute = false;
			
			FPubnubChatOperationResult LiftResult = Chat->SetRestrictions(LiftRestriction);
			TestFalse("LiftRestrictions should succeed", LiftResult.Error);
			
			// Verify restriction is lifted (should return empty restriction)
			FPubnubChatGetRestrictionResult GetAfterLiftResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions after lift should succeed", GetAfterLiftResult.Result.Error);
			TestFalse("Restriction Ban should be false after lifting", GetAfterLiftResult.Restriction.Ban);
			TestFalse("Restriction Mute should be false after lifting", GetAfterLiftResult.Restriction.Mute);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Tests that setting restrictions emits moderation events to the internal moderation channel.
 * Verifies event payload contains correct channelId, restriction type, and reason.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsEventEmissionTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.4Advanced.EventEmission", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatSetRestrictionsEventEmissionTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_event_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_event_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_event_channel";
	const FString TestReason = TEXT("Test reason for restriction");
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Shared state for event reception
	TSharedPtr<bool> bEventReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatRestriction> ReceivedRestriction = MakeShared<FPubnubChatRestriction>();
	
	// Stream moderation events via target user restrictions stream
	CreateUserResult.User->OnRestrictionChangedNative.AddLambda([this, bEventReceived, ReceivedRestriction](const FPubnubChatRestriction& RestrictionEvent)
	{
		*bEventReceived = true;
		*ReceivedRestriction = RestrictionEvent;
	});
	
	FPubnubChatOperationResult StreamResult = CreateUserResult.User->StreamRestrictions();
	TestFalse("StreamRestrictions should succeed", StreamResult.Error);
	
	// Wait a bit for subscription to be ready
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TargetUserID, TestChannelID, TestReason]()
	{
		// Set restriction
		FPubnubChatRestriction Restriction;
		Restriction.UserID = TargetUserID;
		Restriction.ChannelID = TestChannelID;
		Restriction.Ban = true;
		Restriction.Mute = false;
		Restriction.Reason = TestReason;
		
		FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
		TestFalse("SetRestrictions should succeed", SetResult.Error);
	}, 0.5f));
	
	// Wait until event is received
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bEventReceived]() -> bool {
		return *bEventReceived;
	}, MAX_WAIT_TIME));
	
	// Verify event was received and has correct payload
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bEventReceived, ReceivedRestriction, TestChannelID, TestReason]()
	{
		if(!*bEventReceived)
		{
			AddError("Moderation event was not received");
		}
		else
		{
			FString ExpectedModerationChannel = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			TestEqual("Restriction channelId should match moderation channel", ReceivedRestriction->ChannelID, ExpectedModerationChannel);
			TestTrue("Restriction should be banned", ReceivedRestriction->Ban);
			TestEqual("Restriction reason should match", ReceivedRestriction->Reason, TestReason);
		}
	}, 0.1f));
	
	// Cleanup: Stop streaming, remove restriction, delete channel and user
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CreateUserResult, Chat, TestChannelID, TargetUserID]()
	{
		if(CreateUserResult.User)
		{
			CreateUserResult.User->StopStreamingRestrictions();
		}
		
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

/**
 * Tests setting both Ban and Mute restrictions simultaneously.
 * Verifies that both restrictions are properly stored.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsBanAndMuteTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictions.4Advanced.BanAndMute", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsBanAndMuteTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_banmute_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_banmute_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_banmute_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set both Ban and Mute restrictions
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = true;
	Restriction.Reason = TEXT("Severe violation");
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Verify both restrictions are set
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
			TestTrue("Restriction Mute should be true", GetResult.Restriction.Mute);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETUSERRESTRICTIONS TESTS (UPubnubChatChannel::GetUserRestrictions)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUserRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUserRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUserRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_user_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel object without initializing it properly
		UPubnubChatChannel* Channel = NewObject<UPubnubChatChannel>();
		UPubnubChatUser* User = Chat->GetCurrentUser();
		
		if(Channel && User)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(User);
			
			TestTrue("GetUserRestrictions should fail when Channel is not initialized", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUserRestrictionsInvalidUserTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUserRestrictions.1Validation.InvalidUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUserRestrictionsInvalidUserTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_user_restrictions_invalid_user";
	const FString TestChannelID = SDK_PREFIX + "test_get_user_restrictions_invalid_user_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
		TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
		
		UPubnubChatChannel* Channel = CreateChannelResult.Channel;
		if(Channel)
		{
			// Try with null user
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(nullptr);
			
			TestTrue("GetUserRestrictions should fail with null User", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
		
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUserRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUserRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUserRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_user_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction first
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get user restrictions
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(Channel && TargetUser)
	{
		FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
		
		TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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

// GetUserRestrictions only takes User parameter, so full test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUserRestrictionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUserRestrictions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUserRestrictionsFullParametersTest::RunTest(const FString& Parameters)
{
	// GetUserRestrictions only takes User parameter, so full test is same as happy path
	// Reuse the happy path test implementation
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_restrictions_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_restrictions_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_user_restrictions_full_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction first
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get user restrictions
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(Channel && TargetUser)
	{
		FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
		
		TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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
 * Tests getting restrictions for a user that has no restrictions.
 * Verifies that function returns successfully with empty restriction (Ban=false, Mute=false).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUserRestrictionsNoRestrictionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUserRestrictions.4Advanced.NoRestrictions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUserRestrictionsNoRestrictionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_user_restrictions_no_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_user_restrictions_no_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_user_restrictions_no_channel";
	
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
	
	// Create target user and channel (but don't set any restrictions)
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Get user restrictions (should return empty restriction)
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(Channel && TargetUser)
	{
		FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
		
		TestFalse("GetUserRestrictions should succeed even when no restrictions exist", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestFalse("Restriction Ban should be false when no restrictions", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false when no restrictions", GetResult.Restriction.Mute);
	}
	
	// Cleanup
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
// GETUSERSRESTRICTIONS TESTS (UPubnubChatChannel::GetUsersRestrictions)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUsersRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUsersRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUsersRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_users_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel object without initializing it properly
		UPubnubChatChannel* Channel = NewObject<UPubnubChatChannel>();
		
		if(Channel)
		{
			FPubnubChatGetRestrictionsResult GetResult = Channel->GetUsersRestrictions();
			
			TestTrue("GetUsersRestrictions should fail when Channel is not initialized", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUsersRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUsersRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUsersRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_users_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_users_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get all users restrictions
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		FPubnubChatGetRestrictionsResult GetResult = Channel->GetUsersRestrictions();
		
		TestFalse("GetUsersRestrictions should succeed", GetResult.Result.Error);
		TestTrue("Should have at least one restriction", GetResult.Restrictions.Num() >= 1);
		
		// Find the restriction for our target user
		bool bFoundRestriction = false;
		for(const FPubnubChatRestriction& RestrictionItem : GetResult.Restrictions)
		{
			if(RestrictionItem.UserID == TargetUserID && RestrictionItem.ChannelID == TestChannelID)
			{
				bFoundRestriction = true;
				TestTrue("Found restriction Ban should be true", RestrictionItem.Ban);
				break;
			}
		}
		TestTrue("Should find restriction for target user", bFoundRestriction);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUsersRestrictionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUsersRestrictions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUsersRestrictionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_restrictions_full_init";
	const FString TargetUserID1 = SDK_PREFIX + "test_get_users_restrictions_full_target1";
	const FString TargetUserID2 = SDK_PREFIX + "test_get_users_restrictions_full_target2";
	const FString TestChannelID = SDK_PREFIX + "test_get_users_restrictions_full_channel";
	
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
	
	// Create target users and channel
	FPubnubChatUserResult CreateUserResult1 = Chat->CreateUser(TargetUserID1);
	TestFalse("CreateUser1 should succeed", CreateUserResult1.Result.Error);
	
	FPubnubChatUserResult CreateUserResult2 = Chat->CreateUser(TargetUserID2);
	TestFalse("CreateUser2 should succeed", CreateUserResult2.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restrictions for both users
	FPubnubChatRestriction Restriction1;
	Restriction1.UserID = TargetUserID1;
	Restriction1.ChannelID = TestChannelID;
	Restriction1.Ban = true;
	Restriction1.Mute = false;
	
	FPubnubChatOperationResult SetResult1 = Chat->SetRestrictions(Restriction1);
	TestFalse("SetRestrictions1 should succeed", SetResult1.Error);
	
	FPubnubChatRestriction Restriction2;
	Restriction2.UserID = TargetUserID2;
	Restriction2.ChannelID = TestChannelID;
	Restriction2.Ban = false;
	Restriction2.Mute = true;
	
	FPubnubChatOperationResult SetResult2 = Chat->SetRestrictions(Restriction2);
	TestFalse("SetRestrictions2 should succeed", SetResult2.Error);
	
	// Get all users restrictions with Limit, Sort, and Page parameters
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		FPubnubMemberSort Sort;
		FPubnubMemberSingleSort SingleSort;
		SingleSort.SortType = EPubnubMemberSortType::PMeST_UserID;
		SingleSort.SortOrder = false; // Ascending
		Sort.MemberSort.Add(SingleSort);
		FPubnubPage Page;
		
		FPubnubChatGetRestrictionsResult GetResult = Channel->GetUsersRestrictions(10, Sort, Page);
		
		TestFalse("GetUsersRestrictions should succeed", GetResult.Result.Error);
		TestTrue("Should have at least 2 restrictions", GetResult.Restrictions.Num() >= 2);
		
		// Verify both restrictions are present
		bool bFoundRestriction1 = false;
		bool bFoundRestriction2 = false;
		for(const FPubnubChatRestriction& RestrictionItem : GetResult.Restrictions)
		{
			if(RestrictionItem.UserID == TargetUserID1)
			{
				bFoundRestriction1 = true;
				TestTrue("Restriction1 Ban should be true", RestrictionItem.Ban);
			}
			if(RestrictionItem.UserID == TargetUserID2)
			{
				bFoundRestriction2 = true;
				TestTrue("Restriction2 Mute should be true", RestrictionItem.Mute);
			}
		}
		TestTrue("Should find restriction for user1", bFoundRestriction1);
		TestTrue("Should find restriction for user2", bFoundRestriction2);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			TArray<FString> UserIDsToRemove = {TargetUserID1, TargetUserID2};
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, UserIDsToRemove, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restrictions during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID1);
		Chat->DeleteUser(TargetUserID1);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

/**
 * Tests getting restrictions when channel has no restrictions.
 * Verifies that function returns successfully with empty restrictions array.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelGetUsersRestrictionsEmptyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.GetUsersRestrictions.4Advanced.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelGetUsersRestrictionsEmptyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_users_restrictions_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_get_users_restrictions_empty_channel";
	
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
	
	// Create channel (but don't set any restrictions)
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Get all users restrictions (should return empty array)
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		FPubnubChatGetRestrictionsResult GetResult = Channel->GetUsersRestrictions();
		
		TestFalse("GetUsersRestrictions should succeed even when no restrictions exist", GetResult.Result.Error);
		TestTrue("Restrictions array should be empty", GetResult.Restrictions.Num() == 0);
	}
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETCHANNELRESTRICTIONS TESTS (UPubnubChatUser::GetChannelRestrictions)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channel_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user object without initializing it properly
		UPubnubChatUser* User = NewObject<UPubnubChatUser>();
		FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(SDK_PREFIX + "test_channel");
		
		if(User && CreateChannelResult.Channel)
		{
			FPubnubChatGetRestrictionResult GetResult = User->GetChannelRestrictions(CreateChannelResult.Channel);
			
			TestTrue("GetChannelRestrictions should fail when User is not initialized", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
		
		if(CreateChannelResult.Channel)
		{
			Chat->DeleteChannel(SDK_PREFIX + "test_channel");
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsInvalidChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelRestrictions.1Validation.InvalidChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsInvalidChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channel_restrictions_invalid_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* User = Chat->GetCurrentUser();
		if(User)
		{
			// Try with null channel
			FPubnubChatGetRestrictionResult GetResult = User->GetChannelRestrictions(nullptr);
			
			TestTrue("GetChannelRestrictions should fail with null Channel", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channel_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction for the target user
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get channel restrictions from user perspective
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(TargetUser && Channel)
	{
		FPubnubChatGetRestrictionResult GetResult = TargetUser->GetChannelRestrictions(Channel);
		
		TestFalse("GetChannelRestrictions should succeed", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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

// GetChannelRestrictions only takes Channel parameter, so full test is same as happy path
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelRestrictions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsFullParametersTest::RunTest(const FString& Parameters)
{
	// GetChannelRestrictions only takes Channel parameter, so full test is same as happy path
	// Reuse the happy path test implementation
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_restrictions_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channel_restrictions_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_restrictions_full_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction for the target user
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get channel restrictions from user perspective
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(TargetUser && Channel)
	{
		FPubnubChatGetRestrictionResult GetResult = TargetUser->GetChannelRestrictions(Channel);
		
		TestFalse("GetChannelRestrictions should succeed", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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
 * Tests getting restrictions for a channel when user has no restrictions.
 * Verifies that function returns successfully with empty restriction (Ban=false, Mute=false).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelRestrictionsNoRestrictionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelRestrictions.4Advanced.NoRestrictions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelRestrictionsNoRestrictionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channel_restrictions_no_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channel_restrictions_no_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_channel_restrictions_no_channel";
	
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
	
	// Create target user and channel (but don't set any restrictions)
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Get channel restrictions (should return empty restriction)
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(TargetUser && Channel)
	{
		FPubnubChatGetRestrictionResult GetResult = TargetUser->GetChannelRestrictions(Channel);
		
		TestFalse("GetChannelRestrictions should succeed even when no restrictions exist", GetResult.Result.Error);
		TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
		TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		TestFalse("Restriction Ban should be false when no restrictions", GetResult.Restriction.Ban);
		TestFalse("Restriction Mute should be false when no restrictions", GetResult.Restriction.Mute);
	}
	
	// Cleanup
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
// GETCHANNELSRESTRICTIONS TESTS (UPubnubChatUser::GetChannelsRestrictions)
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelsRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelsRestrictions.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelsRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_channels_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user object without initializing it properly
		UPubnubChatUser* User = NewObject<UPubnubChatUser>();
		
		if(User)
		{
			FPubnubChatGetRestrictionsResult GetResult = User->GetChannelsRestrictions();
			
			TestTrue("GetChannelsRestrictions should fail when User is not initialized", GetResult.Result.Error);
			TestFalse("ErrorMessage should not be empty", GetResult.Result.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelsRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelsRestrictions.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelsRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channels_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channels_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_get_channels_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set a restriction for the target user
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	
	FPubnubChatOperationResult SetResult = Chat->SetRestrictions(Restriction);
	TestFalse("SetRestrictions should succeed", SetResult.Error);
	
	// Get all channels restrictions from user perspective
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(TargetUser)
	{
		FPubnubChatGetRestrictionsResult GetResult = TargetUser->GetChannelsRestrictions();
		
		TestFalse("GetChannelsRestrictions should succeed", GetResult.Result.Error);
		TestTrue("Should have at least one restriction", GetResult.Restrictions.Num() >= 1);
		
		// Find the restriction for our test channel
		bool bFoundRestriction = false;
		for(const FPubnubChatRestriction& RestrictionItem : GetResult.Restrictions)
		{
			if(RestrictionItem.UserID == TargetUserID && RestrictionItem.ChannelID == TestChannelID)
			{
				bFoundRestriction = true;
				TestTrue("Found restriction Ban should be true", RestrictionItem.Ban);
				break;
			}
		}
		TestTrue("Should find restriction for test channel", bFoundRestriction);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
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

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelsRestrictionsFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelsRestrictions.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelsRestrictionsFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channels_restrictions_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channels_restrictions_full_target";
	const FString TestChannelID1 = SDK_PREFIX + "test_get_channels_restrictions_full_channel1";
	const FString TestChannelID2 = SDK_PREFIX + "test_get_channels_restrictions_full_channel2";
	
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
	
	// Create target user and channels
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult1 = Chat->CreatePublicConversation(TestChannelID1);
	TestFalse("CreateChannel1 should succeed", CreateChannelResult1.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult2 = Chat->CreatePublicConversation(TestChannelID2);
	TestFalse("CreateChannel2 should succeed", CreateChannelResult2.Result.Error);
	
	// Set restrictions for both channels
	FPubnubChatRestriction Restriction1;
	Restriction1.UserID = TargetUserID;
	Restriction1.ChannelID = TestChannelID1;
	Restriction1.Ban = true;
	Restriction1.Mute = false;
	
	FPubnubChatOperationResult SetResult1 = Chat->SetRestrictions(Restriction1);
	TestFalse("SetRestrictions1 should succeed", SetResult1.Error);
	
	FPubnubChatRestriction Restriction2;
	Restriction2.UserID = TargetUserID;
	Restriction2.ChannelID = TestChannelID2;
	Restriction2.Ban = false;
	Restriction2.Mute = true;
	
	FPubnubChatOperationResult SetResult2 = Chat->SetRestrictions(Restriction2);
	TestFalse("SetRestrictions2 should succeed", SetResult2.Error);
	
	// Get all channels restrictions with Limit, Sort, and Page parameters
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(TargetUser)
	{
		FPubnubMembershipSort Sort;
		FPubnubMembershipSingleSort SingleSort;
		SingleSort.SortType = EPubnubMembershipSortType::PMST_ChannelID;
		SingleSort.SortOrder = false; // Ascending
		Sort.MembershipSort.Add(SingleSort);
		FPubnubPage Page;
		
		FPubnubChatGetRestrictionsResult GetResult = TargetUser->GetChannelsRestrictions(10, Sort, Page);
		
		TestFalse("GetChannelsRestrictions should succeed", GetResult.Result.Error);
		TestTrue("Should have at least 2 restrictions", GetResult.Restrictions.Num() >= 2);
		
		// Verify both restrictions are present
		bool bFoundRestriction1 = false;
		bool bFoundRestriction2 = false;
		for(const FPubnubChatRestriction& RestrictionItem : GetResult.Restrictions)
		{
			if(RestrictionItem.ChannelID == TestChannelID1)
			{
				bFoundRestriction1 = true;
				TestTrue("Restriction1 Ban should be true", RestrictionItem.Ban);
			}
			if(RestrictionItem.ChannelID == TestChannelID2)
			{
				bFoundRestriction2 = true;
				TestTrue("Restriction2 Mute should be true", RestrictionItem.Mute);
			}
		}
		TestTrue("Should find restriction for channel1", bFoundRestriction1);
		TestTrue("Should find restriction for channel2", bFoundRestriction2);
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID1 = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID1);
			FString ModerationChannelID2 = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID2);
			FPubnubChannelMembersResult RemoveResult1 = PubnubClient->RemoveChannelMembers(ModerationChannelID1, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			FPubnubChannelMembersResult RemoveResult2 = PubnubClient->RemoveChannelMembers(ModerationChannelID2, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult1.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction1 during cleanup: %s"), *RemoveResult1.Result.ErrorMessage);
			}
			if(RemoveResult2.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction2 during cleanup: %s"), *RemoveResult2.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID1);
		Chat->DeleteChannel(TestChannelID1);
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
 * Tests getting restrictions when user has no restrictions in any channel.
 * Verifies that function returns successfully with empty restrictions array.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserGetChannelsRestrictionsEmptyTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.GetChannelsRestrictions.4Advanced.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserGetChannelsRestrictionsEmptyTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_get_channels_restrictions_empty_init";
	const FString TargetUserID = SDK_PREFIX + "test_get_channels_restrictions_empty_target";
	
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
	
	// Create target user (but don't set any restrictions)
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	// Get all channels restrictions (should return empty array)
	UPubnubChatUser* TargetUser = CreateUserResult.User;
	if(TargetUser)
	{
		FPubnubChatGetRestrictionsResult GetResult = TargetUser->GetChannelsRestrictions();
		
		TestFalse("GetChannelsRestrictions should succeed even when no restrictions exist", GetResult.Result.Error);
		TestTrue("Restrictions array should be empty", GetResult.Restrictions.Num() == 0);
	}
	
	// Cleanup
	if(Chat)
	{
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// SETRESTRICTIONS FROM ENTITY TESTS (Pass-through methods)
// ============================================================================

// ============================================================================
// UPubnubChatUser::SetRestrictions TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserSetRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.SetRestrictions.Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserSetRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_set_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create user object without initializing it properly
		UPubnubChatUser* User = NewObject<UPubnubChatUser>();
		
		if(User)
		{
			FPubnubChatOperationResult SetResult = User->SetRestrictions(SDK_PREFIX + "test_channel", true, false);
			
			TestTrue("SetRestrictions should fail when User is not initialized", SetResult.Error);
			TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserSetRestrictionsEmptyChannelIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.SetRestrictions.Validation.EmptyChannelID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserSetRestrictionsEmptyChannelIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_user_set_restrictions_empty_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatUser* User = Chat->GetCurrentUser();
		if(User)
		{
			FPubnubChatOperationResult SetResult = User->SetRestrictions(TEXT(""), true, false);
			
			TestTrue("SetRestrictions should fail with empty ChannelID", SetResult.Error);
			TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatUserSetRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.User.SetRestrictions.HappyPath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatUserSetRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_user_set_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_user_set_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_user_set_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restriction from User entity
	UPubnubChatUser* InitUser = Chat->GetCurrentUser();
	if(InitUser)
	{
		FPubnubChatOperationResult SetResult = InitUser->SetRestrictions(TestChannelID, true, false, TEXT("Test reason"));
		TestFalse("SetRestrictions should succeed", SetResult.Error);
		
		// Verify restriction was set correctly
		UPubnubChatChannel* Channel = CreateChannelResult.Channel;
		if(Channel)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(InitUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
			TestFalse("Restriction Mute should be false", GetResult.Restriction.Mute);
			TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, InitUserID);
			TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {InitUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// UPubnubChatChannel::SetRestrictions TESTS
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelSetRestrictionsNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.SetRestrictions.Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelSetRestrictionsNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_channel_set_restrictions_not_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel object without initializing it properly
		UPubnubChatChannel* Channel = NewObject<UPubnubChatChannel>();
		
		if(Channel)
		{
			FPubnubChatOperationResult SetResult = Channel->SetRestrictions(SDK_PREFIX + "test_user", true, false);
			
			TestTrue("SetRestrictions should fail when Channel is not initialized", SetResult.Error);
			TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelSetRestrictionsEmptyUserIDTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.SetRestrictions.Validation.EmptyUserID", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelSetRestrictionsEmptyUserIDTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_channel_set_restrictions_empty_user";
	const FString TestChannelID = SDK_PREFIX + "test_channel_set_restrictions_empty_user_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
		TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
		
		UPubnubChatChannel* Channel = CreateChannelResult.Channel;
		if(Channel)
		{
			FPubnubChatOperationResult SetResult = Channel->SetRestrictions(TEXT(""), true, false);
			
			TestTrue("SetRestrictions should fail with empty UserID", SetResult.Error);
			TestFalse("ErrorMessage should not be empty", SetResult.ErrorMessage.IsEmpty());
		}
		
		Chat->DeleteChannel(TestChannelID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatChannelSetRestrictionsHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Channel.SetRestrictions.HappyPath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatChannelSetRestrictionsHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_channel_set_restrictions_happy_init";
	const FString TargetUserID = SDK_PREFIX + "test_channel_set_restrictions_happy_target";
	const FString TestChannelID = SDK_PREFIX + "test_channel_set_restrictions_happy_channel";
	
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
	
	// Create target user and channel
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	// Set restriction from Channel entity
	UPubnubChatChannel* Channel = CreateChannelResult.Channel;
	if(Channel)
	{
		FPubnubChatOperationResult SetResult = Channel->SetRestrictions(TargetUserID, false, true, TEXT("Test reason"));
		TestFalse("SetRestrictions should succeed", SetResult.Error);
		
		// Verify restriction was set correctly
		UPubnubChatUser* TargetUser = CreateUserResult.User;
		if(TargetUser)
		{
			FPubnubChatGetRestrictionResult GetResult = Channel->GetUserRestrictions(TargetUser);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestFalse("Restriction Ban should be false", GetResult.Restriction.Ban);
			TestTrue("Restriction Mute should be true", GetResult.Restriction.Mute);
			TestEqual("Restriction UserID should match", GetResult.Restriction.UserID, TargetUserID);
			TestEqual("Restriction ChannelID should match", GetResult.Restriction.ChannelID, TestChannelID);
		}
	}
	
	// Cleanup
	if(Chat)
	{
		UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
		if(PubnubClient)
		{
			FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
			FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
			if(RemoveResult.Result.Error)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
			}
		}
		Chat->DeleteChannel(TestChannelID);
		Chat->DeleteUser(TargetUserID);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ASYNC FULL PARAMETER TESTS (Moderation)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatSetRestrictionsAsyncFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.Chat.Moderation.SetRestrictionsAsync.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatSetRestrictionsAsyncFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_set_restrictions_async_full_init";
	const FString TargetUserID = SDK_PREFIX + "test_set_restrictions_async_full_target";
	const FString TestChannelID = SDK_PREFIX + "test_set_restrictions_async_full_channel";
	const FString TestReason = TEXT("Async moderation reason");
	
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
	
	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TargetUserID);
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
	FPubnubChatChannelResult CreateChannelResult = Chat->CreatePublicConversation(TestChannelID);
	TestFalse("CreateChannel should succeed", CreateChannelResult.Result.Error);
	
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TargetUserID;
	Restriction.ChannelID = TestChannelID;
	Restriction.Ban = true;
	Restriction.Mute = false;
	Restriction.Reason = TestReason;
	
	TSharedPtr<bool> bCallbackReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatOperationResult> CallbackResult = MakeShared<FPubnubChatOperationResult>();
	FOnPubnubChatOperationResponseNative OnOperationResponse;
	OnOperationResponse.BindLambda([bCallbackReceived, CallbackResult](const FPubnubChatOperationResult& OperationResult)
	{
		*CallbackResult = OperationResult;
		*bCallbackReceived = true;
	});
	
	Chat->SetRestrictionsAsync(Restriction, OnOperationResponse);
	
	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bCallbackReceived]() { return *bCallbackReceived; }, MAX_WAIT_TIME));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackResult, CreateChannelResult, CreateUserResult]()
	{
		TestFalse("SetRestrictionsAsync should succeed", CallbackResult->Error);
		if(CreateChannelResult.Channel && CreateUserResult.User)
		{
			FPubnubChatGetRestrictionResult GetResult = CreateChannelResult.Channel->GetUserRestrictions(CreateUserResult.User);
			TestFalse("GetUserRestrictions should succeed", GetResult.Result.Error);
			TestTrue("Restriction Ban should be true", GetResult.Restriction.Ban);
		}
	}, 0.1f));
	
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TargetUserID]()
	{
		if(Chat)
		{
			UPubnubClient* PubnubClient = GetPubnubClientFromChat(Chat);
			if(PubnubClient)
			{
				FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(TestChannelID);
				FPubnubChannelMembersResult RemoveResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {TargetUserID}, FPubnubMemberInclude::FromValue(false), 1);
				if(RemoveResult.Result.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to remove restriction during cleanup: %s"), *RemoveResult.Result.ErrorMessage);
				}
			}
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteUser(TargetUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.1f));
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

