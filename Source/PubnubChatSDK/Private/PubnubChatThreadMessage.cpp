// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatThreadMessage.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"


FPubnubChatOperationResult UPubnubChatThreadMessage::PinMessageToParentChannel()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	
	FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(ParentChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result);
	
	FPubnubChatOperationResult PinMessageResult = GetChannelResult.Channel->PinMessage(this);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinMessageResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatThreadMessage::UnpinMessageFromParentChannel()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	
	FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(ParentChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result);
	
	FPubnubChatMessageResult PinnedMessageResult = GetChannelResult.Channel->GetPinnedMessage();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinnedMessageResult.Result);
	
	//Unpin message only if this message is actually pinned to the channel
	if (PinnedMessageResult.Message && PinnedMessageResult.Message->GetMessageTimetoken() == Timetoken)
	{
		FPubnubChatOperationResult PinResult = GetChannelResult.Channel->UnpinMessage();
		PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinResult);
	}
	return FinalResult;
}

void UPubnubChatThreadMessage::InitThreadMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken, const FString InParentChannelID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InParentChannelID.IsEmpty(), TEXT("Can't init Thread Message, InParentChannelID is empty"));
	
	ParentChannelID = InParentChannelID;
	
	InitMessage(InPubnubClient, InChat, InChannelID, InTimetoken);
}
