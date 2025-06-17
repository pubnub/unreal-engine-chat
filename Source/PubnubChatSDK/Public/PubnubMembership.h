// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <pubnub_chat/membership.hpp>
#include "PubnubChatStructLibrary.h"
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
	~UPubnubMembership(){delete InternalMembership;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	UPubnubChannel* GetChannel() {return Channel;};
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	UPubnubUser* GetUser() {return User;};

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership", meta=(DeprecatedFunction,
		DeprecationMessage="Function has been deprecated, Please use function GetMembershipData instead - it returns more Membership information"))
	FString GetCustomData();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Membership")
	FPubnubChatMembershipData GetMembershipData();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	UPubnubMembership* Update(FPubnubChatMembershipData MembershipData);
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
	static UPubnubMembership* Create(Pubnub::Membership Membership);
	
	//Internal usage only
	Pubnub::Membership* GetInternalMembership(){return InternalMembership;};

private:
	Pubnub::Membership* InternalMembership;
	UPROPERTY()
	UPubnubChannel* Channel = nullptr;
	UPROPERTY()
	UPubnubUser* User = nullptr;
	
	bool IsInternalMembershipValid();
	
	
};
