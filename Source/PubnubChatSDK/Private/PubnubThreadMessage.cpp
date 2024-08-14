// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubThreadMessage.h"
#include "PubnubMessage.h"
#include "PubnubChannel.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


UPubnubThreadMessage* UPubnubThreadMessage::Create(Pubnub::ThreadMessage ThreadMessage)
{
	UPubnubThreadMessage* NewMessage = NewObject<UPubnubThreadMessage>();
	NewMessage->InternalMessage = new Pubnub::ThreadMessage(ThreadMessage);
	NewMessage->IsThreadMessage = true;
	return NewMessage;
}

FString UPubnubThreadMessage::GetParentChannelID()
{
	if(!IsInternalThreadMessageValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(GetInternalThreadMessage()->parent_channel_id());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Message Get Parent Channel ID error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

UPubnubChannel* UPubnubThreadMessage::PinToParentChannel()
{
	if(!IsInternalThreadMessageValid()) {return nullptr;}

	try
	{
		return UPubnubChannel::Create(GetInternalThreadMessage()->pin_to_parent_channel());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Message Pin To Parent Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubThreadMessage::UnpinFromParentChannel()
{
	if(!IsInternalThreadMessageValid()) {return nullptr;}

	try
	{
		return UPubnubChannel::Create(GetInternalThreadMessage()->unpin_from_parent_channel());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Message Unpin From Parent Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubThreadMessage::StreamThreadMessageUpdatesOn(TArray<UPubnubThreadMessage*> Messages, FOnPubnubThreadMessagesStreamUpdateOnReceived MessageUpdateCallback)
{
	if(!IsInternalThreadMessageValid()) {return nullptr;}

	try
	{
		auto lambda = [MessageUpdateCallback](Pubnub::Vector<Pubnub::ThreadMessage> Messages)
		{
			auto StdMessages = Messages.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [MessageUpdateCallback, StdMessages]()
			{
				TArray<UPubnubThreadMessage*> FinalMessages;
			
				for(auto CppMessage : StdMessages)
				{
					UPubnubThreadMessage* NewMessage = UPubnubThreadMessage::Create(CppMessage);
					FinalMessages.Add(NewMessage);
				}
			
				MessageUpdateCallback.ExecuteIfBound(FinalMessages);
			});
		};
	
		auto CppMessages = UPubnubChatUtilities::UnrealThreadMessagesToCppTMessages(Messages);

		auto CppCallbackStop = GetInternalThreadMessage()->stream_updates_on(CppMessages, lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Message Stream Thread Message Updates On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

Pubnub::ThreadMessage* UPubnubThreadMessage::GetInternalThreadMessage()
{
	return (Pubnub::ThreadMessage*)(InternalMessage);
}

bool UPubnubThreadMessage::IsInternalThreadMessageValid()
{
	if(GetInternalThreadMessage() == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubMessage is invalid"));
		return false;
	}
	return true;
}