// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessage.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"


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

FString UPubnubChatMessage::GetCurrentText()
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED("");
	
	FPubnubChatMessageData MessageData = GetMessageData();
	
	//Quick return - if there are no message actions MessageData.Text is the actual text
	if (MessageData.MessageActions.IsEmpty())
	{ return MessageData.Text; }
	
	// Filter all edited message actions
	TArray<FPubnubChatMessageAction> EditedActions;
	for (const FPubnubChatMessageAction& Action : MessageData.MessageActions)
	{
		if (Action.Type == EPubnubChatMessageActionType::PCMAT_Edited)
		{
			EditedActions.Add(Action);
		}
	}
	
	// If no edited actions found, return original text
	if (EditedActions.IsEmpty())
	{
		return MessageData.Text;
	}
	
	// Sort edited actions by timetoken (ascending order - most recent will be last)
	UPubnubChatInternalUtilities::SortMessageActionsByTimetoken(EditedActions);
	
	// Return the most recent edit (last in sorted array)
	return EditedActions.Last().Value;
}

FPubnubChatOperationResult UPubnubChatMessage::EditText(const FString NewText)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(NewText);
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	//Add message action by Pubnub Client
	FString ActionType = UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType::PCMAT_Edited);
	FPubnubAddMessageActionResult AddActionResult =  PubnubClient->AddMessageAction(CurrentMessageData.ChannelID, GetMessageTimetoken(), ActionType, NewText);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, AddActionResult.Result, "AddMessageAction");
	
	//Add this new message action to MessageData and update ObjectsRepository
	CurrentMessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(AddActionResult.MessageActionData));
	Chat->ObjectsRepository->UpdateMessageData(GetInternalMessageID(), CurrentMessageData);
	
	return FinalResult;
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

