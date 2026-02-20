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
	// blueprint.mrz3chqy
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetMembershipDataSample();

	// snippet.membership_get_user
	// blueprint.728h6-nl
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUserSample();

	// snippet.membership_get_channel
	// blueprint.5ifhpyz5
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetChannelSample();

	// snippet.membership_get_user_id
	// blueprint.s0exvs7w
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUserIDSample();

	// snippet.membership_get_channel_id
	// blueprint.kap6sob8
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetChannelIDSample();

	// snippet.get_last_read_message_timetoken
	// blueprint.d72i5f4z
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetLastReadMessageTimetokenSample();

	// snippet.delete_membership
	// blueprint.q6in72b6
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void DeleteMembershipSample();

	// snippet.update_membership
	// blueprint.pe9tpr-6
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void UpdateMembershipSample();

	// snippet.set_last_read_message_timetoken
	// blueprint.sth629iy
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void SetLastReadMessageTimetokenSample();

	// snippet.set_last_read_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void SetLastReadMessageSample();

	// snippet.stream_membership_updates
	// blueprint.e15f_qdb
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void StreamUpdatesSample();

	void OnMembershipUpdateReceived(FString ChannelID, FString UserID, const FPubnubChatMembershipData& MembershipData);
	void OnMembershipDeleted();

	// snippet.get_unread_messages_count
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMembership")
	void GetUnreadMessagesCountSample();

	UFUNCTION()
	void OnGetUnreadMessagesCountResponse(const FPubnubChatGetUnreadMessagesCountResult& Result);

	// snippet.end
};
