// Copyright 2024 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"
#include "PubnubUser.h"
#include "PubnubMessage.h"
#include "PubnubThreadChannel.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "PubnubAccessManager.h"
#include "PubnubMacroUtilities.h"
#include "Async/Async.h"
#include "FunctionLibraries/PubnubChatUtilities.h"
#include "FunctionLibraries/PubnubLogUtilities.h"

DEFINE_LOG_CATEGORY(PubnubChatLog)


UPubnubChat* UPubnubChat::Create(Pubnub::Chat Chat)
{
	UPubnubChat* NewChat = NewObject<UPubnubChat>();
	NewChat->InternalChat = new Pubnub::Chat(Chat);
	Chat.register_logger_callback(LogCppChatMessage);
	
	return NewChat;
}

void UPubnubChat::InitConnectionListener()
{
	if(!IsInternalChatValid()) {return;}
	
	auto ConnectionStatusListener = [this](Pubnub::pn_connection_status status, Pubnub::ConnectionStatusData status_data)
	{
		FPubnubConnectionStatusData StatusData = FPubnubConnectionStatusData({UPubnubChatUtilities::PubnubStringToFString(status_data.reason)});
		OnConnectionStatusChanged.Broadcast((EPubnubConnectionStatus)status, StatusData);
		OnConnectionStatusChangedNative.Broadcast((EPubnubConnectionStatus)status, StatusData);
	};
	InternalChat->add_connection_status_listener(ConnectionStatusListener);
}

void UPubnubChat::DestroyChat()
{
	InternalChat->remove_connection_status_listener();
	delete InternalChat;
	InternalChat = nullptr;
	OnChatDestroyed.Broadcast();
}

UPubnubChannel* UPubnubChat::CreatePublicConversation(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, nullptr);
	
	try
	{
		auto channel = InternalChat->create_public_conversation(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateGroupConversation(TArray<UPubnubUser*> Users, FString ChannelID, FPubnubChatChannelData ChannelData, FPubnubChatMembershipData MembershipData)
{
	if(!IsInternalChatValid()) {return FPubnubCreatedChannelWrapper();}

	auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);
	
	PUBNUB_RETURN_IF_EMPTY_CPP_VECTOR(CppUsers, FPubnubCreatedChannelWrapper());
	PUBNUB_RETURN_IF_EMPTY(ChannelID, FPubnubCreatedChannelWrapper());
	
	try
	{
		auto CppWrapper = InternalChat->create_group_conversation(CppUsers, UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), MembershipData.GetCppChatMembershipData());
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubCreatedChannelWrapper();
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateGroupConversation(TArray<UPubnubUser*> Users, FString ChannelID, FPubnubChatChannelData ChannelData, FString MembershipData)
{
	return this->CreateGroupConversation(Users, ChannelID, ChannelData, FPubnubChatMembershipData(MembershipData, "", ""));
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateDirectConversation(UPubnubUser* User, FString ChannelID, FPubnubChatChannelData ChannelData, FPubnubChatMembershipData MembershipData)
{
	if(!IsInternalChatValid()) {return FPubnubCreatedChannelWrapper();}

	PUBNUB_RETURN_IF_NULL(User, FPubnubCreatedChannelWrapper());
	PUBNUB_RETURN_IF_EMPTY(ChannelID, FPubnubCreatedChannelWrapper());
	
	try
	{
		auto CppWrapper = InternalChat->create_direct_conversation(*User->GetInternalUser(), UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), MembershipData.GetCppChatMembershipData());
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubCreatedChannelWrapper();
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateDirectConversation(UPubnubUser* User, FString ChannelID, FPubnubChatChannelData ChannelData, FString MembershipData)
{
	return this->CreateDirectConversation(User, ChannelID, ChannelData, FPubnubChatMembershipData(MembershipData, "", ""));
}

UPubnubChannel* UPubnubChat::GetChannel(FString ChannelID)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, nullptr);
	
	try
	{
		auto channel = InternalChat->get_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubChannelsResponseWrapper UPubnubChat::GetChannels(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChatValid()) {return FPubnubChannelsResponseWrapper();}
	
	try
	{
		TArray<UPubnubChannel*> FinalChannels;
		auto CppWrapper = InternalChat->get_channels(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubChannelsResponseWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChannelsResponseWrapper();
}

UPubnubChannel* UPubnubChat::UpdateChannel(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, nullptr);
	
	try
	{
		auto new_channel_data = ChannelData.GetCppChatChannelData();
		auto channel = InternalChat->update_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteChannel(FString ChannelID)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID);
	
	try
	{
		InternalChat->delete_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::PinMessageToChannel(UPubnubMessage* Message, UPubnubChannel* Channel)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_NULL(Message);
	PUBNUB_RETURN_IF_NULL(Channel);
	
	try
	{
		InternalChat->pin_message_to_channel(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::UnpinMessageFromChannel(UPubnubChannel* Channel)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_NULL(Channel);
	
	try
	{
		InternalChat->unpin_message_from_channel(*Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubUser* UPubnubChat::CurrentUser()
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppUser = InternalChat->current_user();
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::CreateUser(FString UserID, FPubnubChatUserData UserData)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(UserID, nullptr);
	
	try
	{
		auto CppUser = InternalChat->create_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::GetUser(FString UserID)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(UserID, nullptr);
	
	try
	{
		auto CppUser = InternalChat->get_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubUsersResponseWrapper UPubnubChat::GetUsers(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChatValid()) {return FPubnubUsersResponseWrapper();}
	
	try
	{
		auto CppWrapper = InternalChat->get_users(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubUsersResponseWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubUsersResponseWrapper();
}

UPubnubUser* UPubnubChat::UpdateUser(FString UserID, FPubnubChatUserData UserData)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(UserID, nullptr);
	
	try
	{
		auto CppUser = InternalChat->update_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteUser(FString UserID)
{
	if(!IsInternalChatValid()) {return;}
	
	PUBNUB_RETURN_IF_EMPTY(UserID);
	
	try
	{
		InternalChat->delete_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FString> UPubnubChat::WherePresent(FString UserID)
{
	if(!IsInternalChatValid()) {return {};}

	PUBNUB_RETURN_IF_EMPTY(UserID, {});
	
	try
	{
		TArray<FString> PresentChannels;

		auto ResponseChannels = InternalChat->where_present(UPubnubChatUtilities::FStringToPubnubString(UserID));
		PresentChannels = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseChannels);
	
		return PresentChannels;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

TArray<FString> UPubnubChat::WhoIsPresent(FString ChannelID)
{
	if(!IsInternalChatValid()) {return {};}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, {});
	
	try
	{
		TArray<FString> PresentUsers;
	
		auto ResponseUsers = InternalChat->who_is_present(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		PresentUsers = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseUsers);
	
		return PresentUsers;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubChat::IsPresent(FString UserID, FString ChannelID)
{
	if(!IsInternalChatValid()) {return false;}

	PUBNUB_RETURN_IF_EMPTY(UserID, false);
	PUBNUB_RETURN_IF_EMPTY(ChannelID, false);
	
	try
	{
		return InternalChat->is_present(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubChat::SetRestrictions(FString UserID, FString ChannelID, FPubnubRestriction Restrictions)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(UserID);
	PUBNUB_RETURN_IF_EMPTY(ChannelID);
	
	try
	{
		InternalChat->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID), Restrictions.GetCppRestriction());
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::EmitChatEvent(EPubnubChatEventType ChatEventType, FString ChannelID, FString Payload)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID);
	PUBNUB_RETURN_IF_EMPTY(Payload);
	
	try
	{
		InternalChat->emit_chat_event((Pubnub::pubnub_chat_event_type)(uint8)ChatEventType, UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(Payload));
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubChat::ListenForEvents(FString ChannelID, EPubnubChatEventType ChatEventType, FOnPubnubEventReceived EventCallback)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, nullptr);
	
	try
	{
		auto lambda = [EventCallback](Pubnub::Event Event)
		{
			AsyncTask(ENamedThreads::GameThread, [EventCallback, Event]()
			{
				EventCallback.ExecuteIfBound(Event);
			});
		};

		auto CppCallbackStop = InternalChat->listen_for_events(UPubnubChatUtilities::FStringToPubnubString(ChannelID), (Pubnub::pubnub_chat_event_type)(uint8)ChatEventType, lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubEventsHistoryWrapper UPubnubChat::GetEventsHistory(FString ChannelID, FString StartTimetoken, FString EndTimetoken, int Count)
{
	if(!IsInternalChatValid()) {return FPubnubEventsHistoryWrapper();}

	PUBNUB_RETURN_IF_EMPTY(ChannelID, FPubnubEventsHistoryWrapper());
	
	try
	{
		auto CppWrapper = InternalChat->get_events_history(UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);
		auto UEWrapper = FPubnubEventsHistoryWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubEventsHistoryWrapper();
}

void UPubnubChat::ForwardMessage(UPubnubChannel* Channel, UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_NULL(Channel);
	PUBNUB_RETURN_IF_NULL(Message);
	
	try
	{
		InternalChat->forward_message(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FPubnubUnreadMessageWrapper> UPubnubChat::GetUnreadMessagesCounts(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChatValid()) {return {};}
	
	try
	{
		auto MessageWrappers = InternalChat->get_unread_messages_counts(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		auto MessageWrappersStd = MessageWrappers.into_std_vector();
		TArray<FPubnubUnreadMessageWrapper> FinalWrappers;
	
		for(auto Wrapper : MessageWrappersStd)
		{
			FinalWrappers.Add(Wrapper);
		}
	
		return FinalWrappers;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

FPubnubMarkMessagesAsReadWrapper UPubnubChat::MarkAllMessagesAsRead(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChatValid()) {return FPubnubMarkMessagesAsReadWrapper();}
	
	try
	{
		auto CppWrapper = InternalChat->mark_all_messages_as_read(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubMarkMessagesAsReadWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMarkMessagesAsReadWrapper();
}


FPubnubUserMentionDataList UPubnubChat::GetCurrentUserMentions(FString StartTimetoken, FString EndTimetoken, int Count)
{
	if(!IsInternalChatValid()) {return FPubnubUserMentionDataList();}
	
    try {
        auto list = InternalChat->get_current_user_mentions(UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);

        FPubnubUserMentionDataList FinalList(list);

        return FinalList;
    } catch (std::exception& Exception) {
        UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
    }
    return FPubnubUserMentionDataList();
}

UPubnubThreadChannel* UPubnubChat::CreateThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_NULL(Message, nullptr);
	
	try
	{
		auto CppThread = InternalChat->create_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubChat::GetThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return nullptr;}

	PUBNUB_RETURN_IF_NULL(Message, nullptr);
	
	try
	{
		auto CppThread = InternalChat->get_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::RemoveThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return;}

	PUBNUB_RETURN_IF_NULL(Message);
	
	try
	{
		InternalChat->remove_thread_channel(*Message->GetInternalMessage());
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubAccessManager* UPubnubChat::GetAccessManager()
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	return UPubnubAccessManager::Create(InternalChat->access_manager());
}

bool UPubnubChat::ReconnectSubscriptions()
{
	if(!IsInternalChatValid()) {return false;}

	try
	{
		return InternalChat->reconnect_subscriptions();
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}

	return false;
}

bool UPubnubChat::DisconnectSubscriptions()
{
	if(!IsInternalChatValid()) {return false;}

	try
	{
		return InternalChat->disconnect_subscriptions();
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}

	return false;
}

bool UPubnubChat::IsInternalChatValid()
{
	if(InternalChat == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Chat is invalid. Are you trying to call functions on destroyed chat? Try creating new one."));
		return false;
	}
	return true;
}

//Logs from C-Core that are false warnings as they are sent during normal C-Core operations flow
TArray<FString> UPubnubChat::FalseCCoreLogPhrases =
	{
	"errno=0('No error')",
	"errno=9('Bad file descriptor')",
	"errno=2('No such file or directory')",
	"errno=35('Resource temporarily unavailable')"
};

bool UPubnubChat::ShouldCCoreLogBeSkipped(FString Message)
{
	for(FString& LogSkipPhrases : FalseCCoreLogPhrases)
	{
		if(Message.Contains(LogSkipPhrases))
		{
			return true;
		}
	}
	return false;
}

void UPubnubChat::LogCppChatMessage(Pubnub::pn_log_level log_level, const char* message)
{
	//This is temporal solution to skip false warnings from C-Core.
	//It should be fixed on C-Core level, but until it's done we filter them out here
	if(ShouldCCoreLogBeSkipped(FString(message)))
	{
		return;
	}
	
	switch (log_level)
	{
	case Pubnub::pn_log_level::Warning:
		UE_LOG(PubnubChatLog, Warning, TEXT("%s"), UTF8_TO_TCHAR(message));
		break;
	case Pubnub::pn_log_level::Error:
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), UTF8_TO_TCHAR(message));
		break;
	default:
		UE_LOG(PubnubChatLog, Log, TEXT("%s"), UTF8_TO_TCHAR(message));
		break;
	};
}
