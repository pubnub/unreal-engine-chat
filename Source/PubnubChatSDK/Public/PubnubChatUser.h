// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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
UCLASS()
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Update(FPubnubChatUpdateUserInputData UpdateUserData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Restore();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatIsDeletedResult IsDeleted();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatWherePresentResult WherePresent();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatIsPresentResult IsPresentOn(const FString ChannelID);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatMembershipsResult GetMemberships(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult SetRestrictions(const FString ChannelID, bool Ban, bool Mute, FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionResult GetChannelRestrictions(UPubnubChatChannel* Channel);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionsResult GetChannelsRestrictions(const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingUpdates();
	
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
