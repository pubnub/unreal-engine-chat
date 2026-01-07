// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessage.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatUser.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"


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

FPubnubChatOperationResult UPubnubChatMessage::Delete(bool Soft)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	//Hard Delete
	if (!Soft)
	{
		FPubnubDeleteMessagesSettings DeleteSettings;
		DeleteSettings.Start = UPubnubTimetokenUtilities::AddIntToTimetoken(Timetoken, 1);
		DeleteSettings.End = Timetoken;
		FPubnubOperationResult DeleteResult = PubnubClient->DeleteMessages(CurrentMessageData.ChannelID, DeleteSettings);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, DeleteResult, "DeleteMessages");
		
		//Remove Message data from the repository
		Chat->ObjectsRepository->RemoveMessageData(GetInternalMessageID());
		
		return FinalResult;
	}
	
	//Soft delete - just add message action without actually deleting the message
	
	//If message is already deleted, don't add new message action
	if (IsDeleted().IsDeleted)
	{ return FinalResult; }

	FString ActionType = UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType::PCMAT_Deleted);
	FPubnubAddMessageActionResult AddActionResult =  PubnubClient->AddMessageAction(CurrentMessageData.ChannelID, GetMessageTimetoken(), ActionType, Pubnub_Chat_Soft_Deleted_Action_Value);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, AddActionResult.Result, "AddMessageAction");
	
	//Add this new message action to MessageData and update ObjectsRepository
	CurrentMessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(AddActionResult.MessageActionData));
	Chat->ObjectsRepository->UpdateMessageData(GetInternalMessageID(), CurrentMessageData);
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::Restore()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	//Remove all "deleted" message actions
	for (int i = CurrentMessageData.MessageActions.Num() - 1; i >= 0; i--)
	{
		if (CurrentMessageData.MessageActions[i].Type == EPubnubChatMessageActionType::PCMAT_Deleted)
		{
			FPubnubOperationResult RemoveActionResult = PubnubClient->RemoveMessageAction(CurrentMessageData.ChannelID, Timetoken, CurrentMessageData.MessageActions[i].Timetoken);
			PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveActionResult, "RemoveMessageAction");
			CurrentMessageData.MessageActions.RemoveAt(i);
		}
	}
	
	//Update repository with new data
	Chat->ObjectsRepository->UpdateMessageData(GetInternalMessageID(), CurrentMessageData);
	
	return FinalResult;
}

FPubnubChatIsDeletedResult UPubnubChatMessage::IsDeleted()
{
	FPubnubChatIsDeletedResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	for (auto& MessageAction : CurrentMessageData.MessageActions)
	{
		if (MessageAction.Type == EPubnubChatMessageActionType::PCMAT_Deleted)
		{
			FinalResult.IsDeleted = true;
			return FinalResult;
		}
	}
	
	FinalResult.IsDeleted = false;
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::Pin()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	FPubnubChatChannelResult ChannelResult = Chat->GetChannel(CurrentMessageData.ChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ChannelResult.Result);
	
	if (!ChannelResult.Channel)
	{
		FinalResult.Error = true;
		FinalResult.ErrorMessage = TEXT("Channel related to this Message doesn't exist.");
		return FinalResult;
	}
	
	FPubnubChatOperationResult PinResult = Chat->PinMessageToChannel(this, ChannelResult.Channel);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::Unpin()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	FPubnubChatChannelResult ChannelResult = Chat->GetChannel(CurrentMessageData.ChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ChannelResult.Result);
	
	if (!ChannelResult.Channel)
	{
		FinalResult.Error = true;
		FinalResult.ErrorMessage = TEXT("Channel related to this Message doesn't exist.");
		return FinalResult;
	}
	
	FPubnubChatMessageResult PinnedMessageResult = ChannelResult.Channel->GetPinnedMessage();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinnedMessageResult.Result);
	
	//Unpin message only if this message is actually pinned to the channel
	if (PinnedMessageResult.Message && PinnedMessageResult.Message->Timetoken == Timetoken)
	{
		FPubnubChatOperationResult PinResult = Chat->UnpinMessageFromChannel(ChannelResult.Channel);
		PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::ToggleReaction(const FString Reaction)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Reaction);
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	FPubnubChatGetReactionsResult GetReactionsResult = GetReactions();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetReactionsResult.Result);
	
	FPubnubChatMessageAction ReactionToToggle = UPubnubChatInternalUtilities::GetMessageReactionForUserID(GetReactionsResult.Reactions, Reaction, Chat->CurrentUserID);
	
	//If there is already such reaction from CurrentUser, we remove it
	if (!ReactionToToggle.Timetoken.IsEmpty())
	{
		FPubnubOperationResult RemoveActionResult = PubnubClient->RemoveMessageAction(CurrentMessageData.ChannelID, Timetoken, ReactionToToggle.Timetoken);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveActionResult, "RemoveMessageAction");
		
		//Remove this message action from message data
		UPubnubChatInternalUtilities::RemoveReactionFromReactionsArray(CurrentMessageData.MessageActions, ReactionToToggle);
	}
	else
	{
		FString ActionType = UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType::PCMAT_Reaction);
		FPubnubAddMessageActionResult AddActionResult = PubnubClient->AddMessageAction(CurrentMessageData.ChannelID, Timetoken, ActionType, Reaction);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, AddActionResult.Result, "AddMessageAction");
		
		CurrentMessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(AddActionResult.MessageActionData));
	}
	
	//Update repository with new MessageData (with added or removed message action
	Chat->ObjectsRepository->UpdateMessageData(GetInternalMessageID(), CurrentMessageData);
	
	return FinalResult;
}

FPubnubChatGetReactionsResult UPubnubChatMessage::GetReactions()
{
	FPubnubChatGetReactionsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	//Filter message actions of type Reaction
	FinalResult.Reactions = UPubnubChatInternalUtilities::FilterMessageActionsOfType(CurrentMessageData.MessageActions, EPubnubChatMessageActionType::PCMAT_Reaction);
	
	return FinalResult;
}

FPubnubChatHasReactionResult UPubnubChatMessage::HasUserReaction(const FString Reaction)
{
	FPubnubChatHasReactionResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, Reaction);
	
	FPubnubChatGetReactionsResult GetReactionsResult = GetReactions();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetReactionsResult.Result);
	
	FPubnubChatMessageAction MessageReaction = UPubnubChatInternalUtilities::GetMessageReactionForUserID(GetReactionsResult.Reactions, Reaction, Chat->CurrentUserID);
	FinalResult.HasReaction = !MessageReaction.Timetoken.IsEmpty();
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

