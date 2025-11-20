// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"
#include "PubnubChatSubsystem.h"
#include "PubnubMacroUtilities.h"
#include "PubnubClient.h"

DEFINE_LOG_CATEGORY(PubnubChatLog)



void UPubnubChat::DestroyChat()
{
	//remove connection status listener
	
	OnChatDestroyed.Broadcast();
}



void UPubnubChat::InitChat(FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Chat, PubnubClient is invalid"));
	}

	ChatConfig = InChatConfig;
	PubnubClient = InPubnubClient;
	
	//Init conection listener
}
