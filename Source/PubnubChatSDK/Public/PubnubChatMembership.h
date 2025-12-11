// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatStructLibrary.h"

#include "PubnubChatMembership.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatChannel;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatMembership : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
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
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult Update(const FPubnubChatMembershipData& MembershipData);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessageTimetoken(const FString Timetoken);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessage(UPubnubChatMessage* Message);

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatUser> User = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatChannel> Channel = nullptr;
	

	bool IsInitialized = false;

	void InitMembership(UPubnubClient* InPubnubClient, UPubnubChat* InChat, UPubnubChatUser* InUser, UPubnubChatChannel* InChannel);

	/**
	 * Gets the internal composite membership ID used for repository operations.
	 * Format: [ChannelID].[UserID]
	 * @return Composite membership identifier
	 */
	FString GetInternalMembershipID() const;
};

