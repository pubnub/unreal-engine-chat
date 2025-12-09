// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"


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
	FPubnubChatInternalUser* InternalUser = Chat->ObjectsRepository->GetUserData(UserID);
	if (InternalUser)
	{
		return InternalUser->UserData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("User data not found in repository for UserID: %s"), *UserID);
	return FPubnubChatUserData();
}

FPubnubChatOperationResult UPubnubChatUser::Update(FPubnubChatUserData UserData)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatUserResult UpdateUserResult = Chat->UpdateUser(UserID, UserData);
	return UpdateUserResult.Result;
}

FPubnubChatOperationResult UPubnubChatUser::Delete(bool Soft)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatUserResult DeleteUserResult = Chat->DeleteUser(UserID, Soft);
	return DeleteUserResult.Result;
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
