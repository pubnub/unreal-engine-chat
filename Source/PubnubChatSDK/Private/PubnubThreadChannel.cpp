// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubThreadChannel.h"
#include "PubnubMessage.h"
#include "PubnubChatSubsystem.h"
#include "PubnubThreadMessage.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


UPubnubThreadChannel* UPubnubThreadChannel::Create(Pubnub::ThreadChannel ThreadChannel)
{
	UPubnubThreadChannel* NewChannel = NewObject<UPubnubThreadChannel>();
	NewChannel->InternalChannel = new Pubnub::ThreadChannel(ThreadChannel);
	NewChannel->IsThreadChannel = true;
	return NewChannel;
}

UPubnubMessage* UPubnubThreadChannel::GetParentMessage()
{
	if(!IsInternalThreadChannelValid()) {return nullptr;}

	try
	{
		return UPubnubMessage::Create(GetInternalThreadChannel()->parent_message());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Get Parent Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FString UPubnubThreadChannel::GetParentChannelID()
{
	if(!IsInternalThreadChannelValid()) {return "";}
	
	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(GetInternalThreadChannel()->parent_channel_id());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Get Parent Channel ID error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

TArray<UPubnubThreadMessage*> UPubnubThreadChannel::GetThreadHistory(int Limit, FString Start, FString End)
{
	if(!IsInternalThreadChannelValid()) {return {};}

	try
	{
		auto CppHistoryMessages = GetInternalThreadChannel()->get_thread_history(UPubnubChatUtilities::FStringToPubnubString(Start), UPubnubChatUtilities::FStringToPubnubString(End), Limit);
	
		TArray<UPubnubThreadMessage*> FinalMessages = UPubnubChatUtilities::CppThreadMessagesToUnrealTMessages(CppHistoryMessages);
		return FinalMessages;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Get Thread History error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

UPubnubThreadChannel* UPubnubThreadChannel::PinMessageToThread(UPubnubThreadMessage* ThreadMessage)
{
	if(!IsInternalThreadChannelValid()) {return nullptr;}

	try
	{
		return Create(GetInternalThreadChannel()->pin_message_to_thread((*ThreadMessage->GetInternalThreadMessage())));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Pin Message To Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubThreadChannel::UnpinMessageFromThread()
{
	if(!IsInternalThreadChannelValid()) {return nullptr;}

	try
	{
		return Create(GetInternalThreadChannel()->unpin_message_from_thread());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Unpin Message from Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubThreadChannel::PinMessageToParentChannel(UPubnubThreadMessage* ThreadMessage)
{
	if(!IsInternalThreadChannelValid()) {return nullptr;}

	try
	{
		return UPubnubChannel::Create(GetInternalThreadChannel()->pin_message_to_parent_channel(*ThreadMessage->GetInternalThreadMessage()));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Pin Message To Parent Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubThreadChannel::UnpinMessageFromParentChannel()
{
	if(!IsInternalThreadChannelValid()) {return nullptr;}

	try
	{
		return UPubnubChannel::Create(GetInternalThreadChannel()->unpin_message_from_parent_channel());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Thread Channel Unpin Message From Parent Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}


Pubnub::ThreadChannel* UPubnubThreadChannel::GetInternalThreadChannel()
{
	return (Pubnub::ThreadChannel*)(InternalChannel);
}

bool UPubnubThreadChannel::IsInternalThreadChannelValid()
{
	if(GetInternalThreadChannel() == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubChannel is invalid"));
		return false;
	}
	return true;
}