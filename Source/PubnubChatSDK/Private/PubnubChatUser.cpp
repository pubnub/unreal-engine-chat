// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatSubsystem.h"


void UPubnubChatUser::BeginDestroy()
{
	UObject::BeginDestroy();
	IsInitialized = false;
}

void UPubnubChatUser::InitUser(UPubnubClient* InPubnubClient, const FString InUserID, const FPubnubChatUserData& InUserData)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init User, PubnubClient is invalid"));
		return;
	}

	UserID = InUserID;
	PubnubClient = InPubnubClient;
	UserData = InUserData;
	
	IsInitialized = true;
}
