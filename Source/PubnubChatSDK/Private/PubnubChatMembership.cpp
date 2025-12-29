// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMembership.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatAccessManager.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"


FString UPubnubChatMembership::GetInternalMembershipID() const
{
	if (!User || !Channel)
	{
		return TEXT("");
	}
	return FString::Printf(TEXT("%s.%s"), *Channel->GetChannelID(), *User->GetUserID());
}

void UPubnubChatMembership::BeginDestroy()
{
	// Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && User && Channel)
	{
		Chat->ObjectsRepository->UnregisterMembership(GetInternalMembershipID());
	}
	
	UObject::BeginDestroy();
	IsInitialized = false;
}

FPubnubChatMembershipData UPubnubChatMembership::GetMembershipData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatMembershipData());

	// Get membership data from repository
	if (FPubnubChatInternalMembership* InternalMembership = Chat->ObjectsRepository->GetMembershipData(GetInternalMembershipID()))
	{
		return InternalMembership->MembershipData;
	}

	FString UserIDStr = User ? User->GetUserID() : TEXT("");
	FString ChannelIDStr = Channel ? Channel->GetChannelID() : TEXT("");
	UE_LOG(PubnubChatLog, Error, TEXT("Membership data not found in repository for UserID: %s, ChannelID: %s"), *UserIDStr, *ChannelIDStr);
	return FPubnubChatMembershipData();
}

FString UPubnubChatMembership::GetUserID() const
{
	return User ? User->GetUserID() : TEXT("");
}

FString UPubnubChatMembership::GetChannelID() const
{
	return Channel ? Channel->GetChannelID() : TEXT("");
}

FPubnubChatOperationResult UPubnubChatMembership::Update(const FPubnubChatMembershipData& MembershipData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	//SetMemberships by PubnubClient
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(GetUserID(), {MembershipData.ToPubnubMembershipInputData(GetChannelID())}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");

	//Update repository with updated user data
	Chat->ObjectsRepository->UpdateMembershipData(GetInternalMembershipID(), MembershipData);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMembership::SetLastReadMessageTimetoken(const FString Timetoken)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Timetoken);

	//Add Timetoken to MembershipData
	FPubnubChatMembershipData MembershipData = GetMembershipData();
	UPubnubChatInternalUtilities::AddLastReadMessageTimetokenToMembershipData(MembershipData, Timetoken);

	//Update Membership with new data
	FPubnubChatOperationResult UpdateResult = Update(MembershipData);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);

	//If this is not public channel Emit Receipt Event
	if(Channel->GetChannelData().Type != "public")
	{
		//Check if event can be emitted with provided AccessToken
		bool CanIEmit = Chat->AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Write, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, GetChannelID());
		FPubnubChatOperationResult EmitEventResult;
		if(CanIEmit)
		{
			EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Receipt, GetChannelID(), UPubnubChatInternalUtilities::GetReceiptEventPayload(Timetoken));
		}
		else
		{
			//It's not an error, but add it to the result
			EmitEventResult.AddStep("EmitChatEvent", FPubnubOperationResult(0, false, TEXT("Can't emit chat event, user doesn't have permissions")));
		}

		FinalResult.Merge(EmitEventResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMembership::SetLastReadMessage(UPubnubChatMessage* Message)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);

	return SetLastReadMessageTimetoken(Message->GetMessageTimetoken());
}

void UPubnubChatMembership::InitMembership(UPubnubClient* InPubnubClient, UPubnubChat* InChat, UPubnubChatUser* InUser, UPubnubChatChannel* InChannel)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Membership, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Membership, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InUser, TEXT("Can't init Membership, User is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChannel, TEXT("Can't init Membership, Channel is invalid"));

	User = InUser;
	Channel = InChannel;
	PubnubClient = InPubnubClient;
	Chat = InChat;
	
	// Register this membership object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterMembership(GetInternalMembershipID());
	}
	
	IsInitialized = true;
}

