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
	// blueprint.px82xfxe
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetUserDataSample();

	// snippet.user_get_id
	// blueprint.vnfjb7tt
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetUserIDSample();

	// snippet.user_update
	// blueprint.btbt5i5w
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void UpdateUserSample();

	// snippet.user_delete
	// blueprint.uz50yfrv
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void DeleteUserSample();

	// snippet.is_active
	// blueprint.khfx0thh
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void IsActiveSample();

	// snippet.get_last_active_timestamp
	// blueprint.xlavewc-
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetLastActiveTimestampSample();

	// snippet.user_where_present
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
	// blueprint.xrt70kmt
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetMembershipsSample();

	UFUNCTION()
	void OnGetMembershipsResponse(const FPubnubChatMembershipsResult& Result);

	// snippet.user_set_restrictions
	// blueprint.oner-uli
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void SetRestrictionsSample();

	// snippet.get_channel_restrictions
	// blueprint.7qfdfo8k
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetChannelRestrictionsSample();

	UFUNCTION()
	void OnGetChannelRestrictionsResponse(const FPubnubChatGetRestrictionResult& Result);

	// snippet.get_channels_restrictions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void GetChannelsRestrictionsSample();

	UFUNCTION()
	void OnGetChannelsRestrictionsResponse(const FPubnubChatGetRestrictionsResult& Result);

	// snippet.stream_user_updates
	// blueprint.nnutxy66
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatUser")
	void StreamUpdatesSample();

	void OnUserUpdateReceived(FString UserID, const FPubnubChatUserData& UserData);
	void OnUserDeleted();

	// snippet.end
};
