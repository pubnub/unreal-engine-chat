// Fill out your copyright notice in the Description page of Project Settings.

#include "PubnubChat.h"
#include "PubnubUser.h"
#include "PubnubMessage.h"
#include "PubnubThreadChannel.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "PubnubAccessManager.h"
#include "FunctionLibraries/PubnubChatUtilities.h"

DEFINE_LOG_CATEGORY(PubnubLog)


UPubnubChat* UPubnubChat::Create(Pubnub::Chat Chat)
{
	UPubnubChat* NewChat = NewObject<UPubnubChat>();
	NewChat->InternalChat = new Pubnub::Chat(Chat);
	return NewChat;
}

UPubnubChannel* UPubnubChat::CreatePublicConversation(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	try
	{
		auto channel = InternalChat->create_public_conversation(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Create Public Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateGroupConversation(TArray<UPubnubUser*> Users, FString ChannelID, FPubnubChatChannelData ChannelData, FString MembershipData)
{
	try
	{
		auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);
		auto CppWrapper = InternalChat->create_group_conversation(CppUsers, UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), UPubnubChatUtilities::FStringToPubnubString(MembershipData));
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Create Group Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubCreatedChannelWrapper();
}

FPubnubCreatedChannelWrapper UPubnubChat::CreateDirectConversation(UPubnubUser* User, FString ChannelID, FPubnubChatChannelData ChannelData, FString MembershipData)
{
	try
	{
		auto CppWrapper = InternalChat->create_direct_conversation(*User->GetInternalUser(), UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData(), UPubnubChatUtilities::FStringToPubnubString(MembershipData));
		FPubnubCreatedChannelWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Create Direct Conversation error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubCreatedChannelWrapper();
}

UPubnubChannel* UPubnubChat::GetChannel(FString ChannelID)
{
	try
	{
		auto channel = InternalChat->get_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubChannelsResponseWrapper UPubnubChat::GetChannels(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	try
	{
		TArray<UPubnubChannel*> FinalChannels;
		auto CppWrapper = InternalChat->get_channels(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubChannelsResponseWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Channels error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChannelsResponseWrapper();
}

UPubnubChannel* UPubnubChat::UpdateChannel(FString ChannelID, FPubnubChatChannelData ChannelData)
{
	try
	{
		auto new_channel_data = ChannelData.GetCppChatChannelData();
		auto channel = InternalChat->update_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID), ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(channel);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Update Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteChannel(FString ChannelID)
{
	try
	{
		InternalChat->delete_channel(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::PinMessageToChannel(UPubnubMessage* Message, UPubnubChannel* Channel)
{
	try
	{
		InternalChat->pin_message_to_channel(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Pin Message to Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::UnpinMessageFromChannel(UPubnubChannel* Channel)
{
	try
	{
		InternalChat->unpin_message_from_channel(*Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Unpin Message from Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<UPubnubChannel*> UPubnubChat::GetChannelSuggestions(FString Text, int Limit)
{
	try
	{
		auto CppChannels = InternalChat->get_channel_suggestions(UPubnubChatUtilities::FStringToPubnubString(Text), Limit);
		TArray<UPubnubChannel*> FinalChannels = UPubnubChatUtilities::CppChannelsToUnrealChannels(CppChannels);
		return FinalChannels;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Channel Suggestions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

UPubnubUser* UPubnubChat::CurrentUser()
{
	try
	{
		auto CppUser = InternalChat->current_user();
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Current User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::CreateUser(FString UserID, FPubnubChatUserData UserData)
{
	try
	{
		auto CppUser = InternalChat->create_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Create User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubUser* UPubnubChat::GetUser(FString UserID)
{
	try
	{
		auto CppUser = InternalChat->get_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubUsersResponseWrapper UPubnubChat::GetUsers(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	try
	{
		auto CppWrapper = InternalChat->get_users(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubUsersResponseWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Users error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubUsersResponseWrapper();
}

UPubnubUser* UPubnubChat::UpdateUser(FString UserID, FPubnubChatUserData UserData)
{
	try
	{
		auto CppUser = InternalChat->update_user(UPubnubChatUtilities::FStringToPubnubString(UserID), UserData.GetCppChatUserData());
		return UPubnubUser::Create(CppUser);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Update User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::DeleteUser(FString UserID)
{
	try
	{
		InternalChat->delete_user(UPubnubChatUtilities::FStringToPubnubString(UserID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<UPubnubUser*> UPubnubChat::GetUserSuggestions(FString Text, int Limit)
{
	try
	{
		auto CppUsers = InternalChat->get_user_suggestions(UPubnubChatUtilities::FStringToPubnubString(Text), Limit);
		TArray<UPubnubUser*> FinalUsers = UPubnubChatUtilities::CppUsersToUnrealUsers(CppUsers);
		return FinalUsers;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get User Suggestions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

TArray<FString> UPubnubChat::WherePresent(FString UserID)
{
	try
	{
		TArray<FString> PresentChannels;

		auto ResponseChannels = InternalChat->where_present(UPubnubChatUtilities::FStringToPubnubString(UserID));
		PresentChannels = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseChannels);
	
		return PresentChannels;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Where Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

TArray<FString> UPubnubChat::WhoIsPresent(FString ChannelID)
{
	try
	{
		TArray<FString> PresentUsers;
	
		auto ResponseUsers = InternalChat->who_is_present(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
		PresentUsers = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseUsers);
	
		return PresentUsers;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Who is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubChat::IsPresent(FString UserID, FString ChannelID)
{
	try
	{
		return InternalChat->is_present(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubChat::SetRestrictions(FString UserID, FString ChannelID, FPubnubRestriction Restrictions)
{
	try
	{
		InternalChat->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(ChannelID), Restrictions.GetCppRestriction());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Set Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChat::EmitChatEvent(EPubnubChatEventType ChatEventType, FString ChannelID, FString Payload)
{
	try
	{
		InternalChat->emit_chat_event((Pubnub::pubnub_chat_event_type)(uint8)ChatEventType, UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(Payload));
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Emit Chat Event error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubChat::ListenForEvents(FString ChannelID, EPubnubChatEventType ChatEventType, FOnPubnubEventReceived EventCallback)
{
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
		UE_LOG(PubnubLog, Error, TEXT("Listen for Events error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubEventsHistoryWrapper UPubnubChat::GetEventsHistory(FString ChannelID, FString StartTimetoken, FString EndTimetoken, int Count)
{
	try
	{
		auto CppWrapper = InternalChat->get_events_history(UPubnubChatUtilities::FStringToPubnubString(ChannelID), UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);
		auto UEWrappe = FPubnubEventsHistoryWrapper(CppWrapper);
		return CppWrapper;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Events History error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubEventsHistoryWrapper();
}

void UPubnubChat::ForwardMessage(UPubnubChannel* Channel, UPubnubMessage* Message)
{
	try
	{
		InternalChat->forward_message(*Message->GetInternalMessage(), *Channel->GetInternalChannel());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Forward Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FPubnubUnreadMessageWrapper> UPubnubChat::GetUnreadMessagesCounts(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
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
		UE_LOG(PubnubLog, Error, TEXT("Get Unread Messages Counts error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

FPubnubMarkMessagesAsReadWrapper UPubnubChat::MarkAllMessagesAsRead(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	try
	{
		auto CppWrapper = InternalChat->mark_all_messages_as_read(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubMarkMessagesAsReadWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Mark All Messages as Read error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMarkMessagesAsReadWrapper();
}

UPubnubThreadChannel* UPubnubChat::CreateThreadChannel(UPubnubMessage* Message)
{
	try
	{
		auto CppThread = InternalChat->create_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Create Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubThreadChannel* UPubnubChat::GetThreadChannel(UPubnubMessage* Message)
{
	try
	{
		auto CppThread = InternalChat->get_thread_channel(*Message->GetInternalMessage());
		return UPubnubThreadChannel::Create(CppThread);
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChat::RemoveThreadChannel(UPubnubMessage* Message)
{
	try
	{
		InternalChat->remove_thread_channel(*Message->GetInternalMessage());
	}
	catch(std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Remove Thread Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubAccessManager* UPubnubChat::GetAccessManager()
{
	return UPubnubAccessManager::Create(InternalChat->access_manager());
}

