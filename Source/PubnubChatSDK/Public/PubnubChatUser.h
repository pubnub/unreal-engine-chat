// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "PubnubChatEnumLibrary.h"

#include "PubnubChatUser.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubSubscription;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatUserUpdateReceived, EPubnubChatStreamedUpdateType, UpdateType, FString, UserID, FPubnubChatUserData, UserData);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatUserUpdateReceivedNative, EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatUser : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatUserUpdateReceived OnUserUpdateReceived;
	FOnPubnubChatUserUpdateReceivedNative OnUserUpdateReceivedNative;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatUserData GetUserData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetUserID() const { return UserID; }

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Update(FPubnubChatUpdateUserInputData UpdateUserData);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponse OnOperationResponse);
	void UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse, bool Soft = false);
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Restore();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RestoreAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void RestoreAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatIsDeletedResult IsDeleted();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	void IsDeletedAsync(FOnPubnubChatIsDeletedResponse OnIsDeletedResponse);
	void IsDeletedAsync(FOnPubnubChatIsDeletedResponseNative OnIsDeletedResponseNative);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	bool IsActive() const;
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	void IsActiveAsync(FOnPubnubChatIsActiveResponse OnIsActiveResponse);
	void IsActiveAsync(FOnPubnubChatIsActiveResponseNative OnIsActiveResponseNative);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetLastActiveTimestamp() const;
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatWherePresentResult WherePresent();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void WherePresentAsync(FOnPubnubChatWherePresentResponse OnWherePresentResponse);
	void WherePresentAsync(FOnPubnubChatWherePresentResponseNative OnWherePresentResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatIsPresentResult IsPresentOn(const FString ChannelID);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponse OnIsPresentResponse);
	void IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatMembershipsResult GetMemberships(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetMembershipsAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	void GetMembershipsAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult SetRestrictions(const FString ChannelID, bool Ban, bool Mute, FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponse OnOperationResponse, FString Reason = "");
	void SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionResult GetChannelRestrictions(UPubnubChatChannel* Channel);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponse OnRestrictionResponse);
	void GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionsResult GetChannelsRestrictions(const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponse OnRestrictionsResponse, const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	void GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponseNative, const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatUser*>& Users);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString UserID = "";
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;

	void InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID);
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};
