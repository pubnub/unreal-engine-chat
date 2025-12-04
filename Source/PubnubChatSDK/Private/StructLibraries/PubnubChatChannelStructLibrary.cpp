// Fill out your copyright notice in the Description page of Project Settings.


#include "StructLibraries/PubnubChatChannelStructLibrary.h"

FPubnubChannelData FPubnubChatChannelData::ToPubnubChannelData()
{
	FPubnubChannelData ChannelData;
	ChannelData.ChannelName = ChannelName;
	ChannelData.Description = Description;
	ChannelData.Custom = Custom;
	ChannelData.Status = Status;
	ChannelData.Type = Type;
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
