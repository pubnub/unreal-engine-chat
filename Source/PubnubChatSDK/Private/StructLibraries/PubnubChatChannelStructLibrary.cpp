// Copyright 2026 PubNub Inc. All Rights Reserved.


#include "StructLibraries/PubnubChatChannelStructLibrary.h"

FPubnubChannelInputData FPubnubChatChannelData::ToPubnubChannelInputData() const
{
	FPubnubChannelInputData ChannelData;
	ChannelData.ChannelName = ChannelName;
	ChannelData.Description = Description;
	ChannelData.Custom = Custom;
	ChannelData.Status = Status;
	ChannelData.Type = Type;
	ChannelData.ForceSetAllFields();
	return ChannelData;
}

FPubnubChatChannelData FPubnubChatChannelData::FromPubnubChannelData(const FPubnubChannelData& PubnubChannelData)
{
	FPubnubChatChannelData ChatChannelData;
	ChatChannelData.ChannelName = PubnubChannelData.ChannelName;
	ChatChannelData.Description = PubnubChannelData.Description;
	ChatChannelData.Custom = PubnubChannelData.Custom;
	ChatChannelData.Status = PubnubChannelData.Status;
	ChatChannelData.Type = PubnubChannelData.Type;

	return ChatChannelData;
}

FPubnubChannelInputData FPubnubChatUpdateChannelInputData::ToPubnubChannelInputData() const
{
	FPubnubChannelInputData ChannelData;
	ChannelData.ChannelName = ChannelName;
	ChannelData.Description = Description;
	ChannelData.Custom = Custom;
	ChannelData.Status = Status;
	ChannelData.Type = Type;
	ChannelData.ForceSetChannelName = ForceSetChannelName;
	ChannelData.ForceSetDescription = ForceSetDescription;
	ChannelData.ForceSetCustom = ForceSetCustom;
	ChannelData.ForceSetStatus = ForceSetStatus;
	ChannelData.ForceSetType = ForceSetType;
	return ChannelData;
}

FPubnubChatUpdateChannelInputData FPubnubChatUpdateChannelInputData::FromChatChannelData(const FPubnubChatChannelData& PubnubChannelData)
{
	FPubnubChatUpdateChannelInputData UpdateChannelInputData;
	UpdateChannelInputData.ChannelName = PubnubChannelData.ChannelName;
	UpdateChannelInputData.Description = PubnubChannelData.Description;
	UpdateChannelInputData.Custom = PubnubChannelData.Custom;
	UpdateChannelInputData.Status = PubnubChannelData.Status;
	UpdateChannelInputData.Type = PubnubChannelData.Type;
	
	return UpdateChannelInputData;
}
