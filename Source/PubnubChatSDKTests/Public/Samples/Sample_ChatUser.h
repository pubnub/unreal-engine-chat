// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"

// snippet.end

#include "Sample_ChatUser.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatUser : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_user_data
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetUserDataSample();

	// snippet.get_user_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetUserIDSample();

	// snippet.update_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void UpdateUserSample();

	// snippet.delete_user
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void DeleteUserSample();

	// snippet.is_active
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void IsActiveSample();

	// snippet.get_last_active_timestamp
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetLastActiveTimestampSample();

	// snippet.where_present
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void WherePresentSample();

	UFUNCTION()
	void OnWherePresentResponse(const FPubnubChatWherePresentResult& Result);

	// snippet.is_present_on
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void IsPresentOnSample();

	UFUNCTION()
	void OnIsPresentOnResponse(const FPubnubChatIsPresentResult& Result);

	// snippet.get_memberships
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetMembershipsSample();

	UFUNCTION()
	void OnGetMembershipsResponse(const FPubnubChatMembershipsResult& Result);

	// snippet.set_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void SetRestrictionsSample();

	// snippet.get_channel_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetChannelRestrictionsSample();

	UFUNCTION()
	void OnGetChannelRestrictionsResponse(const FPubnubChatGetRestrictionResult& Result);

	// snippet.get_channels_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetChannelsRestrictionsSample();

	UFUNCTION()
	void OnGetChannelsRestrictionsResponse(const FPubnubChatGetRestrictionsResult& Result);

	// snippet.stream_updates
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void StreamUpdatesSample();

	void OnUserUpdateReceived(EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData);

	// snippet.end
};
