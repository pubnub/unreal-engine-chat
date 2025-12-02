// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatSubsystem.h"


void UPubnubChatUser::InitUser(UPubnubClient* InPubnubClient, const FPubnubUserData& InUserData)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init User, PubnubClient is invalid"));
		return;
	}
	
	PubnubClient = InPubnubClient;
	UserData = InUserData;
	
	IsInitialized = true;
}
