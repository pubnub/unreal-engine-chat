// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatChannel.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"


void UPubnubChatChannel::BeginDestroy()
{
	// Unregister from repository before destruction
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

	// Get channel data from repository
	FPubnubChatInternalChannel* InternalChannel = Chat->ObjectsRepository->GetChannelData(ChannelID);
	if (InternalChannel)
	{
		return InternalChannel->ChannelData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("Channel data not found in repository for ChannelID: %s"), *ChannelID);
	return FPubnubChatChannelData();
}

void UPubnubChatChannel::InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Channel, PubnubClient is invalid"));
		return;
	}
	
	if(!InChat)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Channel, Chat is invalid"));
		return;
	}

	if(InChannelID.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Channel, ChannelID is empty"));
		return;
	}

	ChannelID = InChannelID;
	PubnubClient = InPubnubClient;
	Chat = InChat;
	
	// Register this channel object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterChannel(ChannelID);
	}
	
	IsInitialized = true;
}
