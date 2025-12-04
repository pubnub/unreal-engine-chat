// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatObjectsRepository.h"
#include "Misc/ScopeLock.h"


void UPubnubChatObjectsRepository::RegisterUser(const FString& UserID)
{
	if (UserID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&UsersCriticalSection);
	
	// Increment reference count
	int32& Count = UserReferenceCounts.FindOrAdd(UserID, 0);
	Count++;
	
	// If this is the first reference, ensure data exists
	if (Count == 1)
	{
		if (!Users.Contains(UserID))
		{
			FPubnubChatInternalUser NewUser;
			NewUser.UserID = UserID;
			Users.Add(UserID, NewUser);
		}
	}
}

void UPubnubChatObjectsRepository::UnregisterUser(const FString& UserID)
{
	if (UserID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&UsersCriticalSection);
	
	int32* CountPtr = UserReferenceCounts.Find(UserID);
	if (!CountPtr)
	{
		return; // Already cleaned up or never registered
	}
	
	// Decrement reference count
	(*CountPtr)--;
	
	// If no more references, clean up data
	if (*CountPtr <= 0)
	{
		Users.Remove(UserID);
		UserReferenceCounts.Remove(UserID);
	}
}

void UPubnubChatObjectsRepository::RegisterChannel(const FString& ChannelID)
{
	if (ChannelID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&ChannelsCriticalSection);
	
	// Increment reference count
	int32& Count = ChannelReferenceCounts.FindOrAdd(ChannelID, 0);
	Count++;
	
	// If this is the first reference, ensure data exists
	if (Count == 1)
	{
		if (!Channels.Contains(ChannelID))
		{
			FPubnubChatInternalChannel NewChannel;
			NewChannel.ChannelID = ChannelID;
			Channels.Add(ChannelID, NewChannel);
		}
	}
}

void UPubnubChatObjectsRepository::UnregisterChannel(const FString& ChannelID)
{
	if (ChannelID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&ChannelsCriticalSection);
	
	int32* CountPtr = ChannelReferenceCounts.Find(ChannelID);
	if (!CountPtr)
	{
		return; // Already cleaned up or never registered
	}
	
	// Decrement reference count
	(*CountPtr)--;
	
	// If no more references, clean up data
	if (*CountPtr <= 0)
	{
		Channels.Remove(ChannelID);
		ChannelReferenceCounts.Remove(ChannelID);
	}
}

FPubnubChatInternalUser& UPubnubChatObjectsRepository::GetOrCreateUserData(const FString& UserID)
{
	FScopeLock Lock(&UsersCriticalSection);
	
	if (!Users.Contains(UserID))
	{
		FPubnubChatInternalUser NewUser;
		NewUser.UserID = UserID;
		Users.Add(UserID, NewUser);
	}
	return Users[UserID];
}

FPubnubChatInternalUser* UPubnubChatObjectsRepository::GetUserData(const FString& UserID)
{
	FScopeLock Lock(&UsersCriticalSection);
	return Users.Find(UserID);
}

void UPubnubChatObjectsRepository::UpdateUserData(const FString& UserID, const FPubnubChatUserData& UserData)
{
	FScopeLock Lock(&UsersCriticalSection);
	
	if (!Users.Contains(UserID))
	{
		FPubnubChatInternalUser NewUser;
		NewUser.UserID = UserID;
		Users.Add(UserID, NewUser);
	}
	
	FPubnubChatInternalUser& InternalUser = Users[UserID];
	InternalUser.UserData = UserData;
	InternalUser.LastUpdated = FDateTime::Now();
}

bool UPubnubChatObjectsRepository::RemoveUserData(const FString& UserID)
{
	FScopeLock Lock(&UsersCriticalSection);
	return Users.Remove(UserID) > 0;
}

FPubnubChatInternalChannel& UPubnubChatObjectsRepository::GetOrCreateChannelData(const FString& ChannelID)
{
	FScopeLock Lock(&ChannelsCriticalSection);
	
	if (!Channels.Contains(ChannelID))
	{
		FPubnubChatInternalChannel NewChannel;
		NewChannel.ChannelID = ChannelID;
		Channels.Add(ChannelID, NewChannel);
	}
	return Channels[ChannelID];
}

FPubnubChatInternalChannel* UPubnubChatObjectsRepository::GetChannelData(const FString& ChannelID)
{
	FScopeLock Lock(&ChannelsCriticalSection);
	return Channels.Find(ChannelID);
}

void UPubnubChatObjectsRepository::UpdateChannelData(const FString& ChannelID, const FPubnubChatChannelData& ChannelData)
{
	FScopeLock Lock(&ChannelsCriticalSection);
	
	if (!Channels.Contains(ChannelID))
	{
		FPubnubChatInternalChannel NewChannel;
		NewChannel.ChannelID = ChannelID;
		Channels.Add(ChannelID, NewChannel);
	}
	
	FPubnubChatInternalChannel& InternalChannel = Channels[ChannelID];
	InternalChannel.ChannelData = ChannelData;
	InternalChannel.LastUpdated = FDateTime::Now();
}

bool UPubnubChatObjectsRepository::RemoveChannelData(const FString& ChannelID)
{
	FScopeLock Lock(&ChannelsCriticalSection);
	return Channels.Remove(ChannelID) > 0;
}

void UPubnubChatObjectsRepository::ClearAll()
{
	// Lock all mutexes to ensure atomic clearing of all data
	FScopeLock UsersLock(&UsersCriticalSection);
	FScopeLock ChannelsLock(&ChannelsCriticalSection);
	
	Users.Empty();
	Channels.Empty();
	UserReferenceCounts.Empty();
	ChannelReferenceCounts.Empty();
}

