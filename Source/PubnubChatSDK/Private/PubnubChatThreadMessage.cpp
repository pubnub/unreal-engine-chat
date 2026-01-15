// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatThreadMessage.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"



void UPubnubChatThreadMessage::InitThreadMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken, const FString InParentChannelID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InParentChannelID.IsEmpty(), TEXT("Can't init Thread Message, InParentChannelID is empty"));
	
	ParentChannelID = InParentChannelID;
	
	InitMessage(InPubnubClient, InChat, InChannelID, InTimetoken);
}
