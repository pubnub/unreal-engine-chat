// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"

#include "PubnubChatInternalMacros.h"
#include "PubnubClient.h"
#include "PubnubEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatSubsystem.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"


DEFINE_LOG_CATEGORY(PubnubChatLog)



void UPubnubChat::DestroyChat()
{
	
	PubnubClient->OnPubnubSubscriptionStatusChanged.RemoveDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	IsInitialized = false;
	
	OnChatDestroyed.Broadcast();
	OnChatDestroyedNative.Broadcast();
}

UPubnubChatUser* UPubnubChat::GetCurrentUser()
{
	return CurrentUser;
}

FPubnubChatUserResult UPubnubChat::CreateUser(FString UserID, FPubnubChatUserData UserData)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//Check if such user doesn't exist. If it does, just return an error
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	if(!GetUserResult.Result.Error)
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: This user already exists. Try using GetUser instead."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		FinalResult.Result.CreateError(0, ErrorMessage);
		FinalResult.Result.AddStep("GetUserMetadata", GetUserResult.Result);
	}

	//SetUserMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserData());
	FinalResult.Result.AddStep("SetUserMetadata", SetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetUserResult.Result.Error)
	{return FinalResult;}
	
	//Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, UserID, UserData);
	FinalResult.User = NewUser;
	return FinalResult;
}

FPubnubChatUserResult UPubnubChat::GetUser(FString UserID)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//GetUserMetadata from PubnubClient
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	FinalResult.Result.AddStep("GetUserMetadata", GetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetUserResult.Result.Error)
	{return FinalResult;}

	//Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, UserID, FPubnubChatUserData::FromPubnubUserData(GetUserResult.UserData));
	FinalResult.User = NewUser;
	return FinalResult;
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

FPubnubChatInitChatResult UPubnubChat::InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient)
{
	FPubnubChatInitChatResult FinalResult;
	
	if(!InPubnubClient)
	{
		FString ErrorMessage = TEXT("Can't init Chat, PubnubClient is invalid");
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		FinalResult.Result = FPubnubChatOperationResult::CreateError(0, ErrorMessage);
		return FinalResult;
	}

	ChatConfig = InChatConfig;
	PubnubClient = InPubnubClient;
	
	//Add callback for subscription status - it will be translated to chat connection status
	PubnubClient->OnPubnubSubscriptionStatusChanged.AddDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	//Get or create user for this chat instance
	FPubnubChatUserResult GetUserForInitResult = GetUserForInit(InUserID);

	//Return if any error happened on the way
	PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(FinalResult, GetUserForInitResult);

	FinalResult.Result.Merge(GetUserForInitResult.Result);
		
	CurrentUser = GetUserForInitResult.User;
	IsInitialized = true;
	FinalResult.Chat = this;

	return FinalResult;
}

FPubnubChatUserResult UPubnubChat::GetUserForInit(const FString InUserID)
{
	FPubnubChatUserResult FinalResult;
	FPubnubUserData FinalUserData;
	
	//Try to get user from the server
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(InUserID, FPubnubGetMetadataInclude::FromValue(true));
	if(!GetUserResult.Result.Error)
	{
		FinalUserData = GetUserResult.UserData;
		FinalResult.Result.AddStep("GetUserMetadata", GetUserResult.Result);
	}
	else
	{
		//If user doesn't exist on the server, just create it
		FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(InUserID, FPubnubUserData());
		FinalResult.Result.AddStep("SetUserMetadata", SetUserResult.Result);

		if(SetUserResult.Result.Error)
		{return FinalResult;}
		
		FinalUserData = SetUserResult.UserData;
	}
    
	// Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, InUserID, FPubnubChatUserData::FromPubnubUserData(FinalUserData));
	FinalResult.User = NewUser;
	return FinalResult;
}
