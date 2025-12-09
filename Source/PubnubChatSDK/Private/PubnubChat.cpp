// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"

#include "PubnubChatInternalMacros.h"
#include "PubnubClient.h"
#include "PubnubEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatSubsystem.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"


DEFINE_LOG_CATEGORY(PubnubChatLog)



void UPubnubChat::DestroyChat()
{
	
	PubnubClient->OnPubnubSubscriptionStatusChanged.RemoveDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	// Clear repository data
	if (ObjectsRepository)
	{
		ObjectsRepository->ClearAll();
		ObjectsRepository = nullptr;
	}

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
		FinalResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		FinalResult.Result.AddStep("GetUserMetadata", GetUserResult.Result);
		return FinalResult;
	}

	//SetUserMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserData());
	FinalResult.Result.AddStep("SetUserMetadata", SetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetUserResult.Result.Error)
	{return FinalResult;}
	
	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, UserData);
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

	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, GetUserResult.UserData);
	return FinalResult;
}

FPubnubChatGetUsersResult UPubnubChat::GetUsers(const int Limit, const FString Filter, FPubnubGetAllSort Sort, FPubnubPage Page)
{
	FPubnubChatGetUsersResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubGetAllUserMetadataResult GetAllUserResult = PubnubClient->GetAllUserMetadata(FPubnubGetAllInclude::FromValue(true), Limit, Filter, Sort, Page);
	FinalResult.Result.AddStep("GetAllUserMetadata", GetAllUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetAllUserResult.Result.Error)
	{return FinalResult;}

	for (auto &UserData : GetAllUserResult.UsersData)
	{
		FinalResult.Users.Add(CreateUserObject(UserData.UserID, UserData));
	}
	
	// Copy pagination and total count information
	FinalResult.Page = GetAllUserResult.Page;
	FinalResult.Total = GetAllUserResult.TotalCount;

	return FinalResult;

}

FPubnubChatUserResult UPubnubChat::UpdateUser(const FString UserID, FPubnubChatUserData UserData)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//SetUserMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserData());
	FinalResult.Result.AddStep("SetUserMetadata", SetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetUserResult.Result.Error)
	{return FinalResult;}

	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, UserData);
	return FinalResult;
}

FPubnubChatUserResult UPubnubChat::DeleteUser(const FString UserID, bool Soft)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//If it's not soft, remove user metadata from the server
	if(!Soft)
	{
		//RemoveUserMetadata by PubnubClient
		FPubnubOperationResult RemoveUserResult = PubnubClient->RemoveUserMetadata(UserID);
		FinalResult.Result.AddStep("RemoveUserMetadata", RemoveUserResult);
		
		//Remove user from repository
		if (!RemoveUserResult.Error)
		{
			ObjectsRepository->RemoveUserData(UserID);
		}
		
		return FinalResult;
	}

	//Soft Delete - just update User Metadata

	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	FinalResult.Result.AddStep("GetUserMetadata", GetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetUserResult.Result.Error)
	{return FinalResult;}

	//Add Deleted property to Custom field
	GetUserResult.UserData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(GetUserResult.UserData.Custom);

	//SetUserMetadata updated metadata
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, GetUserResult.UserData);
	FinalResult.Result.AddStep("SetUserMetadata", SetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetUserResult.Result.Error)
	{return FinalResult;}
	
	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, GetUserResult.UserData);
	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//SetChannelMetadata by PubnubClient
	FPubnubChannelMetadataResult SetUserResult = PubnubClient->SetChannelMetadata(ChannelID, ChannelData.ToPubnubChannelData());
	FinalResult.Result.AddStep("SetChannelMetadata", SetUserResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetUserResult.Result.Error)
	{return FinalResult;}
	
	//Create Channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, ChannelData);
	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::GetChannel(const FString ChannelID)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//GetChannelMetadata from PubnubClient
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	FinalResult.Result.AddStep("GetChannelMetadata", GetChannelResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetChannelResult.Result.Error)
	{return FinalResult;}

	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, GetChannelResult.ChannelData);
	return FinalResult;
}

FPubnubChatGetChannelsResult UPubnubChat::GetChannels(const int Limit, const FString Filter, FPubnubGetAllSort Sort, FPubnubPage Page)
{
	FPubnubChatGetChannelsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubGetAllChannelMetadataResult GetAllChannelResult = PubnubClient->GetAllChannelMetadata(FPubnubGetAllInclude::FromValue(true), Limit, Filter, Sort, Page);
	FinalResult.Result.AddStep("GetAllChannelMetadata", GetAllChannelResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetAllChannelResult.Result.Error)
	{return FinalResult;}

	for (auto &ChannelData : GetAllChannelResult.ChannelsData)
	{
		FinalResult.Channels.Add(CreateChannelObject(ChannelData.ChannelID, ChannelData));
	}
	
	// Copy pagination and total count information
	FinalResult.Page = GetAllChannelResult.Page;
	FinalResult.Total = GetAllChannelResult.TotalCount;

	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::UpdateChannel(const FString ChannelID, FPubnubChatChannelData ChannelData)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//SetChannelMetadata by PubnubClient
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, ChannelData.ToPubnubChannelData());
	FinalResult.Result.AddStep("SetChannelMetadata", SetChannelResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetChannelResult.Result.Error)
	{return FinalResult;}

	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, ChannelData);
	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::DeleteChannel(const FString ChannelID, bool Soft)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//If it's not soft, remove channel metadata from the server
	if(!Soft)
	{
		//RemoveChannelMetadata by PubnubClient
		FPubnubOperationResult RemoveChannelResult = PubnubClient->RemoveChannelMetadata(ChannelID);
		FinalResult.Result.AddStep("RemoveChannelMetadata", RemoveChannelResult);
		
		//Remove channel from repository
		if (!RemoveChannelResult.Error)
		{
			ObjectsRepository->RemoveChannelData(ChannelID);
		}
		
		return FinalResult;
	}

	//Soft Delete - just update Channel Metadata

	//GetChannelMetadata from PubnubClient to have up to date data
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	FinalResult.Result.AddStep("GetChannelMetadata", GetChannelResult.Result);

	//Return if there was any error during PubnubClient operation
	if(GetChannelResult.Result.Error)
	{return FinalResult;}

	//Add Deleted property to Custom field
	GetChannelResult.ChannelData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(GetChannelResult.ChannelData.Custom);

	//SetChannelMetadata updated metadata
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, GetChannelResult.ChannelData);
	FinalResult.Result.AddStep("SetChannelMetadata", SetChannelResult.Result);

	//Return if there was any error during PubnubClient operation
	if(SetChannelResult.Result.Error)
	{return FinalResult;}
	
	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, GetChannelResult.ChannelData);
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
		FinalResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		return FinalResult;
	}

	ChatConfig = InChatConfig;
	PubnubClient = InPubnubClient;
	
	//Create repository for managing shared User and Channel data
	ObjectsRepository = NewObject<UPubnubChatObjectsRepository>(this);
	
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

	//Create user object and return final result
	FinalResult.User = CreateUserObject(InUserID, FinalUserData);
	return FinalResult;
}

UPubnubChatUser* UPubnubChat::CreateUserObject(const FString UserID, const FPubnubChatUserData& ChatUserData)
{
	//Update repository with updated user data
	ObjectsRepository->UpdateUserData(UserID, ChatUserData);

	//Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, this, UserID);
	return NewUser;
}

UPubnubChatUser* UPubnubChat::CreateUserObject(const FString UserID, const FPubnubUserData& UserData)
{
	//Update repository with updated user data
	ObjectsRepository->UpdateUserData(UserID, FPubnubChatUserData::FromPubnubUserData(UserData));

	//Create and return the user object
	UPubnubChatUser* NewUser = NewObject<UPubnubChatUser>(this);
	NewUser->InitUser(PubnubClient, this, UserID);
	return NewUser;
}

UPubnubChatChannel* UPubnubChat::CreateChannelObject(const FString ChannelID, const FPubnubChatChannelData& ChatChannelData)
{
	//Update repository with updated channel data
	ObjectsRepository->UpdateChannelData(ChannelID, ChatChannelData);

	//Create and return the channel object
	UPubnubChatChannel* NewChannel = NewObject<UPubnubChatChannel>(this);
	NewChannel->InitChannel(PubnubClient, this, ChannelID);
	return NewChannel;
}

UPubnubChatChannel* UPubnubChat::CreateChannelObject(const FString ChannelID, const FPubnubChannelData& ChannelData)
{
	//Update repository with updated channel data
	ObjectsRepository->UpdateChannelData(ChannelID, FPubnubChatChannelData::FromPubnubChannelData(ChannelData));

	//Create and return the channel object
	UPubnubChatChannel* NewChannel = NewObject<UPubnubChatChannel>(this);
	NewChannel->InitChannel(PubnubClient, this, ChannelID);
	return NewChannel;
}

UPubnubChatMessage* UPubnubChat::CreateMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData)
{
	//Create and init the message object
	UPubnubChatMessage* NewMessage = NewObject<UPubnubChatMessage>(this);
	NewMessage->InitMessage(PubnubClient, this, ChatMessageData.ChannelID, Timetoken);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewMessage->GetInternalMessageID(), ChatMessageData);

	return NewMessage;
}

UPubnubChatMessage* UPubnubChat::CreateMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData)
{
	//Create and init the message object
	UPubnubChatMessage* NewMessage = NewObject<UPubnubChatMessage>(this);
	NewMessage->InitMessage(PubnubClient, this, MessageData.Channel, Timetoken);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewMessage->GetInternalMessageID(), FPubnubChatMessageData::FromPubnubMessageData(MessageData));

	return NewMessage;
}
