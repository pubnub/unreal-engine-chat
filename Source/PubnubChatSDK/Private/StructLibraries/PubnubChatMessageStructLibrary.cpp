// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"

FPubnubMessageActionData FPubnubChatMessageAction::ToPubnubMessageActionData() const
{
	FPubnubMessageActionData MessageActionData;
	
	MessageActionData.Type = UPubnubChatInternalConverters::ChatMessageActionTypeToString(Type);
	MessageActionData.Value = Value;
	MessageActionData.ActionTimetoken = Timetoken;
	MessageActionData.UserID = UserID;
	
	return MessageActionData;
}

FPubnubChatMessageAction FPubnubChatMessageAction::FromPubnubMessageActionData(const FPubnubMessageActionData& PubnubMessageActionData)
{
	FPubnubChatMessageAction ChatMessageAction;
	
	ChatMessageAction.Type = UPubnubChatInternalConverters::StringToChatMessageActionType(PubnubMessageActionData.Type);
	ChatMessageAction.Value = PubnubMessageActionData.Value;
	ChatMessageAction.Timetoken = PubnubMessageActionData.ActionTimetoken;
	ChatMessageAction.UserID = PubnubMessageActionData.UserID;
	
	return ChatMessageAction;
}

FPubnubChatMessageData FPubnubChatMessageData::FromPubnubMessageData(const FPubnubMessageData& PubnubMessageData)
{
	FPubnubChatMessageData ChatMessageData;
	ChatMessageData.Text = UPubnubChatInternalUtilities::PublishedStringToChatMessage(PubnubMessageData.Message);
	ChatMessageData.ChannelID = PubnubMessageData.Channel;
	ChatMessageData.UserID = PubnubMessageData.UserID;
	ChatMessageData.Meta = PubnubMessageData.Metadata;
	ChatMessageData.Type = PubnubMessageData.CustomMessageType;
	
	return ChatMessageData;
}

