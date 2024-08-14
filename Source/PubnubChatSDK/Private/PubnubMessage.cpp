// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubMessage.h"
#include "PubnubThreadChannel.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "FunctionLibraries/PubnubChatUtilities.h"

UPubnubMessage* UPubnubMessage::Create(Pubnub::Message Message)
{
	UPubnubMessage* NewMessage = NewObject<UPubnubMessage>();
	NewMessage->InternalMessage = new Pubnub::Message(Message);
	return NewMessage;
}

UPubnubMessage::~UPubnubMessage()
{
	if(!IsThreadMessage)
	{
		{delete InternalMessage;}
	}
}


FString UPubnubMessage::GetTimetoken()
{
	if(!IsInternalMessageValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalMessage->timetoken());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Get Timetoken error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

FPubnubChatMessageData UPubnubMessage::GetMessageData()
{
	if(!IsInternalMessageValid()) {return FPubnubChatMessageData();}

	try
	{
		auto CppMessageData = InternalMessage->message_data();
	
		return FPubnubChatMessageData(CppMessageData);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Get Message Data error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChatMessageData();
}

void UPubnubMessage::EditText(FString NewText)
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->edit_text(UPubnubChatUtilities::FStringToPubnubString(NewText));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Edit Text error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

FString UPubnubMessage::Text()
{
	if(!IsInternalMessageValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalMessage->text());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Text error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

UPubnubMessage* UPubnubMessage::DeleteMessage()
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		return Create(InternalMessage->delete_message());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

bool UPubnubMessage::DeleteMessageHard()
{
	if(!IsInternalMessageValid()) {return false;}

	try
	{
		return InternalMessage->delete_message_hard();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete Message Hard error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

bool UPubnubMessage::Deleted()
{
	if(!IsInternalMessageValid()) {return false;}

	try
	{
		return InternalMessage->deleted();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Deleted error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

UPubnubMessage* UPubnubMessage::Restore()
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		return Create(InternalMessage->restore());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Restore error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

EPubnubChatMessageType UPubnubMessage::Type()
{
	if(!IsInternalMessageValid()) {return EPubnubChatMessageType::PCMT_TEXT;}

	try
	{
		return (EPubnubChatMessageType)(uint8)InternalMessage->type();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Type error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return EPubnubChatMessageType::PCMT_TEXT;
}

void UPubnubMessage::Pin()
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->pin();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Pin error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessage::Unpin()
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->unpin();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Unpin error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubMessage* UPubnubMessage::ToggleReaction(FString Reaction)
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto CppMessage = InternalMessage->toggle_reaction(UPubnubChatUtilities::FStringToPubnubString(Reaction));
		return Create(CppMessage);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Toggle Reaction error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<FPubnubMessageAction> UPubnubMessage::Reactions()
{
	if(!IsInternalMessageValid()) {return {};}

	try
	{
		auto CppActions = InternalMessage->reactions();
		return UPubnubChatUtilities::CppMessageActionsToUnrealMessageActions(CppActions);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Reactions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubMessage::HasUserReaction(FString Reaction)
{
	if(!IsInternalMessageValid()) {return false;}

	try
	{
		return InternalMessage->has_user_reaction(UPubnubChatUtilities::FStringToPubnubString(Reaction));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Reactions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubMessage::Forward(FString ChannelID)
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->forward(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Forward error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessage::Report(FString Reason)
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->report(UPubnubChatUtilities::FStringToPubnubString(Reason));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Report error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubMessage::StreamUpdates(FOnPubnubMessageStreamUpdateReceived MessageUpdateCallback)
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto lambda = [MessageUpdateCallback](Pubnub::Message Message)
		{
			AsyncTask(ENamedThreads::GameThread, [MessageUpdateCallback, Message]()
			{
				UPubnubMessage* NewMessage = UPubnubMessage::Create(Message);
				MessageUpdateCallback.ExecuteIfBound(NewMessage);
			});

		};
		auto CppCallbackStop = InternalMessage->stream_updates(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Stream Updates error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubMessage::StreamUpdatesOn(TArray<UPubnubMessage*> Messages, FOnPubnubMessagesStreamUpdateOnReceived MessageUpdateCallback)
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto lambda = [MessageUpdateCallback](Pubnub::Vector<Pubnub::Message> Messages)
		{
			auto StdMessages = Messages.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [MessageUpdateCallback, StdMessages]()
			{
				TArray<UPubnubMessage*> FinalMessages;
			
				for(auto CppMessage : StdMessages)
				{
					UPubnubMessage* NewMessage = UPubnubMessage::Create(CppMessage);
					FinalMessages.Add(NewMessage);
				}
			
				MessageUpdateCallback.ExecuteIfBound(FinalMessages);
			});
		};
	
		auto CppMessages = UPubnubChatUtilities::UnrealMessagesToCppMessages(Messages);

		auto CppCallbackStop = InternalMessage->stream_updates_on(CppMessages, lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Stream Updates On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubMessage::CreateThread()
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto CppThread = InternalMessage->create_thread();
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Create Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubMessage::GetThread()
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto CppThread = InternalMessage->get_thread();
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Get Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

bool UPubnubMessage::HasThread()
{
	if(!IsInternalMessageValid()) {return false;}

	try
	{
		return InternalMessage->has_thread();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Has Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubMessage::RemoveThread()
{
	if(!IsInternalMessageValid()) {return;}

	try
	{
		InternalMessage->remove_thread();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Remove Thread error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FPubnubMentionedUser> UPubnubMessage::MentionedUsers()
{
	if(!IsInternalMessageValid()) {return {};}

	try
	{
		TArray<FPubnubMentionedUser> FinalMentionedUsers;
		auto CppMentionedUsers = InternalMessage->mentioned_users();
		auto StdMentionedUsers = CppMentionedUsers.into_std_vector();
		for(auto MentionedUser : StdMentionedUsers)
		{
			FinalMentionedUsers.Add(MentionedUser);
		}
		return FinalMentionedUsers;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Mentioned Users error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

TArray<FPubnubReferencedChannel> UPubnubMessage::ReferencedChannels()
{
	if(!IsInternalMessageValid()) {return {};}

	try
	{
		TArray<FPubnubReferencedChannel> FinalReferencedChannels;
		auto CppReferencedChannels = InternalMessage->referenced_channels();
		auto StdReferencedChannels = CppReferencedChannels.into_std_vector();
		for(auto ReferencedChannel : StdReferencedChannels)
		{
			FinalReferencedChannels.Add(ReferencedChannel);
		}
		return FinalReferencedChannels;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Referenced Channels error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

UPubnubMessage* UPubnubMessage::QuotedMessage()
{
	if(!IsInternalMessageValid()) {return nullptr;}

	try
	{
		auto MaybeMessage = InternalMessage->quoted_message();
		if(MaybeMessage.has_value())
		{
			return UPubnubMessage::Create(MaybeMessage.value());
		}

		return nullptr;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Quoted Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<FPubnubTextLink> UPubnubMessage::TextLinks()
{
	if(!IsInternalMessageValid()) {return {};}

	try
	{
		TArray<FPubnubTextLink> FinalTextLinks;
		auto CppTextLinks = InternalMessage->text_links();
		auto StdTextLinks = CppTextLinks.into_std_vector();
		for(auto TextLink : StdTextLinks)
		{
			FinalTextLinks.Add(TextLink);
		}
		return FinalTextLinks;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Message Text Links error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubMessage::IsInternalMessageValid()
{
	if(InternalMessage == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubMessage is invalid"));
		return false;
	}
	return true;
}
