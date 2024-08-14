// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubMembership.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChannel.h"
#include "PubnubMessage.h"
#include "PubnubUser.h"
#include "PubnubCallbackStop.h"
#include "FunctionLibraries/PubnubChatUtilities.h"

UPubnubMembership* UPubnubMembership::Create(Pubnub::Membership Membership)
{
	UPubnubMembership* NewMembership = NewObject<UPubnubMembership>();
	NewMembership->InternalMembership = new Pubnub::Membership(Membership);
	Channel = UPubnubChannel::Create(Membership.channel);
	User = UPubnubUser::Create(Membership.user);
	return NewMembership;
}

FString UPubnubMembership::GetCustomData()
{
	if(!IsInternalMembershipValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalMembership->custom_data());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Get Custom Data error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

UPubnubMembership* UPubnubMembership::Update(FString CustomData)
{
	if(!IsInternalMembershipValid()) {return nullptr;}

	try
	{
		auto CppMembership = InternalMembership->update(UPubnubChatUtilities::FStringToPubnubString(CustomData));
		return Create(CppMembership);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Update error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubMembership::StreamUpdates(FOnPubnubMembershipStreamUpdateReceived MembershipUpdateCallback)
{
	if(!IsInternalMembershipValid()) {return nullptr;}

	try
	{
		auto lambda = [MembershipUpdateCallback](Pubnub::Membership Membership)
		{
			AsyncTask(ENamedThreads::GameThread, [MembershipUpdateCallback, Membership]()
			{
				UPubnubMembership* NewMembership = UPubnubMembership::Create(Membership);
				MembershipUpdateCallback.ExecuteIfBound(NewMembership);
			});

		};
		auto CppCallbackStop = InternalMembership->stream_updates(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Stream Updates error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubMembership::StreamUpdatesOn(TArray<UPubnubMembership*> Memberships, FOnPubnubMembershipsStreamUpdateOnReceived MembershipUpdateCallback)
{
	if(!IsInternalMembershipValid()) {return nullptr;}

	try
	{
		auto lambda = [MembershipUpdateCallback](Pubnub::Vector<Pubnub::Membership> Memberships)
		{
			auto StdMemberships = Memberships.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [MembershipUpdateCallback, StdMemberships]()
			{
				TArray<UPubnubMembership*> FinalMemberships;
			
				for(auto CppMembership : StdMemberships)
				{
					UPubnubMembership* NewMembership = UPubnubMembership::Create(CppMembership);
					FinalMemberships.Add(NewMembership);
				}
				MembershipUpdateCallback.ExecuteIfBound(FinalMemberships);
			});
		};
	
		auto CppMemberships = UPubnubChatUtilities::UnrealMembershipsToCppMemberships(Memberships);

		auto CppCallbackStop = InternalMembership->stream_updates_on(CppMemberships, lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Stream Updates On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

FString UPubnubMembership::LastReadMessageTimetoken()
{
	if(!IsInternalMembershipValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalMembership->last_read_message_timetoken());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Last Read Message Timetoken error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

UPubnubMembership* UPubnubMembership::SetLastReadMessageTimetoken(FString Timetoken)
{
	if(!IsInternalMembershipValid()) {return nullptr;}

	try
	{
		auto CppMembership = InternalMembership->set_last_read_message_timetoken(UPubnubChatUtilities::FStringToPubnubString(Timetoken));

		return Create(CppMembership);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Set Last Read Message Timetoken error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubMembership* UPubnubMembership::SetLastReadMessage(UPubnubMessage* Message)
{
	if(!IsInternalMembershipValid() || !Message) {return nullptr;}

	try
	{
		auto CppMembership = InternalMembership->set_last_read_message(*Message->GetInternalMessage());

		return Create(CppMembership);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Set Last Read Message error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

int UPubnubMembership::GetUnreadMessageCount()
{
	if(!IsInternalMembershipValid()) {return 0;}

	try
	{
		return InternalMembership->get_unread_messages_count();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Membership Get Unread Message Count error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return 0;
}

bool UPubnubMembership::IsInternalMembershipValid()
{
	if(InternalMembership == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubMembership is invalid"));
		return false;
	}
	return true;
}
