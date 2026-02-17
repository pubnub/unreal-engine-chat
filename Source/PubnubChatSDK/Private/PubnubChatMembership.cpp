// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMembership.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatAccessManager.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"
#include "PubnubChatConst.h"
#include "PubnubChatMessage.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "Threads/PubnubFunctionThread.h"


FString UPubnubChatMembership::GetInternalMembershipID() const
{
	if (!User || !Channel)
	{
		return TEXT("");
	}
	return FString::Printf(TEXT("%s.%s"), *Channel->GetChannelID(), *User->GetUserID());
}

void UPubnubChatMembership::BeginDestroy()
{
	CleanUp();
	
	Super::BeginDestroy();
}

FPubnubChatMembershipData UPubnubChatMembership::GetMembershipData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatMembershipData());

	// Get membership data from repository
	if (FPubnubChatInternalMembership* InternalMembership = Chat->ObjectsRepository->GetMembershipData(GetInternalMembershipID()))
	{
		return InternalMembership->MembershipData;
	}

	FString UserIDStr = User ? User->GetUserID() : TEXT("");
	FString ChannelIDStr = Channel ? Channel->GetChannelID() : TEXT("");
	UE_LOG(PubnubChatLog, Error, TEXT("Membership data not found in repository for UserID: %s, ChannelID: %s"), *UserIDStr, *ChannelIDStr);
	return FPubnubChatMembershipData();
}

FString UPubnubChatMembership::GetUserID() const
{
	return User ? User->GetUserID() : TEXT("");
}

FString UPubnubChatMembership::GetChannelID() const
{
	return Channel ? Channel->GetChannelID() : TEXT("");
}

FString UPubnubChatMembership::GetLastReadMessageTimetoken() const
{
	return UPubnubChatInternalUtilities::GetLastReadMessageTimetokenFromMembershipData(GetMembershipData());
}

FPubnubChatOperationResult UPubnubChatMembership::Delete()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;
	
	//RemoveMemberships by PubnubClient
	FPubnubMembershipsResult RemoveMembershipResult = PubnubClient->RemoveMemberships(GetUserID(), {GetChannelID()}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, RemoveMembershipResult.Result, "RemoveMemberships");

	return FinalResult;
}

void UPubnubChatMembership::DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	DeleteAsync(NativeCallback);
}

void UPubnubChatMembership::DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult DeleteResult = WeakThis.Get()->Delete();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, DeleteResult);
	});
}

FPubnubChatOperationResult UPubnubChatMembership::Update(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	//SetMemberships by PubnubClient
	FString Filter = UPubnubChatInternalUtilities::GetFilterForChannelID(GetChannelID());
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(GetUserID(), {UpdateMembershipData.ToPubnubMembershipInputData(GetChannelID())}, FPubnubMembershipInclude::FromValue(true), 1, Filter);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");
	
	//This should never happen in case of successful SetMemberships, but check just in case
	if (SetMembershipResult.MembershipsData.IsEmpty())
	{
		FinalResult.Error = true;
		FinalResult.ErrorMessage  = FString::Printf(TEXT("[%s]: SetMemberships succeeded, but returned no Membership. There is mismatch between local and server data."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		return FinalResult;
	}

	//Update repository with updated membership data
	Chat->ObjectsRepository->UpdateMembershipData(GetInternalMembershipID(), FPubnubChatMembershipData::FromPubnubMembershipData(SetMembershipResult.MembershipsData[0]));
	
	return FinalResult;
}

void UPubnubChatMembership::UpdateAsync(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	UpdateAsync(UpdateMembershipData, NativeCallback);
}

void UPubnubChatMembership::UpdateAsync(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, UpdateMembershipData, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult UpdateResult = WeakThis.Get()->Update(UpdateMembershipData);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, UpdateResult);
	});
}

FPubnubChatOperationResult UPubnubChatMembership::SetLastReadMessageTimetoken(const FString Timetoken)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Timetoken);

	//Add Timetoken to MembershipData
	FPubnubChatMembershipData MembershipData = GetMembershipData();
	UPubnubChatInternalUtilities::AddLastReadMessageTimetokenToMembershipData(MembershipData, Timetoken);

	//Update Membership with new data
	FPubnubChatOperationResult UpdateResult = Update(FPubnubChatUpdateMembershipInputData::FromChatMembershipData(MembershipData));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);

	//If this is not public channel Emit Receipt Event
	if(Channel->GetChannelData().Type != "public")
	{
		//Check if event can be emitted with provided AccessToken
		bool CanIEmit = Chat->AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Write, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, GetChannelID());
		FPubnubChatOperationResult EmitEventResult;
		if(CanIEmit)
		{
			EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Receipt, GetChannelID(), UPubnubChatInternalUtilities::GetReceiptEventPayload(Timetoken));
		}
		else
		{
			//It's not an error, but add it to the result
			EmitEventResult.AddStep("EmitChatEvent", FPubnubOperationResult({0, false, TEXT("Can't emit chat event, user doesn't have permissions")}));
		}

		FinalResult.Merge(EmitEventResult);
	}
	
	return FinalResult;
}

void UPubnubChatMembership::SetLastReadMessageTimetokenAsync(const FString Timetoken, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SetLastReadMessageTimetokenAsync(Timetoken, NativeCallback);
}

void UPubnubChatMembership::SetLastReadMessageTimetokenAsync(const FString Timetoken, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Timetoken, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult SetTimetokenResult = WeakThis.Get()->SetLastReadMessageTimetoken(Timetoken);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, SetTimetokenResult);
	});
}

FPubnubChatOperationResult UPubnubChatMembership::SetLastReadMessage(UPubnubChatMessage* Message)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);

	return SetLastReadMessageTimetoken(Message->GetMessageTimetoken());
}

void UPubnubChatMembership::SetLastReadMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SetLastReadMessageAsync(Message, NativeCallback);
}

void UPubnubChatMembership::SetLastReadMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, Message, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult SetMessageResult = WeakThis.Get()->SetLastReadMessage(Message);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, SetMessageResult);
	});
}

FPubnubChatOperationResult UPubnubChatMembership::StreamUpdates()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already streaming
	if (IsStreamingUpdates)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatMembership> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add listener to subscription with provided callback
	UpdatesSubscription->OnPubnubObjectEventNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatMembership* ThisMembership = ThisWeak.Get();

		if(!ThisMembership->Chat)
		{return;}
		
		//If this is not MembershipUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageMembershipUpdate(MessageData.Message))
		{
			//Check if membership was deleted or updated
			if (UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(MessageData.Message))
			{
				//Remove this membership from repository
				ThisMembership->Chat->ObjectsRepository->RemoveMembershipData(ThisMembership->GetInternalMembershipID());
				
				//Call delegates with Deleted type
				ThisMembership->OnMembershipUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisMembership->GetChannelID(), ThisMembership->GetUserID(), FPubnubChatMembershipData());
				ThisMembership->OnMembershipUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisMembership->GetChannelID(), ThisMembership->GetUserID(), FPubnubChatMembershipData());
			}
			else
			{
				//Adjust this membership data based on the update message
				FPubnubChatMembershipData ChatMembershipData = ThisMembership->GetMembershipData();
				FPubnubMembershipUpdateData MembershipUpdateData = UPubnubJsonUtilities::GetMembershipUpdateDataFromMessageContent(MessageData.Message);
				UPubnubChatInternalUtilities::UpdateChatMembershipFromPubnubMembershipUpdateData(MembershipUpdateData, ChatMembershipData);
							
				//Update repository with new membership data
				ThisMembership->Chat->ObjectsRepository->UpdateMembershipData(ThisMembership->GetInternalMembershipID(), ChatMembershipData);
				
				//Call delegates with new data
				ThisMembership->OnMembershipUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisMembership->GetChannelID(), ThisMembership->GetUserID(), ChatMembershipData);
				ThisMembership->OnMembershipUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisMembership->GetChannelID(), ThisMembership->GetUserID(), ChatMembershipData);
			}
		}
	});
	
	//Subscribe with UpdatesSubscription to receive membership metadata updates
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

void UPubnubChatMembership::StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StreamUpdatesAsync(NativeCallback);
}

void UPubnubChatMembership::StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StreamUpdatesResult = WeakThis.Get()->StreamUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StreamUpdatesResult);
	});
}

FPubnubChatOperationResult UPubnubChatMembership::StreamUpdatesOn(const TArray<UPubnubChatMembership*>& Memberships)
{
	FPubnubChatOperationResult FinalResult;
	for (auto& Membership : Memberships)
	{
		FPubnubChatOperationResult StreamUpdatesResult = Membership->StreamUpdates();
		FinalResult.Merge(StreamUpdatesResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMembership::StopStreamingUpdates()
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

void UPubnubChatMembership::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	StopStreamingUpdatesAsync(NativeCallback);
}

void UPubnubChatMembership::StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult StopResult = WeakThis.Get()->StopStreamingUpdates();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, StopResult);
	});
}

FPubnubChatGetUnreadMessagesCountResult UPubnubChatMembership::GetUnreadMessagesCount()
{
	FPubnubChatGetUnreadMessagesCountResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	FString LRMTimetoken = GetLastReadMessageTimetoken().IsEmpty() ? Pubnub_Chat_Empty_Timetoken : GetLastReadMessageTimetoken();
	FPubnubMessageCountsResult MessageCountsResult = PubnubClient->MessageCounts(GetChannelID(), LRMTimetoken);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, MessageCountsResult.Result, "MessageCounts");
	
	FinalResult.Count = MessageCountsResult.MessageCounts;
	return FinalResult;
}

void UPubnubChatMembership::GetUnreadMessagesCountAsync(FOnPubnubChatGetUnreadMessagesCountResponse OnUnreadMessagesCountResponse)
{
	FOnPubnubChatGetUnreadMessagesCountResponseNative NativeCallback;
	NativeCallback.BindLambda([OnUnreadMessagesCountResponse](const FPubnubChatGetUnreadMessagesCountResult& UnreadMessagesCountResult)
	{
		OnUnreadMessagesCountResponse.ExecuteIfBound(UnreadMessagesCountResult);
	});

	GetUnreadMessagesCountAsync(NativeCallback);
}

void UPubnubChatMembership::GetUnreadMessagesCountAsync(FOnPubnubChatGetUnreadMessagesCountResponseNative OnUnreadMessagesCountResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnUnreadMessagesCountResponseNative, FPubnubChatGetUnreadMessagesCountResult());
	
	TWeakObjectPtr<UPubnubChatMembership> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnUnreadMessagesCountResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetUnreadMessagesCountResult UnreadMessagesCountResult = WeakThis.Get()->GetUnreadMessagesCount();
		UPubnubUtilities::CallPubnubDelegate(OnUnreadMessagesCountResponseNative, UnreadMessagesCountResult);
	});
}

void UPubnubChatMembership::InitMembership(UPubnubClient* InPubnubClient, UPubnubChat* InChat, UPubnubChatUser* InUser, UPubnubChatChannel* InChannel)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Membership, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Membership, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InUser, TEXT("Can't init Membership, User is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChannel, TEXT("Can't init Membership, Channel is invalid"));

	User = InUser;
	Channel = InChannel;
	PubnubClient = InPubnubClient;
	Chat = InChat;
	
	// Create ChannelEntity and subscription for streaming updates
	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(Channel->GetChannelID());
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ChannelEntity, TEXT("Can't init Membership, Failed to create ChannelEntity"));
	
	UpdatesSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UpdatesSubscription, TEXT("Can't init Membership, Failed to create Updates Subscription"));
	
	// Register this membership object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterMembership(GetInternalMembershipID());
	}
	
	//Add delegate to OnChatDestroyed so this object is cleaned up as well
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatMembership::OnChatDestroyed);
	
	IsInitialized = true;
}

void UPubnubChatMembership::OnChatDestroyed(FString InUserID)
{
	CleanUp();
}

void UPubnubChatMembership::ClearAllSubscriptions()
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

void UPubnubChatMembership::CleanUp()
{
	//Clean up subscription if membership is being destroyed while connected
	if (IsInitialized)
	{
		ClearAllSubscriptions();
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && User && Channel)
	{
		Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatMembership::OnChatDestroyed);
		Chat->ObjectsRepository->UnregisterMembership(GetInternalMembershipID());
	}
	
	IsInitialized = false;
}

