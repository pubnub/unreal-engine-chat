// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatChannel.h"
#include "PubnubClient.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "PubnubChat.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatMembership.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatMessage.h"
#include "PubnubChatUser.h"
#include "PubnubChatThreadMessage.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformProcess.h"
#include <cmath> 

#include "PubnubChatMessageDraft.h"
#include "Threads/PubnubFunctionThread.h"


void UPubnubChatChannel::BeginDestroy()
{
	CleanUp();
	
	Super::BeginDestroy();
}

FPubnubChatChannelData UPubnubChatChannel::GetChannelData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatChannelData());

	//Get channel data from repository
	if (FPubnubChatInternalChannel* InternalChannel = Chat->ObjectsRepository->GetChannelData(ChannelID))
	{
		return InternalChannel->ChannelData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("Channel data not found in repository for ChannelID: %s"), *ChannelID);
	return FPubnubChatChannelData();
}

FPubnubChatOperationResult UPubnubChatChannel::Update(FPubnubChatUpdateChannelInputData UpdateChannelData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//SetChannelMetadata by PubnubClient - include all fields in response
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, UpdateChannelData.ToPubnubChannelInputData(), FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//Update repository with updated channel data
	Chat->ObjectsRepository->UpdateChannelData(ChannelID, FPubnubChatChannelData::FromPubnubChannelData(SetChannelResult.ChannelData));

	return FinalResult;
}

void UPubnubChatChannel::UpdateAsync(FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	UpdateAsync(UpdateChannelData, NativeCallback);
}

void UPubnubChatChannel::UpdateAsync(FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UpdateChannelData = MoveTemp(UpdateChannelData), OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult UpdateResult = WeakThis.Get()->Update(UpdateChannelData);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, UpdateResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::Connect()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already connected
	if (IsConnected)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add lister to subscription with provided callback
	AddOnMessageReceivedLambdaToSubscription(ThisWeak);
	
	//Subscribe with this channel Subscription
	FPubnubOperationResult SubscribeResult = ConnectSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsConnected = true;
	
	return FinalResult;
}

void UPubnubChatChannel::ConnectAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	ConnectAsync(NativeCallback);
}

void UPubnubChatChannel::ConnectAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult ConnectResult = WeakThis.Get()->Connect();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, ConnectResult);
	});
}

FPubnubChatJoinResult UPubnubChatChannel::Join(FPubnubChatMembershipData MembershipData)
{
	FPubnubChatJoinResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//SetMemberships by PubnubClient
	FPubnubMembershipInputData MembershipInputData = MembershipData.ToPubnubMembershipInputData(ChannelID);
	//This forces to reset status if not provided by User. Otherwise, Status could stay as "pending" for previously invited user.
	MembershipInputData.ForceSetStatus = true;
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(Chat->CurrentUserID, {MembershipInputData}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");

	//Create membership objects
	UPubnubChatMembership* CreatedMembership = Chat->CreateMembershipObject(Chat->CurrentUser, this, MembershipData);

	//SetLastReadMessageTimetoken for created membership
	FPubnubChatOperationResult SetLRMTResult =  CreatedMembership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetLRMTResult);

	//Fill required data to the result
	FinalResult.Membership = CreatedMembership;
	
	return FinalResult;
}

void UPubnubChatChannel::JoinAsync(FOnPubnubChatJoinResponse OnJoinResponse, FPubnubChatMembershipData MembershipData)
{
	FOnPubnubChatJoinResponseNative NativeCallback;
	NativeCallback.BindLambda([OnJoinResponse](const FPubnubChatJoinResult& JoinResult)
	{
		OnJoinResponse.ExecuteIfBound(JoinResult);
	});

	JoinAsync(NativeCallback, MembershipData);
}

void UPubnubChatChannel::JoinAsync(FOnPubnubChatJoinResponseNative OnJoinResponseNative, FPubnubChatMembershipData MembershipData)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnJoinResponseNative, FPubnubChatJoinResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, MembershipData = MoveTemp(MembershipData), OnJoinResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatJoinResult JoinResult = WeakThis.Get()->Join(MembershipData);
		UPubnubUtilities::CallPubnubDelegate(OnJoinResponseNative, JoinResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::Disconnect()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Remove message related delegates
	ConnectSubscription->OnPubnubMessageNative.Clear();
	
	//Skip if it's not connected
	if (!IsConnected)
	{ return FinalResult; }

	//Unsubscribe and return result
	FPubnubOperationResult UnsubscribeResult = ConnectSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsConnected = false;
	return FinalResult;
}

void UPubnubChatChannel::DisconnectAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	DisconnectAsync(NativeCallback);
}

void UPubnubChatChannel::DisconnectAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult DisconnectResult = WeakThis.Get()->Disconnect();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, DisconnectResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::Leave()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatOperationResult FinalResult;

	//RemoveMemberships by PubnubClient
	FPubnubMembershipsResult RemoveMembershipsResult = PubnubClient->RemoveMemberships(Chat->CurrentUserID, {ChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
	FinalResult.AddStep("RemoveMemberships", RemoveMembershipsResult.Result);

	return FinalResult;
}

void UPubnubChatChannel::LeaveAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	LeaveAsync(NativeCallback);
}

void UPubnubChatChannel::LeaveAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult LeaveResult = WeakThis.Get()->Leave();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, LeaveResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::SendText(const FString Message, FPubnubChatSendTextParams SendTextParams)
{
	return SendTextInternal(Message, SendTextParams);
}

void UPubnubChatChannel::SendTextAsync(const FString Message, FOnPubnubChatOperationResponse OnOperationResponse, FPubnubChatSendTextParams SendTextParams)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SendTextAsync(Message, NativeCallback, SendTextParams);
}

void UPubnubChatChannel::SendTextAsync(const FString Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative, FPubnubChatSendTextParams SendTextParams)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Message, SendTextParams = MoveTemp(SendTextParams), OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult SendTextResult = WeakThis.Get()->SendText(Message, SendTextParams);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, SendTextResult);
	});
}

FPubnubChatInviteResult UPubnubChatChannel::Invite(UPubnubChatUser* User)
{
	FPubnubChatInviteResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);

	//GetChannelMembers from PubnubClient to check if the user is not already member of that channel
	FString Filter = UPubnubChatInternalUtilities::GetFilterForUserID(User->GetUserID());
	FPubnubMemberInclude Include = FPubnubMemberInclude({.IncludeCustom=true, .IncludeStatus=true, .IncludeType=true});
	FPubnubChannelMembersResult GetChannelMembersResult = PubnubClient->GetChannelMembers(ChannelID, Include, 1, Filter);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelMembersResult.Result, "GetChannelMembers");
	
	//If User is already member of that channel, we just return the membership
	if(!GetChannelMembersResult.MembersData.IsEmpty())
	{
		FinalResult.Membership = Chat->CreateMembershipObject(User, this, GetChannelMembersResult.MembersData[0]);
		return FinalResult;
	}

	//Create Membership with "pending" status
	FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData{.Status = Pubnub_Chat_Invited_User_Membership_status};
	
	//SetMemberships by PubnubClient
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(User->GetUserID(), {MembershipData.ToPubnubMembershipInputData(ChannelID)}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");

	//Emit Invite event
	FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Invite, User->GetUserID(), UPubnubChatInternalUtilities::GetInviteEventPayload(ChannelID, GetChannelData().Type));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, EmitEventResult);

	//Create Membership Object
	FinalResult.Membership = Chat->CreateMembershipObject(User, this, MembershipData);

	//Set Last Read Timetoken on created Membership
	FPubnubChatOperationResult SetLRMTResult = FinalResult.Membership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
	FinalResult.Result.Merge(SetLRMTResult);

	return FinalResult;
}

void UPubnubChatChannel::InviteAsync(UPubnubChatUser* User, FOnPubnubChatInviteResponse OnInviteResponse)
{
	FOnPubnubChatInviteResponseNative NativeCallback;
	NativeCallback.BindLambda([OnInviteResponse](const FPubnubChatInviteResult& InviteResult)
	{
		OnInviteResponse.ExecuteIfBound(InviteResult);
	});

	InviteAsync(User, NativeCallback);
}

void UPubnubChatChannel::InviteAsync(UPubnubChatUser* User, FOnPubnubChatInviteResponseNative OnInviteResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnInviteResponseNative, FPubnubChatInviteResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, User, OnInviteResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatInviteResult InviteResult = WeakThis.Get()->Invite(User);
		UPubnubUtilities::CallPubnubDelegate(OnInviteResponseNative, InviteResult);
	});
}

FPubnubChatInviteMultipleResult UPubnubChatChannel::InviteMultiple(TArray<UPubnubChatUser*> Users)
{
	FPubnubChatInviteMultipleResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	TArray<UPubnubChatUser*> ValidUsers = UPubnubChatInternalUtilities::RemoveInvalidObjects(Users);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, !ValidUsers.IsEmpty(), TEXT("At least one valid user has to be provided"));

	FString Filter = UPubnubChatInternalUtilities::GetFilterForMultipleUsersID(ValidUsers);
	TArray<FPubnubChannelMemberInputData> MembersInput;
	//Create Membership with "pending" status
	FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData{.Status = Pubnub_Chat_Invited_User_Membership_status};

	for(auto& User : ValidUsers)
	{
		MembersInput.Add(MembershipData.ToPubnubChannelMemberInputData(User->GetUserID()));
	}

	//SetChannelMembers by PubnubClient
	FPubnubChannelMembersResult SetMembersResult = PubnubClient->SetChannelMembers(ChannelID, MembersInput, FPubnubMemberInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembersResult.Result, "SetChannelMembers");

	//For every invited user create a Membership and send invitation events
	for(auto& User : ValidUsers)
	{
		UPubnubChatMembership* Membership = Chat->CreateMembershipObject(User, this, MembershipData);
		FinalResult.Memberships.Add(Membership);

		//Emit Invite event
		FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Invite, User->GetUserID(), UPubnubChatInternalUtilities::GetInviteEventPayload(ChannelID, GetChannelData().Type));
		FinalResult.Result.Merge(EmitEventResult);

		//Set Last Read Timetoken on created Membership
		FPubnubChatOperationResult SetLRMTResult = Membership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
		FinalResult.Result.Merge(SetLRMTResult);
	}
	
	return FinalResult;
}

void UPubnubChatChannel::InviteMultipleAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatInviteMultipleResponse OnInviteMultipleResponse)
{
	FOnPubnubChatInviteMultipleResponseNative NativeCallback;
	NativeCallback.BindLambda([OnInviteMultipleResponse](const FPubnubChatInviteMultipleResult& InviteMultipleResult)
	{
		OnInviteMultipleResponse.ExecuteIfBound(InviteMultipleResult);
	});

	InviteMultipleAsync(Users, NativeCallback);
}

void UPubnubChatChannel::InviteMultipleAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatInviteMultipleResponseNative OnInviteMultipleResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnInviteMultipleResponseNative, FPubnubChatInviteMultipleResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Users, OnInviteMultipleResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatInviteMultipleResult InviteMultipleResult = WeakThis.Get()->InviteMultiple(Users);
		UPubnubUtilities::CallPubnubDelegate(OnInviteMultipleResponseNative, InviteMultipleResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::PinMessage(UPubnubChatMessage* Message)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	
	//Check if message is from this Channel
	bool IsMessageFromThisChannel = false;
	if (Message->GetMessageData().ChannelID == ChannelID)
	{ IsMessageFromThisChannel = true; }
	//Or from any Thread Channel that has this Channel as Parent
	else
	{
		UPubnubChatThreadMessage* ThreadMessage = Cast<UPubnubChatThreadMessage>(Message);
		if (ThreadMessage && ThreadMessage->GetParentChannelID() == ChannelID)
		{ IsMessageFromThisChannel = true; }
	}
	
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(IsMessageFromThisChannel, TEXT("Can't pin Message from another Channel"));
	
	//Add pinned message to ChannelData
	FPubnubChatChannelData ChannelData = GetChannelData();
	UPubnubChatInternalUtilities::AddPinnedMessageToChannelData(ChannelData, Message);
	
	//Update Channel data
	FPubnubChatOperationResult UpdateResult = Update(FPubnubChatUpdateChannelInputData::FromChatChannelData(ChannelData));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);
	
	return FinalResult;
}

void UPubnubChatChannel::PinMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	PinMessageAsync(Message, NativeCallback);
}

void UPubnubChatChannel::PinMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Message, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult PinResult = WeakThis.Get()->PinMessage(Message);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, PinResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::UnpinMessage()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Remove pinned message from ChannelData
	FPubnubChatChannelData ChannelData = GetChannelData();
	if (UPubnubChatInternalUtilities::RemovePinnedMessageFromChannelData(ChannelData))
	{
		//Update Channel data - only if there was pinned message removed
		FPubnubChatUpdateChannelInputData UpdateChannelInputData = FPubnubChatUpdateChannelInputData::FromChatChannelData(ChannelData);
		UpdateChannelInputData.ForceSetCustom = true; //After removing pinned message Custom field might be empty, so we force to set it
		FPubnubChatOperationResult UpdateResult = Update(UpdateChannelInputData);
		PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);
	}
	
	return FinalResult;
}

void UPubnubChatChannel::UnpinMessageAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	UnpinMessageAsync(NativeCallback);
}

void UPubnubChatChannel::UnpinMessageAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult UnpinResult = WeakThis.Get()->UnpinMessage();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, UnpinResult);
	});
}

FPubnubChatMessageResult UPubnubChatChannel::GetPinnedMessage()
{
	FPubnubChatMessageResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	FPubnubChatChannelData ChannelData = GetChannelData();
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	FString PinnedMessageTimetoken;
	FString PinnedMessageChannelID;
	JsonObject->TryGetStringField(UPubnubChatInternalUtilities::GetPinnedMessageTimetokenPropertyKey(), PinnedMessageTimetoken);
	JsonObject->TryGetStringField(UPubnubChatInternalUtilities::GetPinnedMessageChannelIDPropertyKey(), PinnedMessageChannelID);
	
	//If there is no pinned message, just return
	if (PinnedMessageTimetoken.IsEmpty() || PinnedMessageChannelID.IsEmpty())
	{ return FinalResult; }
	
	//If pinned message is from this channel, just return GetMessage result
	if (PinnedMessageChannelID == ChannelID)
	{
		return GetMessage(PinnedMessageTimetoken);
	}
	
	//If we didn't get pinned message above, it has to be from a Thread
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, UPubnubChatInternalUtilities::IsChannelAThread(PinnedMessageChannelID), TEXT("Pinned Message is from incorrect channel."));
	
	//Get Thread Channel
	FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(PinnedMessageChannelID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, GetChannelResult.Channel, TEXT("Pinned Message is from non-existing Channel."));
	
	//Get that message from Thread Channel
	FPubnubChatMessageResult GetMessageResult = GetChannelResult.Channel->GetMessage(PinnedMessageTimetoken);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMessageResult.Result);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, GetMessageResult.Message, TEXT("Pinned message doesn't exist."));

	//Create ThreadMessage object from regular message, so it has full data if later cast to the ThreadMessage
	UPubnubChatThreadMessage* ThreadMessage = Chat->CreateThreadMessageObject(GetMessageResult.Message->GetMessageTimetoken(), GetMessageResult.Message->GetMessageData(), ChannelID);
	FinalResult.Message = ThreadMessage;
	return FinalResult;
}

void UPubnubChatChannel::GetPinnedMessageAsync(FOnPubnubChatMessageResponse OnMessageResponse)
{
	FOnPubnubChatMessageResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMessageResponse](const FPubnubChatMessageResult& MessageResult)
	{
		OnMessageResponse.ExecuteIfBound(MessageResult);
	});

	GetPinnedMessageAsync(NativeCallback);
}

void UPubnubChatChannel::GetPinnedMessageAsync(FOnPubnubChatMessageResponseNative OnMessageResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMessageResponseNative, FPubnubChatMessageResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnMessageResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMessageResult MessageResult = WeakThis.Get()->GetPinnedMessage();
		UPubnubUtilities::CallPubnubDelegate(OnMessageResponseNative, MessageResult);
	});
}

FPubnubChatWhoIsPresentResult UPubnubChatChannel::WhoIsPresent(int Limit, int Offset)
{
	FPubnubChatWhoIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->WhoIsPresent(ChannelID, Limit, Offset);
}

void UPubnubChatChannel::WhoIsPresentAsync(FOnPubnubChatWhoIsPresentResponse OnWhoIsPresentResponse, int Limit, int Offset)
{
	FOnPubnubChatWhoIsPresentResponseNative NativeCallback;
	NativeCallback.BindLambda([OnWhoIsPresentResponse](const FPubnubChatWhoIsPresentResult& WhoIsPresentResult)
	{
		OnWhoIsPresentResponse.ExecuteIfBound(WhoIsPresentResult);
	});

	WhoIsPresentAsync(NativeCallback, Limit, Offset);
}

void UPubnubChatChannel::WhoIsPresentAsync(FOnPubnubChatWhoIsPresentResponseNative OnWhoIsPresentResponseNative, int Limit, int Offset)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnWhoIsPresentResponseNative, FPubnubChatWhoIsPresentResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Offset, OnWhoIsPresentResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatWhoIsPresentResult WhoIsPresentResult = WeakThis.Get()->WhoIsPresent(Limit, Offset);
		UPubnubUtilities::CallPubnubDelegate(OnWhoIsPresentResponseNative, WhoIsPresentResult);
	});
}

FPubnubChatIsPresentResult UPubnubChatChannel::IsPresent(const FString UserID)
{
	FPubnubChatIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->IsPresent(UserID, ChannelID);
}

void UPubnubChatChannel::IsPresentAsync(const FString UserID, FOnPubnubChatIsPresentResponse OnIsPresentResponse)
{
	FOnPubnubChatIsPresentResponseNative NativeCallback;
	NativeCallback.BindLambda([OnIsPresentResponse](const FPubnubChatIsPresentResult& IsPresentResult)
	{
		OnIsPresentResponse.ExecuteIfBound(IsPresentResult);
	});

	IsPresentAsync(UserID, NativeCallback);
}

void UPubnubChatChannel::IsPresentAsync(const FString UserID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnIsPresentResponseNative, FPubnubChatIsPresentResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UserID, OnIsPresentResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatIsPresentResult IsPresentResult = WeakThis.Get()->IsPresent(UserID);
		UPubnubUtilities::CallPubnubDelegate(OnIsPresentResponseNative, IsPresentResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::Delete()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatOperationResult DeleteChannelResult = Chat->DeleteChannel(ChannelID);
	return DeleteChannelResult;
}

void UPubnubChatChannel::DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	DeleteAsync(NativeCallback);
}

void UPubnubChatChannel::DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);

	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatOperationResult DeleteResult = WeakThis.Get()->Delete();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, DeleteResult);
	});
}

FPubnubChatMembershipsResult UPubnubChatChannel::GetMembers(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatMembershipsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//GetChannelMembers using PubnubClient
	FPubnubChannelMembersResult GetMembersResult = PubnubClient->GetChannelMembers(ChannelID, FPubnubMemberInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result, "GetChannelMembers");
	
	//Create corresponding Chat objects for all returned memberships
	for (auto& MembershipData : GetMembersResult.MembersData)
	{
		UPubnubChatUser* User =  Chat->CreateUserObject(MembershipData.User.UserID, MembershipData.User);
		UPubnubChatMembership* Membership = Chat->CreateMembershipObject(User, this, MembershipData);
		FinalResult.Memberships.Add(Membership);
	}
	
	FinalResult.Page = GetMembersResult.Page;
	FinalResult.Total = GetMembersResult.TotalCount;
	return FinalResult;
}

void UPubnubChatChannel::GetMembersAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FOnPubnubChatMembershipsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMembershipsResponse](const FPubnubChatMembershipsResult& MembershipsResult)
	{
		OnMembershipsResponse.ExecuteIfBound(MembershipsResult);
	});

	GetMembersAsync(NativeCallback, Limit, Filter, Sort, Page);
}

void UPubnubChatChannel::GetMembersAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMembershipsResponseNative, FPubnubChatMembershipsResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Filter, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnMembershipsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMembershipsResult MembersResult = WeakThis.Get()->GetMembers(Limit, Filter, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnMembershipsResponseNative, MembersResult);
	});
}

FPubnubChatMembershipResult UPubnubChatChannel::GetMember(const FString UserID)
{
	FPubnubChatMembershipResult FinalResult;
	FinalResult.Membership = nullptr;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);

	FPubnubChatMembershipsResult GetMembersResult = GetMembers(1, UPubnubChatInternalUtilities::GetFilterForUserID(UserID));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result);

	if (!GetMembersResult.Memberships.IsEmpty())
	{
		FinalResult.Membership = GetMembersResult.Memberships[0];
	}

	return FinalResult;
}

void UPubnubChatChannel::GetMemberAsync(const FString UserID, FOnPubnubChatMembershipResponse OnMembershipResponse)
{
	FOnPubnubChatMembershipResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMembershipResponse](const FPubnubChatMembershipResult& MembershipResult)
	{
		OnMembershipResponse.ExecuteIfBound(MembershipResult);
	});

	GetMemberAsync(UserID, NativeCallback);
}

void UPubnubChatChannel::GetMemberAsync(const FString UserID, FOnPubnubChatMembershipResponseNative OnMembershipResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMembershipResponseNative, FPubnubChatMembershipResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UserID, OnMembershipResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMembershipResult MembershipResult = WeakThis.Get()->GetMember(UserID);
		UPubnubUtilities::CallPubnubDelegate(OnMembershipResponseNative, MembershipResult);
	});
}

FPubnubChatHasMemberResult UPubnubChatChannel::HasMember(const FString UserID)
{
	FPubnubChatHasMemberResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubChatMembershipResult GetMemberResult = GetMember(UserID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMemberResult.Result);

	FinalResult.HasMember = IsValid(GetMemberResult.Membership);
	return FinalResult;
}

void UPubnubChatChannel::HasMemberAsync(const FString UserID, FOnPubnubChatHasMemberResponse OnHasMemberResponse)
{
	FOnPubnubChatHasMemberResponseNative NativeCallback;
	NativeCallback.BindLambda([OnHasMemberResponse](const FPubnubChatHasMemberResult& HasMemberResult)
	{
		OnHasMemberResponse.ExecuteIfBound(HasMemberResult);
	});

	HasMemberAsync(UserID, NativeCallback);
}

void UPubnubChatChannel::HasMemberAsync(const FString UserID, FOnPubnubChatHasMemberResponseNative OnHasMemberResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnHasMemberResponseNative, FPubnubChatHasMemberResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UserID, OnHasMemberResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatHasMemberResult HasMemberResult = WeakThis.Get()->HasMember(UserID);
		UPubnubUtilities::CallPubnubDelegate(OnHasMemberResponseNative, HasMemberResult);
	});
}


FPubnubChatMembershipsResult UPubnubChatChannel::GetInvitees(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatMembershipsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	//Create final Filter so status == "pending" and eventually additional Filter provided by user
	FString FinalFilter = FString::Printf(TEXT(R"(status == "%s")"), *Pubnub_Chat_Invited_User_Membership_status);
	if (!Filter.IsEmpty())
	{
		FinalFilter += " && ";
		FinalFilter += Filter;
	}
	
	return GetMembers(Limit, FinalFilter, Sort, Page);
}

void UPubnubChatChannel::GetInviteesAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FOnPubnubChatMembershipsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMembershipsResponse](const FPubnubChatMembershipsResult& MembershipsResult)
	{
		OnMembershipsResponse.ExecuteIfBound(MembershipsResult);
	});

	GetInviteesAsync(NativeCallback, Limit, Filter, Sort, Page);
}

void UPubnubChatChannel::GetInviteesAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMembershipsResponseNative, FPubnubChatMembershipsResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Filter, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnMembershipsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMembershipsResult InviteesResult = WeakThis.Get()->GetInvitees(Limit, Filter, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnMembershipsResponseNative, InviteesResult);
	});
}

FPubnubChatFetchReadReceiptsResult UPubnubChatChannel::FetchReadReceipts(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatFetchReadReceiptsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	FPubnubChatMembershipsResult GetMembersResult = GetMembers(Limit, Filter, Sort, Page);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result);

	// Map each membership to a read receipt: UserID and LastReadMessageTimetoken.
	for (UPubnubChatMembership* Membership : GetMembersResult.Memberships)
	{
		if (!IsValid(Membership))
		{
			continue;
		}

		FPubnubChatReadReceipt ReadReceipt;
		ReadReceipt.UserID = Membership->GetUserID();
		ReadReceipt.LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
		FinalResult.ReadReceipts.Add(ReadReceipt);
	}

	// Preserve pagination and total count from the members result.
	FinalResult.Page = GetMembersResult.Page;
	FinalResult.Total = GetMembersResult.Total;
	return FinalResult;
}

void UPubnubChatChannel::FetchReadReceiptsAsync(FOnPubnubChatFetchReadReceiptsResponse OnFetchReadReceiptsResponse, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FOnPubnubChatFetchReadReceiptsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnFetchReadReceiptsResponse](const FPubnubChatFetchReadReceiptsResult& FetchReadReceiptsResult)
	{
		OnFetchReadReceiptsResponse.ExecuteIfBound(FetchReadReceiptsResult);
	});

	FetchReadReceiptsAsync(NativeCallback, Limit, Filter, Sort, Page);
}

void UPubnubChatChannel::FetchReadReceiptsAsync(FOnPubnubChatFetchReadReceiptsResponseNative OnFetchReadReceiptsResponseNative, const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnFetchReadReceiptsResponseNative, FPubnubChatFetchReadReceiptsResult());

	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Filter, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnFetchReadReceiptsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }

		FPubnubChatFetchReadReceiptsResult FetchReadReceiptsResult = WeakThis.Get()->FetchReadReceipts(Limit, Filter, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnFetchReadReceiptsResponseNative, FetchReadReceiptsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::SetRestrictions(const FString UserID, bool Ban, bool Mute, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(UserID);
	
	return Chat->SetRestrictions(FPubnubChatRestriction(UserID, ChannelID, Ban, Mute, Reason));
}

void UPubnubChatChannel::SetRestrictionsAsync(const FString UserID, bool Ban, bool Mute, FOnPubnubChatOperationResponse OnOperationResponse, FString Reason)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SetRestrictionsAsync(UserID, Ban, Mute, NativeCallback, Reason);
}

void UPubnubChatChannel::SetRestrictionsAsync(const FString UserID, bool Ban, bool Mute, FOnPubnubChatOperationResponseNative OnOperationResponseNative, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UserID, Ban, Mute, Reason, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult SetRestrictionsResult = WeakThis.Get()->SetRestrictions(UserID, Ban, Mute, Reason);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, SetRestrictionsResult);
	});
}

FPubnubChatGetRestrictionResult UPubnubChatChannel::GetUserRestrictions(UPubnubChatUser* User)
{
	FPubnubChatGetRestrictionResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);
	
	FPubnubChatGetRestrictionsResult GetRestrictionsResult = GetRestrictions(1, UPubnubChatInternalUtilities::GetFilterForUserID(User->GetUserID()));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetRestrictionsResult.Result);

	//If there was any restriction returned, just add it. If not, there is no restriction
	if (!GetRestrictionsResult.Restrictions.IsEmpty())
	{
		FinalResult.Restriction = GetRestrictionsResult.Restrictions[0];
	}
	else
	{
		FinalResult.Restriction = FPubnubChatRestriction({.UserID = User->GetUserID(), .ChannelID = ChannelID});
	}
	
	return FinalResult;
}

void UPubnubChatChannel::GetUserRestrictionsAsync(UPubnubChatUser* User, FOnPubnubChatGetRestrictionResponse OnRestrictionResponse)
{
	FOnPubnubChatGetRestrictionResponseNative NativeCallback;
	NativeCallback.BindLambda([OnRestrictionResponse](const FPubnubChatGetRestrictionResult& RestrictionResult)
	{
		OnRestrictionResponse.ExecuteIfBound(RestrictionResult);
	});

	GetUserRestrictionsAsync(User, NativeCallback);
}

void UPubnubChatChannel::GetUserRestrictionsAsync(UPubnubChatUser* User, FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnRestrictionResponseNative, FPubnubChatGetRestrictionResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, User, OnRestrictionResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetRestrictionResult RestrictionResult = WeakThis.Get()->GetUserRestrictions(User);
		UPubnubUtilities::CallPubnubDelegate(OnRestrictionResponseNative, RestrictionResult);
	});
}

FPubnubChatGetRestrictionsResult UPubnubChatChannel::GetUsersRestrictions(const int Limit, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return GetRestrictions(Limit, "", Sort, Page);
}

void UPubnubChatChannel::GetUsersRestrictionsAsync(FOnPubnubChatGetRestrictionsResponse OnRestrictionsResponse, const int Limit, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FOnPubnubChatGetRestrictionsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnRestrictionsResponse](const FPubnubChatGetRestrictionsResult& RestrictionsResult)
	{
		OnRestrictionsResponse.ExecuteIfBound(RestrictionsResult);
	});

	GetUsersRestrictionsAsync(NativeCallback, Limit, Sort, Page);
}

void UPubnubChatChannel::GetUsersRestrictionsAsync(FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponseNative, const int Limit, FPubnubMemberSort Sort, FPubnubPage Page)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnRestrictionsResponseNative, FPubnubChatGetRestrictionsResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Limit, Sort = MoveTemp(Sort), Page = MoveTemp(Page), OnRestrictionsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetRestrictionsResult RestrictionsResult = WeakThis.Get()->GetUsersRestrictions(Limit, Sort, Page);
		UPubnubUtilities::CallPubnubDelegate(OnRestrictionsResponseNative, RestrictionsResult);
	});
}

FPubnubChatGetHistoryResult UPubnubChatChannel::GetHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count)
{
	FPubnubChatGetHistoryResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, StartTimetoken);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, EndTimetoken);
	
	FPubnubFetchHistorySettings FetchHistorySettings;
	FetchHistorySettings.MaxPerChannel = Count;
	FetchHistorySettings.Start = StartTimetoken;
	FetchHistorySettings.End = EndTimetoken;
	FetchHistorySettings.IncludeUserID = true;
	FetchHistorySettings.IncludeMessageActions = true;
	FetchHistorySettings.IncludeMeta = true;
	FPubnubFetchHistoryResult FetchHistoryResult = PubnubClient->FetchHistory(ChannelID, FetchHistorySettings);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, FetchHistoryResult.Result, "FetchHistory");
	
	// Check if this Channel is a ThreadChannel, if so parse ID to get ParentChannelID
	bool IsThreadChannel = UPubnubChatInternalUtilities::IsChannelAThread(ChannelID);
	FString ParentChannelID = "";
	if (IsThreadChannel)
	{
		ParentChannelID = UPubnubChatInternalUtilities::GetParentChannelIDFromThreadID(ChannelID);
	}
	
	for (auto& MessageData : FetchHistoryResult.Messages)
	{
		// Create Messages, if this channel is a thread, these have to be ThreadMessages
		UPubnubChatMessage* Message = IsThreadChannel ? Chat->CreateThreadMessageObject(MessageData.Timetoken, MessageData, ParentChannelID)
		: Chat->CreateMessageObject(MessageData.Timetoken, MessageData);

		FinalResult.Messages.Add(Message);
	}
	
	//If we got the exact amount of messages as specified count, probably there are more events in a given range
	FinalResult.IsMore = FetchHistoryResult.Messages.Num() == Count;
	
	return FinalResult;
}

void UPubnubChatChannel::GetHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetHistoryResponse OnHistoryResponse, const int Count)
{
	FOnPubnubChatGetHistoryResponseNative NativeCallback;
	NativeCallback.BindLambda([OnHistoryResponse](const FPubnubChatGetHistoryResult& HistoryResult)
	{
		OnHistoryResponse.ExecuteIfBound(HistoryResult);
	});

	GetHistoryAsync(StartTimetoken, EndTimetoken, NativeCallback, Count);
}

void UPubnubChatChannel::GetHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetHistoryResponseNative OnHistoryResponseNative, const int Count)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnHistoryResponseNative, FPubnubChatGetHistoryResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, StartTimetoken, EndTimetoken, Count, OnHistoryResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetHistoryResult HistoryResult = WeakThis.Get()->GetHistory(StartTimetoken, EndTimetoken, Count);
		UPubnubUtilities::CallPubnubDelegate(OnHistoryResponseNative, HistoryResult);
	});
}

FPubnubChatMessageResult UPubnubChatChannel::GetMessage(const FString Timetoken)
{
	FPubnubChatMessageResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, Timetoken);
	
	FString StartTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(Timetoken, 1);
	FPubnubChatGetHistoryResult GetHistoryResult = GetHistory(StartTimetoken, Timetoken, 1);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetHistoryResult.Result);
	
	if (!GetHistoryResult.Messages.IsEmpty())
	{
		FinalResult.Message = GetHistoryResult.Messages[0];
	}
	
	return FinalResult;
}

void UPubnubChatChannel::GetMessageAsync(const FString Timetoken, FOnPubnubChatMessageResponse OnMessageResponse)
{
	FOnPubnubChatMessageResponseNative NativeCallback;
	NativeCallback.BindLambda([OnMessageResponse](const FPubnubChatMessageResult& MessageResult)
	{
		OnMessageResponse.ExecuteIfBound(MessageResult);
	});

	GetMessageAsync(Timetoken, NativeCallback);
}

void UPubnubChatChannel::GetMessageAsync(const FString Timetoken, FOnPubnubChatMessageResponseNative OnMessageResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnMessageResponseNative, FPubnubChatMessageResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Timetoken, OnMessageResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatMessageResult MessageResult = WeakThis.Get()->GetMessage(Timetoken);
		UPubnubUtilities::CallPubnubDelegate(OnMessageResponseNative, MessageResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::ForwardMessage(UPubnubChatMessage* Message)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	
	return Chat->ForwardMessage(Message, this);
}

void UPubnubChatChannel::ForwardMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	ForwardMessageAsync(Message, NativeCallback);
}

void UPubnubChatChannel::ForwardMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Message, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult ForwardResult = WeakThis.Get()->ForwardMessage(Message);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, ForwardResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::EmitCustomEvent(FString Payload, FString Type, bool StoreInHistory)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Payload);
	
	FPubnubPublishSettings PublishSettings;
	PublishSettings.StoreInHistory = StoreInHistory;
	PublishSettings.CustomMessageType = Type;
	
	//Add event type to the payload
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(Payload, JsonObject);
	JsonObject->SetStringField(ANSI_TO_TCHAR("type"), ANSI_TO_TCHAR("custom"));

	FPubnubPublishMessageResult PublishResult = PubnubClient->PublishMessage(ChannelID, UPubnubJsonUtilities::JsonObjectToString(JsonObject), PublishSettings);
	FPubnubChatOperationResult FinalResult;
	FinalResult.AddStep("PublishMessage", PublishResult.Result);
	return FinalResult;
}

void UPubnubChatChannel::EmitCustomEventAsync(FString Payload, FOnPubnubChatOperationResponse OnOperationResponse, FString Type, bool StoreInHistory)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	EmitCustomEventAsync(Payload, NativeCallback, Type, StoreInHistory);
}

void UPubnubChatChannel::EmitCustomEventAsync(FString Payload, FOnPubnubChatOperationResponseNative OnOperationResponseNative, FString Type, bool StoreInHistory)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Payload = MoveTemp(Payload), OnOperationResponseNative, Type = MoveTemp(Type), StoreInHistory]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult EmitResult = WeakThis.Get()->EmitCustomEvent(Payload, Type, StoreInHistory);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, EmitResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::EmitUserMention(const FString UserID, const FString Timetoken, const FString Text)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(UserID);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Timetoken);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Text);
	
	FString EventPayload = CreateMentionEventPayload(Timetoken, Text);
	return Chat->EmitChatEvent(EPubnubChatEventType::PCET_Mention, UserID, EventPayload);
}

void UPubnubChatChannel::EmitUserMentionAsync(const FString UserID, const FString Timetoken, const FString Text, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	EmitUserMentionAsync(UserID, Timetoken, Text, NativeCallback);
}

void UPubnubChatChannel::EmitUserMentionAsync(const FString UserID, const FString Timetoken, const FString Text, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UserID, Timetoken, Text, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult EmitResult = WeakThis.Get()->EmitUserMention(UserID, Timetoken, Text);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, EmitResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamUpdates()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already streaming
	if (IsStreamingUpdates)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add lister to subscription with provided callback
	UpdatesSubscription->OnPubnubObjectEventNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();

		if(!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingUpdates)
		{return;}
		
		//If this is not ChannelUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageChannelUpdate(MessageData.Message))
		{
			//Check if channel was deleted or updated
			if (UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(MessageData.Message))
			{
				//Remove this channel from repository
				ThisChannel->Chat->ObjectsRepository->RemoveChannelData(ThisChannel->ChannelID);
				
				//Call OnDeleted delegates
				ThisChannel->OnDeleted.Broadcast();
				ThisChannel->OnDeletedNative.Broadcast();
			}
			else
			{
				//Adjust this channel data based on the update message
				FPubnubChatChannelData ChatChannelData = ThisChannel->GetChannelData();
				FPubnubChannelUpdateData ChannelUpdateData = UPubnubJsonUtilities::GetChannelUpdateDataFromMessageContent(MessageData.Message);
				UPubnubChatInternalUtilities::UpdateChatChannelFromPubnubChannelUpdateData(ChannelUpdateData, ChatChannelData);
							
				//Update repository with new channel data
				ThisChannel->Chat->ObjectsRepository->UpdateChannelData(ThisChannel->ChannelID, ChatChannelData);
							
				//Call OnUpdated delegates with new channel data
				ThisChannel->OnUpdated.Broadcast(ThisChannel->ChannelID, ChatChannelData);
				ThisChannel->OnUpdatedNative.Broadcast(ThisChannel->ChannelID, ChatChannelData);
			}
		}
	});
	
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	FinalResult.AddStep("Subscribe", SubscribeResult);
	if (SubscribeResult.Error)
	{
		UpdatesSubscription->OnPubnubObjectEventNative.Clear();
		return FinalResult;
	}
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

void UPubnubChatChannel::StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamUpdatesAsync(NativeCallback);
}

void UPubnubChatChannel::StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamUpdatesResult = WeakThis.Get()->StreamUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamUpdatesResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamUpdatesOn(const TArray<UPubnubChatChannel*>& Channels)
{
	FPubnubChatOperationResult FinalResult;
	for (auto& Channel : Channels)
	{
		FPubnubChatOperationResult StreamUpdatesResult = Channel->StreamUpdates();
		FinalResult.Merge(StreamUpdatesResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingUpdates()
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

void UPubnubChatChannel::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingUpdatesAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamPresence()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Skip if it's already streaming
	if (IsStreamingPresence)
	{ return FinalResult; }
	
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(PresenceSubscription, TEXT("Presence subscription is invalid."));
	
	FPubnubChatWhoIsPresentResult WhoIsPresentResult = WhoIsPresent();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, WhoIsPresentResult.Result);
	
	//Save currently present users
	StreamPresenceUserIDs = WhoIsPresentResult.Users;
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	PresenceSubscription->OnPubnubPresenceEventNative.AddLambda([ThisWeak](const FPubnubMessageData& Message)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();

		if(!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingPresence)
		{return;}
		
		UPubnubChatInternalUtilities::UpdateUserIDByPresenceEvent(ThisChannel->StreamPresenceUserIDs, Message.Message);
		ThisChannel->OnPresenceChanged.Broadcast(ThisChannel->StreamPresenceUserIDs);
		ThisChannel->OnPresenceChangedNative.Broadcast(ThisChannel->StreamPresenceUserIDs);
	});

	FPubnubOperationResult SubscribeResult = PresenceSubscription->Subscribe();
	FinalResult.AddStep("Subscribe", SubscribeResult);
	if (SubscribeResult.Error)
	{
		PresenceSubscription->OnPubnubPresenceEventNative.Clear();
		StreamPresenceUserIDs.Empty();
		return FinalResult;
	}

	IsStreamingPresence = true;
	return FinalResult;
}

void UPubnubChatChannel::StreamPresenceAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamPresenceAsync(NativeCallback);
}

void UPubnubChatChannel::StreamPresenceAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamPresenceResult = WeakThis.Get()->StreamPresence();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamPresenceResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingPresence()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	if (!PresenceSubscription)
	{
		IsStreamingPresence = false;
		return FinalResult;
	}

	//Remove presence related delegates
	PresenceSubscription->OnPubnubPresenceEventNative.Clear();
	
	//Skip if it's not streaming presence
	if (!IsStreamingPresence)
	{ return FinalResult; }
	
	// Removed cached PresentUsers 
	StreamPresenceUserIDs.Empty();

	//Unsubscribe and return result
	FPubnubOperationResult UnsubscribeResult = PresenceSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsStreamingPresence = false;
	return FinalResult;
}

void UPubnubChatChannel::StopStreamingPresenceAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingPresenceAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingPresenceAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingPresence();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StartTyping()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((GetChannelData().Type != "public"), TEXT("Typing is not supported on public channels"));

	FDateTime Now = FDateTime::UtcNow();
	
	//Check if StartTyping call is within minimal threshold. If yes, skip the call not to spam events
	if (LastTypingEventTime != FDateTime::MinValue())
	{
		FDateTime ThresholdTime = Now - FTimespan::FromMilliseconds(Chat->ChatConfig.TypingTimeout - Pubnub_Chat_Typing_Timeout_Margin);
		if (LastTypingEventTime > ThresholdTime)
		{ return FinalResult; }
	}
	
	LastTypingEventTime = Now;
	
	//Emit Typing Event
	FString TypingEventPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(true);
	FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, ChannelID, TypingEventPayload);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, EmitEventResult);
	
	return FinalResult;
}

void UPubnubChatChannel::StartTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StartTypingAsync(NativeCallback);
}

void UPubnubChatChannel::StartTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StartTypingResult = WeakThis.Get()->StartTyping();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StartTypingResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopTyping()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((GetChannelData().Type != "public"), TEXT("Typing is not supported on public channels"));

	//If we never StartedTyping then we can't stop
	if (LastTypingEventTime == FDateTime())
	{ return FinalResult; }
	
	FDateTime Now = FDateTime::UtcNow();
	FDateTime ThresholdTime = Now - FTimespan::FromMilliseconds(Chat->ChatConfig.TypingTimeout);
	
	//If LastEvent was earlier than the Threshold, don't send event
	if (LastTypingEventTime < ThresholdTime)
	{ return FinalResult; }
	
	//Clear LastTypingEventTime, so it can be set again by  StartTyping
	LastTypingEventTime = FDateTime();
	
	//Emit Typing Event
	FString TypingEventPayload = UPubnubChatInternalUtilities::GetTypingEventPayload(false);
	FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Typing, ChannelID, TypingEventPayload);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, EmitEventResult);
	
	return FinalResult;
}

void UPubnubChatChannel::StopTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopTypingAsync(NativeCallback);
}

void UPubnubChatChannel::StopTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopTypingResult = WeakThis.Get()->StopTyping();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopTypingResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamTyping()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((GetChannelData().Type != "public"), TEXT("Typing is not supported on public channels"));

	//Skip if it's already streaming
	if (IsStreamingTyping)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();
		
		if (!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingTyping)
		{ return; }
		
		FString UserID = Event.UserID;
		bool IsTyping = UPubnubChatInternalUtilities::GetIsTypingFromEventPayload(Event.Payload);
		FDateTime Now = FDateTime::UtcNow();
		
		TArray<FString> TypingUsers;
		
		//Lock access to typing indicators
		{
			FScopeLock Lock(&ThisChannel->TypingIndicatorsCriticalSection);
			
			//Remove expired typing indicators immediately
			UPubnubChatInternalUtilities::RemoveExpiredTypingIndicators(ThisChannel->TypingIndicators, ThisChannel->Chat->ChatConfig.TypingTimeout, Now);
			
			if (!IsTyping)
			{
				//Stop typing: invalidate the timer and remove UserID from typing indicators
				if (FTypingIndicatorData* IndicatorData = ThisChannel->TypingIndicators.Find(UserID))
				{
					IndicatorData->TimerHandle.Invalidate();
					ThisChannel->TypingIndicators.Remove(UserID);
				}
			}
			else
			{
				//Start typing: if this user has typing indicator, invalidate the old timer
				if (FTypingIndicatorData* IndicatorData = ThisChannel->TypingIndicators.Find(UserID))
				{
					IndicatorData->TimerHandle.Invalidate();
					ThisChannel->TypingIndicators.Remove(UserID);
				}
				
				//Store timestamp - timer will be set up after lock is released
				//Create new timer with callback that broadcasts delegates
				FTimerDelegate TimerDelegate;
				TimerDelegate.BindLambda([ThisWeak, UserID]()
				{
					if(!ThisWeak.IsValid())
					{return;}
			
					UPubnubChatChannel* ThisChannel = ThisWeak.Get();
			
					if (!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingTyping)
					{ return; }
					
					TArray<FString> TypingUsersList;
					
					//Lock access to typing indicators (first lock was already released when timer fires)
					{
						FScopeLock Lock(&ThisChannel->TypingIndicatorsCriticalSection);
						
						// Remove expired user
						ThisChannel->TypingIndicators.Remove(UserID);
						
						//Get current typing users list
						ThisChannel->TypingIndicators.GenerateKeyArray(TypingUsersList);
					} //Lock released here
					
					//Broadcast updated typing users list (Fix #2: Broadcast delegates when timer expires)
					ThisChannel->OnTypingChanged.Broadcast(TypingUsersList);
					ThisChannel->OnTypingChangedNative.Broadcast(TypingUsersList);
				});
				FTimerHandle TimerHandle;
				float TimerDelay = ThisChannel->Chat->ChatConfig.TypingTimeout + 10.0f; // Add 10 MS to be sure that it will really expire
				ThisChannel->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, TimerDelay, false);
				
				//Store timer handle and timestamp
				ThisChannel->TypingIndicators.Add(UserID, FTypingIndicatorData(TimerHandle, Now));
			}
			
			//Get current typing users list
			ThisChannel->TypingIndicators.GenerateKeyArray(TypingUsers);
		} //Lock released here
		
		//Call delegates with typing users (outside of lock)
		ThisChannel->OnTypingChanged.Broadcast(TypingUsers);
		ThisChannel->OnTypingChangedNative.Broadcast(TypingUsers);
	});
	
	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(ChannelID, EPubnubChatEventType::PCET_Typing, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	
	TypingCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingTyping = true;
	
	return FinalResult;
}

void UPubnubChatChannel::StreamTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamTypingAsync(NativeCallback);
}

void UPubnubChatChannel::StreamTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamTypingResult = WeakThis.Get()->StreamTyping();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamTypingResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingTyping()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	if (!IsStreamingTyping)
	{ return FinalResult; }
	
	if (!TypingCallbackStop)
	{ return FinalResult; }
	
	FPubnubChatOperationResult StopResult = TypingCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);
	
	TypingCallbackStop = nullptr;
	IsStreamingTyping = false;

	return FinalResult;
}

void UPubnubChatChannel::StopStreamingTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingTypingAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopTypingResult = WeakThis.Get()->StopStreamingTyping();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopTypingResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamReadReceipts()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	//Skip if it's already streaming
	if (IsStreamingReadReceipts)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);

	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();
		
		if (!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingReadReceipts)
		{ return; }

		FPubnubChatReadReceipt ReadReceipt;
		ReadReceipt.UserID = Event.UserID;

		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		if (!UPubnubJsonUtilities::StringToJsonObject(Event.Payload, JsonObject))
		{
			return;
		}

		if (!JsonObject->TryGetStringField(ANSI_TO_TCHAR("messageTimetoken"), ReadReceipt.LastReadTimetoken))
		{
			return;
		}

		ThisChannel->OnReadReceiptReceived.Broadcast(ReadReceipt);
		ThisChannel->OnReadReceiptReceivedNative.Broadcast(ReadReceipt);
	});

	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(ChannelID, EPubnubChatEventType::PCET_Receipt, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((ListenForEventsResult.CallbackStop != nullptr), TEXT("Failed to start streaming read receipts"));

	ReadReceiptsCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingReadReceipts = true;
	
	return FinalResult;
}

void UPubnubChatChannel::StreamReadReceiptsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamReadReceiptsAsync(NativeCallback);
}

void UPubnubChatChannel::StreamReadReceiptsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamReadReceiptsResult = WeakThis.Get()->StreamReadReceipts();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamReadReceiptsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingReadReceipts()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	if (!IsStreamingReadReceipts)
	{ return FinalResult; }
	
	if (!ReadReceiptsCallbackStop)
	{
		IsStreamingReadReceipts = false;
		return FinalResult;
	}

	FPubnubChatOperationResult StopResult = ReadReceiptsCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);

	ReadReceiptsCallbackStop = nullptr;
	IsStreamingReadReceipts = false;
	
	return FinalResult;
}

void UPubnubChatChannel::StopStreamingReadReceiptsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingReadReceiptsAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingReadReceiptsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopReadReceiptsResult = WeakThis.Get()->StopStreamingReadReceipts();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopReadReceiptsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamMessageReports()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	//Skip if it's already streaming
	if (IsStreamingMessageReports)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	FOnPubnubChatEventReceivedNative OnEventReceived;
	OnEventReceived.BindLambda([ThisWeak](const FPubnubChatEvent& Event)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();
		
		if (!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingMessageReports)
		{ return; }

		const FPubnubChatReportEvent ReportEvent = UPubnubChatInternalUtilities::GetReportEventFromChatEvent(Event);
		ThisChannel->OnMessageReported.Broadcast(ReportEvent);
		ThisChannel->OnMessageReportedNative.Broadcast(ReportEvent);
	});
		
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(ChannelID);
	FPubnubChatListenForEventsResult ListenForEventsResult = Chat->ListenForEvents(ModerationChannelID, EPubnubChatEventType::PCET_Report, OnEventReceived);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ListenForEventsResult.Result);
	
	MessageReportsCallbackStop = ListenForEventsResult.CallbackStop;
	IsStreamingMessageReports = true;
	
	return FinalResult;
}

void UPubnubChatChannel::StreamMessageReportsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamMessageReportsAsync(NativeCallback);
}

void UPubnubChatChannel::StreamMessageReportsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamMessageReportsResult = WeakThis.Get()->StreamMessageReports();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamMessageReportsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingMessageReports()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	if (!IsStreamingMessageReports)
	{ return FinalResult; }
	
	if (!MessageReportsCallbackStop)
	{ return FinalResult; }
	
	FPubnubChatOperationResult StopResult = MessageReportsCallbackStop->Stop();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, StopResult);
	
	MessageReportsCallbackStop = nullptr;
	IsStreamingMessageReports = false;

	return FinalResult;
}

void UPubnubChatChannel::StopStreamingMessageReportsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingMessageReportsAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingMessageReportsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopMessageReportsResult = WeakThis.Get()->StopStreamingMessageReports();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopMessageReportsResult);
	});
}

FPubnubChatEventsResult UPubnubChatChannel::GetMessageReportsHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count)
{
	FPubnubChatEventsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, StartTimetoken);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, EndTimetoken);
	
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(ChannelID);
	return Chat->GetEventsHistory(ModerationChannelID, StartTimetoken, EndTimetoken, Count);
}

void UPubnubChatChannel::GetMessageReportsHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponse OnEventsResponse, const int Count)
{
	FOnPubnubChatEventsResponseNative NativeCallback;
	NativeCallback.BindLambda([OnEventsResponse](const FPubnubChatEventsResult& EventsResult)
	{
		OnEventsResponse.ExecuteIfBound(EventsResult);
	});

	GetMessageReportsHistoryAsync(StartTimetoken, EndTimetoken, NativeCallback, Count);
}

void UPubnubChatChannel::GetMessageReportsHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponseNative OnEventsResponseNative, const int Count)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnEventsResponseNative, FPubnubChatEventsResult());
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, StartTimetoken, EndTimetoken, Count, OnEventsResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatEventsResult EventsResult = WeakThis.Get()->GetMessageReportsHistory(StartTimetoken, EndTimetoken, Count);
		UPubnubUtilities::CallPubnubDelegate(OnEventsResponseNative, EventsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StreamCustomEvents()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	//Skip if it's already streaming
	if (IsStreamingCustomEvents)
	{ return FinalResult; }

	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(CustomEventsSubscription, TEXT("Custom events subscription is invalid."));
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);

	auto OnCustomEventMessage = [ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();
		
		if (!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsStreamingCustomEvents)
		{ return; }

		if (!UPubnubChatInternalUtilities::IsCustomEventMessage(MessageData.Message))
		{
			return;
		}

		FPubnubChatCustomEvent CustomEvent = UPubnubChatInternalUtilities::GetCustomEventFromPubnubMessageData(MessageData);
		
		ThisChannel->OnCustomEventReceived.Broadcast(CustomEvent);
		ThisChannel->OnCustomEventReceivedNative.Broadcast(CustomEvent);
	};

	CustomEventsSubscription->OnPubnubMessageNative.AddLambda(OnCustomEventMessage);
	CustomEventsSubscription->OnPubnubSignalNative.AddLambda(OnCustomEventMessage);

	FPubnubOperationResult SubscribeResult = CustomEventsSubscription->Subscribe();
	FinalResult.AddStep("Subscribe", SubscribeResult);
	if (SubscribeResult.Error)
	{
		CustomEventsSubscription->OnPubnubMessageNative.Clear();
		CustomEventsSubscription->OnPubnubSignalNative.Clear();
		return FinalResult;
	}
	
	IsStreamingCustomEvents = true;
	
	return FinalResult;
}

void UPubnubChatChannel::StreamCustomEventsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamCustomEventsAsync(NativeCallback);
}

void UPubnubChatChannel::StreamCustomEventsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamCustomEventsResult = WeakThis.Get()->StreamCustomEvents();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamCustomEventsResult);
	});
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingCustomEvents()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	if (!CustomEventsSubscription)
	{
		IsStreamingCustomEvents = false;
		return FinalResult;
	}

	CustomEventsSubscription->OnPubnubMessageNative.Clear();
	CustomEventsSubscription->OnPubnubSignalNative.Clear();
	
	if (!IsStreamingCustomEvents)
	{ return FinalResult; }

	FPubnubOperationResult UnsubscribeResult = CustomEventsSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsStreamingCustomEvents = false;
	
	return FinalResult;
}

void UPubnubChatChannel::StopStreamingCustomEventsAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingCustomEventsAsync(NativeCallback);
}

void UPubnubChatChannel::StopStreamingCustomEventsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopCustomEventsResult = WeakThis.Get()->StopStreamingCustomEvents();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopCustomEventsResult);
	});
}

UPubnubChatMessageDraft* UPubnubChatChannel::CreateMessageDraft(FPubnubChatMessageDraftConfig MessageDraftConfig)
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(nullptr);
	
	UPubnubChatMessageDraft* MessageDraft = NewObject<UPubnubChatMessageDraft>(this);
	MessageDraft->InitMessageDraft(this, MessageDraftConfig);
	
	return MessageDraft;
}

void UPubnubChatChannel::InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Channel, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Channel, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InChannelID.IsEmpty(), TEXT("Can't init Channel, ChannelID is empty"));

	ChannelID = InChannelID;
	PubnubClient = InPubnubClient;
	Chat = InChat;

	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(ChannelID);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ChannelEntity, TEXT("Can't init Channel, Failed to create ChannelEntity"));

	ConnectSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ConnectSubscription, TEXT("Can't init Channel, Failed to create Connect Subscription"));
	
	UpdatesSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UpdatesSubscription, TEXT("Can't init Channel, Failed to create Updates Subscription"));
	
	FPubnubSubscribeSettings PresenceSubscribeSettings;
	PresenceSubscribeSettings.ReceivePresenceEvents = true;
	PresenceSubscription = ChannelEntity->CreateSubscription(PresenceSubscribeSettings);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PresenceSubscription, TEXT("Can't init Channel, Failed to create Presence Subscription"));

	CustomEventsSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(CustomEventsSubscription, TEXT("Can't init Channel, Failed to create Custom Events Subscription"));
	
	// Register this channel object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterChannel(ChannelID);
	}
	
	//Add delegate to OnChatDestroyed to this object is cleaned up as well
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatChannel::OnChatDestroyed);
	
	IsInitialized = true;
}

FPubnubChatGetRestrictionsResult UPubnubChatChannel::GetRestrictions(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	
	//Getting restrictions is actually getting members from moderation channel
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(ChannelID);
	FPubnubMemberInclude Include = FPubnubMemberInclude({.IncludeCustom = true, .IncludeTotalCount = true}); 
	FPubnubChannelMembersResult GetMembersResult = PubnubClient->GetChannelMembers(ModerationChannelID, Include, Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result, "GetChannelMembers");
	
	//Convert Custom fields to Restrictions
	for (auto& MemberData : GetMembersResult.MembersData)
	{
		FPubnubChatRestriction Restriction = UPubnubChatInternalUtilities::GetRestrictionFromChannelMemberCustom(MemberData.Custom);
		Restriction.UserID = MemberData.User.UserID;
		Restriction.ChannelID = ChannelID;
		FinalResult.Restrictions.Add(Restriction);
	}
	
	FinalResult.Page = GetMembersResult.Page;
	FinalResult.Total = GetMembersResult.TotalCount;
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::SendTextInternal(const FString Message, FPubnubChatSendTextParams SendTextParams, UPubnubChatMessage* QuotedMessage, TMap<FString,FString> MentionedUsers)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Message);
	FPubnubChatOperationResult FinalResult;
	
	//Validate quoted message if it was added to the params
	if(QuotedMessage)
	{
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((QuotedMessage->GetMessageData().ChannelID == ChannelID), TEXT("You cannot quote messages from other channels"));
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((!QuotedMessage->GetMessageTimetoken().IsEmpty()), TEXT("Quoted message has empty invalid timetoken"));
	}

	//Calculate if SendText should be delayed by the RateLimiter
	float DelaySeconds = CalculateSendTextRateLimiterDelay();
	if (DelaySeconds > 0.0f)
	{
		FPlatformProcess::Sleep(DelaySeconds);
	}

	//Configure settings specified in the params
	FPubnubPublishSettings PublishSettings;
	PublishSettings.MetaData = UPubnubChatInternalUtilities::SendTextMetaFromParams(SendTextParams, QuotedMessage);
	PublishSettings.StoreInHistory = SendTextParams.StoreInHistory;
	if(SendTextParams.SendByPost)
	{
		PublishSettings.PublishMethod = EPubnubPublishMethod::PPM_SendViaPOST;
	}

	FPubnubPublishMessageResult PublishResult = PubnubClient->PublishMessage(ChannelID, UPubnubChatInternalUtilities::ChatMessageToPublishString(Message), PublishSettings);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PublishResult.Result, "PublishMessage");

	//Update RateLimiter state after successful send
	{
		FScopeLock Lock(&SendTextRateLimitCriticalSection);
		LastSendTextTime = FDateTime::UtcNow();
	}

	//Here it's just an empty function, but ThreadChannel uses it to AddMessageAction about it's creation
	FPubnubChatOperationResult OnSendTextResult = OnSendText();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, OnSendTextResult);
	
	for (auto& MentionedUser : MentionedUsers)
	{
		FPubnubChatOperationResult EmitMentionResult = EmitUserMention(MentionedUser.Key, PublishResult.PublishedMessage.Timetoken, MentionedUser.Value);
		FinalResult.Merge(EmitMentionResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::OnSendText()
{
	return FPubnubChatOperationResult();
}

FString UPubnubChatChannel::CreateMentionEventPayload(FString Timetoken, FString Text)
{
	return UPubnubChatInternalUtilities::GetMentionEventPayload(ChannelID, Timetoken, Text);
}

void UPubnubChatChannel::AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak)
{
	ConnectSubscription->OnPubnubMessageNative.AddLambda([ThisChannelWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisChannelWeak.IsValid())
		{return;}
			
		UPubnubChatChannel* ThisChannel = ThisChannelWeak.Get();

		if(!ThisChannel->IsInitialized || !ThisChannel->Chat || !ThisChannel->IsConnected)
		{return;}
			
		ThisChannel->OnMessageReceived.Broadcast(ThisChannel->Chat->CreateMessageObject(MessageData.Timetoken, MessageData));
		ThisChannel->OnMessageReceivedNative.Broadcast(ThisChannel->Chat->CreateMessageObject(MessageData.Timetoken, MessageData));
	});
}

void UPubnubChatChannel::OnChatDestroyed(FString UserID)
{
	CleanUp();
}

void UPubnubChatChannel::ClearAllSubscriptions()
{
	if (ConnectSubscription)
	{
		ConnectSubscription->OnPubnubMessageNative.Clear();
		if (IsConnected)
		{
			ConnectSubscription->Unsubscribe();
			IsConnected = false;
		}

		ConnectSubscription = nullptr;
	}
	
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

	if (PresenceSubscription)
	{
		PresenceSubscription->OnPubnubPresenceEventNative.Clear();
		if (IsStreamingPresence)
		{
			PresenceSubscription->Unsubscribe();
			IsStreamingPresence = false;
		}

		PresenceSubscription = nullptr;
	}

	if (CustomEventsSubscription)
	{
		CustomEventsSubscription->OnPubnubMessageNative.Clear();
		CustomEventsSubscription->OnPubnubSignalNative.Clear();
		if (IsStreamingCustomEvents)
		{
			CustomEventsSubscription->Unsubscribe();
			IsStreamingCustomEvents = false;
		}

		CustomEventsSubscription = nullptr;
	}
	
	if (TypingCallbackStop)
	{
		TypingCallbackStop->Stop();
		TypingCallbackStop = nullptr;
	}

	if (ReadReceiptsCallbackStop)
	{
		ReadReceiptsCallbackStop->Stop();
		ReadReceiptsCallbackStop = nullptr;
	}
	
	// Clean up typing subscription and indicators
	if (MessageReportsCallbackStop)
	{
		MessageReportsCallbackStop->Stop();
		MessageReportsCallbackStop = nullptr;
	}

	// Invalidate all typing indicator timers and clear the map
	{
		FScopeLock Lock(&TypingIndicatorsCriticalSection);
		for (auto& IndicatorPair : TypingIndicators)
		{
			IndicatorPair.Value.TimerHandle.Invalidate();
		}
		TypingIndicators.Empty();
	}
	
	IsStreamingTyping = false;
	IsStreamingPresence = false;
	IsStreamingReadReceipts = false;
	IsStreamingMessageReports = false;
	IsStreamingCustomEvents = false;
}

float UPubnubChatChannel::CalculateSendTextRateLimiterDelay()
{
	FScopeLock Lock(&SendTextRateLimitCriticalSection);

	FPubnubChatChannelData ChannelData = GetChannelData();
	if (ChannelData.Type.IsEmpty())
	{ return 0.0f; }
	
	int32* RateLimitMsPtr = Chat->ChatConfig.RateLimiter.RateLimitPerChannel.Find(ChannelData.Type);
	int32 BaseIntervalMs = RateLimitMsPtr ? *RateLimitMsPtr : 0;
	
	if (BaseIntervalMs <= 0)
	{ return 0.0f; }
	
	if (LastSendTextTime == FDateTime::MinValue())
	{ return 0.0f; }
	
	FDateTime CurrentTime = FDateTime::UtcNow();
	FTimespan Elapsed = CurrentTime - LastSendTextTime;
	int32 ElapsedMs = static_cast<int32>(Elapsed.GetTotalMilliseconds());
	
	//Clamp Exponential factor to a reasonable values
	float ExponentialFactor = FMath::Clamp(Chat->ChatConfig.RateLimiter.RateLimitFactor, 1.0f, 10.0f);
	
	//Calculate required interval with exponential backoff: base * (factor ^ penalty)
	//Penalty increases on each rate limit hit, causing exponential growth in delay - for now hardcoded 100 as max Penalty
	int32 RequiredIntervalMs = BaseIntervalMs;
	if (SendTextRateLimitPenalty > 0)
	{
		float PenaltyMultiplier = FMath::Pow(ExponentialFactor, static_cast<float>(FMath::Min(SendTextRateLimitPenalty, 100)));
		RequiredIntervalMs = static_cast<int32>(BaseIntervalMs * PenaltyMultiplier);
	}
	
	//If enough time has passed since last send, reset penalty and allow immediate send
	if (ElapsedMs >= RequiredIntervalMs)
	{
		SendTextRateLimitPenalty = 0;
		return 0.0f;
	}
	
	//Calculate remaining delay needed and increase penalty for next rate limit hit
	int32 RemainingDelayMs = FMath::Min(RequiredIntervalMs - ElapsedMs, Pubnub_Chat_Max_Rate_Limiter_Delay);
	SendTextRateLimitPenalty++;
	
	return static_cast<float>(RemainingDelayMs) / 1000.0f;
}

void UPubnubChatChannel::CleanUp()
{
	//Clean up subscription if channel is being destroyed while connected
	if (IsInitialized)
	{
		ClearAllSubscriptions();
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !ChannelID.IsEmpty())
	{
		Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatChannel::OnChatDestroyed);
		Chat->ObjectsRepository->UnregisterChannel(ChannelID);
	}
	
	IsInitialized = false;
}
