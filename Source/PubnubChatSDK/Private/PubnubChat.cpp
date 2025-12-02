// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"
#include "PubnubClient.h"
#include "PubnubEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatSubsystem.h"
#include "PubnubMacroUtilities.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"


DEFINE_LOG_CATEGORY(PubnubChatLog)



void UPubnubChat::DestroyChat()
{
	
	PubnubClient->OnPubnubSubscriptionStatusChanged.RemoveDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);
	
	OnChatDestroyed.Broadcast();
	OnChatDestroyedNative.Broadcast();
}



void UPubnubChat::InitChat(const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient)
{
	if(!InPubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Can't init Chat, PubnubClient is invalid"));
		return;
	}

	ChatConfig = InChatConfig;
	PubnubClient = InPubnubClient;

	//Add callback for subscription status - it will be translated to chat connection status
	PubnubClient->OnPubnubSubscriptionStatusChanged.AddDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	IsInitialized = true;
}

void UPubnubChat::OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData)
{
	//Don't call the listener if the subscription status is changed - this type is not supported in Chat SDK
	if(Status == EPubnubSubscriptionStatus::PSS_SubscriptionChanged)
	{
		return;
	}
	
	TWeakObjectPtr<UPubnubChat> WeakThis(this);
	AsyncTask(ENamedThreads::GameThread, [WeakThis, Status, StatusData]()
	{
		if(!WeakThis.IsValid())
		{return;}
		
		WeakThis.Get()->OnConnectionStatusChanged.Broadcast(UPubnubChatInternalConverters::SubscriptionStatusToChatConnectionStatus(Status), UPubnubChatInternalConverters::SubscriptionStatusDataToChatConnectionStatusData(StatusData));
		WeakThis.Get()->OnConnectionStatusChangedNative.Broadcast(UPubnubChatInternalConverters::SubscriptionStatusToChatConnectionStatus(Status), UPubnubChatInternalConverters::SubscriptionStatusDataToChatConnectionStatusData(StatusData));
	});
}

UPubnubChatUser* UPubnubChat::GetUserForInit(const FString UserID)
{
	if(!PubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("GetUserForInit failed: PubnubClient is invalid"));
		return nullptr;
	}

	FPubnubUserData FinalUserData;
	
	//Try to get user from the server
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	if(!GetUserResult.Result.Error)
	{
		FinalUserData = GetUserResult.UserData;
	}
	else
	{
		//If user doesn't exist on the server, just create it
		FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, FPubnubUserData());
		if(SetUserResult.Result.Error)
		{
			//TODO:: Print error log here
			return nullptr;
		}
		FinalUserData = GetUserResult.UserData;
	}
    
	// Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, FinalUserData);
	return NewUser;
}
