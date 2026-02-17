// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatChannel.h"

// snippet.end

#include "Sample_ChatChannel.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatChannel : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_channel_data
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetChannelDataSample();

	// snippet.channel_get_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetChannelIDSample();

	// snippet.channel_update
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void UpdateChannelSample();

	UFUNCTION()
	void OnUpdateChannelResponse(const FPubnubChatOperationResult& Result);

	// snippet.connect
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void ConnectSample();

	void OnChannelMessageReceived(UPubnubChatMessage* Message);

	// snippet.join
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void JoinSample();

	void OnChannelMessageReceived_JoinSample(UPubnubChatMessage* Message);

	UFUNCTION()
	void OnJoinResponse(const FPubnubChatJoinResult& Result);

	// snippet.disconnect
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void DisconnectSample();

	// snippet.leave
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void LeaveSample();

	// snippet.send_text
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void SendTextSample();

	// snippet.invite
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void InviteSample();

	UFUNCTION()
	void OnInviteResponse(const FPubnubChatInviteResult& Result);

	// snippet.invite_multiple
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void InviteMultipleSample();

	UFUNCTION()
	void OnInviteMultipleResponse(const FPubnubChatInviteMultipleResult& Result);

	// snippet.channel_pin_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void PinMessageSample();

	// snippet.channel_unpin_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void UnpinMessageSample();

	// snippet.get_pinned_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetPinnedMessageSample();

	UFUNCTION()
	void OnGetPinnedMessageResponse(const FPubnubChatMessageResult& Result);

	// snippet.channel_who_is_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void WhoIsPresentSample();

	UFUNCTION()
	void OnWhoIsPresentResponse(const FPubnubChatWhoIsPresentResult& Result);

	// snippet.channel_is_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void IsPresentSample();

	UFUNCTION()
	void OnIsPresentResponse(const FPubnubChatIsPresentResult& Result);

	// snippet.delete
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void DeleteSample();

	// snippet.get_members
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetMembersSample();

	UFUNCTION()
	void OnGetMembersResponse(const FPubnubChatMembershipsResult& Result);

	// snippet.get_invitees
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetInviteesSample();

	UFUNCTION()
	void OnGetInviteesResponse(const FPubnubChatMembershipsResult& Result);

	// snippet.channel_set_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void SetRestrictionsSample();
	
	// snippet.lift_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void LiftRestrictionsSample();

	// snippet.get_user_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetUserRestrictionsSample();

	UFUNCTION()
	void OnGetUserRestrictionsResponse(const FPubnubChatGetRestrictionResult& Result);

	// snippet.get_users_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetUsersRestrictionsSample();

	UFUNCTION()
	void OnGetUsersRestrictionsResponse(const FPubnubChatGetRestrictionsResult& Result);

	// snippet.get_history
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetHistorySample();

	UFUNCTION()
	void OnGetHistoryResponse(const FPubnubChatGetHistoryResult& Result);

	// snippet.get_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetMessageSample();

	UFUNCTION()
	void OnGetMessageResponse(const FPubnubChatMessageResult& Result);

	// snippet.channel_forward_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void ForwardMessageSample();

	// snippet.emit_user_mention
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void EmitUserMentionSample();

	// snippet.create_message_draft
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void CreateMessageDraftSample();

	// snippet.get_message_reports_history
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void GetMessageReportsHistorySample();

	UFUNCTION()
	void OnGetMessageReportsHistoryResponse(const FPubnubChatEventsResult& Result);

	// snippet.stream_channel_updates
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void StreamUpdatesSample();

	void OnChannelUpdateReceived(EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, const FPubnubChatChannelData& ChannelData);

	// snippet.stream_typing
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void StreamTypingSample();

	void OnTypingReceived(const TArray<FString>& TypingUserIDs);

	// snippet.stream_read_receipts
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void StreamReadReceiptsSample();

	void OnReadReceiptReceived(const FPubnubChatReadReceipts& ReadReceipts);

	// snippet.stream_message_reports
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatChannel")
	void StreamMessageReportsSample();

	void OnMessageReportReceived(const FPubnubChatEvent& ReportEvent);

	// snippet.end
};
