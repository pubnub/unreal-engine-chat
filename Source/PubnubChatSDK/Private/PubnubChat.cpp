// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"

#include "PubnubChatAccessManager.h"
#include "PubnubChatCallbackStop.h"
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
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatConst.h"
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "Misc/DateTime.h"
#include "Threads/PubnubFunctionThread.h"

DEFINE_LOG_CATEGORY(PubnubChatLog)


void UPubnubChat::DestroyChat()
{
	// Clear timers for user activity timestamp
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance)
	{
		FTimerManager& TimerManager = GameInstance->GetTimerManager();
		TimerManager.ClearTimer(LastSavedActivityIntervalTimerHandle);
		TimerManager.ClearTimer(RunWithDelayTimerHandle);
	}
	
	if(AsyncFunctionsThread)
	{
		AsyncFunctionsThread->Stop();
	}
	
	delete AsyncFunctionsThread;
	AsyncFunctionsThread = nullptr;
	
	PubnubClient->OnSubscriptionStatusChanged.RemoveDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	// Clear repository data
	if (ObjectsRepository)
	{
		ObjectsRepository->ClearAll();
		ObjectsRepository = nullptr;
	}

	IsInitialized = false;
	
	OnChatDestroyed.Broadcast(CurrentUserID);
	OnChatDestroyedNative.Broadcast(CurrentUserID);
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
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, UserData);
	return FinalResult;
}

void UPubnubChat::CreateUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse, FPubnubChatUserData UserData)
{
	FOnPubnubChatUserResponseNative NativeCallback;
	NativeCallback.BindLambda([OnUserResponse](const FPubnubChatUserResult& UserResult)
	{
		OnUserResponse.ExecuteIfBound(UserResult);
	});

	CreateUserAsync(UserID, NativeCallback, UserData);
}

void UPubnubChat::CreateUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative, FPubnubChatUserData UserData)
{
	PUBNUB_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnUserResponseNative, FPubnubChatUserResult());
	
	TWeakObjectPtr<UPubnubChat> WeakThis = MakeWeakObjectPtr<UPubnubChat>(this);

	AsyncFunctionsThread->AddFunctionToQueue( [WeakThis, UserID, OnUserResponseNative, UserData = MoveTemp(UserData)]
	{
		if(!WeakThis.IsValid())
		{return;}
		
		FPubnubChatUserResult CreateUserResult = WeakThis.Get()->CreateUser(UserID, UserData);

		//Execute provided delegate with results
		UPubnubUtilities::CallPubnubDelegate(OnUserResponseNative, CreateUserResult);
	});
}

FPubnubChatUserResult UPubnubChat::GetUser(FString UserID)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//GetUserMetadata from PubnubClient
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, GetUserResult.UserData);
	return FinalResult;
}

void UPubnubChat::GetUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse)
{
	FOnPubnubChatUserResponseNative NativeCallback;
	NativeCallback.BindLambda([OnUserResponse](const FPubnubChatUserResult& UserResult)
	{
		OnUserResponse.ExecuteIfBound(UserResult);
	});

	GetUserAsync(UserID, NativeCallback);
}

void UPubnubChat::GetUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative)
{
	PUBNUB_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnUserResponseNative, FPubnubChatUserResult());
	
	TWeakObjectPtr<UPubnubChat> WeakThis = MakeWeakObjectPtr<UPubnubChat>(this);

	AsyncFunctionsThread->AddFunctionToQueue( [WeakThis, UserID, OnUserResponseNative]
	{
		if(!WeakThis.IsValid())
		{return;}
		
		FPubnubChatUserResult GetUserResult = WeakThis.Get()->GetUser(UserID);

		//Execute provided delegate with results
		UPubnubUtilities::CallPubnubDelegate(OnUserResponseNative, GetUserResult);
	});
}

FPubnubChatGetUsersResult UPubnubChat::GetUsers(const int Limit, const FString Filter, FPubnubGetAllSort Sort, FPubnubPage Page)
{
	FPubnubChatGetUsersResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubGetAllUserMetadataResult GetAllUserResult = PubnubClient->GetAllUserMetadata(FPubnubGetAllInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetAllUserResult.Result, "GetAllUserMetadata");

	for (auto &UserData : GetAllUserResult.UsersData)
	{
		FinalResult.Users.Add(CreateUserObject(UserData.UserID, UserData));
	}
	
	//Copy pagination and total count information
	FinalResult.Page = GetAllUserResult.Page;
	FinalResult.Total = GetAllUserResult.TotalCount;

	return FinalResult;
}

FPubnubChatUserResult UPubnubChat::UpdateUser(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//Make sure such User exists
	FPubnubChatUserResult GetUserResult = GetUser(UserID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result);
	
	//SetUserMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UpdateUserData.ToPubnubUserInputData(), FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");

	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, SetUserResult.UserData);
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::DeleteUser(const FString UserID, bool Soft)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(UserID);

	FPubnubChatOperationResult FinalResult;

	//If it's not soft, remove user metadata from the server
	if(!Soft)
	{
		//RemoveUserMetadata by PubnubClient
		FPubnubOperationResult RemoveUserResult = PubnubClient->RemoveUserMetadata(UserID);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveUserResult, "RemoveUserMetadata");
		
		//Remove user from repository
		ObjectsRepository->RemoveUserData(UserID);
		
		return FinalResult;
	}

	//Soft Delete - just update User Metadata

	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Add Deleted property to Custom field
	FPubnubUserInputData NewUserData = FPubnubUserInputData::FromPubnubUserData(GetUserResult.UserData);
	NewUserData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(NewUserData.Custom);

	//SetUserMetadata with updated metadata
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, NewUserData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	//Update User data on the repository
	ObjectsRepository->UpdateUserData(UserID, FPubnubChatUserData::FromPubnubUserData(SetUserResult.UserData));
	
	return FinalResult;
}

FPubnubChatGetUserSuggestionsResult UPubnubChat::GetUserSuggestions(const FString Text, int Limit)
{
	FPubnubChatGetUserSuggestionsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, Text);

	FString Filter = FString::Printf(TEXT(R"(name LIKE "%s*")"), *Text);
	FPubnubChatGetUsersResult GetUsersResult = GetUsers(Limit, Filter);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUsersResult.Result);

	FinalResult.Users = GetUsersResult.Users;
	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//Regardless of the provided Channel Type, this method creates public channel
	ChannelData.Type = "public";

	//SetChannelMetadata by PubnubClient
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, ChannelData.ToPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//Create Channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, ChannelData);
	return FinalResult;
}

FPubnubChatCreateGroupConversationResult UPubnubChat::CreateGroupConversation(TArray<UPubnubChatUser*> Users, const FString ChannelID, FPubnubChatChannelData ChannelData, FPubnubChatMembershipData HostMembershipData)
{
	FPubnubChatCreateGroupConversationResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	TArray<UPubnubChatUser*> ValidUsers = UPubnubChatInternalUtilities::RemoveInvalidObjects(Users);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, !ValidUsers.IsEmpty(), TEXT("At least one valid user has to be provided"));


	//If channel ID was not provided, generate Guid
	FString FinalChannelID = ChannelID.IsEmpty() ? FGuid::NewGuid().ToString(EGuidFormats::UniqueObjectGuid) : ChannelID;

	//Regardless of the provided Channel Type, this method creates group channel
	ChannelData.Type = "group";

	//SetChannelMetadata by PubnubClient and create Channel
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(FinalChannelID, ChannelData.ToPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	UPubnubChatChannel* CreatedChannel = CreateChannelObject(FinalChannelID, ChannelData);
	FinalResult.Channel = CreatedChannel;

	//SetMemberships by PubnubClient and create host Membership
	FPubnubMembershipsResult SetMembershipsResult = PubnubClient->SetMemberships(CurrentUserID, {HostMembershipData.ToPubnubMembershipInputData(FinalChannelID)}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipsResult.Result, "SetMemberships");
	UPubnubChatMembership* CreatedHostMembership = CreateMembershipObject(CurrentUser, CreatedChannel, HostMembershipData);
	FinalResult.HostMembership = CreatedHostMembership;
	
	//Invite User to created channel
	FPubnubChatInviteMultipleResult InviteMultipleResult =  CreatedChannel->InviteMultiple(ValidUsers);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, InviteMultipleResult.Result);
	FinalResult.InviteesMemberships = InviteMultipleResult.Memberships;
	
	return FinalResult;
}

FPubnubChatCreateDirectConversationResult UPubnubChat::CreateDirectConversation(UPubnubChatUser* User, const FString ChannelID, FPubnubChatChannelData ChannelData, FPubnubChatMembershipData HostMembershipData)
{
	FPubnubChatCreateDirectConversationResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);
	
	//If channel ID was not provided generate ID by sorting users and hashing their IDs
	FString FinalChannelID = ChannelID;
	if (ChannelID.IsEmpty())
	{
		TArray<FString> SortedUsers = {CurrentUserID, User->GetUserID()};
		SortedUsers.Sort();
		uint64 ChannelHash = UPubnubChatInternalUtilities::HashString(FString::Printf(TEXT("%s&%s"), *SortedUsers[0], *SortedUsers[1]));
		FinalChannelID = FString::Printf(TEXT("direct.%llu"), ChannelHash);
	}

	//Regardless of the provided Channel Type, this method creates public channel
	ChannelData.Type = "direct";

	//Set channel metadata by PubnubClient and create the channel
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(FinalChannelID, ChannelData.ToPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	UPubnubChatChannel* CreatedChannel = CreateChannelObject(FinalChannelID, ChannelData);
	FinalResult.Channel = CreatedChannel;

	//SetMemberships by PubnubClient and create host membership
	FPubnubMembershipsResult SetMembershipsResult = PubnubClient->SetMemberships(CurrentUserID, {HostMembershipData.ToPubnubMembershipInputData(FinalChannelID)}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipsResult.Result, "SetMemberships");
	UPubnubChatMembership* CreatedHostMembership = CreateMembershipObject(CurrentUser, CreatedChannel, HostMembershipData);
	FinalResult.HostMembership = CreatedHostMembership;

	//Invite User to created channel
	FPubnubChatInviteResult InviteResult =  CreatedChannel->Invite(User);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, InviteResult.Result);
	FinalResult.InviteeMembership = InviteResult.Membership;
	
	return FinalResult;
}


FPubnubChatChannelResult UPubnubChat::GetChannel(const FString ChannelID)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//GetChannelMetadata from PubnubClient
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelResult.Result, "GetChannelMetadata");

	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, GetChannelResult.ChannelData);
	return FinalResult;
}

FPubnubChatGetChannelsResult UPubnubChat::GetChannels(const int Limit, const FString Filter, FPubnubGetAllSort Sort, FPubnubPage Page)
{
	FPubnubChatGetChannelsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubGetAllChannelMetadataResult GetAllChannelResult = PubnubClient->GetAllChannelMetadata(FPubnubGetAllInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetAllChannelResult.Result, "GetAllChannelMetadata");

	for (auto &ChannelData : GetAllChannelResult.ChannelsData)
	{
		FinalResult.Channels.Add(CreateChannelObject(ChannelData.ChannelID, ChannelData));
	}
	
	// Copy pagination and total count information
	FinalResult.Page = GetAllChannelResult.Page;
	FinalResult.Total = GetAllChannelResult.TotalCount;

	return FinalResult;
}

FPubnubChatChannelResult UPubnubChat::UpdateChannel(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//Make sure such channel exists
	FPubnubChatChannelResult GetChannelResult = GetChannel(ChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelResult.Result);

	//SetChannelMetadata by PubnubClient - include all fields in response
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, UpdateChannelData.ToPubnubChannelInputData(), FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");

	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, SetChannelResult.ChannelData);
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::DeleteChannel(const FString ChannelID, bool Soft)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(ChannelID);

	FPubnubChatOperationResult FinalResult;
	
	//If it's not soft, remove channel metadata from the server
	if(!Soft)
	{
		//RemoveChannelMetadata by PubnubClient
		FPubnubOperationResult RemoveChannelResult = PubnubClient->RemoveChannelMetadata(ChannelID);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveChannelResult, "RemoveChannelMetadata");
		
		//Remove channel from repository
		ObjectsRepository->RemoveChannelData(ChannelID);
		
		return FinalResult;
	}

	//Soft Delete - just update Channel Metadata

	//GetChannelMetadata from PubnubClient to have up to date data
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result, "GetChannelMetadata");

	//Add Deleted property to Custom field
	FPubnubChannelInputData NewChannelData = FPubnubChannelInputData::FromPubnubChannelData(GetChannelResult.ChannelData);
	NewChannelData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(NewChannelData.Custom);

	//SetChannelMetadata updated metadata
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, NewChannelData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//Update ObjectsRepository with updated channel data
	ObjectsRepository->UpdateChannelData(ChannelID, FPubnubChatChannelData::FromPubnubChannelData(GetChannelResult.ChannelData));
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::PinMessageToChannel(UPubnubChatMessage* Message, UPubnubChatChannel* Channel)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Channel);
	
	return Channel->PinMessage(Message);
}

FPubnubChatOperationResult UPubnubChat::UnpinMessageFromChannel(UPubnubChatChannel* Channel)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Channel);
	return Channel->UnpinMessage();
}

FPubnubChatGetChannelSuggestionsResult UPubnubChat::GetChannelSuggestions(const FString Text, int Limit)
{
	FPubnubChatGetChannelSuggestionsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, Text);

	FString Filter = FString::Printf(TEXT(R"(name LIKE "%s*")"), *Text);
	FPubnubChatGetChannelsResult GetChannelsResult = GetChannels(Limit, Filter);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelsResult.Result);

	FinalResult.Channels = GetChannelsResult.Channels;
	return FinalResult;
}

FPubnubChatWherePresentResult UPubnubChat::WherePresent(const FString UserID)
{
	FPubnubChatWherePresentResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);
	
	//Use PubnubClient ListUserSubscribedChannels (WhereNow) to get all subscribed channels
	FPubnubListUsersSubscribedChannelsResult WhereNowResult = PubnubClient->ListUserSubscribedChannels(UserID);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, WhereNowResult.Result, "ListUserSubscribedChannels");
	
	FinalResult.Channels = WhereNowResult.Channels;
	return FinalResult;
}

FPubnubChatWhoIsPresentResult UPubnubChat::WhoIsPresent(const FString ChannelID, int Limit, int Offset)
{
	FPubnubChatWhoIsPresentResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);
	
	//Use PubnubClient ListUserSubscribedChannels (WhereNow) to get all subscribed channels
	FPubnubListUsersFromChannelSettings HereNowSettings = FPubnubListUsersFromChannelSettings{.DisableUserID = false, .Limit = Limit, .Offset = Offset};
	FPubnubListUsersFromChannelResult HereNowResult = PubnubClient->ListUsersFromChannel(ChannelID, HereNowSettings);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, HereNowResult.Result, "ListUserSubscribedChannels");
	
	//Add all Users into the FinalResult
	HereNowResult.Data.UsersState.GetKeys(FinalResult.Users);
	
	return FinalResult;
}

FPubnubChatIsPresentResult UPubnubChat::IsPresent(const FString UserID, const FString ChannelID)
{
	FPubnubChatIsPresentResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);
	
	//Use WherePresent for given UserID
	FPubnubChatWherePresentResult WherePresentResult = WherePresent(UserID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, WherePresentResult.Result);
	
	//And just check if provided channel is in the list
	FinalResult.IsPresent = WherePresentResult.Channels.Contains(ChannelID);
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::SetRestrictions(FPubnubChatRestriction Restriction)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!Restriction.UserID.IsEmpty(), TEXT("UserID in provided Restriction can't be empty"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!Restriction.ChannelID.IsEmpty(), TEXT("ChannelID in provided Restriction can't be empty"));
	
	FPubnubChatOperationResult FinalResult;
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(Restriction.ChannelID);
	
	//Make sure moderation channel exists
	FPubnubChannelMetadataResult ChannelMetadataResult =  PubnubClient->SetChannelMetadata(ModerationChannelID, FPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ChannelMetadataResult.Result, "SetChannelMetadata");
	
	FString RestrictionType;
	
	//Lift restrictions if ban and mute are false
	if (!Restriction.Ban && !Restriction.Mute)
	{
		//Lifting restriction is simply removing this user membership from moderation channel
		FPubnubChannelMembersResult RemoveChannelMembersResult = PubnubClient->RemoveChannelMembers(ModerationChannelID, {Restriction.UserID}, FPubnubMemberInclude::FromValue(false), 1);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveChannelMembersResult.Result, "RemoveChannelMembers");
		
		RestrictionType = "Lifted";
	}
	else
	{
		//Setting restriction is actually SettingChannelMembers on an internal moderation channel
		FPubnubChannelMemberInputData ModerationMemberInputData;
		ModerationMemberInputData.User = Restriction.UserID;
		ModerationMemberInputData.Custom = UPubnubChatInternalUtilities::GetChannelMemberCustomForRestriction(Restriction);
	
		FPubnubChannelMembersResult SetChannelMembersResult = PubnubClient->SetChannelMembers(ModerationChannelID, {ModerationMemberInputData}, FPubnubMemberInclude::FromValue(false), 1);
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelMembersResult.Result, "SetChannelMembers");
		
		RestrictionType = Restriction.Ban? "banned" : "muted";
	}
	
	//Emit moderation event that restriction was lifted
	FString ModerationEventChannel = UPubnubChatInternalUtilities::GetModerationEventChannelForUserID(Restriction.UserID);
	FString EventPayload = UPubnubChatInternalUtilities::GetModerationEventPayload(ModerationChannelID, RestrictionType, Restriction.Reason);
	FPubnubChatOperationResult EmitResult =  EmitChatEvent(EPubnubChatEventType::PCET_Moderation, ModerationEventChannel, EventPayload);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, EmitResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::EmitChatEvent(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, EPubnubChatEventMethod EventMethod)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(ChannelID);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Payload);

	//Add event type to the payload
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(Payload, JsonObject);
	JsonObject->SetStringField(ANSI_TO_TCHAR("type"), UPubnubChatInternalConverters::ChatEventTypeToString(EventType));

	//If event method is default, get dedicated method for this event type
	if(EventMethod == EPubnubChatEventMethod::PCEM_Default)
	{
		EventMethod = UPubnubChatInternalUtilities::GetDefaultChatEventMethodForEventType(EventType);
	}

	//Use Publish or Signal for sending event depending on specified method
	if(EventMethod == EPubnubChatEventMethod::PCEM_Publish)
	{
		FPubnubPublishMessageResult PublishResult =  PubnubClient->PublishMessage(ChannelID, UPubnubJsonUtilities::JsonObjectToString(JsonObject));
		FinalResult.AddStep("PublishMessage", PublishResult.Result);
	}
	else
	{
		FPubnubSignalResult SignalResult =  PubnubClient->Signal(ChannelID, UPubnubJsonUtilities::JsonObjectToString(JsonObject));
		FinalResult.AddStep("Signal", SignalResult.Result);
	}

	return FinalResult;
}

FPubnubChatEventsResult UPubnubChat::GetEventsHistory(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, const int Count)
{
	FPubnubChatEventsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, StartTimetoken);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, EndTimetoken);
	
	FPubnubFetchHistorySettings FetchHistorySettings;
	FetchHistorySettings.MaxPerChannel = Count;
	FetchHistorySettings.Start = StartTimetoken;
	FetchHistorySettings.End = EndTimetoken;
	FetchHistorySettings.IncludeUserID = true; //Include UserID so events can have their UserID populated
	FPubnubFetchHistoryResult FetchHistoryResult = PubnubClient->FetchHistory(ChannelID, FetchHistorySettings);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, FetchHistoryResult.Result, "FetchHistory");
	
	for (auto& Message : FetchHistoryResult.Messages)
	{
		if (UPubnubChatInternalUtilities::IsThisEventMessage(Message.Message))
		{
			FinalResult.Events.Add(UPubnubChatInternalUtilities::GetEventFromPubnubHistoryMessageData(Message));
		}
	}
	
	//If we got the exact amount of messages as specified count, probably there are more events in a given range
	FinalResult.IsMore = FetchHistoryResult.Messages.Num() == Count;
	
	return FinalResult;
}

FPubnubChatListenForEventsResult UPubnubChat::ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceived EventCallback)
{
	FOnPubnubChatEventReceivedNative EventCallbackNative;
	EventCallbackNative.BindLambda([EventCallback](FPubnubChatEvent Event)
	{
		EventCallback.ExecuteIfBound(Event);
	});
	return ListenForEvents(ChannelID, EventType, EventCallbackNative);
}

FPubnubChatListenForEventsResult UPubnubChat::ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceivedNative EventCallbackNative)
{
	FPubnubChatListenForEventsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);


	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(ChannelID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, ChannelEntity, TEXT("Can't ListenForEvents, Failed to create ChannelEntity"));

	UPubnubSubscription* Subscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, Subscription, TEXT("Can't ListenForEvents, Failed to create Subscription"));

	ListenForEventsSubscriptions.Add(Subscription);
	
	TWeakObjectPtr<UPubnubChat> ThisWeak = MakeWeakObjectPtr(this);
	
	//Create listener for events
	auto EventLambda = [ThisWeak, EventType, EventCallbackNative](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		//Just skip if this is not an event
		if (!UPubnubChatInternalUtilities::IsThisEventMessage(MessageData.Message))
		{return;}

		FPubnubChatEvent Event = UPubnubChatInternalUtilities::GetEventFromPubnubMessageData(MessageData);
		
		//Execute callback only if received event matches the type that we are listening for
		if(Event.Type == EventType)
		{
			EventCallbackNative.ExecuteIfBound(Event);
		}
	};

	//Events can be received as messages and signals, so we need to bind to both of them
	Subscription->OnPubnubMessageNative.AddLambda(EventLambda);
	Subscription->OnPubnubSignalNative.AddLambda(EventLambda);

	//Subscribe with this channel Subscription
	FPubnubOperationResult SubscribeResult = Subscription->Subscribe();
	FinalResult.Result.AddStep("Subscribe", SubscribeResult);

	//Create CallbackStop with function to unsubscribe (stop listening for events)
	UPubnubChatCallbackStop* CallbackStop = NewObject<UPubnubChatCallbackStop>(this);
	auto DisconnectLambda = [ThisWeak, Subscription]()->FPubnubChatOperationResult
	{
		if(!ThisWeak.IsValid())
		{return FPubnubChatOperationResult::CreateError("Chat is already destroyed");}
		
		UPubnubChat* ThisChat = ThisWeak.Get();

		if(!ThisChat->IsInitialized)
		{return FPubnubChatOperationResult::CreateError("Chat is already deinitialized");}

		if(!Subscription)
		{return FPubnubChatOperationResult::CreateError("This subscription is already destroyed");}

		ThisChat->ListenForEventsSubscriptions.Remove(Subscription);
		Subscription->OnPubnubMessageNative.Clear();
		Subscription->OnPubnubSignalNative.Clear();

		FPubnubChatOperationResult FinalResult;
		FPubnubOperationResult UnsubscribeResult = Subscription->Unsubscribe();
		FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
		return FinalResult;
	};
	CallbackStop->InitCallbackStop(DisconnectLambda);
	
	FinalResult.CallbackStop = CallbackStop;
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::ForwardMessage(UPubnubChatMessage* Message, UPubnubChatChannel* Channel)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Channel);
	
	FPubnubChatMessageData MessageData = Message->GetMessageData();
	
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Channel->GetChannelID() != MessageData.ChannelID), TEXT("Can't forward message to the same channel"));
	
	//PublishMessage by PubnubClient
	FPubnubPublishSettings PublishSettings;
	PublishSettings.MetaData = UPubnubChatInternalUtilities::GetForwardedMessageMeta(MessageData.Meta, MessageData.UserID, MessageData.ChannelID);
	FPubnubPublishMessageResult PublishResult =  PubnubClient->PublishMessage(Channel->GetChannelID(), UPubnubChatInternalUtilities::ChatMessageToPublishString(Message->GetCurrentText()), PublishSettings);

	FPubnubChatOperationResult FinalResult;
	FinalResult.AddStep("PublishMessage", PublishResult.Result);
	
	return FinalResult;
}

FPubnubChatGetUnreadMessagesCountsResult UPubnubChat::GetUnreadMessagesCounts(const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatGetUnreadMessagesCountsResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships(Limit, Filter, Sort, Page);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipsResult.Result);
	
	TArray<FString> Channels;
	TArray<FString> Timetokens;
	
	//From all User memberships for array of Channels and LRM Timetokens
	for (auto& Membership : GetMembershipsResult.Memberships)
	{
		//Skip our internal channels
		if (UPubnubChatInternalUtilities::IsPubnubInternalChannel(Membership->GetChannelID()))
		{ continue; }
		
		FString Timetoken = Membership->GetLastReadMessageTimetoken();
		Timetoken.IsEmpty() ? Timetokens.Add(Pubnub_Chat_Empty_Timetoken) : Timetokens.Add(Timetoken);
		Channels.Add(Membership->GetChannelID());
	}
	
	//Use PubnubClient to get "MessageCounts" - unread messages since provided timetoken
	FPubnubMessageCountsMultipleResult MessageCountsResult = PubnubClient->MessageCountsMultiple(Channels, Timetokens);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, MessageCountsResult.Result, "MessageCountsMultiple");
	
	//Find value of Unread Message Counts for each membeship
	for (auto& Membership : GetMembershipsResult.Memberships)
	{
		if (int* MessageCountsPtr = MessageCountsResult.MessageCountsPerChannel.Find(Membership->GetChannelID()))
		{
			FinalResult.UnreadMessagesCounts.Add(FPubnubChatUnreadMessagesCountsWrapper({Membership->Channel, Membership, *MessageCountsPtr}));
		}
	}
	
	FinalResult.Page = GetMembershipsResult.Page;
	FinalResult.Total = GetMembershipsResult.Total;
	return FinalResult;
}

FPubnubChatMarkAllMessagesAsReadResult UPubnubChat::MarkAllMessagesAsRead(const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatMarkAllMessagesAsReadResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	FPubnubChatMembershipsResult GetMembershipsResult = CurrentUser->GetMemberships(Limit, Filter, Sort, Page);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipsResult.Result);
	
	FString CurrentTimetoken = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	TArray<FPubnubMembershipInputData> SetMembershipsChannels;
	
	//For all Membership add CurrentTimetoken as LRM Timetoken to the Custom field
	for (auto& Membership : GetMembershipsResult.Memberships)
	{
		FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
		UPubnubChatInternalUtilities::AddLastReadMessageTimetokenToMembershipData(MembershipData, CurrentTimetoken);
		SetMembershipsChannels.Add(MembershipData.ToPubnubMembershipInputData(Membership->GetChannelID()));
	}
	
	//Use PubnubClient to update all memberships
	FPubnubMembershipsResult SetMembershipsResult = PubnubClient->SetMemberships(CurrentUserID, SetMembershipsChannels, FPubnubMembershipInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipsResult.Result, "SetMembershipsResult");
	
	//Create new Membership objects and send Receipt event for every Membership
	for (auto& MembershipData : SetMembershipsResult.MembershipsData)
	{
		UPubnubChatChannel* Channel = CreateChannelObject(MembershipData.Channel.ChannelID, MembershipData.Channel);
		UPubnubChatMembership* NewMembership = CreateMembershipObject(GetCurrentUser(), Channel, MembershipData);
		FinalResult.Memberships.Add(NewMembership);
		
		//Don't send events on public channels
		if (Channel->GetChannelData().Type == "public")
		{ continue;}
		
		//Emit Receipt event - we check for error, but not stop execution in case of occuring one, just combine ErrorMessage for all events result
		FPubnubChatOperationResult EmitResult = EmitChatEvent(EPubnubChatEventType::PCET_Receipt, Channel->GetChannelID(), UPubnubChatInternalUtilities::GetReceiptEventPayload(CurrentTimetoken));
		if (EmitResult.Error)
		{
			FinalResult.Result.Error = true;
			FinalResult.Result.ErrorMessage += FString::Printf(TEXT(" | %s"), *EmitResult.ErrorMessage);
		}
	}
	
	FinalResult.Page = GetMembershipsResult.Page;
	FinalResult.Total = GetMembershipsResult.Total;
	
	return FinalResult;
}

FPubnubChatThreadChannelResult UPubnubChat::CreateThreadChannel(UPubnubChatMessage* Message)
{
	FPubnubChatThreadChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, Message);
	
	FPubnubChatMessageData MessageData = Message->GetMessageData();
	
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, !MessageData.ChannelID.StartsWith(Pubnub_Chat_Message_Thread_ID_Prefix), TEXT("This message is already in a Thread - only one level of thread nesting is allowed"));
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, !Message->IsDeleted().IsDeleted, TEXT("Can't create Thread on deleted message"));
	
	FString ThreadChannelID = UPubnubChatInternalUtilities::GetThreadID(MessageData.ChannelID, Message->GetMessageTimetoken());
	
	//Make sure this message hasn't already got the Thread
	FPubnubChatChannelResult GetChannelResult = GetChannel(ThreadChannelID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, GetChannelResult.Result.Error, TEXT("Thread for this message already exists"));
	
	FPubnubChatChannelData ThreadChannelData;
	ThreadChannelData.Description = UPubnubChatInternalUtilities::GetThreadID(MessageData.ChannelID, Message->GetMessageTimetoken());
	
	//This is just local creation, not pushed to the server. Thread will be created on api during first SendText on that thread
	FinalResult.ThreadChannel = CreateThreadChannelObject(ThreadChannelID, ThreadChannelData, Message, false);
	
	return FinalResult;
}

FPubnubChatThreadChannelResult UPubnubChat::GetThreadChannel(UPubnubChatMessage* Message)
{
	FPubnubChatThreadChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, Message);
	
	FPubnubChatMessageData MessageData = Message->GetMessageData();
	FString ThreadChannelID = UPubnubChatInternalUtilities::GetThreadID(MessageData.ChannelID, Message->GetMessageTimetoken());
	
	//GetChannelMetadata from PubnubClient
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ThreadChannelID, FPubnubGetMetadataInclude::FromValue(true));

	if (GetChannelResult.Result.Error)
	{
		FinalResult.Result.Error = true;
		FinalResult.Result.ErrorMessage = "Thread on this message doesn't exist";
		return FinalResult;
	}
	
	//Thread already exists on server (we just retrieved its metadata), so create thread channel object with IsThreadConfirmed = true
	FinalResult.ThreadChannel = CreateThreadChannelObject(ThreadChannelID, GetChannelResult.ChannelData, Message, true);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::RemoveThreadChannel(UPubnubChatMessage* Message)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	
	FPubnubChatOperationResult FinalResult;
	
	//Check if this Message has Thread
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(Message->HasThread().HasThread, TEXT("There is no Thread on this Message to be removed"));
	
	//Get ThreadRoot MessageAction
	FPubnubChatMessageData MessageData = Message->GetMessageData();
	FPubnubChatMessageAction ThreadRootMessageAction = UPubnubChatInternalUtilities::GetThreadRootMessageAction(MessageData.MessageActions);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!ThreadRootMessageAction.Timetoken.IsEmpty(), TEXT("This Message has invalid ThreadRoot MessageAction - Timetoken is empty"));
	
	//Remove ThreadRoot MessageAction using PubnubClient
	FPubnubOperationResult RemoveActionResult = PubnubClient->RemoveMessageAction(MessageData.ChannelID, Message->GetMessageTimetoken(), ThreadRootMessageAction.Timetoken);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveActionResult, "RemoveMessageAction");
	
	//Update MessageData (remove that action) and update repository
	UPubnubChatInternalUtilities::RemoveThreadRootFromMessageActions(MessageData.MessageActions);
	Message->UpdateMessageData(MessageData);
	
	//Delete ThreadChannel
	FString ThreadChannelID = UPubnubChatInternalUtilities::GetThreadID(MessageData.ChannelID, Message->GetMessageTimetoken());
	FPubnubChatOperationResult DeleteChannelResult = DeleteChannel(ThreadChannelID, false);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, DeleteChannelResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::ReconnectSubscriptions(const FString Timetoken)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	FPubnubOperationResult ReconnectResult = PubnubClient->ReconnectSubscriptions(Timetoken);
	FinalResult.AddStep("ReconnectSubscriptions", ReconnectResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChat::DisconnectSubscriptions()
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	FPubnubOperationResult DisconnectResult = PubnubClient->DisconnectSubscriptions();
	FinalResult.AddStep("DisconnectSubscriptions", DisconnectResult);
	
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
	CurrentUserID = InUserID;
	
	//Create repository for managing shared User and Channel data
	ObjectsRepository = NewObject<UPubnubChatObjectsRepository>(this);

	//Create Access Manager
	AccessManager = NewObject<UPubnubChatAccessManager>(this);
	AccessManager->InitAccessManager(PubnubClient);
	
	//SetAuthToken if any token was provided in config
	if(!InChatConfig.AuthKey.IsEmpty())
	{
		AccessManager->SetAuthToken(InChatConfig.AuthKey);
	}
	
	//Add callback for subscription status - it will be translated to chat connection status
	PubnubClient->OnSubscriptionStatusChanged.AddDynamic(this, &UPubnubChat::OnPubnubSubscriptionStatusChanged);

	//Get or create user for this chat instance
	FPubnubChatUserResult GetUserForInitResult = GetUserForInit(InUserID);

	//Return if any error happened on the way
	PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(FinalResult, GetUserForInitResult);

	FinalResult.Result.Merge(GetUserForInitResult.Result);
		
	CurrentUser = GetUserForInitResult.User;
	IsInitialized = true;
	FinalResult.Chat = this;

	// Start storing user activity timestamps if enabled
	if (ChatConfig.StoreUserActivityTimestamps)
	{
		StoreUserActivityTimestamp();
	}
	
	//Create new thread to queue all async chat operations
	AsyncFunctionsThread = new FPubnubFunctionThread;
	

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
		FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(InUserID, FPubnubUserInputData());
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
		
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

UPubnubChatMessage* UPubnubChat::CreateMessageObject(const FString Timetoken, const FPubnubHistoryMessageData& HistoryMessageData)
{
	//Create and init the message object
	UPubnubChatMessage* NewMessage = NewObject<UPubnubChatMessage>(this);
	NewMessage->InitMessage(PubnubClient, this, HistoryMessageData.Channel, Timetoken);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewMessage->GetInternalMessageID(), FPubnubChatMessageData::FromPubnubHistoryMessageData(HistoryMessageData));

	return NewMessage;
}

UPubnubChatMembership* UPubnubChat::CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChatMembershipData& ChatMembershipData)
{
	//Create and init the membership object
	UPubnubChatMembership* NewMembership = NewObject<UPubnubChatMembership>(this);
	NewMembership->InitMembership(PubnubClient, this, User, Channel);
	
	//Update repository with updated membership data
	ObjectsRepository->UpdateMembershipData(NewMembership->GetInternalMembershipID(), ChatMembershipData);

	return NewMembership;
}

UPubnubChatMembership* UPubnubChat::CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubMembershipData& MembershipData)
{
	//Create and init the membership object
	UPubnubChatMembership* NewMembership = NewObject<UPubnubChatMembership>(this);
	NewMembership->InitMembership(PubnubClient, this, User, Channel);
	
	//Update repository with updated membership data
	ObjectsRepository->UpdateMembershipData(NewMembership->GetInternalMembershipID(), FPubnubChatMembershipData::FromPubnubMembershipData(MembershipData));

	return NewMembership;
}

UPubnubChatMembership* UPubnubChat::CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChannelMemberData& ChannelMemberData)
{
	//Create and init the membership object
	UPubnubChatMembership* NewMembership = NewObject<UPubnubChatMembership>(this);
	NewMembership->InitMembership(PubnubClient, this, User, Channel);
	
	//Update repository with updated membership data
	ObjectsRepository->UpdateMembershipData(NewMembership->GetInternalMembershipID(), FPubnubChatMembershipData::FromPubnubChannelMemberData(ChannelMemberData));

	return NewMembership;
}

UPubnubChatThreadChannel* UPubnubChat::CreateThreadChannelObject(const FString ThreadChannelID, const FPubnubChatChannelData& ThreadChannelData, UPubnubChatMessage* Message, bool IsThreadAlreadyConfirmed)
{
	//Update repository with updated thread channel data (for ObjectsRepository we treat ThreadChannels as regular Channels)
	ObjectsRepository->UpdateChannelData(ThreadChannelID, ThreadChannelData);

	//Create and return the thread channel object
	UPubnubChatThreadChannel* NewThreadChannel = NewObject<UPubnubChatThreadChannel>(this);
	NewThreadChannel->InitThreadChannel(PubnubClient, this, ThreadChannelID, Message, IsThreadAlreadyConfirmed);
	
	return NewThreadChannel;
}

UPubnubChatThreadChannel* UPubnubChat::CreateThreadChannelObject(const FString ThreadChannelID, const FPubnubChannelData& ChannelData, UPubnubChatMessage* Message, bool IsThreadAlreadyConfirmed)
{
	//Update repository with updated thread channel data (for ObjectsRepository we treat ThreadChannels as regular Channels)
	ObjectsRepository->UpdateChannelData(ThreadChannelID, FPubnubChatChannelData::FromPubnubChannelData(ChannelData));

	//Create and return the thread channel object
	UPubnubChatThreadChannel* NewThreadChannel = NewObject<UPubnubChatThreadChannel>(this);
	NewThreadChannel->InitThreadChannel(PubnubClient, this, ThreadChannelID, Message, IsThreadAlreadyConfirmed);
	
	return NewThreadChannel;
}

UPubnubChatThreadMessage* UPubnubChat::CreateThreadMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData, const FString ParentChannelID)
{
	//Create and init the message object (for ObjectsRepository we treat ThreadMessages as regular Messages)
	UPubnubChatThreadMessage* NewThreadMessage = NewObject<UPubnubChatThreadMessage>(this);
	NewThreadMessage->InitThreadMessage(PubnubClient, this, ChatMessageData.ChannelID, Timetoken, ParentChannelID);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewThreadMessage->GetInternalMessageID(), ChatMessageData);

	return NewThreadMessage;
}

UPubnubChatThreadMessage* UPubnubChat::CreateThreadMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData, const FString ParentChannelID)
{
	//Create and init the message object (for ObjectsRepository we treat ThreadMessages as regular Messages)
	UPubnubChatThreadMessage* NewThreadMessage = NewObject<UPubnubChatThreadMessage>(this);
	NewThreadMessage->InitThreadMessage(PubnubClient, this, MessageData.Channel, Timetoken, ParentChannelID);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewThreadMessage->GetInternalMessageID(), FPubnubChatMessageData::FromPubnubMessageData(MessageData));

	return NewThreadMessage;
}

UPubnubChatThreadMessage* UPubnubChat::CreateThreadMessageObject(const FString Timetoken, const FPubnubHistoryMessageData& HistoryMessageData, const FString ParentChannelID)
{
	//Create and init the message object (for ObjectsRepository we treat ThreadMessages as regular Messages)
	UPubnubChatThreadMessage* NewThreadMessage = NewObject<UPubnubChatThreadMessage>(this);
	NewThreadMessage->InitThreadMessage(PubnubClient, this, HistoryMessageData.Channel, Timetoken, ParentChannelID);
	
	//Update repository with updated message data
	ObjectsRepository->UpdateMessageData(NewThreadMessage->GetInternalMessageID(), FPubnubChatMessageData::FromPubnubHistoryMessageData(HistoryMessageData));

	return NewThreadMessage;
}

void UPubnubChat::StoreUserActivityTimestamp()
{
	if (!IsInitialized || !CurrentUser)
	{ return; }

	// Clear any existing timers
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance)
	{ return; }

	FTimerManager& TimerManager = GameInstance->GetTimerManager();
	TimerManager.ClearTimer(LastSavedActivityIntervalTimerHandle);
	TimerManager.ClearTimer(RunWithDelayTimerHandle);

	// Get current user data
	FPubnubChatUserData UserData = CurrentUser->GetUserData();
	FString LastActiveTimestamp = UPubnubChatInternalUtilities::GetLastActiveTimestampFromCustom(UserData.Custom);

	if (LastActiveTimestamp.IsEmpty())
	{
		// No timestamp exists, start the interval immediately
		RunSaveTimestampInterval();
		return;
	}

	// Calculate elapsed time since last check
	if (!LastActiveTimestamp.IsNumeric())
	{
		// Invalid timestamp format, start interval immediately
		RunSaveTimestampInterval();
		return;
	}

	int64 LastTimestampValue = 0;
	LexFromString(LastTimestampValue, *LastActiveTimestamp);

	// Get current timetoken (17-digit format in 100ns units)
	FString CurrentTimetokenString = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	int64 CurrentTimetoken = 0;
	if (!CurrentTimetokenString.IsNumeric())
	{
		// Fallback to immediate save if timetoken generation fails
		RunSaveTimestampInterval();
		return;
	}
	LexFromString(CurrentTimetoken, *CurrentTimetokenString);

	// Calculate elapsed time in timetoken units (100ns), then convert to milliseconds
	// 1 millisecond = 10,000 timetoken units (100ns units)
	int64 ElapsedTimeTimetokenUnits = CurrentTimetoken - LastTimestampValue;
	int64 ElapsedTimeMs = ElapsedTimeTimetokenUnits / 10000LL;

	if (ElapsedTimeMs >= ChatConfig.StoreUserActivityInterval)
	{
		// Enough time has passed, start interval immediately
		RunSaveTimestampInterval();
		return;
	}

	// Schedule timer for remaining time
	int64 RemainingTimeMs = ChatConfig.StoreUserActivityInterval - ElapsedTimeMs;
	float RemainingTimeSeconds = RemainingTimeMs / 1000.0f;

	TWeakObjectPtr<UPubnubChat> WeakThis = MakeWeakObjectPtr(this);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->RunSaveTimestampInterval();
		}
	});

	TimerManager.SetTimer(RunWithDelayTimerHandle, TimerDelegate, RemainingTimeSeconds, false);
}

void UPubnubChat::SaveTimestamp()
{
	if (!IsInitialized || !CurrentUser)
	{ return; }

	// Get current timetoken in 17-digit format (100ns units) - standard PubNub format
	FString TimestampString = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();

	// Get current user data
	FPubnubChatUserData UserData = CurrentUser->GetUserData();

	// Add timestamp to custom data
	FString UpdatedCustom = UPubnubChatInternalUtilities::AddLastActiveTimestampToCustom(UserData.Custom, TimestampString);

	// Update user with new custom data
	FPubnubChatUpdateUserInputData UpdateUserData;
	UpdateUserData.Custom = UpdatedCustom;
	UpdateUserData.ForceSetCustom = true;

	// Update user asynchronously (don't wait for result)
	CurrentUser->Update(UpdateUserData);
}

void UPubnubChat::RunSaveTimestampInterval()
{
	// Save timestamp immediately
	SaveTimestamp();

	// Schedule periodic timer
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance)
	{ return; }

	FTimerManager& TimerManager = GameInstance->GetTimerManager();
	float IntervalSeconds = ChatConfig.StoreUserActivityInterval / 1000.0f;

	TWeakObjectPtr<UPubnubChat> WeakThis = MakeWeakObjectPtr(this);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->SaveTimestamp();
		}
	});

	TimerManager.SetTimer(LastSavedActivityIntervalTimerHandle, TimerDelegate, IntervalSeconds, true);
}
