// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "PubnubChannel.h"
#include "PubnubMacroUtilities.h"
#include "PubnubMessage.h"
#include "PubnubMembership.h"
#include "PubnubChatSubsystem.h"
#include "PubnubUser.h"
#include "PubnubCallbackStop.h"
#include "PubnubMessageDraft.h"
#include "Async/Async.h"
#include "FunctionLibraries/PubnubChatUtilities.h"
#include "FunctionLibraries/PubnubLogUtilities.h"


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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubChannel::Connect(FOnPubnubChannelMessageReceived MessageCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}

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
		auto CppCallbackStop = InternalChannel->connect(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
		return nullptr;
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}
UPubnubCallbackStop* UPubnubChannel::Join(FOnPubnubChannelMessageReceived MessageCallback, FPubnubChatMembershipData MembershipData)
{
	if(!IsInternalChannelValid()) {return nullptr;}

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
		auto CppCallbackStop = InternalChannel->join(lambda, MembershipData.GetCppChatMembershipData());
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
		return nullptr;
	}
}
UPubnubCallbackStop* UPubnubChannel::Join(FOnPubnubChannelMessageReceived MessageCallback, FString CustomData)
{
	return this->Join(MessageCallback, FPubnubChatMembershipData(CustomData, "", ""));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::SendText(FString Message, FPubnubSendTextParams SendTextParams)
{
	if(!IsInternalChannelValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(Message);

	try
	{
		InternalChannel->send_text(UPubnubChatUtilities::FStringToPubnubString(Message), SendTextParams.GetCppSendTextParams());
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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

		PUBNUB_RETURN_IF_EMPTY_CPP_VECTOR(CppChannels, nullptr);

		auto CppCallbackStop = InternalChannel->stream_updates_on(Pubnub::Vector<Pubnub::Channel>(std::move(CppChannels)), lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubChannel::IsPresent(FString UserID)
{
	if(!IsInternalChannelValid()) {return false;}
	
	PUBNUB_RETURN_IF_EMPTY(UserID, false);

	try
	{
		return InternalChannel->is_present(UPubnubChatUtilities::FStringToPubnubString(UserID));
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubChannel::SetRestrictions(FString UserID, FPubnubRestriction Restrictions)
{
	if(!IsInternalChannelValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(UserID);

	try
	{
		InternalChannel->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(UserID), Restrictions.GetCppRestriction());
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

FPubnubRestriction UPubnubChannel::GetUserRestrictions(UPubnubUser* User)
{
	if(!IsInternalChannelValid()) {return FPubnubRestriction();}

	PUBNUB_RETURN_IF_NULL(User, FPubnubRestriction());

	try
	{
		return FPubnubRestriction(InternalChannel->get_user_restrictions(*User->GetInternalUser()));
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubUsersRestrictionsWrapper();
}

UPubnubMessage* UPubnubChannel::GetMessage(FString Timetoken)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	PUBNUB_RETURN_IF_EMPTY(Timetoken, nullptr);
	
	try
	{
		auto CppMessage = InternalChannel->get_message(UPubnubChatUtilities::FStringToPubnubString(Timetoken));
		return UPubnubMessage::Create(CppMessage);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMembersResponseWrapper();
}

UPubnubMembership* UPubnubChannel::Invite(UPubnubUser* User)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	PUBNUB_RETURN_IF_NULL(User, nullptr);

	try
	{
		auto CppMembership = InternalChannel->invite(*User->GetInternalUser());
		return UPubnubMembership::Create(CppMembership);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

TArray<UPubnubMembership*> UPubnubChannel::InviteMultiple(TArray<UPubnubUser*> Users)
{
	TArray<UPubnubMembership*> FinalMemberships;
	if(!IsInternalChannelValid()) {return FinalMemberships;}

	auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);
	PUBNUB_RETURN_IF_EMPTY_CPP_VECTOR(CppUsers, FinalMemberships);

	try
	{
		auto CppMemberships = InternalChannel->invite_multiple(std::move(CppUsers));
		FinalMemberships = UPubnubChatUtilities::CppMembershipsToUnrealMemberships(CppMemberships);
	
		return FinalMemberships;
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubChannel* UPubnubChannel::PinMessage(UPubnubMessage* Message)
{
	if(!IsInternalChannelValid()) {return nullptr;}

	PUBNUB_RETURN_IF_NULL(Message, nullptr);

	try
	{
		return Create(InternalChannel->pin_message(*Message->GetInternalMessage()));
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubChannel::ForwardMessage(UPubnubMessage* Message)
{
	if(!IsInternalChannelValid()) {return;}

	PUBNUB_RETURN_IF_NULL(Message);

	try
	{
		InternalChannel->forward_message(*Message->GetInternalMessage());
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubChannel::EmitUserMention(FString UserID, FString Timetoken, FString Text)
{
	if(!IsInternalChannelValid()) {return;}

	PUBNUB_RETURN_IF_EMPTY(UserID);
	PUBNUB_RETURN_IF_EMPTY(Timetoken);
	PUBNUB_RETURN_IF_EMPTY(Text);

	try
	{
		InternalChannel->emit_user_mention(UPubnubChatUtilities::FStringToPubnubString(UserID), UPubnubChatUtilities::FStringToPubnubString(Timetoken), UPubnubChatUtilities::FStringToPubnubString(Text));
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
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
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubChannel::StreamMessageReports(FOnPubnubStreamMessageReportsReceived MessageReportsCallback)
{
	if(!IsInternalChannelValid()) {return nullptr;}
	
	try
	{
		auto lambda = [MessageReportsCallback](Pubnub::Event CppEvent)
		{
			AsyncTask(ENamedThreads::GameThread, [MessageReportsCallback, CppEvent]()
			{
				MessageReportsCallback.ExecuteIfBound(CppEvent);
			});
		};
	
		auto CppCallbackStop = InternalChannel->stream_message_reports(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FPubnubMessageReportsHistoryWrapper UPubnubChannel::GetMessageReportsHistory(FString StartTimetoken, FString EndTimetoken, int Count)
{
	if(!IsInternalChannelValid()) {return FPubnubMessageReportsHistoryWrapper();}
	
	try
	{
		auto CppWrapper = InternalChannel->get_messsage_reports_history(UPubnubChatUtilities::FStringToPubnubString(StartTimetoken), UPubnubChatUtilities::FStringToPubnubString(EndTimetoken), Count);
		auto UEWrapper = FPubnubMessageReportsHistoryWrapper(CppWrapper);
		return UEWrapper;
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMessageReportsHistoryWrapper();
}

UPubnubMessageDraft* UPubnubChannel::CreateMessageDraft(FPubnubMessageDraftConfig MessageDraftConfig)
{
	if(!IsInternalChannelValid()) {return nullptr;}
	
	try
	{
		return UPubnubMessageDraft::Create(InternalChannel->create_message_draft(MessageDraftConfig.GetCppMessageDraftConfig()));
	}
	catch(std::exception& Exception)
	{
		UPubnubLogUtilities::PrintFunctionError(ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}


bool UPubnubChannel::IsInternalChannelValid()
{
	if(InternalChannel == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("This PubnubChannel is invalid"));
		return false;
	}
	return true;
}
