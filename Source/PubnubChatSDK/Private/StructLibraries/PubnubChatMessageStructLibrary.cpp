// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"

FPubnubMessageActionData FPubnubChatMessageAction::ToPubnubMessageActionData()
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

FPubnubMessageData FPubnubChatMessageData::ToPubnubMessageData()
{
	FPubnubMessageData MessageData;
	MessageData.Message = Text;
	MessageData.Channel = ChannelID;
	MessageData.UserID = UserID;
	MessageData.Metadata = Meta;
	MessageData.CustomMessageType = Type;
	
	return MessageData;
}

FPubnubChatMessageData FPubnubChatMessageData::FromPubnubMessageData(const FPubnubMessageData& PubnubMessageData)
{
	FPubnubChatMessageData ChatMessageData;
	ChatMessageData.Text = PubnubMessageData.Message;
	ChatMessageData.ChannelID = PubnubMessageData.Channel;
	ChatMessageData.UserID = PubnubMessageData.UserID;
	ChatMessageData.Meta = PubnubMessageData.Metadata;
	ChatMessageData.Type = PubnubMessageData.CustomMessageType;
	
	return ChatMessageData;
}

