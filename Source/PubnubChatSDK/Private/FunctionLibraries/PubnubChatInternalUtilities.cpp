// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalConverters.h"
#include "PubnubChatMessage.h"


FString UPubnubChatInternalUtilities::GetSoftDeletedObjectPropertyKey()
{
	return Pubnub_Chat_Soft_Deleted_Property_Name;
}

FString UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->SetBoolField(GetSoftDeletedObjectPropertyKey(), true);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->RemoveField(GetSoftDeletedObjectPropertyKey());
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
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
	Event.Type = UPubnubChatInternalConverters::StringToChatEventType(JsonObject->GetStringField(ANSI_TO_TCHAR("type")));

	//Event type shouldn't be in the payload, so we have to remove it. Remaining message content is the payload
	JsonObject->RemoveField(ANSI_TO_TCHAR("type"));
	Event.Payload = UPubnubJsonUtilities::JsonObjectToString(JsonObject);

	return Event;
}
