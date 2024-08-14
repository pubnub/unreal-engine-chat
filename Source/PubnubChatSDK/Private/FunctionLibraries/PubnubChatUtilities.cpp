// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibraries/PubnubChatUtilities.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChannel.h"
#include "PubnubUser.h"
#include "PubnubMessage.h"
#include "PubnubMembership.h"
#include "PubnubThreadMessage.h"
#include "PubnubThreadChannel.h"


FString UPubnubChatUtilities::PubnubStringToFString(Pubnub::String PubnubString)
{
	return FString(PubnubString);
}

Pubnub::String UPubnubChatUtilities::FStringToPubnubString(FString UEString)
{
	if(UEString.IsEmpty())
	{
		return Pubnub::String();
	}
	return Pubnub::String(TCHAR_TO_UTF8(*UEString));
}

TArray<FString> UPubnubChatUtilities::PubnubStringsToFStrings(Pubnub::Vector<Pubnub::String>& PubnubStrings)
{
	TArray<FString> UEStrings;

	auto PubnubStdStrings = PubnubStrings.into_std_vector();

	for(auto &PubnubString : PubnubStdStrings)
	{
		UEStrings.Add(PubnubStringToFString(PubnubString));
	}
	return UEStrings;
}

Pubnub::Vector<Pubnub::String> UPubnubChatUtilities::FStringsToPubnubStrings(TArray<FString>& UEStrings)
{
	Pubnub::Vector<Pubnub::String> CppStrings;

	for(auto &UEString : UEStrings)
	{
		CppStrings.push_back(FStringToPubnubString(UEString));
	}
	return CppStrings;
}

TArray<UPubnubChannel*> UPubnubChatUtilities::CppChannelsToUnrealChannels(Pubnub::Vector<Pubnub::Channel> &CppChannels)
{
	TArray<UPubnubChannel*> PubnubChannels;
	auto CppStdChannels = CppChannels.into_std_vector();

	for(auto &CppChannel : CppStdChannels)
	{
		PubnubChannels.Add(UPubnubChannel::Create(CppChannel));
	}
	return PubnubChannels;
}

TArray<UPubnubUser*> UPubnubChatUtilities::CppUsersToUnrealUsers(Pubnub::Vector<Pubnub::User> &CppUsers)
{
	TArray<UPubnubUser*> PubnubUsers;
	auto CppStdUsers = CppUsers.into_std_vector();

	for(auto &CppUser : CppStdUsers)
	{
		PubnubUsers.Add(UPubnubUser::Create(CppUser));
	}
	return PubnubUsers;
}

TArray<UPubnubMessage*> UPubnubChatUtilities::CppMessagesToUnrealMessages(Pubnub::Vector<Pubnub::Message> &CppMessages)
{
	TArray<UPubnubMessage*> PubnubMessages;
	auto CppStdMessages = CppMessages.into_std_vector();

	for(auto &CppMessage : CppStdMessages)
	{
		PubnubMessages.Add(UPubnubMessage::Create(CppMessage));
	}
	return PubnubMessages;
}

TArray<UPubnubThreadMessage*> UPubnubChatUtilities::CppThreadMessagesToUnrealTMessages(Pubnub::Vector<Pubnub::ThreadMessage>& CppThreadMessages)
{
	TArray<UPubnubThreadMessage*> PubnubMessages;
	auto CppStdMessages = CppThreadMessages.into_std_vector();

	for(auto &CppMessage : CppStdMessages)
	{
		PubnubMessages.Add(UPubnubThreadMessage::Create(CppMessage));
	}
	return PubnubMessages;
}

TArray<UPubnubMembership*> UPubnubChatUtilities::CppMembershipsToUnrealMemberships(Pubnub::Vector<Pubnub::Membership> &CppMemberships)
{
	TArray<UPubnubMembership*> PubnubMembership;
	auto CppStdMemberships = CppMemberships.into_std_vector();

	for(auto &CppMembership : CppStdMemberships)
	{
		PubnubMembership.Add(UPubnubMembership::Create(CppMembership));
	}
	return PubnubMembership;
}

TArray<FPubnubMessageAction> UPubnubChatUtilities::CppMessageActionsToUnrealMessageActions(Pubnub::Vector<Pubnub::MessageAction>& CppMessageActions)
{
	TArray<FPubnubMessageAction> PubnubMessageActions;
	auto CppStdMessageActions = CppMessageActions.into_std_vector();

	for(auto &CppMessageAction : CppStdMessageActions)
	{
		PubnubMessageActions.Add(FPubnubMessageAction(CppMessageAction));
	}
	return PubnubMessageActions;
}

Pubnub::Vector<Pubnub::Channel> UPubnubChatUtilities::UnrealChannelsToCppChannels(TArray<UPubnubChannel*> &PubnubChannels)
{
	//Pubnub::Vector<Pubnub::Channel> CppChannels;
	std::vector<Pubnub::Channel> CppChannels;

	for(auto &PubnubChannel : PubnubChannels)
	{
		CppChannels.push_back(*PubnubChannel->GetInternalChannel());
	}
	return Pubnub::Vector<Pubnub::Channel>(std::move(CppChannels));
}

Pubnub::Vector<Pubnub::User> UPubnubChatUtilities::UnrealUsersToCppUsers(TArray<UPubnubUser*> &PubnubUsers)
{
	Pubnub::Vector<Pubnub::User> CppUsers;

	for(auto &PubnubUser : PubnubUsers)
	{
		CppUsers.push_back(*PubnubUser->GetInternalUser());
	}
	return CppUsers;
}

Pubnub::Vector<Pubnub::Message> UPubnubChatUtilities::UnrealMessagesToCppMessages(TArray<UPubnubMessage*> &PubnubMessages)
{
	Pubnub::Vector<Pubnub::Message> CppMessages;

	for(auto &PubnubMessage : PubnubMessages)
	{
		CppMessages.push_back(*PubnubMessage->GetInternalMessage());
	}
	return CppMessages;
}

Pubnub::Vector<Pubnub::ThreadMessage> UPubnubChatUtilities::UnrealThreadMessagesToCppTMessages(TArray<UPubnubThreadMessage*>& PubnubThreadMessages)
{
	Pubnub::Vector<Pubnub::ThreadMessage> CppMessages;

	for(auto &PubnubMessage : PubnubThreadMessages)
	{
		CppMessages.push_back(*PubnubMessage->GetInternalThreadMessage());
	}
	return CppMessages;
}

Pubnub::Vector<Pubnub::Membership> UPubnubChatUtilities::UnrealMembershipsToCppMemberships(TArray<UPubnubMembership*> &PubnubMemberships)
{
	Pubnub::Vector<Pubnub::Membership> CppMemberships;

	for(auto &PubnubMembership : PubnubMemberships)
	{
		CppMemberships.push_back(*PubnubMembership->GetInternalMembership());
	}
	return CppMemberships;
}

Pubnub::Vector<Pubnub::MessageAction> UPubnubChatUtilities::UnrealMessageActionsToCppMessageActions(TArray<FPubnubMessageAction>& PubnubMessageActions)
{
	Pubnub::Vector<Pubnub::MessageAction> CppMessageActions;

	for(auto &PubnubMessageAction : PubnubMessageActions)
	{
		CppMessageActions.push_back(PubnubMessageAction.GetCppMessageAction());
	}
	return CppMessageActions;
}
