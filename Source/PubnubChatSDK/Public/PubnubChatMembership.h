// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"

#include "PubnubChatMembership.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatChannel;
class UPubnubSubscription;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPubnubChatMembershipUpdateReceived, EPubnubChatStreamedUpdateType, UpdateType, FString, ChannelID, FString, UserID, FPubnubChatMembershipData, MembershipData);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnPubnubChatMembershipUpdateReceivedNative, EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, FString UserID, const FPubnubChatMembershipData& MembershipData);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMembership : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;

	/* DELEGATES */

	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMembershipUpdateReceived OnMembershipUpdateReceived;
	FOnPubnubChatMembershipUpdateReceivedNative OnMembershipUpdateReceivedNative;

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FPubnubChatMembershipData GetMembershipData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	UPubnubChatUser* GetUser() const { return User; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	UPubnubChatChannel* GetChannel() const { return Channel; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FString GetUserID() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FString GetChannelID() const;
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FString GetLastReadMessageTimetoken();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult Delete();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult Update(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessageTimetoken(const FString Timetoken);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessage(UPubnubChatMessage* Message);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatGetUnreadMessagesCountResult GetUnreadMessagesCount();
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatUser> User = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatChannel> Channel = nullptr;
	
	UPROPERTY()
	bool IsInitialized = false;
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;
	
	bool IsStreamingUpdates = false;

	void InitMembership(UPubnubClient* InPubnubClient, UPubnubChat* InChat, UPubnubChatUser* InUser, UPubnubChatChannel* InChannel);

	/**
	 * Gets the internal composite membership ID used for repository operations.
	 * Format: [ChannelID].[UserID]
	 * @return Composite membership identifier
	 */
	FString GetInternalMembershipID() const;
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

