// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatMembership.h"
#include "PubnubChatMessage.h"

// snippet.end

#include "Sample_ChatMembership.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatMembership : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_membership_data
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetMembershipDataSample();

	// snippet.get_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUserSample();

	// snippet.get_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetChannelSample();

	// snippet.get_user_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUserIDSample();

	// snippet.get_channel_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetChannelIDSample();

	// snippet.get_last_read_message_timetoken
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetLastReadMessageTimetokenSample();

	// snippet.delete_membership
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void DeleteMembershipSample();

	// snippet.update_membership
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void UpdateMembershipSample();

	// snippet.set_last_read_message_timetoken
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void SetLastReadMessageTimetokenSample();

	// snippet.set_last_read_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void SetLastReadMessageSample();

	// snippet.stream_updates
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void StreamUpdatesSample();

	void OnMembershipUpdateReceived(EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, FString UserID, const FPubnubChatMembershipData& MembershipData);

	// snippet.get_unread_messages_count
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUnreadMessagesCountSample();

	UFUNCTION()
	void OnGetUnreadMessagesCountResponse(const FPubnubChatGetUnreadMessagesCountResult& Result);

	// snippet.end
};
