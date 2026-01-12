// Fill out your copyright notice in the Description page of Project Settings.


#include "StructLibraries/PubnubChatUserStructLibrary.h"

FPubnubUserInputData FPubnubChatUserData::ToPubnubUserInputData() const
{
	FPubnubUserInputData UserData;
	UserData.UserName = UserName;
	UserData.ExternalID = ExternalID;
	UserData.ProfileUrl = ProfileUrl;
	UserData.Email = Email;
	UserData.Custom = Custom;
	UserData.Status = Status;
	UserData.Type = Type;
	UserData.ForceSetAllFields();
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

FPubnubUserInputData FPubnubChatUpdateUserInputData::ToPubnubUserInputData() const
{
	FPubnubUserInputData UserData;
	UserData.UserName = UserName;
	UserData.ExternalID = ExternalID;
	UserData.ProfileUrl = ProfileUrl;
	UserData.Email = Email;
	UserData.Custom = Custom;
	UserData.Status = Status;
	UserData.Type = Type;
	UserData.ForceSetUserName = ForceSetUserName;
	UserData.ForceSetExternalID = ForceSetExternalID;
	UserData.ForceSetProfileUrl = ForceSetProfileUrl;
	UserData.ForceSetEmail = ForceSetEmail;
	UserData.ForceSetCustom = ForceSetCustom;
	UserData.ForceSetStatus = ForceSetStatus;
	UserData.ForceSetType = ForceSetType;
	return UserData;
}

FPubnubChatUpdateUserInputData FPubnubChatUpdateUserInputData::FromChatUserData(const FPubnubChatUserData& PubnubUserData)
{
	FPubnubChatUpdateUserInputData UpdateUserInputData;
	UpdateUserInputData.UserName = PubnubUserData.UserName;
	UpdateUserInputData.ExternalID = PubnubUserData.ExternalID;
	UpdateUserInputData.ProfileUrl = PubnubUserData.ProfileUrl;
	UpdateUserInputData.Email = PubnubUserData.Email;
	UpdateUserInputData.Custom = PubnubUserData.Custom;
	UpdateUserInputData.Status = PubnubUserData.Status;
	UpdateUserInputData.Type = PubnubUserData.Type;
	
	return UpdateUserInputData;
}
