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
#include "PubnubChatMessage.h"
#include "PubnubChatMembership.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"


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
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
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
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, GetUserResult.UserData);
	return FinalResult;
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

FPubnubChatUserResult UPubnubChat::UpdateUser(const FString UserID, FPubnubChatUserData UserData)
{
	FPubnubChatUserResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	//Make sure such User exists
	FPubnubChatUserResult GetUserResult = GetUser(UserID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result);
	
	//SetUserMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UserData.ToPubnubUserData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");

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
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, RemoveUserResult, "RemoveUserMetadata");
		
		//Remove user from repository
		ObjectsRepository->RemoveUserData(UserID);
		
		return FinalResult;
	}

	//Soft Delete - just update User Metadata

	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Add Deleted property to Custom field
	GetUserResult.UserData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(GetUserResult.UserData.Custom);

	//SetUserMetadata updated metadata
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, GetUserResult.UserData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	//Create user object and return final result
	FinalResult.User = CreateUserObject(UserID, GetUserResult.UserData);
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
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, ChannelData.ToPubnubChannelData());
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
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(FinalChannelID, ChannelData.ToPubnubChannelData());
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
	
	//If channel ID was not provided, generate Guid
	FString FinalChannelID = ChannelID.IsEmpty() ? FGuid::NewGuid().ToString(EGuidFormats::UniqueObjectGuid) : ChannelID;

	//Regardless of the provided Channel Type, this method creates public channel
	ChannelData.Type = "direct";

	//Set channel metadata by PubnubClient and create the channel
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(FinalChannelID, ChannelData.ToPubnubChannelData());
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

FPubnubChatChannelResult UPubnubChat::UpdateChannel(const FString ChannelID, FPubnubChatChannelData ChannelData)
{
	FPubnubChatChannelResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	//Make sure such channel exists
	FPubnubChatChannelResult GetChannelResult = GetChannel(ChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelResult.Result);

	//SetChannelMetadata by PubnubClient
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, ChannelData.ToPubnubChannelData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");

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
		PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, RemoveChannelResult, "RemoveChannelMetadata");
		
		//Remove channel from repository
		ObjectsRepository->RemoveChannelData(ChannelID);
		
		return FinalResult;
	}

	//Soft Delete - just update Channel Metadata

	//GetChannelMetadata from PubnubClient to have up to date data
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelResult.Result, "GetChannelMetadata");

	//Add Deleted property to Custom field
	GetChannelResult.ChannelData.Custom = UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(GetChannelResult.ChannelData.Custom);

	//SetChannelMetadata updated metadata
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, GetChannelResult.ChannelData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//Create channel object and return final result
	FinalResult.Channel = CreateChannelObject(ChannelID, GetChannelResult.ChannelData);
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
