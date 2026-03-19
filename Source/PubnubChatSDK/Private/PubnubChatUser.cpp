// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatUser.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatMembership.h"
#include "PubnubChatObjectsRepository.h"
#include "Entities/PubnubUserMetadataEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "PubnubChatConst.h"
#include "Threads/PubnubFunctionThread.h"


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

void UPubnubChatUser::UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	UpdateAsync(UpdateUserData, NativeCallback);
}

void UPubnubChatUser::UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UpdateUserData = MoveTemp(UpdateUserData), OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult UpdateResult = WeakThis.Get()->Update(UpdateUserData);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, UpdateResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::Delete()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatOperationResult DeleteUserResult = Chat->DeleteUser(UserID);
	return DeleteUserResult;
}

void UPubnubChatUser::DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	DeleteAsync(NativeCallback);
}

void UPubnubChatUser::DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult DeleteResult = WeakThis.Get()->Delete();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, DeleteResult);
	});
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

void UPubnubChatUser::WherePresentAsync(FOnPubnubChatWherePresentResponse OnWherePresentResponse)
{
	FOnPubnubChatWherePresentResponseNative NativeCallback;
	NativeCallback.BindLambda([OnWherePresentResponse](const FPubnubChatWherePresentResult& WherePresentResult)
	{
		OnWherePresentResponse.ExecuteIfBound(WherePresentResult);
	});

	WherePresentAsync(NativeCallback);
}

void UPubnubChatUser::WherePresentAsync(FOnPubnubChatWherePresentResponseNative OnWherePresentResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnWherePresentResponseNative, FPubnubChatWherePresentResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnWherePresentResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatWherePresentResult WherePresentResult = WeakThis.Get()->WherePresent();
		UPubnubUtilities::CallPubnubDelegate(OnWherePresentResponseNative, WherePresentResult);
	});
}

FPubnubChatIsPresentResult UPubnubChatUser::IsPresentOn(const FString ChannelID)
{
	FPubnubChatIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->IsPresent(UserID, ChannelID);
}

void UPubnubChatUser::IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponse OnIsPresentResponse)
{
	FOnPubnubChatIsPresentResponseNative NativeCallback;
	NativeCallback.BindLambda([OnIsPresentResponse](const FPubnubChatIsPresentResult& IsPresentResult)
	{
		OnIsPresentResponse.ExecuteIfBound(IsPresentResult);
	});

	IsPresentOnAsync(ChannelID, NativeCallback);
}

void UPubnubChatUser::IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnIsPresentResponseNative, FPubnubChatIsPresentResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, ChannelID, OnIsPresentResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatIsPresentResult IsPresentResult = WeakThis.Get()->IsPresentOn(ChannelID);
		UPubnubUtilities::CallPubnubDelegate(OnIsPresentResponseNative, IsPresentResult);
	});
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

void UPubnubChatUser::GetMembershipsAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FOnPubnubChatMembershipsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMembershipsResponse](const FPubnubChatMembershipsResult& MembershipsResult)
	{
		OnMembershipsResponse.ExecuteIfBound(MembershipsResult);
	});

	GetMembershipsAsync(NativeCallback, Limit, Filter, Sort, Page);
}

void UPubnubChatUser::GetMembershipsAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit, const FString Filter, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMembershipsResponseNative, FPubnubChatMembershipsResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Filter, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnMembershipsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMembershipsResult MembershipsResult = WeakThis.Get()->GetMemberships(Limit, Filter, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnMembershipsResponseNative, MembershipsResult);
	});
}

FPubnubChatMembershipResult UPubnubChatUser::GetMembership(const FString ChannelID)
{
	FPubnubChatMembershipResult FinalResult;
	FinalResult.Membership = nullptr;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, ChannelID);

	FPubnubChatMembershipsResult GetMembershipsResult = GetMemberships(1, UPubnubChatInternalUtilities::GetFilterForChannelID(ChannelID));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipsResult.Result);

	if (!GetMembershipsResult.Memberships.IsEmpty())
	{
		FinalResult.Membership = GetMembershipsResult.Memberships[0];
	}

	return FinalResult;
}

void UPubnubChatUser::GetMembershipAsync(const FString ChannelID, FOnPubnubChatMembershipResponse OnMembershipResponse)
{
	FOnPubnubChatMembershipResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMembershipResponse](const FPubnubChatMembershipResult& MembershipResult)
	{
		OnMembershipResponse.ExecuteIfBound(MembershipResult);
	});

	GetMembershipAsync(ChannelID, NativeCallback);
}

void UPubnubChatUser::GetMembershipAsync(const FString ChannelID, FOnPubnubChatMembershipResponseNative OnMembershipResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMembershipResponseNative, FPubnubChatMembershipResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, ChannelID, OnMembershipResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMembershipResult MembershipResult = WeakThis.Get()->GetMembership(ChannelID);
		UPubnubUtilities::CallPubnubDelegate(OnMembershipResponseNative, MembershipResult);
	});
}

FPubnubChatIsMemberOnResult UPubnubChatUser::IsMemberOn(const FString ChannelID)
{
	FPubnubChatIsMemberOnResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubChatMembershipResult GetMembershipResult = GetMembership(ChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembershipResult.Result);

	FinalResult.IsMemberOn = IsValid(GetMembershipResult.Membership);
	return FinalResult;
}

void UPubnubChatUser::IsMemberOnAsync(const FString ChannelID, FOnPubnubChatIsMemberOnResponse OnIsMemberOnResponse)
{
	FOnPubnubChatIsMemberOnResponseNative NativeCallback;
	NativeCallback.BindLambda([OnIsMemberOnResponse](const FPubnubChatIsMemberOnResult& IsMemberOnResult)
	{
		OnIsMemberOnResponse.ExecuteIfBound(IsMemberOnResult);
	});

	IsMemberOnAsync(ChannelID, NativeCallback);
}

void UPubnubChatUser::IsMemberOnAsync(const FString ChannelID, FOnPubnubChatIsMemberOnResponseNative OnIsMemberOnResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnIsMemberOnResponseNative, FPubnubChatIsMemberOnResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, ChannelID, OnIsMemberOnResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatIsMemberOnResult IsMemberOnResult = WeakThis.Get()->IsMemberOn(ChannelID);
		UPubnubUtilities::CallPubnubDelegate(OnIsMemberOnResponseNative, IsMemberOnResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::SetRestrictions(const FString ChannelID, bool Ban, bool Mute, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(ChannelID);
	
	return Chat->SetRestrictions(FPubnubChatRestriction(UserID, ChannelID, Ban, Mute, Reason));
}

void UPubnubChatUser::SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponse OnOperationResponse, FString Reason)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SetRestrictionsAsync(ChannelID, Ban, Mute, NativeCallback, Reason);
}

void UPubnubChatUser::SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponseNative OnOperationResponseNative, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, ChannelID, Ban, Mute, Reason, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult SetRestrictionsResult = WeakThis.Get()->SetRestrictions(ChannelID, Ban, Mute, Reason);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, SetRestrictionsResult);
	});
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

void UPubnubChatUser::GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponse OnRestrictionResponse)
{
	FOnPubnubChatGetRestrictionResponseNative NativeCallback;
	NativeCallback.BindLambda([OnRestrictionResponse](const FPubnubChatGetRestrictionResult& RestrictionResult)
	{
		OnRestrictionResponse.ExecuteIfBound(RestrictionResult);
	});

	GetChannelRestrictionsAsync(Channel, NativeCallback);
}

void UPubnubChatUser::GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnRestrictionResponseNative, FPubnubChatGetRestrictionResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Channel, OnRestrictionResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetRestrictionResult RestrictionResult = WeakThis.Get()->GetChannelRestrictions(Channel);
		UPubnubUtilities::CallPubnubDelegate(OnRestrictionResponseNative, RestrictionResult);
	});
}

FPubnubChatGetRestrictionsResult UPubnubChatUser::GetChannelsRestrictions(const int Limit, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return GetRestrictions(Limit, UPubnubChatInternalUtilities::GetFilterForChannelsRestrictions(), Sort, Page);
}

void UPubnubChatUser::GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponse OnRestrictionsResponse, const int Limit, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	FOnPubnubChatGetRestrictionsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnRestrictionsResponse](const FPubnubChatGetRestrictionsResult& RestrictionsResult)
	{
		OnRestrictionsResponse.ExecuteIfBound(RestrictionsResult);
	});

	GetChannelsRestrictionsAsync(NativeCallback, Limit, Sort, Page);
}

void UPubnubChatUser::GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponseNative, const int Limit, FPubnubMembershipSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnRestrictionsResponseNative, FPubnubChatGetRestrictionsResult());
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnRestrictionsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetRestrictionsResult RestrictionsResult = WeakThis.Get()->GetChannelsRestrictions(Limit, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnRestrictionsResponseNative, RestrictionsResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StreamMentions()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Skip if it's already streaming
	if (IsStreamingMentions)
	{ return FinalResult; }

	TWeakObjectPtr<UPubnubChatUser> ThisWeak = MakeWeakObjectPtr(this);

	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}

		UPubnubChatUser* ThisUser = ThisWeak.Get();
		if (!ThisUser->IsInitialized || !ThisUser->Chat || !ThisUser->IsStreamingMentions)
		{ return; }

		FPubnubChatUserMention UserMention = UPubnubChatInternalUtilities::GetUserMentionFromChatEvent(Event);
		if (UserMention.MessageTimetoken.IsEmpty() || UserMention.ChannelID.IsEmpty())
		{ return; }

		ThisUser->OnMentioned.Broadcast(UserMention);
		ThisUser->OnMentionedNative.Broadcast(UserMention);
	});

	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(UserID, EPubnubChatEventType::PCET_Mention, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((ListenForEventsResult.CallbackStop != nullptr), TEXT("Failed to start streaming mentions"));

	MentionedCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingMentions = true;

	return FinalResult;
}

void UPubnubChatUser::StreamMentionsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamMentionsAsync(NativeCallback);
}

void UPubnubChatUser::StreamMentionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StreamMentionsResult = WeakThis.Get()->StreamMentions();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamMentionsResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StopStreamingMentions()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	if (!IsStreamingMentions)
	{ return FinalResult; }

	if (!MentionedCallbackStop)
	{
		IsStreamingMentions = false;
		return FinalResult;
	}

	FPubnubChatOperationResult StopResult = MentionedCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);

	MentionedCallbackStop = nullptr;
	IsStreamingMentions = false;

	return FinalResult;
}

void UPubnubChatUser::StopStreamingMentionsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingMentionsAsync(NativeCallback);
}

void UPubnubChatUser::StopStreamingMentionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingMentions();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StreamInvitations()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Skip if it's already streaming
	if (IsStreamingInvitations)
	{ return FinalResult; }

	TWeakObjectPtr<UPubnubChatUser> ThisWeak = MakeWeakObjectPtr(this);

	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}

		UPubnubChatUser* ThisUser = ThisWeak.Get();
		if (!ThisUser->IsInitialized || !ThisUser->Chat || !ThisUser->IsStreamingInvitations)
		{ return; }

		FPubnubChatInviteEvent InviteEvent = UPubnubChatInternalUtilities::GetInviteEventFromChatEvent(Event);
		if (InviteEvent.ChannelID.IsEmpty() || InviteEvent.ChannelType.IsEmpty())
		{ return; }

		ThisUser->OnInvited.Broadcast(InviteEvent);
		ThisUser->OnInvitedNative.Broadcast(InviteEvent);
	});

	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(UserID, EPubnubChatEventType::PCET_Invite, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((ListenForEventsResult.CallbackStop != nullptr), TEXT("Failed to start streaming invitations"));

	InvitedCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingInvitations = true;

	return FinalResult;
}

void UPubnubChatUser::StreamInvitationsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamInvitationsAsync(NativeCallback);
}

void UPubnubChatUser::StreamInvitationsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StreamInvitationsResult = WeakThis.Get()->StreamInvitations();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamInvitationsResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StopStreamingInvitations()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	if (!IsStreamingInvitations)
	{ return FinalResult; }

	if (!InvitedCallbackStop)
	{
		IsStreamingInvitations = false;
		return FinalResult;
	}

	FPubnubChatOperationResult StopResult = InvitedCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);

	InvitedCallbackStop = nullptr;
	IsStreamingInvitations = false;

	return FinalResult;
}

void UPubnubChatUser::StopStreamingInvitationsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingInvitationsAsync(NativeCallback);
}

void UPubnubChatUser::StopStreamingInvitationsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingInvitations();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StreamRestrictions()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Skip if it's already streaming
	if (IsStreamingRestrictions)
	{ return FinalResult; }

	const FString ModerationEventChannel = UPubnubChatInternalUtilities::GetModerationEventChannelForUserID(UserID);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!ModerationEventChannel.IsEmpty(), TEXT("Failed to resolve moderation event channel"));

	TWeakObjectPtr<UPubnubChatUser> ThisWeak = MakeWeakObjectPtr(this);

	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}

		UPubnubChatUser* ThisUser = ThisWeak.Get();
		if (!ThisUser->IsInitialized || !ThisUser->Chat || !ThisUser->IsStreamingRestrictions)
		{ return; }

		FPubnubChatRestriction Restriction = UPubnubChatInternalUtilities::GetRestrictionFromModerationEvent(Event, ThisUser->UserID);
		if (Restriction.ChannelID.IsEmpty())
		{ return; }

		ThisUser->OnRestrictionChanged.Broadcast(Restriction);
		ThisUser->OnRestrictionChangedNative.Broadcast(Restriction);
	});

	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(ModerationEventChannel, EPubnubChatEventType::PCET_Moderation, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((ListenForEventsResult.CallbackStop != nullptr), TEXT("Failed to start streaming restrictions"));

	RestrictionCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingRestrictions = true;

	return FinalResult;
}

void UPubnubChatUser::StreamRestrictionsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamRestrictionsAsync(NativeCallback);
}

void UPubnubChatUser::StreamRestrictionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StreamRestrictionsResult = WeakThis.Get()->StreamRestrictions();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamRestrictionsResult);
	});
}

FPubnubChatOperationResult UPubnubChatUser::StopStreamingRestrictions()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	if (!IsStreamingRestrictions)
	{ return FinalResult; }

	if (!RestrictionCallbackStop)
	{
		IsStreamingRestrictions = false;
		return FinalResult;
	}

	FPubnubChatOperationResult StopResult = RestrictionCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);

	RestrictionCallbackStop = nullptr;
	IsStreamingRestrictions = false;

	return FinalResult;
}

void UPubnubChatUser::StopStreamingRestrictionsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingRestrictionsAsync(NativeCallback);
}

void UPubnubChatUser::StopStreamingRestrictionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingRestrictions();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
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

		if(!ThisUser->IsInitialized || !ThisUser->Chat || !ThisUser->IsStreamingUpdates)
		{return;}
		
		//If this is not UserUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageUserUpdate(MessageData.Message))
		{
			//Check if user was deleted or updated
			if (UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(MessageData.Message))
			{
				//Remove this user from repository
				ThisUser->Chat->ObjectsRepository->RemoveUserData(ThisUser->UserID);
				
				//Call OnDeleted delegates
				ThisUser->OnDeleted.Broadcast();
				ThisUser->OnDeletedNative.Broadcast();
			}
			else
			{
				//Adjust this user data based on the update message
				FPubnubChatUserData ChatUserData = ThisUser->GetUserData();
				FPubnubUserUpdateData UserUpdateData = UPubnubJsonUtilities::GetUserUpdateDataFromMessageContent(MessageData.Message);
				UPubnubChatInternalUtilities::UpdateChatUserFromPubnubUserUpdateData(UserUpdateData, ChatUserData);
							
				//Update repository with new user data
				ThisUser->Chat->ObjectsRepository->UpdateUserData(ThisUser->UserID, ChatUserData);
							
				//Call OnUpdated delegates with new user data
				ThisUser->OnUpdated.Broadcast(ThisUser->UserID, ChatUserData);
				ThisUser->OnUpdatedNative.Broadcast(ThisUser->UserID, ChatUserData);
			}
		}
	});
	
	//Subscribe with UpdatesSubscription to receive user metadata updates
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

void UPubnubChatUser::StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamUpdatesAsync(NativeCallback);
}

void UPubnubChatUser::StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamUpdatesResult = WeakThis.Get()->StreamUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamUpdatesResult);
	});
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

void UPubnubChatUser::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingUpdatesAsync(NativeCallback);
}

void UPubnubChatUser::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatUser> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
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
	if (MentionedCallbackStop)
	{
		MentionedCallbackStop->Stop();
		MentionedCallbackStop = nullptr;
	}
	IsStreamingMentions = false;

	if (InvitedCallbackStop)
	{
		InvitedCallbackStop->Stop();
		InvitedCallbackStop = nullptr;
	}
	IsStreamingInvitations = false;

	if (RestrictionCallbackStop)
	{
		RestrictionCallbackStop->Stop();
		RestrictionCallbackStop = nullptr;
	}
	IsStreamingRestrictions = false;
	
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

