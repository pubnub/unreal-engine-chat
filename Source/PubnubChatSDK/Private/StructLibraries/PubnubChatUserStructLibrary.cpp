// Fill out your copyright notice in the Description page of Project Settings.


#include "StructLibraries/PubnubChatUserStructLibrary.h"

FPubnubUserData FPubnubChatUserData::ToPubnubUserData() const
{
	FPubnubUserData UserData;
	UserData.UserName = UserName;
	UserData.ExternalID = ExternalID;
	UserData.ProfileUrl = ProfileUrl;
	UserData.Email = Email;
	UserData.Custom = Custom;
	UserData.Status = Status;
	UserData.Type = Type;
	return UserData;
}

FPubnubChatUserData FPubnubChatUserData::FromPubnubUserData(const FPubnubUserData& PubnubUserData)
{
	FPubnubChatUserData ChatUserData;
	ChatUserData.UserName = PubnubUserData.UserName;
	ChatUserData.ExternalID = PubnubUserData.ExternalID;
	ChatUserData.ProfileUrl = PubnubUserData.ProfileUrl;
	ChatUserData.Email = PubnubUserData.Email;
	ChatUserData.Custom = PubnubUserData.Custom;
	ChatUserData.Status = PubnubUserData.Status;
	ChatUserData.Type = PubnubUserData.Type;

	return ChatUserData;
}
