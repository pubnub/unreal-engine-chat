// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalConverters.h"
#include "PubnubChatMessage.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "Algo/Sort.h"


FString UPubnubChatInternalUtilities::GetFilterForUserID(const FString& UserID)
{
	return FString::Printf(TEXT("uuid.id == \"%s\""), *UserID);
}

FString UPubnubChatInternalUtilities::GetFilterForMultipleUsersID(const TArray<UPubnubChatUser*>& Users)
{
	FString FinalFilter = "";
	for(auto& User : Users)
	{
		if(!User)
		{continue;}
		if(!FinalFilter.IsEmpty())
		{
			FinalFilter.Append(" || ");
		}
		FinalFilter.Append(FString::Printf(TEXT(R"(uuid.id == "%s")"), *User->GetUserID()));
	}
	return FinalFilter;
}

FString UPubnubChatInternalUtilities::GetFilterForChannelID(const FString& ChannelID)
{
	return FString::Printf(TEXT("channel.id == \"%s\""), *ChannelID);
}

FString UPubnubChatInternalUtilities::GetFilterForChannelsRestrictions()
{
	return FString::Printf(TEXT("channel.id LIKE \"%s*\""), *Pubnub_Chat_Moderation_Channel_Prefix);
}

FString UPubnubChatInternalUtilities::GetSoftDeletedObjectPropertyKey()
{
	return Pubnub_Chat_Soft_Deleted_Property_Name;
}

FString UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(const FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->SetBoolField(GetSoftDeletedObjectPropertyKey(), true);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(const FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->RemoveField(GetSoftDeletedObjectPropertyKey());
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

bool UPubnubChatInternalUtilities::HasDeletedPropertyInCustom(const FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	return JsonObject->HasField(GetSoftDeletedObjectPropertyKey());
}

FString UPubnubChatInternalUtilities::ChatMessageToPublishString(const FString ChatMessage)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	JsonObject->SetStringField(ANSI_TO_TCHAR("text"), ChatMessage);
	//Currently the only supported type is "text"
	JsonObject->SetStringField(ANSI_TO_TCHAR("type"), "text");

	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::PublishedStringToChatMessage(const FString PublishedMessage)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(PublishedMessage, JsonObject);
	return JsonObject->GetStringField(ANSI_TO_TCHAR("text"));
}

FString UPubnubChatInternalUtilities::SendTextMetaFromParams(const FPubnubChatSendTextParams& SendTextParams)
{
	bool AnyDataAdded = false;
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	//Handle Meta
	if(!SendTextParams.Meta.IsEmpty())
	{
		UPubnubJsonUtilities::StringToJsonObject(SendTextParams.Meta, JsonObject);
		AnyDataAdded = true;
	}

	//Add quoted message
	if(SendTextParams.QuotedMessage)
	{
		FPubnubChatMessageData QuotedMessageData = SendTextParams.QuotedMessage->GetMessageData();
		
		TSharedPtr<FJsonObject> QuotedMessageJsonObject = MakeShareable(new FJsonObject);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("timetoken"), SendTextParams.QuotedMessage->GetMessageTimetoken());
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("text"), QuotedMessageData.Text);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("userID"), QuotedMessageData.UserID);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("channelID"), QuotedMessageData.ChannelID);
		JsonObject->SetObjectField(ANSI_TO_TCHAR("quotedMessage"), QuotedMessageJsonObject);

		AnyDataAdded = true;
	}

	//Return any form of Json only if there was actually any data provided
	if(AnyDataAdded)
	{
		return UPubnubJsonUtilities::JsonObjectToString(JsonObject); 
	}

	return "";
}

FString UPubnubChatInternalUtilities::GetForwardedMessageMeta(const FString& OriginalMessageMeta, const FString& UserID, const FString& ChannelID)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(OriginalMessageMeta, JsonObject);
	
	JsonObject->SetStringField(Pubnub_Chat_ForwardMessage_OriginalPublisher_Property_Name, UserID);
	JsonObject->SetStringField(Pubnub_Chat_ForwardMessage_OriginalChannelID_Property_Name, ChannelID);
	
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(const FString ChannelID)
{
	return FString::Printf(TEXT("%s_%s"), *Pubnub_Chat_Moderation_Channel_Prefix, *ChannelID);
}

FString UPubnubChatInternalUtilities::GetModerationEventChannelForUserID(const FString UserID)
{
	return FString::Printf(TEXT("%s.%s"), *Pubnub_Chat_Moderation_Channel_Prefix, *UserID);
}

FString UPubnubChatInternalUtilities::GetChannelIDFromModerationChannel(const FString ModerationChannelID)
{
	// Extract actual channel ID from moderation channel ID (format: PUBNUB_INTERNAL_MODERATION_channelID)
	FString ModerationPrefix = Pubnub_Chat_Moderation_Channel_Prefix + TEXT("_");
	if(ModerationChannelID.StartsWith(ModerationPrefix))
	{
		return ModerationChannelID.RightChop(ModerationPrefix.Len());
	}
	
	// Fallback if format is unexpected - return the original string
	return ModerationChannelID;
}

FString UPubnubChatInternalUtilities::GetChannelMemberCustomForRestriction(const FPubnubChatRestriction& Restriction)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetBoolField(ANSI_TO_TCHAR("ban"), Restriction.Ban);
	JsonObject->SetBoolField(ANSI_TO_TCHAR("mute"), Restriction.Mute);
	JsonObject->SetStringField(ANSI_TO_TCHAR("reason"), Restriction.Reason);
	
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FPubnubChatRestriction UPubnubChatInternalUtilities::GetRestrictionFromChannelMemberCustom(const FString& Custom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(Custom, JsonObject);
	
	FPubnubChatRestriction Restriction;
	JsonObject->TryGetBoolField(ANSI_TO_TCHAR("ban"), Restriction.Ban);
	JsonObject->TryGetBoolField(ANSI_TO_TCHAR("mute"), Restriction.Mute);
	JsonObject->TryGetStringField(ANSI_TO_TCHAR("reason"), Restriction.Reason);
	
	return Restriction;
}

EPubnubChatEventMethod UPubnubChatInternalUtilities::GetDefaultChatEventMethodForEventType(EPubnubChatEventType EventType)
{
	switch(EventType)
	{
	case EPubnubChatEventType::PCET_Receipt:
		return EPubnubChatEventMethod::PCEM_Signal;
	case EPubnubChatEventType::PCET_Typing:
		return EPubnubChatEventMethod::PCEM_Signal;
	default:
		return EPubnubChatEventMethod::PCEM_Publish;
	}
}

FPubnubChatEvent UPubnubChatInternalUtilities::GetEventFromPubnubMessageData(const FPubnubMessageData& MessageData)
{
	FPubnubChatEvent Event;
	Event.Timetoken = MessageData.Timetoken;
	Event.ChannelID = MessageData.Channel;
	Event.UserID = MessageData.UserID;

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageData.Message, JsonObject);

	//Type is in Message content, so we need to extract it from there
	FString Type;
	JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type);
	Event.Type = UPubnubChatInternalConverters::StringToChatEventType(Type);

	//Event type shouldn't be in the payload, so we have to remove it. Remaining message content is the payload
	JsonObject->RemoveField(ANSI_TO_TCHAR("type"));
	Event.Payload = UPubnubJsonUtilities::JsonObjectToString(JsonObject);

	return Event;
}

//This function assumes that IsThisEventMessage was called before and this is really Message representing a ChatEvent
FPubnubChatEvent UPubnubChatInternalUtilities::GetEventFromPubnubHistoryMessageData(const FPubnubHistoryMessageData& MessageData)
{
	FPubnubChatEvent Event;
	Event.ChannelID = MessageData.Channel;
	Event.UserID = MessageData.UserID;
	Event.Timetoken = MessageData.Timetoken;
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageData.Message, JsonObject);
	
	//Get Event type and remove it from the Payload
	FString Type;
	JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type);
	Event.Type = UPubnubChatInternalConverters::StringToChatEventType(Type);

	//Event type shouldn't be in the payload, so we have to remove it. Remaining message content is the payload
	JsonObject->RemoveField(ANSI_TO_TCHAR("type"));
	Event.Payload = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
	
	return Event;
}

FString UPubnubChatInternalUtilities::GetReceiptEventPayload(const FString& Timetoken)
{
	return FString::Printf(TEXT(R"({"messageTimetoken": "%s"})"), *Timetoken);
}

FString UPubnubChatInternalUtilities::GetInviteEventPayload(const FString ChannelID, const FString ChannelType)
{
	return FString::Printf(TEXT(R"({"channelType": "%s", "channelId": "%s"})"), *ChannelType, *ChannelID);
}

FString UPubnubChatInternalUtilities::GetModerationEventPayload(const FString ModerationChannel, const FString RestrictionType, const FString Reason)
{
	return FString::Printf(TEXT(R"({"channelId": "%s", "restriction": "%s", "reason": "%s"})"), *ModerationChannel, *RestrictionType, *Reason);
}

bool UPubnubChatInternalUtilities::IsThisEventMessage(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString Type;
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type))
	{ return false; }
	
	//Message is an event if it has type field that matches any actual event type
	for (EPubnubChatEventType EventType : TEnumRange<EPubnubChatEventType>())
	{
		if (Type == UPubnubChatInternalConverters::ChatEventTypeToString(EventType))
		{
			return true;
		}
	}
	
	return false;
}

FString UPubnubChatInternalUtilities::GetMentionEventPayload(const FString& ChannelID, const FString& Timetoken, const FString& Text, const FString& ParentChannel)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(ANSI_TO_TCHAR("channel"), ChannelID);
	JsonObject->SetStringField(ANSI_TO_TCHAR("messageTimetoken"), Timetoken);
	JsonObject->SetStringField(ANSI_TO_TCHAR("text"), Text);
	
	if (!ParentChannel.IsEmpty())
	{
		JsonObject->SetStringField(ANSI_TO_TCHAR("parentChannel"), ParentChannel);
	}
	
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::GetReportMessageEventPayload(const FString& Text, const FString& Reason, const FString& ChannelID, const FString& UserID, const FString& Timetoken)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(ANSI_TO_TCHAR("text"), Text);
	JsonObject->SetStringField(ANSI_TO_TCHAR("reason"), Reason);
	JsonObject->SetStringField(ANSI_TO_TCHAR("timetoken"), Timetoken);
	JsonObject->SetStringField(ANSI_TO_TCHAR("channelId"), ChannelID);
	JsonObject->SetStringField(ANSI_TO_TCHAR("userId"), UserID);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::GetTypingEventPayload(const bool IsTyping)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetBoolField(ANSI_TO_TCHAR("value"), IsTyping);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

bool UPubnubChatInternalUtilities::GetIsTypingFromEventPayload(const FString& EventPayload)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(EventPayload, JsonObject);
	bool IsTyping;
	JsonObject->TryGetBoolField(ANSI_TO_TCHAR("value"), IsTyping);
	return IsTyping;
}

FString UPubnubChatInternalUtilities::GetLastReadMessageTimetokenPropertyKey()
{
	return Pubnub_Chat_LRMT_Property_Name;
}

void UPubnubChatInternalUtilities::AddLastReadMessageTimetokenToMembershipData(FPubnubChatMembershipData& MembershipData, const FString Timetoken)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if(!MembershipData.Custom.IsEmpty())
	{
		UPubnubJsonUtilities::StringToJsonObject(MembershipData.Custom, JsonObject);
	}
	JsonObject->SetStringField(GetLastReadMessageTimetokenPropertyKey(), Timetoken);
	MembershipData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::GetLastReadMessageTimetokenFromMembershipData(const FPubnubChatMembershipData& MembershipData)
{
	if(MembershipData.Custom.IsEmpty())
	{ return ""; }
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MembershipData.Custom, JsonObject);
	FString LRMTimetoken;
	JsonObject->TryGetStringField(GetLastReadMessageTimetokenPropertyKey(), LRMTimetoken);
	
	return LRMTimetoken;
}

bool UPubnubChatInternalUtilities::IsPubnubInternalChannel(const FString& ChannelID)
{
	if (ChannelID.Contains(Pubnub_Chat_Moderation_Channel_Prefix))
	{
		return true;
	}
	
	return false;
}

FString UPubnubChatInternalUtilities::GetPinnedMessageTimetokenPropertyKey()
{
	return Pubnub_Chat_PinnedMessageTimetoken_Property_Name;
}

FString UPubnubChatInternalUtilities::GetPinnedMessageChannelIDPropertyKey()
{
	return Pubnub_Chat_PinnedMessageChannelID_Property_Name;
}

//Message should be validated before using this function
void UPubnubChatInternalUtilities::AddPinnedMessageToChannelData(FPubnubChatChannelData& ChannelData, UPubnubChatMessage* Message)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	JsonObject->SetStringField(GetPinnedMessageTimetokenPropertyKey(), Message->GetMessageTimetoken());
	JsonObject->SetStringField(GetPinnedMessageChannelIDPropertyKey(), Message->GetMessageData().ChannelID);
	ChannelData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

bool UPubnubChatInternalUtilities::RemovePinnedMessageFromChannelData(FPubnubChatChannelData& ChannelData)
{
	bool RemovedPinnedMessage = false;
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	if(JsonObject->HasField(GetPinnedMessageTimetokenPropertyKey()))
	{
		RemovedPinnedMessage = true;
		JsonObject->RemoveField(GetPinnedMessageTimetokenPropertyKey());
	}
	if(JsonObject->HasField(GetPinnedMessageChannelIDPropertyKey()))
	{
		RemovedPinnedMessage = true;
		JsonObject->RemoveField(GetPinnedMessageChannelIDPropertyKey());
	}
	if(RemovedPinnedMessage)
	{
		ChannelData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
	}
	return RemovedPinnedMessage;
}

TArray<FPubnubChatMessageAction> UPubnubChatInternalUtilities::FilterMessageActionsOfType(const TArray<FPubnubChatMessageAction>& MessageActions, const EPubnubChatMessageActionType& MessageActionType)
{
	TArray<FPubnubChatMessageAction> FilteredMessageActions;
	for (auto& MessageAction : MessageActions)
	{
		if (MessageAction.Type == MessageActionType)
		{
			FilteredMessageActions.Add(MessageAction);
		}
	}
	return FilteredMessageActions;
}

FPubnubChatMessageAction UPubnubChatInternalUtilities::GetMessageReactionForUserID(const TArray<FPubnubChatMessageAction>& MessageReactions, const FString& Reaction, const FString& UserID)
{
	for (auto& MessageReaction : MessageReactions)
	{
		if (MessageReaction.Value == Reaction && MessageReaction.UserID == UserID)
		{
			return MessageReaction;
		}
	}
	
	return FPubnubChatMessageAction();
}

bool UPubnubChatInternalUtilities::RemoveReactionFromReactionsArray(TArray<FPubnubChatMessageAction>& MessageReactions, const FPubnubChatMessageAction& Reaction)
{
	for (int i = 0; i < MessageReactions.Num(); i++)
	{
		if (MessageReactions[i].UserID == Reaction.UserID && MessageReactions[i].Timetoken == Reaction.Timetoken)
		{
			MessageReactions.RemoveAt(i);
			return true;
		}
	}
	return false;
	
}

bool UPubnubChatInternalUtilities::IsChatMessageActionEqualPubnubAction(const FPubnubChatMessageAction& ChatAction, const FPubnubMessageActionData& PubnubAction)
{
	//Message actions are equal if all those fields are exactly the same
	if (ChatAction.UserID ==  PubnubAction.UserID 
		&& ChatAction.Timetoken == PubnubAction.ActionTimetoken 
		&& UPubnubChatInternalConverters::ChatMessageActionTypeToString(ChatAction.Type) == PubnubAction.Type
		&& ChatAction.Value == PubnubAction.Value)
	{
		return true;
	}
	return false;
}

bool UPubnubChatInternalUtilities::IsPubnubMessageChannelUpdate(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString Source;
	FString Type;
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("source"), Source))
	{ return false; }
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type))
	{ return false; }
	
	//Pubnub Core SDK Message is Channel Update if those 2 fields are exactly matching
	if (Source == "objects" && Type == "channel")
	{ return true; }
	
	return false;
}

bool UPubnubChatInternalUtilities::IsPubnubMessageUserUpdate(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString Source;
	FString Type;
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("source"), Source))
	{ return false; }
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type))
	{ return false; }
	
	//Pubnub Core SDK Message is User Update if those 2 fields are exactly matching
	if (Source == "objects" && Type == "uuid")
	{ return true; }
	
	return false;
}

bool UPubnubChatInternalUtilities::IsPubnubMessageMembershipUpdate(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString Source;
	FString Type;
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("source"), Source))
	{ return false; }
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("type"), Type))
	{ return false; }
	
	//Pubnub Core SDK Message is Membership Update if those 2 fields are exactly matching
	if (Source == "objects" && Type == "membership")
	{ return true; }
	
	return false;
}

bool UPubnubChatInternalUtilities::IsPubnubMessageChatMessageUpdate(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString Source;
	if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("source"), Source))
	{ return false; }
	
	//ChatMessage update is actually adding or removing a message action
	if (Source == "actions")
	{ return true; }
	
	return false;
}

bool UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(const FString& MessageContent)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageContent, JsonObject);
	
	FString EventType;
	if (JsonObject->TryGetStringField(ANSI_TO_TCHAR("event"), EventType))
	{
		return EventType == "delete";
	}
	return false;
}

void UPubnubChatInternalUtilities::UpdateChatChannelFromPubnubChannelUpdateData(const FPubnubChannelUpdateData& PubnubChannelUpdateData, FPubnubChatChannelData& ChannelData)
{
	if (PubnubChannelUpdateData.ChannelNameUpdated)
	{ ChannelData.ChannelName = PubnubChannelUpdateData.ChannelName; }
	if (PubnubChannelUpdateData.DescriptionUpdated)
	{ ChannelData.Description = PubnubChannelUpdateData.Description; }
	if (PubnubChannelUpdateData.CustomUpdated)
	{ ChannelData.Custom = PubnubChannelUpdateData.Custom; }
	if (PubnubChannelUpdateData.StatusUpdated)
	{ ChannelData.Status = PubnubChannelUpdateData.Status; }
	if (PubnubChannelUpdateData.TypeUpdated)
	{ ChannelData.Type = PubnubChannelUpdateData.Type; }
}

void UPubnubChatInternalUtilities::UpdateChatUserFromPubnubUserUpdateData(const FPubnubUserUpdateData& PubnubUserUpdateData, FPubnubChatUserData& UserData)
{
	if (PubnubUserUpdateData.UserNameUpdated)
	{ UserData.UserName = PubnubUserUpdateData.UserName; }
	if (PubnubUserUpdateData.ExternalIDUpdated)
	{ UserData.ExternalID = PubnubUserUpdateData.ExternalID; }
	if (PubnubUserUpdateData.ProfileUrlUpdated)
	{ UserData.ProfileUrl = PubnubUserUpdateData.ProfileUrl; }
	if (PubnubUserUpdateData.EmailUpdated)
	{ UserData.Email = PubnubUserUpdateData.Email; }
	if (PubnubUserUpdateData.CustomUpdated)
	{ UserData.Custom = PubnubUserUpdateData.Custom; }
	if (PubnubUserUpdateData.StatusUpdated)
	{ UserData.Status = PubnubUserUpdateData.Status; }
	if (PubnubUserUpdateData.TypeUpdated)
	{ UserData.Type = PubnubUserUpdateData.Type; }
}

void UPubnubChatInternalUtilities::UpdateChatMembershipFromPubnubMembershipUpdateData(const FPubnubMembershipUpdateData& PubnubMembershipUpdateData, FPubnubChatMembershipData& MembershipData)
{
	if (PubnubMembershipUpdateData.CustomUpdated)
	{ MembershipData.Custom = PubnubMembershipUpdateData.Custom; }
	if (PubnubMembershipUpdateData.StatusUpdated)
	{ MembershipData.Status = PubnubMembershipUpdateData.Status; }
	if (PubnubMembershipUpdateData.TypeUpdated)
	{ MembershipData.Type = PubnubMembershipUpdateData.Type; }
}

bool UPubnubChatInternalUtilities::UpdateChatMessageDataFromPubnubMessage(const FPubnubMessageData& MessageData, const FString& ChatMessageTimetoken, FPubnubChatMessageData& ChatMessageData)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageData.Message, JsonObject);
	bool IsAddingMessageAction = false;
	
	FString Event;
	if (JsonObject->TryGetStringField(ANSI_TO_TCHAR("event"), Event))
	{
		IsAddingMessageAction = Event == "added";
	}
	
	FPubnubMessageActionData ActionData = UPubnubJsonUtilities::GetMessageActionFromMessageData(MessageData);
	
	//Make sure that MessageAction was added/removed to the given ChatMessage (by comparing timetokens)
	if (ChatMessageTimetoken != ActionData.MessageTimetoken)
	{ return false; }
	
	//If the message means that MessageAction was added, simply add new Message Action from MessageData
	if (IsAddingMessageAction)
	{
		ChatMessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(ActionData));
	}
	//If Message Action was removed, find that Message Action in ChatMessageData and remove it
	else
	{
		for (int i = 0; i < ChatMessageData.MessageActions.Num(); i++)
		{
			if (IsChatMessageActionEqualPubnubAction(ChatMessageData.MessageActions[i], ActionData))
			{
				ChatMessageData.MessageActions.RemoveAt(i);
				return true;
			}
		}
	}
	return true;
}

void UPubnubChatInternalUtilities::RemoveExpiredTypingIndicators(TMap<FString, FTypingIndicatorData>& TypingIndicators, const int TypingTimeout, FDateTime CurrentTime)
{
	TArray<FString> ExpiredUserIDs;
	for (auto& IndicatorPair : TypingIndicators)
	{
		FTimespan TimeSinceTyping = CurrentTime - IndicatorPair.Value.LastTypingTime;
		if (TimeSinceTyping >= TypingTimeout)
		{
			// Invalidate timer before removing
			IndicatorPair.Value.TimerHandle.Invalidate();
			ExpiredUserIDs.Add(IndicatorPair.Key);
		}
	}
	for (const FString& ExpiredUserID : ExpiredUserIDs)
	{
		TypingIndicators.Remove(ExpiredUserID);
	}
}

FString UPubnubChatInternalUtilities::GetThreadID(const FString& ChannelID, const FString& Timetoken)
{
	return FString::Printf(TEXT("%s_%s_%s"), *Pubnub_Chat_Message_Thread_ID_Prefix, *ChannelID, *Timetoken);
}

FString UPubnubChatInternalUtilities::GetThreadDescription(const FString& ChannelID, const FString& Timetoken)
{
	return FString::Printf(TEXT("Thread on channel %s with message timetoken %s"), *ChannelID, *Timetoken);
}

bool UPubnubChatInternalUtilities::HasThreadRootMessageAction(const TArray<FPubnubChatMessageAction>& MessageActions)
{
	for (auto& MessageAction : MessageActions)
	{
		if (MessageAction.Type == EPubnubChatMessageActionType::PCMAT_ThreadRootId && !MessageAction.Value.IsEmpty())
		{
			return true;
		}
	}
	
	return false;
}

FPubnubChatMessageAction UPubnubChatInternalUtilities::GetThreadRootMessageAction(const TArray<FPubnubChatMessageAction>& MessageActions)
{
	for (auto& MessageAction : MessageActions)
	{
		if (MessageAction.Type == EPubnubChatMessageActionType::PCMAT_ThreadRootId && !MessageAction.Value.IsEmpty())
		{
			return MessageAction;
		}
	}
	
	return FPubnubChatMessageAction();
}

void UPubnubChatInternalUtilities::RemoveThreadRootFromMessageActions(TArray<FPubnubChatMessageAction>& MessageActions)
{
	//Scan through actions and remove those with ThreadRootId
	for (int i = MessageActions.Num() - 1; i >= 0; i--)
	{
		if (MessageActions[i].Type == EPubnubChatMessageActionType::PCMAT_ThreadRootId)
		{
			MessageActions.RemoveAt(i);
		}
	}
}

bool UPubnubChatInternalUtilities::IsChannelAThread(const FString& ChannelID)
{
	return ChannelID.StartsWith(*Pubnub_Chat_Message_Thread_ID_Prefix);
}


bool UPubnubChatInternalUtilities::CheckResourcePermission(const TSharedPtr<FJsonObject>& ResourcesObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr)
{
	if(!ResourcesObject.IsValid() || ResourceTypeStr.IsEmpty() || ResourceName.IsEmpty() || PermissionStr.IsEmpty())
	{
		return false;
	}

	// Get the resource type object (Channels or Uuids)
	const TSharedPtr<FJsonObject>* ResourceTypeObjectPtr = nullptr;
	if(!ResourcesObject->TryGetObjectField(ResourceTypeStr, ResourceTypeObjectPtr) || !ResourceTypeObjectPtr || !(*ResourceTypeObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceTypeObject = *ResourceTypeObjectPtr;

	// Get the specific resource object
	const TSharedPtr<FJsonObject>* ResourceObjectPtr = nullptr;
	if(!ResourceTypeObject->TryGetObjectField(ResourceName, ResourceObjectPtr) || !ResourceObjectPtr || !(*ResourceObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceObject = *ResourceObjectPtr;

	// Check if the permission field exists and is true
	if(!ResourceObject->HasField(PermissionStr))
	{
		return false;
	}

	return ResourceObject->GetBoolField(PermissionStr);
}

bool UPubnubChatInternalUtilities::CheckPatternPermission(const TSharedPtr<FJsonObject>& PatternsObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr)
{
	if(!PatternsObject.IsValid() || ResourceTypeStr.IsEmpty() || ResourceName.IsEmpty() || PermissionStr.IsEmpty())
	{
		return false;
	}

	// Get the resource type object (Channels or Uuids)
	const TSharedPtr<FJsonObject>* ResourceTypeObjectPtr = nullptr;
	if(!PatternsObject->TryGetObjectField(ResourceTypeStr, ResourceTypeObjectPtr) || !ResourceTypeObjectPtr || !(*ResourceTypeObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceTypeObject = *ResourceTypeObjectPtr;

	// Iterate through all patterns and check if any match the resource name
	TArray<FString> PatternKeys;
	ResourceTypeObject->Values.GetKeys(PatternKeys);

	// Check all matching patterns - return true if ANY pattern grants the permission
	for(const FString& PatternKey : PatternKeys)
	{
		// Check if the pattern matches the resource name using regex
		FRegexMatcher PatternMatcher(FRegexPattern(PatternKey), ResourceName);
		if(PatternMatcher.FindNext())
		{
			// Verify that the match spans the entire string (full match, not substring)
			// This ensures patterns like "channel-[A-Za-z0-9]" don't match "channel-abc123"
			int32 MatchStart = PatternMatcher.GetMatchBeginning();
			int32 MatchEnd = PatternMatcher.GetMatchEnding();
			
			// Only consider it a match if it spans the entire resource name
			if(MatchStart == 0 && MatchEnd == ResourceName.Len())
			{
				// Pattern matches fully, check the permission
				const TSharedPtr<FJsonObject>* PatternObjectPtr = nullptr;
				if(ResourceTypeObject->TryGetObjectField(PatternKey, PatternObjectPtr) && PatternObjectPtr && (*PatternObjectPtr).IsValid())
				{
					const TSharedPtr<FJsonObject>& PatternObject = *PatternObjectPtr;
					if(PatternObject->HasField(PermissionStr))
					{
						// If this pattern grants permission, return true immediately
						if(PatternObject->GetBoolField(PermissionStr))
						{
							return true;
						}
						// If this pattern explicitly denies permission, continue checking other patterns
						// (another pattern might grant it)
					}
				}
			}
		}
	}

	return false;
}

uint64 UPubnubChatInternalUtilities::HashString(const FString& Str, int32 Seed)
{
	// Convert FString to UTF-8 bytes for cross-platform consistency
	FTCHARToUTF8 UTF8Converter(*Str);
	const char* UTF8Bytes = UTF8Converter.Get();
	const int32 UTF8Length = UTF8Converter.Length();

	// Initialize hash accumulators with constants XORed with seed
	int32 h1 = 0xdeadbeef ^ Seed;
	int32 h2 = 0x41c6ce57 ^ Seed;

	// Process each UTF-8 byte
	for (int32 i = 0; i < UTF8Length; ++i)
	{
		const uint8 ch = static_cast<uint8>(UTF8Bytes[i]);
		h1 = (h1 ^ static_cast<int32>(ch)) * 0x85ebca77;
		h2 = (h2 ^ static_cast<int32>(ch)) * 0xc2b2ae3d;
	}

	// Final mixing
	h1 = h1 ^ ((h1 ^ (h2 >> 15)) * 0x735a2d97);
	h2 = h2 ^ ((h2 ^ (h1 >> 15)) * 0xcaf649a9);
	h1 = h1 ^ (h2 >> 16);
	h2 = h2 ^ (h1 >> 16);

	// Combine h1 and h2 into 64-bit result
	// Note: We use unsigned arithmetic to avoid sign issues
	const int64 Result = (2097152LL * static_cast<int64>(h2)) + (static_cast<int64>(h1) >> 11);

	// Handle negative result by masking upper bits (53-bit result)
	if (Result < 0)
	{
		// Mask to 53 bits: clear bits 53-63
		const uint64 Mask53Bits = ~(0xFFFFFFFFFFFFFFFFULL << 53);
		return static_cast<uint64>(Result) & Mask53Bits;
	}
	else
	{
		return static_cast<uint64>(Result);
	}
}

void UPubnubChatInternalUtilities::SortMessageActionsByTimetoken(TArray<FPubnubChatMessageAction>& MessageActions)
{
	Algo::Sort(MessageActions, [](const FPubnubChatMessageAction& A, const FPubnubChatMessageAction& B)
	{
		// Convert timetokens to int64 for proper numeric comparison
		int64 TimetokenA = 0;
		int64 TimetokenB = 0;
		
		if (!A.Timetoken.IsEmpty() && A.Timetoken.IsNumeric())
		{
			LexFromString(TimetokenA, *A.Timetoken);
		}
		
		if (!B.Timetoken.IsEmpty() && B.Timetoken.IsNumeric())
		{
			LexFromString(TimetokenB, *B.Timetoken);
		}
		
		return TimetokenA < TimetokenB;
	});
}

FString UPubnubChatInternalUtilities::GetLastActiveTimestampPropertyKey()
{
	return Pubnub_Chat_LastActiveTimestamp_Property_Name;
}

FString UPubnubChatInternalUtilities::GetLastActiveTimestampFromCustom(const FString& CurrentCustom)
{
	if (CurrentCustom.IsEmpty())
	{
		return FString();
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if (!UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject))
	{
		return FString();
	}

	FString Timestamp;
	if (JsonObject->TryGetStringField(GetLastActiveTimestampPropertyKey(), Timestamp))
	{
		return Timestamp;
	}

	return FString();
}

FString UPubnubChatInternalUtilities::AddLastActiveTimestampToCustom(const FString& CurrentCustom, const FString& Timestamp)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if (!CurrentCustom.IsEmpty())
	{
		UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	}
	
	JsonObject->SetStringField(GetLastActiveTimestampPropertyKey(), Timestamp);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}