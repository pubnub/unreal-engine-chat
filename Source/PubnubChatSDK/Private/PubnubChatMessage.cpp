// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessage.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatUser.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
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
	CleanUp();
	
	Super::BeginDestroy();
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
	
	//If Message has thread, also Delete it, but only if Message.Delete succeeded 
	FPubnubChatThreadChannelResult GetThreadResult = GetThread();
	
	if (!Soft)
	{
		//Hard Delete - really remove message from the server
		
		FPubnubDeleteMessagesSettings DeleteSettings;
		DeleteSettings.Start = UPubnubTimetokenUtilities::AddIntToTimetoken(Timetoken, 1);
		DeleteSettings.End = Timetoken;
		FPubnubOperationResult DeleteResult = PubnubClient->DeleteMessages(CurrentMessageData.ChannelID, DeleteSettings);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, DeleteResult, "DeleteMessages");
		
		//Remove Message data from the repository
		Chat->ObjectsRepository->RemoveMessageData(GetInternalMessageID());
	}
	else
	{
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
	}
	
	//Now we can Delete thread if it exists
	if (GetThreadResult.ThreadChannel)
	{
		FPubnubChatOperationResult DeleteThreadResult = GetThreadResult.ThreadChannel->Delete(Soft);
		PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, DeleteThreadResult);
	}
	
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
	
	//If Message has thread, also restore it
	FPubnubChatThreadChannelResult GetThreadResult = GetThread();
	if (GetThreadResult.ThreadChannel)
	{
		GetThreadResult.ThreadChannel->Restore();
	}
	
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
	
	FPubnubChatMessageResult PinnedMessageResult = ChannelResult.Channel->GetPinnedMessage();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinnedMessageResult.Result);
	
	//Unpin message only if this message is actually pinned to the channel
	if (PinnedMessageResult.Message && PinnedMessageResult.Message->Timetoken == Timetoken)
	{
		FPubnubChatOperationResult PinResult = ChannelResult.Channel->UnpinMessage();
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

FPubnubChatOperationResult UPubnubChatMessage::Forward(UPubnubChatChannel* Channel)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Channel);
	
	return Chat->ForwardMessage(this, Channel);
}

FPubnubChatOperationResult UPubnubChatMessage::Report(const FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	FPubnubChatMessageData CurrentMessageData = GetMessageData();
	
	FString EventChannel = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(CurrentMessageData.ChannelID);
	FString EventPayload = UPubnubChatInternalUtilities::GetReportMessageEventPayload(GetCurrentText(), Reason, CurrentMessageData.ChannelID, CurrentMessageData.UserID, Timetoken);
	FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Report, EventChannel, EventPayload);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, EmitEventResult);
	
	return FinalResult;
}

void UPubnubChatMessage::InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Message, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Message, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InChannelID.IsEmpty(), TEXT("Can't init Message, ChannelID is empty"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InTimetoken.IsEmpty(), TEXT("Can't init Message, Timetoken is empty"));

	ChannelID = InChannelID;
	Timetoken = InTimetoken;
	PubnubClient = InPubnubClient;
	Chat = InChat;

	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(ChannelID);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ChannelEntity, TEXT("Can't init Message, Failed to create ChannelEntity"));

	UpdatesSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UpdatesSubscription, TEXT("Can't init Message, Failed to create Updates Subscription"));
	
	// Register this message object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterMessage(GetInternalMessageID());
	}
	
	//Add delegate to OnChatDestroyed to this object is cleaned up as well
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatMessage::OnChatDestroyed);
	
	IsInitialized = true;
}

void UPubnubChatMessage::UpdateMessageData(const FPubnubChatMessageData& NewMessageData)
{
	Chat->ObjectsRepository->UpdateMessageData(GetInternalMessageID(), NewMessageData);
}

FPubnubChatOperationResult UPubnubChatMessage::StreamUpdates()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already streaming
	if (IsStreamingUpdates)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatMessage> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add listener to subscription with provided callback
	UpdatesSubscription->OnPubnubMessageActionNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatMessage* ThisMessage = ThisWeak.Get();

		if(!ThisMessage->Chat)
		{return;}
		
		//If this is not MessageUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageChatMessageUpdate(MessageData.Message))
		{
			FPubnubChatMessageData ChatMessageData = ThisMessage->GetMessageData();
			
			//Update Message or skip if added/removed action is not related to that Message
			if (UPubnubChatInternalUtilities::UpdateChatMessageDataFromPubnubMessage(MessageData, ThisMessage->Timetoken, ChatMessageData))
			{
				//Update repository with new user data
				ThisMessage->Chat->ObjectsRepository->UpdateMessageData(ThisMessage->GetInternalMessageID(), ChatMessageData);
				
				//Call delegates with new user data
				ThisMessage->OnMessageUpdateReceived.Broadcast(ThisMessage->Timetoken, ChatMessageData);
				ThisMessage->OnMessageUpdateReceivedNative.Broadcast(ThisMessage->Timetoken, ChatMessageData);
			}
		}
	});
	
	//Subscribe with UpdatesSubscription to receive user metadata updates
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::StreamUpdatesOn(const TArray<UPubnubChatMessage*>& Messages)
{
	FPubnubChatOperationResult FinalResult;
	for (auto& Message : Messages)
	{
		FPubnubChatOperationResult StreamUpdatesResult = Message->StreamUpdates();
		FinalResult.Merge(StreamUpdatesResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::StopStreamingUpdates()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Remove message related delegates
	if (UpdatesSubscription)
	{
		UpdatesSubscription->OnPubnubMessageActionNative.Clear();
	}
	
	//Skip if it's not streaming updates
	if (!IsStreamingUpdates)
	{ return FinalResult; }

	//Unsubscribe and return result
	if (UpdatesSubscription)
	{
		FPubnubOperationResult UnsubscribeResult = UpdatesSubscription->Unsubscribe();
		FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	}
	IsStreamingUpdates = false;
	return FinalResult;
}

FPubnubChatThreadChannelResult UPubnubChatMessage::CreateThread()
{
	FPubnubChatThreadChannelResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->CreateThreadChannel(this);
}

FPubnubChatThreadChannelResult UPubnubChatMessage::GetThread()
{
	FPubnubChatThreadChannelResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->GetThreadChannel(this);
}

FPubnubChatHasThreadResult UPubnubChatMessage::HasThread()
{
	FPubnubChatHasThreadResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FinalResult.HasThread = UPubnubChatInternalUtilities::HasThreadRootMessageAction(GetMessageData().MessageActions);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessage::RemoveThread()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	return Chat->RemoveThreadChannel(this);
}

void UPubnubChatMessage::OnChatDestroyed(FString InUserID)
{
	CleanUp();
}

void UPubnubChatMessage::ClearAllSubscriptions()
{
	if (UpdatesSubscription)
	{
		UpdatesSubscription->OnPubnubMessageActionNative.Clear();
		if (IsStreamingUpdates)
		{
			UpdatesSubscription->Unsubscribe();
			IsStreamingUpdates = false;
		}

		UpdatesSubscription = nullptr;
	}
}

void UPubnubChatMessage::CleanUp()
{
	//Clean up subscription if message is being destroyed while streaming
	if (IsInitialized)
	{
		ClearAllSubscriptions();
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !ChannelID.IsEmpty() && !Timetoken.IsEmpty())
	{
		Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatMessage::OnChatDestroyed);
		Chat->ObjectsRepository->UnregisterMessage(GetInternalMessageID());
	}
	
	IsInitialized = false;
}

