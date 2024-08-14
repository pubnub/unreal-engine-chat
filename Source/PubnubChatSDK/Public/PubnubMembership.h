// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <pubnub_chat/membership.hpp>
#include "PubnubMembership.generated.h"

class UPubnubChannel;
class UPubnubUser;
class UPubnubMembership;
class UPubnubMessage;
class UPubnubCallbackStop;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubMembershipStreamUpdateReceived, UPubnubMembership*, PubnubMembership);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubMembershipsStreamUpdateOnReceived, const TArray<UPubnubMembership*>&, PubnubMemberships);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMembership : public UObject
{
	GENERATED_BODY()

public:
	static UPubnubMembership* Create(Pubnub::Membership Membership);
	~UPubnubMembership(){delete InternalMembership;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	UPubnubChannel* GetChannel() {return Channel;};
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	UPubnubUser* GetUser() {return User;};

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	FString GetCustomData();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubMembership* Update(FString CustomData);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubCallbackStop* StreamUpdates(FOnPubnubMembershipStreamUpdateReceived MembershipUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubCallbackStop* StreamUpdatesOn(TArray<UPubnubMembership*> Memberships, FOnPubnubMembershipsStreamUpdateOnReceived MembershipUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FString LastReadMessageTimetoken();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubMembership* SetLastReadMessageTimetoken(FString Timetoken);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubMembership* SetLastReadMessage(UPubnubMessage* Message);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	int GetUnreadMessageCount();

	//Internal usage only
	Pubnub::Membership* GetInternalMembership(){return InternalMembership;};

private:
	Pubnub::Membership* InternalMembership;
	static inline UPubnubChannel* Channel = nullptr;
	static inline UPubnubUser* User = nullptr;
	
	bool IsInternalMembershipValid();
	
	
};
