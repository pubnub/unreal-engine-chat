// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "Entities/PubnubUserMetadataEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "PubnubChatConst.h"


void UPubnubChatUser::BeginDestroy()
{
	CleanUp();
	
	Super::BeginDestroy();
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

FPubnubChatOperationResult UPubnubChatUser::Update(FPubnubChatUpdateUserInputData UpdateUserData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//SetChannelMetadata by PubnubClient
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, UpdateUserData.ToPubnubUserInputData(), FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetUserResult.Result, "SetUserMetadata");
	
	//Update repository with updated channel data
	Chat->ObjectsRepository->UpdateUserData(UserID, FPubnubChatUserData::FromPubnubUserData(SetUserResult.UserData));
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatUser::Delete(bool Soft)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatOperationResult DeleteUserResult = Chat->DeleteUser(UserID, Soft);
	return DeleteUserResult;
}

FPubnubChatOperationResult UPubnubChatUser::Restore()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	
	//GetUserMetadata from PubnubClient to have up to date data
	FPubnubUserMetadataResult GetUserResult = PubnubClient->GetUserMetadata(UserID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetUserResult.Result, "GetUserMetadata");

	//Add Deleted property to Custom field
	FPubnubUserInputData NewUserData = FPubnubUserInputData::FromPubnubUserData(GetUserResult.UserData);
	NewUserData.Custom = UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(NewUserData.Custom);

	//SetUserMetadata with updated metadata
	FPubnubUserMetadataResult SetUserResult = PubnubClient->SetUserMetadata(UserID, NewUserData);
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

bool UPubnubChatUser::IsActive() const
{
	if (!IsInitialized || !Chat)
	{ return false; }

	// Get user data to check lastActiveTimestamp
	FPubnubChatUserData UserData = GetUserData();
	FString LastActiveTimestamp = UPubnubChatInternalUtilities::GetLastActiveTimestampFromCustom(UserData.Custom);

	// If no timestamp exists, user is not active
	if (LastActiveTimestamp.IsEmpty())
	{ return false; }

	// Validate timestamp is numeric
	if (!LastActiveTimestamp.IsNumeric())
	{ return false; }

	// Parse timestamp (stored as timetoken in 100ns units)
	int64 LastTimestampTimetoken = 0;
	LexFromString(LastTimestampTimetoken, *LastActiveTimestamp);

	// Get current timetoken (17-digit format in 100ns units)
	FString CurrentTimetokenString = UPubnubTimetokenUtilities::GetCurrentUnixTimetoken();
	if (!CurrentTimetokenString.IsNumeric())
	{
		return false;
	}

	int64 CurrentTimetoken = 0;
	LexFromString(CurrentTimetoken, *CurrentTimetokenString);

	// Calculate elapsed time in timetoken units (100ns), then convert to milliseconds
	// 1 millisecond = 10,000 timetoken units (100ns units)
	int64 ElapsedTimeTimetokenUnits = CurrentTimetoken - LastTimestampTimetoken;
	int64 ElapsedTimeMs = ElapsedTimeTimetokenUnits / 10000LL;

	// User is active if elapsed time is within the activity interval
	return ElapsedTimeMs <= Chat->ChatConfig.StoreUserActivityInterval;
}

FString UPubnubChatUser::GetLastActiveTimestamp() const
{
	if (!IsInitialized || !Chat)
	{ return ""; }
	
	FPubnubChatUserData UserData = GetUserData();
	return UPubnubChatInternalUtilities::GetLastActiveTimestampFromCustom(UserData.Custom);
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

FPubnubChatMembershipsResult UPubnubChatUser::GetMemberships(const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatMembershipsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//GetMemberships using PubnubClient
	FPubnubMembershipsResult GetMembershipsResult = PubnubClient->GetMemberships(UserID, FPubnubMembershipInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipsResult.Result, "GetMemberships");
	
	//Create corresponding Chat objects for all returned memberships
	for (auto& MembershipData : GetMembershipsResult.MembershipsData)
	{
		UPubnubChatChannel* Channel =  Chat->CreateChannelObject(MembershipData.Channel.ChannelID, MembershipData.Channel);
		UPubnubChatMembership* Membership = Chat->CreateMembershipObject(this, Channel, MembershipData);
		FinalResult.Memberships.Add(Membership);
	}
	
	FinalResult.Page = GetMembershipsResult.Page;
	FinalResult.Total = GetMembershipsResult.TotalCount;
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatUser::SetRestrictions(const FString ChannelID, bool Ban, bool Mute, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(ChannelID);
	
	return Chat->SetRestrictions(FPubnubChatRestriction(UserID, ChannelID, Ban, Mute, Reason));
}

FPubnubChatGetRestrictionResult UPubnubChatUser::GetChannelRestrictions(UPubnubChatChannel* Channel)
{
	FPubnubChatGetRestrictionResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, Channel);
	
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(Channel->GetChannelID());
	FPubnubChatGetRestrictionsResult GetRestrictionsResult = GetRestrictions(1, UPubnubChatInternalUtilities::GetFilterForChannelID(ModerationChannelID));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetRestrictionsResult.Result);

	//If there was any restriction returned, just add it. If not, there is no restriction
	if (!GetRestrictionsResult.Restrictions.IsEmpty())
	{
		FinalResult.Restriction = GetRestrictionsResult.Restrictions[0];
	}
	else
	{
		FinalResult.Restriction = FPubnubChatRestriction({.UserID = UserID, .ChannelID = Channel->GetChannelID()});
	}
	
	return FinalResult;
}

FPubnubChatGetRestrictionsResult UPubnubChatUser::GetChannelsRestrictions(const int Limit, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return GetRestrictions(Limit, UPubnubChatInternalUtilities::GetFilterForChannelsRestrictions(), Sort, Page);
}

FPubnubChatOperationResult UPubnubChatUser::StreamUpdates()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already streaming
	if (IsStreamingUpdates)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatUser> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add listener to subscription with provided callback
	UpdatesSubscription->OnPubnubObjectEventNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatUser* ThisUser = ThisWeak.Get();

		if(!ThisUser->Chat)
		{return;}
		
		//If this is not UserUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageUserUpdate(MessageData.Message))
		{
			//Check if user was deleted or updated
			if (UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(MessageData.Message))
			{
				//Remove this user from repository
				ThisUser->Chat->ObjectsRepository->RemoveUserData(ThisUser->UserID);
				
				//Call delegates with Deleted type
				ThisUser->OnUserUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisUser->UserID, FPubnubChatUserData());
				ThisUser->OnUserUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisUser->UserID, FPubnubChatUserData());
			}
			else
			{
				//Adjust this user data based on the update message
				FPubnubChatUserData ChatUserData = ThisUser->GetUserData();
				FPubnubUserUpdateData UserUpdateData = UPubnubJsonUtilities::GetUserUpdateDataFromMessageContent(MessageData.Message);
				UPubnubChatInternalUtilities::UpdateChatUserFromPubnubUserUpdateData(UserUpdateData, ChatUserData);
							
				//Update repository with new user data
				ThisUser->Chat->ObjectsRepository->UpdateUserData(ThisUser->UserID, ChatUserData);
							
				//Call delegates with new user data
				ThisUser->OnUserUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisUser->UserID, ChatUserData);
				ThisUser->OnUserUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisUser->UserID, ChatUserData);
			}
		}
	});
	
	//Subscribe with UpdatesSubscription to receive user metadata updates
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatUser::StreamUpdatesOn(const TArray<UPubnubChatUser*>& Users)
{
	FPubnubChatOperationResult FinalResult;
	for (auto& User : Users)
	{
		FPubnubChatOperationResult StreamUpdatesResult = User->StreamUpdates();
		FinalResult.Merge(StreamUpdatesResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatUser::StopStreamingUpdates()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Remove message related delegates
	UpdatesSubscription->OnPubnubObjectEventNative.Clear();
	
	//Skip if it's not streaming updates
	if (!IsStreamingUpdates)
	{ return FinalResult; }

	//Unsubscribe and return result
	FPubnubOperationResult UnsubscribeResult = UpdatesSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsStreamingUpdates = false;
	return FinalResult;
}


void UPubnubChatUser::InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init User, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init User, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InUserID.IsEmpty(), TEXT("Can't init User, UserID is empty"));

	UserID = InUserID;
	PubnubClient = InPubnubClient;
	Chat = InChat;

	UPubnubUserMetadataEntity* UserEntity = PubnubClient->CreateUserMetadataEntity(UserID);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UserEntity, TEXT("Can't init User, Failed to create UserMetadataEntity"));

	UpdatesSubscription = UserEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UpdatesSubscription, TEXT("Can't init User, Failed to create Updates Subscription"));
	
	// Register this user object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterUser(UserID);
	}
	
	//Add delegate to OnChatDestroyed to this object is cleaned up as well
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatUser::OnChatDestroyed);
	
	IsInitialized = true;
}

FPubnubChatGetRestrictionsResult UPubnubChatUser::GetRestrictions(const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	
	FPubnubMembershipInclude Include = FPubnubMembershipInclude({.IncludeCustom = true, .IncludeTotalCount = true}); 
	FPubnubMembershipsResult GetMembershipsResult = PubnubClient->GetMemberships(UserID, Include, Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipsResult.Result, "GetMemberships");
	
	//Convert Custom fields to Restrictions
	for (auto& MembershipData : GetMembershipsResult.MembershipsData)
	{
		FPubnubChatRestriction Restriction = UPubnubChatInternalUtilities::GetRestrictionFromChannelMemberCustom(MembershipData.Custom);
		Restriction.UserID = UserID;
		Restriction.ChannelID = UPubnubChatInternalUtilities::GetChannelIDFromModerationChannel(MembershipData.Channel.ChannelID);
		
		FinalResult.Restrictions.Add(Restriction);
	}
	
	FinalResult.Page = GetMembershipsResult.Page;
	FinalResult.Total = GetMembershipsResult.TotalCount;
	
	return FinalResult;
}

void UPubnubChatUser::OnChatDestroyed(FString InUserID)
{
	CleanUp();
}

void UPubnubChatUser::ClearAllSubscriptions()
{
	if (UpdatesSubscription)
	{
		UpdatesSubscription->OnPubnubObjectEventNative.Clear();
		if (IsStreamingUpdates)
		{
			UpdatesSubscription->Unsubscribe();
			IsStreamingUpdates = false;
		}

		UpdatesSubscription = nullptr;
	}
}

void UPubnubChatUser::CleanUp()
{
	//Clean up subscription if channel is being destroyed while connected
	if (IsInitialized)
	{
		ClearAllSubscriptions();
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !UserID.IsEmpty())
	{
		Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatUser::OnChatDestroyed);
		Chat->ObjectsRepository->UnregisterChannel(UserID);
	}
	
	IsInitialized = false;
}

