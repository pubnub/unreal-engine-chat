// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatThreadChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"



void UPubnubChatThreadChannel::InitThreadChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InThreadChannelID, UPubnubChatMessage* InParentMessage, bool InIsThreadConfirmed)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InParentMessage, TEXT("Can't init Thread Channel, InParentMessage is invalid"));
	
	ParentChannelID = InParentMessage->GetMessageData().ChannelID;
	ParentMessage = InParentMessage;
	IsThreadConfirmed = InIsThreadConfirmed;
	
	InitChannel(InPubnubClient, InChat, InThreadChannelID);
}

FPubnubChatOperationResult UPubnubChatThreadChannel::OnSendText()
{
	//If thread is confirmed (exists on server), there is no need to add it again
	if (IsThreadConfirmed)
	{
		return FPubnubChatOperationResult();
	}
	
	//Thread is firstly created as local object and send to server during first SendText.
	
	FPubnubChatOperationResult FinalResult;
	
	//Set Channel Metadata for ThreadChannel using PubnubClient
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, GetChannelData().ToPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//AddMessageAction for the Parent Message, so it knows that is the Root of this ThreadChannel
	FString ActionType = UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType::PCMAT_ThreadRootId);
	FPubnubAddMessageActionResult AddMessageActionResult = PubnubClient->AddMessageAction(ParentChannelID, ParentMessage->GetMessageTimetoken(), ActionType, ChannelID);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, AddMessageActionResult.Result, "AddMessageAction");
	
	//Update MessageData with ThreadRoot MessageAction and update ObjectsRepository with that new MessageData
	FPubnubChatMessageData MessageData = ParentMessage->GetMessageData();
	MessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(AddMessageActionResult.MessageActionData));
	ParentMessage->UpdateMessageData(MessageData);
	
	//Now this thread is Confirmed to be on the server
	IsThreadConfirmed = true;
	
	return FinalResult;
}
