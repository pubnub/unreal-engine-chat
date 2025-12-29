// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"


void UPubnubChatUser::BeginDestroy()
{
	// Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !UserID.IsEmpty())
	{
		Chat->ObjectsRepository->UnregisterUser(UserID);
	}
	
	UObject::BeginDestroy();
	IsInitialized = false;
}

FPubnubChatUserData UPubnubChatUser::GetUserData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatUserData());

	// Get user data from repository
	if (FPubnubChatInternalUser* InternalUser = Chat->ObjectsRepository->GetUserData(UserID))
	{
		return InternalUser->UserData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("User data not found in repository for UserID: %s"), *UserID);
	return FPubnubChatUserData();
}

FPubnubChatOperationResult UPubnubChatUser::Update(FPubnubChatUserData UserData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//SetChannelMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	//Update repository with updated channel data
	Chat->ObjectsRepository->UpdateUserData(UserID, UserData);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatUser::Delete(bool Soft)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatUserResult DeleteUserResult = Chat->DeleteUser(UserID, Soft);
	return DeleteUserResult.Result;
}

FPubnubChatOperationResult UPubnubChatUser::Restore()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	
	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Add Deleted property to Custom field
	GetUserResult.UserData.Custom = UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(GetUserResult.UserData.Custom);

	//SetUserMetadata with updated metadata
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, GetUserResult.UserData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	return FinalResult;
}

FPubnubChatIsDeletedResult UPubnubChatUser::IsDeleted()
{
	FPubnubChatIsDeletedResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");
	
	FinalResult.IsDeleted = UPubnubChatInternalUtilities::HasDeletedPropertyInCustom(GetUserResult.UserData.Custom);
	
	return FinalResult;
}

FPubnubChatWherePresentResult UPubnubChatUser::WherePresent()
{
	FPubnubChatWherePresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->WherePresent(UserID);
}

FPubnubChatIsPresentResult UPubnubChatUser::IsPresentOn(const FString ChannelID)
{
	FPubnubChatIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->IsPresent(UserID, ChannelID);
}

void UPubnubChatUser::InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init User, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init User, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InUserID.IsEmpty(), TEXT("Can't init User, UserID is empty"));

	UserID = InUserID;
	PubnubClient = InPubnubClient;
	Chat = InChat;
	
	// Register this user object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterUser(UserID);
	}
	
	IsInitialized = true;
}
