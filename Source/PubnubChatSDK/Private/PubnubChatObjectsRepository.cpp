// Copyright 2026 PubNub Inc. All Rights Reserved.

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

void UPubnubChatObjectsRepository::RegisterMessage(const FString& MessageID)
{
	if (MessageID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&MessagesCriticalSection);
	
	// Increment reference count
	int32& Count = MessageReferenceCounts.FindOrAdd(MessageID, 0);
	Count++;
	
	// If this is the first reference, ensure data exists
	if (Count == 1)
	{
		if (!Messages.Contains(MessageID))
		{
			FPubnubChatInternalMessage NewMessage;
			NewMessage.MessageID = MessageID;
			Messages.Add(MessageID, NewMessage);
		}
	}
}

void UPubnubChatObjectsRepository::UnregisterMessage(const FString& MessageID)
{
	if (MessageID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&MessagesCriticalSection);
	
	int32* CountPtr = MessageReferenceCounts.Find(MessageID);
	if (!CountPtr)
	{
		return; // Already cleaned up or never registered
	}
	
	// Decrement reference count
	(*CountPtr)--;
	
	// If no more references, clean up data
	if (*CountPtr <= 0)
	{
		Messages.Remove(MessageID);
		MessageReferenceCounts.Remove(MessageID);
	}
}

FPubnubChatInternalMessage* UPubnubChatObjectsRepository::GetMessageData(const FString& MessageID)
{
	FScopeLock Lock(&MessagesCriticalSection);
	return Messages.Find(MessageID);
}

void UPubnubChatObjectsRepository::UpdateMessageData(const FString& MessageID, const FPubnubChatMessageData& MessageData)
{
	FScopeLock Lock(&MessagesCriticalSection);
	
	if (!Messages.Contains(MessageID))
	{
		FPubnubChatInternalMessage NewMessage;
		NewMessage.MessageID = MessageID;
		Messages.Add(MessageID, NewMessage);
	}
	
	FPubnubChatInternalMessage& InternalMessage = Messages[MessageID];
	InternalMessage.MessageData = MessageData;
	InternalMessage.LastUpdated = FDateTime::Now();
}

bool UPubnubChatObjectsRepository::RemoveMessageData(const FString& MessageID)
{
	FScopeLock Lock(&MessagesCriticalSection);
	return Messages.Remove(MessageID) > 0;
}

void UPubnubChatObjectsRepository::RegisterMembership(const FString& MembershipID)
{
	if (MembershipID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&MembershipsCriticalSection);
	
	// Increment reference count
	int32& Count = MembershipReferenceCounts.FindOrAdd(MembershipID, 0);
	Count++;
	
	// If this is the first reference, ensure data exists
	if (Count == 1)
	{
		if (!Memberships.Contains(MembershipID))
		{
			FPubnubChatInternalMembership NewMembership;
			NewMembership.MembershipID = MembershipID;
			Memberships.Add(MembershipID, NewMembership);
		}
	}
}

void UPubnubChatObjectsRepository::UnregisterMembership(const FString& MembershipID)
{
	if (MembershipID.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&MembershipsCriticalSection);
	
	int32* CountPtr = MembershipReferenceCounts.Find(MembershipID);
	if (!CountPtr)
	{
		return; // Already cleaned up or never registered
	}
	
	// Decrement reference count
	(*CountPtr)--;
	
	// If no more references, clean up data
	if (*CountPtr <= 0)
	{
		Memberships.Remove(MembershipID);
		MembershipReferenceCounts.Remove(MembershipID);
	}
}

FPubnubChatInternalMembership* UPubnubChatObjectsRepository::GetMembershipData(const FString& MembershipID)
{
	FScopeLock Lock(&MembershipsCriticalSection);
	return Memberships.Find(MembershipID);
}

void UPubnubChatObjectsRepository::UpdateMembershipData(const FString& MembershipID, const FPubnubChatMembershipData& MembershipData)
{
	FScopeLock Lock(&MembershipsCriticalSection);
	
	if (!Memberships.Contains(MembershipID))
	{
		FPubnubChatInternalMembership NewMembership;
		NewMembership.MembershipID = MembershipID;
		Memberships.Add(MembershipID, NewMembership);
	}
	
	FPubnubChatInternalMembership& InternalMembership = Memberships[MembershipID];
	InternalMembership.MembershipData = MembershipData;
	InternalMembership.LastUpdated = FDateTime::Now();
}

bool UPubnubChatObjectsRepository::RemoveMembershipData(const FString& MembershipID)
{
	FScopeLock Lock(&MembershipsCriticalSection);
	return Memberships.Remove(MembershipID) > 0;
}

void UPubnubChatObjectsRepository::ClearAll()
{
	// Lock all mutexes to ensure atomic clearing of all data
	FScopeLock UsersLock(&UsersCriticalSection);
	FScopeLock ChannelsLock(&ChannelsCriticalSection);
	FScopeLock MessagesLock(&MessagesCriticalSection);
	FScopeLock MembershipsLock(&MembershipsCriticalSection);
	
	Users.Empty();
	Channels.Empty();
	Messages.Empty();
	Memberships.Empty();
	UserReferenceCounts.Empty();
	ChannelReferenceCounts.Empty();
	MessageReferenceCounts.Empty();
	MembershipReferenceCounts.Empty();
}

