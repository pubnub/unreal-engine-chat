// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubUser.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "PubnubChannel.h"
#include "PubnubMembership.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


FPubnubMembershipsResponseWrapper::FPubnubMembershipsResponseWrapper(Pubnub::MembershipsResponseWrapper& Wrapper) :
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

FPubnubChannelsRestrictionsWrapper::FPubnubChannelsRestrictionsWrapper(Pubnub::ChannelsRestrictionsWrapper& Wrapper) :
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


UPubnubUser* UPubnubUser::Create(Pubnub::User User)
{
	UPubnubUser* NewUser = NewObject<UPubnubUser>();
	NewUser->InternalUser = new Pubnub::User(User);
	return NewUser;
}

FString UPubnubUser::GetUserID()
{
	if(!IsInternalUserValid()) {return "";}

	try
	{
		return UPubnubChatUtilities::PubnubStringToFString(InternalUser->user_id());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get User ID error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return "";
}

FPubnubChatUserData UPubnubUser::GetUserData()
{
	if(!IsInternalUserValid()) {return FPubnubChatUserData();}

	try
	{
		return FPubnubChatUserData(InternalUser->user_data());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Get User Data error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChatUserData();
}

UPubnubUser* UPubnubUser::Update(FPubnubChatUserData UserData)
{
	if(!IsInternalUserValid()) {return nullptr;}

	try
	{
		auto CppUser = InternalUser->update(UserData.GetCppChatUserData());
		return Create(CppUser);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Update error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

void UPubnubUser::DeleteUser()
{
	if(!IsInternalUserValid()) {return;}

	try
	{
		InternalUser->delete_user();
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Delete User error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

TArray<FString> UPubnubUser::WherePresent()
{
	if(!IsInternalUserValid()) {return {};}

	try
	{
		auto ResponseChannels = InternalUser->where_present();
		TArray<FString> PresentChannels = UPubnubChatUtilities::PubnubStringsToFStrings(ResponseChannels);
	
		return PresentChannels;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Where Present error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return {};
}

bool UPubnubUser::IsPresentOn(FString ChannelID)
{
	if(!IsInternalUserValid()) {return false;}

	try
	{
		return InternalUser->is_present_on(UPubnubChatUtilities::FStringToPubnubString(ChannelID));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Is Present On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return false;
}

void UPubnubUser::SetRestrictions(FString ChannelID, FPubnubRestriction Restrictions)
{
	if(!IsInternalUserValid()) {return;}

	try
	{
		InternalUser->set_restrictions(UPubnubChatUtilities::FStringToPubnubString(ChannelID), Restrictions.GetCppRestriction());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Set Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

FPubnubRestriction UPubnubUser::GetChannelRestrictions(UPubnubChannel* Channel)
{
	if(!IsInternalUserValid()) {return FPubnubRestriction();}

	try
	{
		return FPubnubRestriction(InternalUser->get_channel_restrictions(*Channel->GetInternalChannel()));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Get Channel Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubRestriction();
}

FPubnubChannelsRestrictionsWrapper UPubnubUser::GetChannelsRestrictions(FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalUserValid()) {return FPubnubChannelsRestrictionsWrapper();}

	try
	{
		auto CppWrapper = InternalUser->get_channels_restrictions(UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		return FPubnubChannelsRestrictionsWrapper(CppWrapper);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Get Channels Restrictions error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubChannelsRestrictionsWrapper();
}

//Deprecated in JS chat
/*void UPubnubUser::Report(FString Reason)
{
	if(!IsInternalUserValid()) {return;}

	try
	{
		InternalUser->report(UPubnubChatUtilities::FStringToPubnubString(Reason));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Report error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}*/

FPubnubMembershipsResponseWrapper UPubnubUser::GetMemberships(FString Filter, FString Sort, int Limit, FPubnubPage Page)
{
	if(!IsInternalUserValid()) {return FPubnubMembershipsResponseWrapper();}

	try
	{
		auto CppWrapper = InternalUser->get_memberships(UPubnubChatUtilities::FStringToPubnubString(Filter), UPubnubChatUtilities::FStringToPubnubString(Sort), Limit, Page.GetCppPage());
		auto UEWrapper = FPubnubMembershipsResponseWrapper(CppWrapper);
	
		return UEWrapper;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Get Memberships error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return FPubnubMembershipsResponseWrapper();
}

UPubnubCallbackStop* UPubnubUser::StreamUpdates(FOnPubnubUserStreamUpdateReceived UserUpdateCallback)
{
	if(!IsInternalUserValid()) {return nullptr;}

	try
	{
		auto lambda = [UserUpdateCallback](Pubnub::User User)
		{
			AsyncTask(ENamedThreads::GameThread, [UserUpdateCallback, User]()
			{
				UPubnubUser* NewUser = UPubnubUser::Create(User);
				UserUpdateCallback.ExecuteIfBound(NewUser);
			});

		};
		auto CppCallbackStop = InternalUser->stream_updates(lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Stream Updates error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

UPubnubCallbackStop* UPubnubUser::StreamUpdatesOn(TArray<UPubnubUser*> Users, FOnPubnubUsersStreamUpdateOnReceived UserUpdateCallback)
{
	if(!IsInternalUserValid()) {return nullptr;}

	try
	{
		auto lambda = [UserUpdateCallback](Pubnub::Vector<Pubnub::User> Users)
		{
			auto StdUsers = Users.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [UserUpdateCallback, StdUsers]()
			{
				TArray<UPubnubUser*> FinalUsers;
			
				for(auto CppUser : StdUsers)
				{
					UPubnubUser* NewUser = UPubnubUser::Create(CppUser);
					FinalUsers.Add(NewUser);
				}
				UserUpdateCallback.ExecuteIfBound(FinalUsers);
			});

		};

		auto CppUsers = UPubnubChatUtilities::UnrealUsersToCppUsers(Users);

		auto CppCallbackStop = InternalUser->stream_updates_on(CppUsers, lambda);
		return UPubnubCallbackStop::Create(CppCallbackStop);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("User Stream Updates On error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
}

bool UPubnubUser::IsInternalUserValid()
{
	if(InternalUser == nullptr)
	{
		UE_LOG(PubnubLog, Error, TEXT("This PubnubUser is invalid"));
		return false;
	}
	return true;
}
