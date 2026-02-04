// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatThreadChannel.h"
#include "PubnubChatMessage.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatObjectsRepository.h"
#include "Entities/PubnubSubscription.h"
#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "Threads/PubnubFunctionThread.h"


FPubnubChatGetThreadHistoryResult UPubnubChatThreadChannel::GetThreadHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count)
{
	FPubnubChatGetThreadHistoryResult FinalResult;
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
	
	for (auto& MessageData : FetchHistoryResult.Messages)
	{
		UPubnubChatThreadMessage* ThreadMessage = Chat->CreateThreadMessageObject(MessageData.Timetoken, MessageData, ParentChannelID);
		FinalResult.ThreadMessages.Add(ThreadMessage);
	}
	
	//If we got the exact amount of messages as specified count, probably there are more events in a given range
	FinalResult.IsMore = FetchHistoryResult.Messages.Num() == Count;
	
	return FinalResult;
}

void UPubnubChatThreadChannel::GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponse OnThreadHistoryResponse, const int Count)
{
	FOnPubnubChatGetThreadHistoryResponseNative NativeCallback;
	NativeCallback.BindLambda([OnThreadHistoryResponse](const FPubnubChatGetThreadHistoryResult& ThreadHistoryResult)
	{
		OnThreadHistoryResponse.ExecuteIfBound(ThreadHistoryResult);
	});

	GetThreadHistoryAsync(StartTimetoken, EndTimetoken, NativeCallback, Count);
}

void UPubnubChatThreadChannel::GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponseNative OnThreadHistoryResponseNative, const int Count)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(OnThreadHistoryResponseNative, FPubnubChatGetThreadHistoryResult());
	
	TWeakObjectPtr<UPubnubChatThreadChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, StartTimetoken, EndTimetoken, Count, OnThreadHistoryResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatGetThreadHistoryResult ThreadHistoryResult = WeakThis.Get()->GetThreadHistory(StartTimetoken, EndTimetoken, Count);
		UPubnubUtilities::CallPubnubDelegate(OnThreadHistoryResponseNative, ThreadHistoryResult);
	});
}

FPubnubChatOperationResult UPubnubChatThreadChannel::PinMessageToParentChannel(UPubnubChatThreadMessage* ThreadMessage)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(ThreadMessage);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((ThreadMessage->GetMessageData().ChannelID == ChannelID), TEXT("Can't pin Message from another Thread Channel"));
	
	FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(ParentChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result);
	
	FPubnubChatOperationResult PinMessageResult = GetChannelResult.Channel->PinMessage(ThreadMessage);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, PinMessageResult);
	
	return FinalResult;
}

void UPubnubChatThreadChannel::PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	PinMessageToParentChannelAsync(ThreadMessage, NativeCallback);
}

void UPubnubChatThreadChannel::PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatThreadChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, ThreadMessage, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult PinResult = WeakThis.Get()->PinMessageToParentChannel(ThreadMessage);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, PinResult);
	});
}

FPubnubChatOperationResult UPubnubChatThreadChannel::UnpinMessageFromParentChannel()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatChannelResult GetChannelResult = Chat->GetChannel(ParentChannelID);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result);
	
	FPubnubChatOperationResult UnpinMessageResult = GetChannelResult.Channel->UnpinMessage();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UnpinMessageResult);
	
	return FinalResult;
}

void UPubnubChatThreadChannel::UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	UnpinMessageFromParentChannelAsync(NativeCallback);
}

void UPubnubChatThreadChannel::UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative)
{
	PUBNUB_CHAT_OBJECT_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(OnOperationResponseNative);
	
	TWeakObjectPtr<UPubnubChatThreadChannel> WeakThis = MakeWeakObjectPtr(this);

	Chat->AsyncFunctionsThread->AddFunctionToQueue([WeakThis, OnOperationResponseNative]
	{
		if (!WeakThis.IsValid())
		{ return; }
		
		FPubnubChatOperationResult UnpinResult = WeakThis.Get()->UnpinMessageFromParentChannel();
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, UnpinResult);
	});
}

void UPubnubChatThreadChannel::InitThreadChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InThreadChannelID, UPubnubChatMessage* InParentMessage, bool InIsThreadConfirmed)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InParentMessage, TEXT("Can't init Thread Channel, InParentMessage is invalid"));
	
	ParentChannelID = InParentMessage->GetMessageData().ChannelID;
	ParentMessage = InParentMessage;
	IsThreadConfirmed = InIsThreadConfirmed;
	
	InitChannel(InPubnubClient, InChat, InThreadChannelID);
}

FPubnubChatOperationResult UPubnubChatThreadChannel::OnSendText()
{
	//If thread is confirmed (exists on server), there is no need to add it again
	if (IsThreadConfirmed)
	{
		return FPubnubChatOperationResult();
	}
	
	//Thread is firstly created as local object and send to server during first SendText.
	
	FPubnubChatOperationResult FinalResult;
	
	//Set Channel Metadata for ThreadChannel using PubnubClient
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, GetChannelData().ToPubnubChannelInputData());
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//AddMessageAction for the Parent Message, so it knows that is the Root of this ThreadChannel
	FString ActionType = UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType::PCMAT_ThreadRootId);
	FPubnubAddMessageActionResult AddMessageActionResult = PubnubClient->AddMessageAction(ParentChannelID, ParentMessage->GetMessageTimetoken(), ActionType, ChannelID);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, AddMessageActionResult.Result, "AddMessageAction");
	
	//Update MessageData with ThreadRoot MessageAction and update ObjectsRepository with that new MessageData
	FPubnubChatMessageData MessageData = ParentMessage->GetMessageData();
	MessageData.MessageActions.Add(FPubnubChatMessageAction::FromPubnubMessageActionData(AddMessageActionResult.MessageActionData));
	ParentMessage->UpdateMessageData(MessageData);
	
	//Now this thread is Confirmed to be on the server
	IsThreadConfirmed = true;
	
	return FinalResult;
}

void UPubnubChatThreadChannel::AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak)
{
	ConnectSubscription->OnPubnubMessageNative.AddLambda([ThisChannelWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisChannelWeak.IsValid())
		{return;}
				
		UPubnubChatThreadChannel* ThisThreadChannel = Cast<UPubnubChatThreadChannel>(ThisChannelWeak.Get());

		if(!ThisThreadChannel || !ThisThreadChannel->Chat)
		{return;}
				
		ThisThreadChannel->OnThreadMessageReceived.Broadcast(ThisThreadChannel->Chat->CreateThreadMessageObject(MessageData.Timetoken, MessageData, ThisThreadChannel->ParentChannelID));
		ThisThreadChannel->OnThreadMessageReceivedNative.Broadcast(ThisThreadChannel->Chat->CreateThreadMessageObject(MessageData.Timetoken, MessageData, ThisThreadChannel->ParentChannelID));
	});
}
