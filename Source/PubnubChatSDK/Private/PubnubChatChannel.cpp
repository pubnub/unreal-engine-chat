// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatChannel.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatMessage.h"
#include "PubnubChatUser.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"


void UPubnubChatChannel::BeginDestroy()
{
	//Clean up subscription if channel is being destroyed while connected
	if (IsInitialized && ConnectSubscription)
	{
		//Clear all listeners and unsubscribe
		ConnectSubscription->OnPubnubMessageNative.Clear();
		ConnectSubscription->Unsubscribe();
		ConnectSubscription = nullptr;
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !ChannelID.IsEmpty())
	{
		Chat->ObjectsRepository->UnregisterChannel(ChannelID);
	}
	
	UObject::BeginDestroy();
	
	IsInitialized = false;
}

FPubnubChatChannelData UPubnubChatChannel::GetChannelData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatChannelData());

	//Get channel data from repository
	FPubnubChatInternalChannel* InternalChannel = Chat->ObjectsRepository->GetChannelData(ChannelID);
	if (InternalChannel)
	{
		return InternalChannel->ChannelData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("Channel data not found in repository for ChannelID: %s"), *ChannelID);
	return FPubnubChatChannelData();
}

FPubnubChatConnectResult UPubnubChatChannel::Connect(FOnPubnubChatChannelMessageReceived MessageCallback)
{
	FOnPubnubChatChannelMessageReceivedNative MessageCallbackNative;
	MessageCallbackNative.BindLambda([MessageCallback](UPubnubChatMessage* Message)
	{
		MessageCallback.ExecuteIfBound(Message);
	});
	return Connect(MessageCallbackNative);
}

FPubnubChatConnectResult UPubnubChatChannel::Connect(FOnPubnubChatChannelMessageReceivedNative MessageCallbackNative)
{
	FPubnubChatConnectResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add lister to subscription with provided callback
	FDelegateHandle DelegateHandle =  ConnectSubscription->OnPubnubMessageNative.AddLambda([ThisWeak, MessageCallbackNative](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}

		if(!ThisWeak.Get()->Chat)
		{return;}
		
		MessageCallbackNative.ExecuteIfBound(ThisWeak.Get()->Chat->CreateMessageObject(MessageData.Timetoken, MessageData));
	});
	
	//Subscribe with this channel Subscription
	FPubnubOperationResult SubscribeResult = ConnectSubscription->Subscribe();
	FinalResult.Result.AddStep("Subscribe", SubscribeResult);

	//Create CallbackStop with remove listener function
	UPubnubChatCallbackStop* CallbackStop = NewObject<UPubnubChatCallbackStop>(this);
	auto DisconnectLambda = [ThisWeak, DelegateHandle]()->FPubnubChatOperationResult
	{
		if(!ThisWeak.IsValid())
		{return FPubnubChatOperationResult::CreateError("This Channel was already destroyed");}

		UPubnubChatChannel* ThisChannel = ThisWeak.Get();

		if(!ThisChannel->ConnectSubscription)
		{return FPubnubChatOperationResult::CreateError("This Channel ConnectionSubscription was already destroyed");}

		//Remove the listener
		ThisChannel->ConnectSubscription->OnPubnubMessageNative.Remove(DelegateHandle);

		//If there are no more listeners, just unsubscribe
		if(!ThisChannel->ConnectSubscription->OnPubnubMessageNative.IsBound())
		{
			FPubnubChatOperationResult FinalResult;
			FPubnubOperationResult UnsubscribeResult = ThisChannel->ConnectSubscription->Unsubscribe();
			FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
			return FinalResult;
		}

		//This is ok, we removed listener, but didn't unsubscribe as there are still other active listeners
		FPubnubChatOperationResult SuccessResult;
		SuccessResult.MarkSuccess();
		return SuccessResult;
	};
	CallbackStop->InitCallbackStop(DisconnectLambda);
	
	FinalResult.CallbackStop = CallbackStop;
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::Disconnect()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	//Remove message related delegates
	ConnectSubscription->OnPubnubMessageNative.Clear();

	//Unsubcribe and return result
	FPubnubChatOperationResult FinalResult;
	FPubnubOperationResult UnsubscribeResult = ConnectSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::SendText(const FString Message, FPubnubChatSendTextParams SendTextParams)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Message);

	//Validate quoted message if it was added to the params
	if(SendTextParams.QuotedMessage)
	{
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((SendTextParams.QuotedMessage->GetMessageData().ChannelID == ChannelID), TEXT("You cannot quote messages from other channels"));
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((!SendTextParams.QuotedMessage->GetMessageTimetoken().IsEmpty()), TEXT("Quoted message has empty invalid timetoken"));
	}

	//TODO:: apply rate limiter here

	//Configure settings specified in the params
	FPubnubPublishSettings PublishSettings;
	PublishSettings.MetaData = UPubnubChatInternalUtilities::SendTextMetaFromParams(SendTextParams);
	PublishSettings.StoreInHistory = SendTextParams.StoreInHistory;
	if(SendTextParams.SendByPost)
	{
		PublishSettings.PublishMethod = EPubnubPublishMethod::PPM_SendViaPOST;
	}

	//PublishMessage by PubnubClient
	FPubnubPublishMessageResult PublishResult =  PubnubClient->PublishMessage(ChannelID, UPubnubChatInternalUtilities::ChatMessageToPublishString(Message), PublishSettings);

	FPubnubChatOperationResult FinalResult;
	FinalResult.AddStep("PublishMessage", PublishResult.Result);
	
	return FinalResult;
}

FPubnubChatInviteResult UPubnubChatChannel::Invite(UPubnubChatUser* User)
{
	FPubnubChatInviteResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);

	//GetChannelMembers from PubnubClient to check if the user is not already member of that channel
	FString Filter = FString::Printf(TEXT("uuid.id == \"%s\""), *User->GetUserID());
	FPubnubMemberInclude Include = FPubnubMemberInclude({.IncludeCustom=true, .IncludeStatus=true, .IncludeType=true});
	FPubnubChannelMembersResult GetChannelMembersResult = PubnubClient->GetChannelMembers(ChannelID, Include, 0, Filter);
	FinalResult.Result.AddStep("GetChannelMembers", GetChannelMembersResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetChannelMembersResult.Result.Error)
	{return FinalResult;}
	
	//If User is already member of that channel, we just return the membership
	if(!GetChannelMembersResult.MembersData.IsEmpty())
	{
		FinalResult.Membership = Chat->CreateMembershipObject(User, this, GetChannelMembersResult.MembersData[0]);
		return FinalResult;
	}

	//TODO:: finish this function

	FPubnubChatMembershipData MembershipData;
	MembershipData.Status = "pending";
	
	{return FinalResult;}
}

FPubnubChatInviteMultipleResult UPubnubChatChannel::InviteMultiple(TArray<UPubnubChatUser*> Users)
{
	return FPubnubChatInviteMultipleResult();
}

void UPubnubChatChannel::InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Channel, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Channel, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InChannelID.IsEmpty(), TEXT("Can't init Channel, ChannelID is empty"));

	ChannelID = InChannelID;
	PubnubClient = InPubnubClient;
	Chat = InChat;

	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(ChannelID);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ChannelEntity, TEXT("Can't init Channel, Failed to create ChannelEntity"));

	ConnectSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ConnectSubscription, TEXT("Can't init Channel, Failed to create Subscription"));
	
	// Register this channel object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterChannel(ChannelID);
	}
	
	IsInitialized = true;
}
