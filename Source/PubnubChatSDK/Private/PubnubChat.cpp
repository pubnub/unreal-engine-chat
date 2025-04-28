// Copyright 2024 PubNub Inc. All Rights Reserved.

#include "PubnubChat.h"
#include "PubnubUser.h"
#include "PubnubMessage.h"
#include "PubnubThreadChannel.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "PubnubAccessManager.h"
#include "Async/Async.h"
#include "FunctionLibraries/PubnubChatUtilities.h"

DEFINE_LOG_CATEGORY(PubnubChatLog)


UPubnubChat* UPubnubChat::Create(Pubnub::Chat Chat)
{
	UPubnubChat* NewChat = NewObject<UPubnubChat>();
	NewChat->InternalChat = new Pubnub::Chat(Chat);
	auto PubnubChatLogLambda = [=](Pubnub::pn_log_level log_level, const char* message)
	{
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
	};
	Chat.register_logger_callback(PubnubChatLogLambda);
	return NewChat;
}

void UPubnubChat::DestroyChat()
{
	delete InternalChat;
	InternalChat = nullptr;
	OnChatDestroyed.Broadcast();
}

UPubnubChannel* UPubnubChat::CreatePublicConversation(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto channel = InternalChat->create_public_conversation(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create Public Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateGroupConversation(TArray<UPubnubUser*> Users, FString ChannelID, FPubnubChatChannelData ChannelData, FPubnubChatMembershipData MembershipData)
{
	if(!IsInternalChatValid()) {return FPubnubCreatedChannelWrapper();}
	
	try
	{
		auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);
		auto CppWrapper = InternalChat->create_group_conversation(CppUsers, UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), MembershipData.GetCppChatMembershipData());
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create Group Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
	if(!User)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create Direct Conversation error: User is invalid"));
		return FPubnubCreatedChannelWrapper();
	}
	
	try
	{
		auto CppWrapper = InternalChat->create_direct_conversation(*User->GetInternalUser(), UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), MembershipData.GetCppChatMembershipData());
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create Direct Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
	
	try
	{
		auto channel = InternalChat->get_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Get Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
		UE_LOG(PubnubChatLog, Error, TEXT("Get Channels error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChannelsResponseWrapper();
}

UPubnubChannel* UPubnubChat::UpdateChannel(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto new_channel_data = ChannelData.GetCppChatChannelData();
		auto channel = InternalChat->update_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Update Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteChannel(FString ChannelID)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->delete_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Delete Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::PinMessageToChannel(UPubnubMessage* Message, UPubnubChannel* Channel)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->pin_message_to_channel(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Pin Message to Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::UnpinMessageFromChannel(UPubnubChannel* Channel)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->unpin_message_from_channel(*Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Unpin Message from Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
		UE_LOG(PubnubChatLog, Error, TEXT("Current User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::CreateUser(FString UserID, FPubnubChatUserData UserData)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppUser = InternalChat->create_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::GetUser(FString UserID)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppUser = InternalChat->get_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Get User error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
		UE_LOG(PubnubChatLog, Error, TEXT("Get Users error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubUsersResponseWrapper();
}

UPubnubUser* UPubnubChat::UpdateUser(FString UserID, FPubnubChatUserData UserData)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppUser = InternalChat->update_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Update User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteUser(FString UserID)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->delete_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Delete User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FString> UPubnubChat::WherePresent(FString UserID)
{
	if(!IsInternalChatValid()) {return {};}
	
	try
	{
		TArray<FString> PresentChannels;

		auto ResponseChannels = InternalChat->where_present(UPubnubChatUtilities::FStringToPubnubString(UserID));
		PresentChannels = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseChannels);
	
		return PresentChannels;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Where Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

TArray<FString> UPubnubChat::WhoIsPresent(FString ChannelID)
{
	if(!IsInternalChatValid()) {return {};}
	
	try
	{
		TArray<FString> PresentUsers;
	
		auto ResponseUsers = InternalChat->who_is_present(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		PresentUsers = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseUsers);
	
		return PresentUsers;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Who is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubChat::IsPresent(FString UserID, FString ChannelID)
{
	if(!IsInternalChatValid()) {return false;}
	
	try
	{
		return InternalChat->is_present(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubChat::SetRestrictions(FString UserID, FString ChannelID, FPubnubRestriction Restrictions)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID), Restrictions.GetCppRestriction());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Set Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::EmitChatEvent(EPubnubChatEventType ChatEventType, FString ChannelID, FString Payload)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->emit_chat_event((Pubnub::pubnub_chat_event_type)(uint8)ChatEventType, UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(Payload));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Emit Chat Event error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubChat::ListenForEvents(FString ChannelID, EPubnubChatEventType ChatEventType, FOnPubnubEventReceived EventCallback)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
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
		UE_LOG(PubnubChatLog, Error, TEXT("Listen for Events error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubEventsHistoryWrapper UPubnubChat::GetEventsHistory(FString ChannelID, FString StartTimetoken, FString EndTimetoken, int Count)
{
	if(!IsInternalChatValid()) {return FPubnubEventsHistoryWrapper();}
	
	try
	{
		auto CppWrapper = InternalChat->get_events_history(UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);
		auto UEWrapper = FPubnubEventsHistoryWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Get Events History error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubEventsHistoryWrapper();
}

void UPubnubChat::ForwardMessage(UPubnubChannel* Channel, UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->forward_message(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Forward Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
		UE_LOG(PubnubChatLog, Error, TEXT("Get Unread Messages Counts error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
		UE_LOG(PubnubChatLog, Error, TEXT("Mark All Messages as Read error: %s"), UTF8_TO_TCHAR(Exception.what()));
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
        UE_LOG(PubnubChatLog, Error, TEXT("Get Current User Mentions error: %s"), UTF8_TO_TCHAR(Exception.what()));
    }
    return FPubnubUserMentionDataList();
}

UPubnubThreadChannel* UPubnubChat::CreateThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppThread = InternalChat->create_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Create Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubChat::GetThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	try
	{
		auto CppThread = InternalChat->get_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Get Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::RemoveThreadChannel(UPubnubMessage* Message)
{
	if(!IsInternalChatValid()) {return;}
	
	try
	{
		InternalChat->remove_thread_channel(*Message->GetInternalMessage());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Remove Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubAccessManager* UPubnubChat::GetAccessManager()
{
	if(!IsInternalChatValid()) {return nullptr;}
	
	return UPubnubAccessManager::Create(InternalChat->access_manager());
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
