// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubChannel.h"
#include "PubnubMessage.h"
#include "PubnubMembership.h"
#include "PubnubChatSubsystem.h"
#include "PubnubUser.h"
#include "PubnubCallbackStop.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


Pubnub::SendTextParams FSendTextParams::GetCppSendTextParams()
{
	Pubnub::SendTextParams FinalParams;

	FinalParams.store_in_history = StoreInHistory;
	FinalParams.send_by_post = SendByPost;
	FinalParams.meta = UPubnubChatUtilities::FStringToPubnubString(Meta);

	if(!MentionedUsers.IsEmpty())
	{
		std::map<int, Pubnub::MentionedUser> CppMentionedUsers;
		for(auto It : MentionedUsers)
		{
			CppMentionedUsers[It.Key] = It.Value.GetCppMentionedUser();
		}
		FinalParams.mentioned_users = CppMentionedUsers;
	}

	if(!ReferencedChannels.IsEmpty())
	{
		std::map<int, Pubnub::ReferencedChannel> CppReferencedChannels;
		for(auto It : ReferencedChannels)
		{
			CppReferencedChannels[It.Key] = It.Value.GetCppReferencedChannel();
		}
		FinalParams.referenced_channels = CppReferencedChannels;
	}

	if(!TextLinks.IsEmpty())
	{
		Pubnub::Vector<Pubnub::TextLink> CppTextLinks;
		for(auto TextLink : TextLinks)
		{
			CppTextLinks.push_back(TextLink.GetCppTextLink());
		}
		FinalParams.text_links = std::move(CppTextLinks);
	}

	if(QuotedMessage)
	{
		FinalParams.quoted_message = *QuotedMessage->GetInternalMessage();
	}
		
	return FinalParams;

}

FPubnubMembersResponseWrapper::FPubnubMembersResponseWrapper(Pubnub::MembersResponseWrapper& Wrapper) :
Page(Wrapper.page),
Total(Wrapper.total),
Status(UPubnubChatUtilities::PubnubStringToFString(Wrapper.status))
{
	auto CppMemberships = Wrapper.memberships.into_std_vector();
	for(auto Membership : CppMemberships)
	{
		Memberships.Add(UPubnubMembership::Create(Membership));
	}
}

FPubnubUsersRestrictionsWrapper::FPubnubUsersRestrictionsWrapper(Pubnub::UsersRestrictionsWrapper& Wrapper) :
Page(Wrapper.page),
Total(Wrapper.total),
Status(UPubnubChatUtilities::PubnubStringToFString(Wrapper.status))
{
	auto CppRestrictions = Wrapper.restrictions.into_std_vector();
	for(auto Restriction : CppRestrictions)
	{
		Restrictions.Add(Restriction);
	}
}

UPubnubChannel* UPubnubChannel::Create(Pubnub::Channel Channel)
{
	UPubnubChannel* NewChannel = NewObject<UPubnubChannel>();
	NewChannel->InternalChannel = new Pubnub::Channel(Channel);
	return NewChannel;
}
UPubnubChannel::~UPubnubChannel()
{
	if(!IsThreadChannel)
	{
		delete InternalChannel;
	}
}

FString UPubnubChannel::GetChannelID()
{
	if(!IsInternalChannelValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalChannel->channel_id());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Channel ID error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

FPubnubChatChannelData UPubnubChannel::GetChannelData()
{
	if(!IsInternalChannelValid()) {return FPubnubChatChannelData();}

	try
	{
		return FPubnubChatChannelData(InternalChannel->channel_data());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get Channel Data error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChatChannelData();
}

UPubnubChannel* UPubnubChannel::Update(FPubnubChatChannelData ChannelData)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto CppChannel = InternalChannel->update(ChannelData.GetCppChatChannelData());
		return UPubnubChannel::Create(CppChannel);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Update error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChannel::Connect(FOnPubnubChannelMessageReceived MessageCallback)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		auto lambda = [MessageCallback](Pubnub::Message Message)
		{
			AsyncTask(ENamedThreads::GameThread, [MessageCallback, Message]()
			{
				UPubnubMessage* NewMessage = UPubnubMessage::Create(Message);
				MessageCallback.ExecuteIfBound(NewMessage);
			});
		};
		InternalChannel->connect(lambda);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Connect error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::Disconnect()
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->disconnect();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Disconnect error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::Join(FOnPubnubChannelMessageReceived MessageCallback, FString CustomData)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		auto lambda = [MessageCallback](Pubnub::Message Message)
		{
			AsyncTask(ENamedThreads::GameThread, [MessageCallback, Message]()
			{
				UPubnubMessage* NewMessage = UPubnubMessage::Create(Message);
				MessageCallback.ExecuteIfBound(NewMessage);
			});

		};
		InternalChannel->join(lambda, UPubnubChatUtilities::FStringToPubnubString(CustomData));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Join error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::Leave()
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->leave();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Leave error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::DeleteChannel()
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->delete_channel();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete Channel error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::SendText(FString Message, FSendTextParams SendTextParams)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->send_text(UPubnubChatUtilities::FStringToPubnubString(Message), SendTextParams.GetCppSendTextParams());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Send Text error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubChannel::StreamUpdates(FOnPubnubChannelStreamUpdateReceived ChannelUpdateCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto lambda = [ChannelUpdateCallback](Pubnub::Channel Channel)
		{
			AsyncTask(ENamedThreads::GameThread, [ChannelUpdateCallback, Channel]()
			{
				UPubnubChannel* NewChannel = UPubnubChannel::Create(Channel);
				ChannelUpdateCallback.ExecuteIfBound(NewChannel);
			});

		};
		auto CppCallbackStop = InternalChannel->stream_updates(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Stream Updates error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubChannel::StreamUpdatesOn(TArray<UPubnubChannel*> Channels, FOnPubnubChannelsStreamUpdateOnReceived ChannelUpdateCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto lambda = [ChannelUpdateCallback](Pubnub::Vector<Pubnub::Channel> Channels)
		{
			auto StdChannels = Channels.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [ChannelUpdateCallback, StdChannels]()
			{
				TArray<UPubnubChannel*> FinalChannels;
			
				for(auto CppChannel : StdChannels)
				{
					UPubnubChannel* NewChannel = UPubnubChannel::Create(CppChannel);
					FinalChannels.Add(NewChannel);
				}
			
				ChannelUpdateCallback.ExecuteIfBound(FinalChannels);
			});
		};
	
		auto CppChannels = UPubnubChatUtilities::UnrealChannelsToCppChannels(Channels);

		auto CppCallbackStop = InternalChannel->stream_updates_on(Pubnub::Vector<Pubnub::Channel>(std::move(CppChannels)), lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Stream Updates On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubChannel::StreamPresence(FOnPubnubChannelStreamPresenceReceived PresenceCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		Pubnub::Vector<Pubnub::String> CppPresentUsers2;
		CppPresentUsers2.into_std_vector();
	
		auto lambda = [PresenceCallback](Pubnub::Vector<Pubnub::String> CppPresentUsers)
		{
			auto StdCppPresentUsers = CppPresentUsers.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [PresenceCallback, StdCppPresentUsers]()
			{
				TArray<FString> FinalPresentUsers;
				for(auto &CppUser : StdCppPresentUsers)
				{
					FinalPresentUsers.Add(UPubnubChatUtilities::PubnubStringToFString(CppUser));
				}
				PresenceCallback.ExecuteIfBound(FinalPresentUsers);
			});

		};
		auto CppCallbackStop = InternalChannel->stream_presence(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Stream Presence error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<FString> UPubnubChannel::WhoIsPresent()
{
	if(!IsInternalChannelValid()) {return {};}

	try
	{
		auto ResponseUsers = InternalChannel->who_is_present();
		TArray<FString> PresentUsers = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseUsers);
	
		return PresentUsers;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Who Is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubChannel::IsPresent(FString UserID)
{
	if(!IsInternalChannelValid()) {return false;}

	try
	{
		return InternalChannel->is_present(UPubnubChatUtilities::FStringToPubnubString(UserID));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Is Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubChannel::SetRestrictions(FString UserID, FPubnubRestriction Restrictions)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(UserID), Restrictions.GetCppRestriction());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Set Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

FPubnubRestriction UPubnubChannel::GetUserRestrictions(UPubnubUser* User)
{
	if(!IsInternalChannelValid()) {return FPubnubRestriction();}

	try
	{
		return FPubnubRestriction(InternalChannel->get_user_restrictions(*User->GetInternalUser()));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get User Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubRestriction();
}

FPubnubUsersRestrictionsWrapper UPubnubChannel::GetUsersRestrictions(FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChannelValid()) {return FPubnubUsersRestrictionsWrapper();}

	try
	{
		auto CppWrapper = InternalChannel->get_users_restrictions(UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		FPubnubUsersRestrictionsWrapper UEWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get Users Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubUsersRestrictionsWrapper();
}

UPubnubMessage* UPubnubChannel::GetMessage(FString Timetoken)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto CppMessage = InternalChannel->get_message(UPubnubChatUtilities::FStringToPubnubString(Timetoken));
		return UPubnubMessage::Create(CppMessage);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<UPubnubMessage*> UPubnubChannel::GetHistory(FString StartTimetoken, FString EndTimetoken, int Count)
{
	if(!IsInternalChannelValid()) {return {};}

	try
	{
		auto CppHistoryMessages = InternalChannel->get_history(UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);
	
		TArray<UPubnubMessage*>  FinalMessages = UPubnubChatUtilities::CppMessagesToUnrealMessages(CppHistoryMessages);

		return FinalMessages;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get History error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

FPubnubMembersResponseWrapper UPubnubChannel::GetMembers(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalChannelValid()) {return FPubnubMembersResponseWrapper();}

	try
	{
		auto CppWrapper = InternalChannel->get_members(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		auto UEWrapper = FPubnubMembersResponseWrapper(CppWrapper);
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get Members error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMembersResponseWrapper();
}

UPubnubMembership* UPubnubChannel::Invite(UPubnubUser* User)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto CppMembership = InternalChannel->invite(*User->GetInternalUser());
		return UPubnubMembership::Create(CppMembership);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Invite error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<UPubnubMembership*> UPubnubChannel::InviteMultiple(TArray<UPubnubUser*> Users)
{
	TArray<UPubnubMembership*> FinalMemberships;
	if(!IsInternalChannelValid()) {return FinalMemberships;}

	try
	{
		auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);
		auto CppMemberships = InternalChannel->invite_multiple(std::move(CppUsers));
		FinalMemberships = UPubnubChatUtilities::CppMembershipsToUnrealMemberships(CppMemberships);
	
		return FinalMemberships;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Invite Multiple error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

void UPubnubChannel::StartTyping()
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->start_typing();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Start Typing error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::StopTyping()
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->stop_typing();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Stop Typing error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

UPubnubCallbackStop* UPubnubChannel::GetTyping(FOnPubnubChannelTypingReceived TypingCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto lambda = [TypingCallback](Pubnub::Vector<Pubnub::String> CppTypingUsers)
		{
			std::vector<Pubnub::String> StdTypingUsers = CppTypingUsers.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [TypingCallback, StdTypingUsers]()
			{
				TArray<FString> FinalTypingUsers;
				for(auto &CppUser : StdTypingUsers)
				{
					FinalTypingUsers.Add(UPubnubChatUtilities::PubnubStringToFString(CppUser));
				}
				TypingCallback.ExecuteIfBound(FinalTypingUsers);
			});
		};
	
		auto CppCallbackStop = InternalChannel->get_typing(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get Typing error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubChannel::PinMessage(UPubnubMessage* Message)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		return Create(InternalChannel->pin_message(*Message->GetInternalMessage()));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Pin Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubChannel::UnpinMessage()
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		return Create(InternalChannel->unpin_message());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Unpin Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubMessage* UPubnubChannel::GetPinnedMessage()
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		return UPubnubMessage::Create(InternalChannel->get_pinned_message());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get Pinned Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChannel::ForwardMessage(UPubnubMessage* Message)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->forward_message(*Message->GetInternalMessage());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Forward Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::EmitUserMention(FString UserID, FString Timetoken, FString Text)
{
	if(!IsInternalChannelValid()) {return;}

	try
	{
		InternalChannel->emit_user_mention(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(Timetoken), UPubnubChatUtilities::FStringToPubnubString(Text));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Emit User Mention error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<UPubnubMembership*> UPubnubChannel::GetUserSuggestions(FString Text, int Limit)
{
	if(!IsInternalChannelValid()) {return {};}

	try
	{
		auto CppMemberships = InternalChannel->get_user_suggestions(UPubnubChatUtilities::FStringToPubnubString(Text), Limit);
		TArray<UPubnubMembership*> FinalMemberships = UPubnubChatUtilities::CppMembershipsToUnrealMemberships(CppMemberships);
		return FinalMemberships;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Channel Get User Suggestions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

UPubnubCallbackStop* UPubnubChannel::StreamReadReceipts(FOnPubnubChannelStreamReadReceiptsReceived ReadReceiptsCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	try
	{
		auto lambda = [ReadReceiptsCallback](Pubnub::Map<Pubnub::String, Pubnub::Vector<Pubnub::String>, Pubnub::StringComparer> CppReadReceipts)
		{
			auto StdCppReadReceipts = CppReadReceipts.into_std_map();
			FPubnubReadReceiptsWrapper ReadReceiptsWrapper;

			TMap<FString, FPubnubStringArrayWrapper> UEMap;
			for(auto it : StdCppReadReceipts)
			{
				FPubnubStringArrayWrapper StringWrapper;
				StringWrapper.Strings = UPubnubChatUtilities::PubnubStringsToFStrings(it.second);
				UEMap.Add(UPubnubChatUtilities::PubnubStringToFString(it.first), StringWrapper);
			}
			ReadReceiptsWrapper.Receipts = UEMap;
		
			AsyncTask(ENamedThreads::GameThread, [ReadReceiptsCallback, ReadReceiptsWrapper]()
			{
				ReadReceiptsCallback.ExecuteIfBound(ReadReceiptsWrapper);
			});
		};
	
		auto CppCallbackStop = InternalChannel->stream_read_receipts(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Stream Read Receipts error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}


bool UPubnubChannel::IsInternalChannelValid()
{
	if(InternalChannel == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubChannel is invalid"));
		return false;
	}
	return true;
}
