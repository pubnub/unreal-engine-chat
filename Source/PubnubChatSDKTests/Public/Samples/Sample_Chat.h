// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"

// snippet.end

#include "Sample_Chat.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_Chat : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_current_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void GetCurrentUserSample();

	// snippet.create_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void CreateUserSample();
	
	UFUNCTION()
	void OnCreateUserResponse(const FPubnubChatUserResult& Result);

	// snippet.chat_get_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void GetUserSample();

	UFUNCTION()
	void OnGetUserResponse(const FPubnubChatUserResult& Result);

	// snippet.get_users
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void GetUsersSample();

	UFUNCTION()
	void OnGetUsersResponse(const FPubnubChatGetUsersResult& Result);

	// snippet.chat_update_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void UpdateUserSample();

	UFUNCTION()
	void OnUpdateUserResponse(const FPubnubChatUserResult& Result);

	// snippet.chat_delete_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void DeleteUserSample();

	UFUNCTION()
	void OnDeleteUserResponse(const FPubnubChatOperationResult& Result);

	// snippet.get_user_suggestions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|User")
	void GetUserSuggestionsSample();

	UFUNCTION()
	void OnGetUserSuggestionsResponse(const FPubnubChatGetUserSuggestionsResult& Result);

	// snippet.create_public_conversation
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void CreatePublicConversationSample();

	UFUNCTION()
	void OnCreatePublicConversationResponse(const FPubnubChatChannelResult& Result);

	// snippet.create_group_conversation
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void CreateGroupConversationSample();

	UFUNCTION()
	void OnCreateGroupConversationResponse(const FPubnubChatCreateGroupConversationResult& Result);

	// snippet.create_direct_conversation
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void CreateDirectConversationSample();

	UFUNCTION()
	void OnCreateDirectConversationResponse(const FPubnubChatCreateDirectConversationResult& Result);

	// snippet.chat_get_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void GetChannelSample();

	UFUNCTION()
	void OnGetChannelResponse(const FPubnubChatChannelResult& Result);

	// snippet.get_channels
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void GetChannelsSample();

	UFUNCTION()
	void OnGetChannelsResponse(const FPubnubChatGetChannelsResult& Result);

	// snippet.chat_update_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void UpdateChannelSample();

	UFUNCTION()
	void OnUpdateChannelResponse(const FPubnubChatChannelResult& Result);

	// snippet.delete_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void DeleteChannelSample();

	UFUNCTION()
	void OnDeleteChannelResponse(const FPubnubChatOperationResult& Result);

	// snippet.pin_message_to_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void PinMessageToChannelSample();

	UFUNCTION()
	void OnPinMessageToChannelResponse(const FPubnubChatOperationResult& Result);

	// snippet.unpin_message_from_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void UnpinMessageFromChannelSample();

	UFUNCTION()
	void OnUnpinMessageFromChannelResponse(const FPubnubChatOperationResult& Result);

	// snippet.get_channel_suggestions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Channel")
	void GetChannelSuggestionsSample();

	UFUNCTION()
	void OnGetChannelSuggestionsResponse(const FPubnubChatGetChannelSuggestionsResult& Result);

	// snippet.chat_where_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Presence")
	void WherePresentSample();

	UFUNCTION()
	void OnWherePresentResponse(const FPubnubChatWherePresentResult& Result);

	// snippet.chat_who_is_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Presence")
	void WhoIsPresentSample();

	UFUNCTION()
	void OnWhoIsPresentResponse(const FPubnubChatWhoIsPresentResult& Result);

	// snippet.chat_is_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Presence")
	void IsPresentSample();

	UFUNCTION()
	void OnIsPresentResponse(const FPubnubChatIsPresentResult& Result);

	// snippet.chat_set_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Moderation")
	void SetRestrictionsSample();

	UFUNCTION()
	void OnSetRestrictionsResponse(const FPubnubChatOperationResult& Result);

	// snippet.emit_chat_event
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Moderation")
	void EmitChatEventSample();

	UFUNCTION()
	void OnEmitChatEventResponse(const FPubnubChatOperationResult& Result);

	// snippet.get_events_history
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Moderation")
	void GetEventsHistorySample();

	UFUNCTION()
	void OnGetEventsHistoryResponse(const FPubnubChatEventsResult& Result);

	// snippet.chat_forward_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Messages")
	void ForwardMessageSample();

	UFUNCTION()
	void OnForwardMessageResponse(const FPubnubChatOperationResult& Result);

	// snippet.get_unread_messages_counts
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Messages")
	void GetUnreadMessagesCountsSample();

	UFUNCTION()
	void OnGetUnreadMessagesCountsResponse(const FPubnubChatGetUnreadMessagesCountsResult& Result);

	// snippet.mark_all_messages_as_read
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Messages")
	void MarkAllMessagesAsReadSample();

	UFUNCTION()
	void OnMarkAllMessagesAsReadResponse(const FPubnubChatMarkAllMessagesAsReadResult& Result);

	// snippet.create_thread_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Threads")
	void CreateThreadChannelSample();

	UFUNCTION()
	void OnCreateThreadChannelResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.get_thread_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Threads")
	void GetThreadChannelSample();

	UFUNCTION()
	void OnGetThreadChannelResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.remove_thread_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Threads")
	void RemoveThreadChannelSample();

	UFUNCTION()
	void OnRemoveThreadChannelResponse(const FPubnubChatOperationResult& Result);

	// snippet.reconnect_subscriptions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Connection Status")
	void ReconnectSubscriptionsSample();

	UFUNCTION()
	void OnReconnectSubscriptionsResponse(const FPubnubChatOperationResult& Result);

	// snippet.disconnect_subscriptions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Connection Status")
	void DisconnectSubscriptionsSample();

	UFUNCTION()
	void OnDisconnectSubscriptionsResponse(const FPubnubChatOperationResult& Result);

	// snippet.connection_status_changed
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Connection Status")
	void ConnectionStatusChangedSample();

	void OnConnectionStatusChanged(EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData);

	// snippet.connection_status_changed_reconnect
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|Chat|Connection Status")
	void ConnectionStatusChangedReconnectSample();
	
	// snippet.end

};
