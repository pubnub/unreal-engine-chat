// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessage.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"


FString UPubnubChatMessage::GetInternalMessageID() const
{
	return FString::Printf(TEXT("%s.%s"), *ChannelID, *Timetoken);
}

void UPubnubChatMessage::BeginDestroy()
{
	// Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !ChannelID.IsEmpty() && !Timetoken.IsEmpty())
	{
		Chat->ObjectsRepository->UnregisterMessage(GetInternalMessageID());
	}
	
	UObject::BeginDestroy();
	IsInitialized = false;
}

FPubnubChatMessageData UPubnubChatMessage::GetMessageData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatMessageData());

	// Get message data from repository
	FPubnubChatInternalMessage* InternalMessage = Chat->ObjectsRepository->GetMessageData(GetInternalMessageID());
	if (InternalMessage)
	{
		return InternalMessage->MessageData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("Message data not found in repository for ChannelID: %s, Timetoken: %s"), *ChannelID, *Timetoken);
	return FPubnubChatMessageData();
}

void UPubnubChatMessage::InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Message, PubnubClient is invalid"));
		return;
	}
	
	if(!InChat)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Message, Chat is invalid"));
		return;
	}

	if(InChannelID.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Message, ChannelID is empty"));
		return;
	}

	if(InTimetoken.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Message, Timetoken is empty"));
		return;
	}

	ChannelID = InChannelID;
	Timetoken = InTimetoken;
	PubnubClient = InPubnubClient;
	Chat = InChat;
	
	// Register this message object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterMessage(GetInternalMessageID());
	}
	
	IsInitialized = true;
}

