// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <pubnub_chat/user.hpp>
#include "UObject/NoExportTypes.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubUser.generated.h"

class UPubnubUser;
class UPubnubMembership;
class UPubnubCallbackStop;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubUserStreamUpdateReceived, UPubnubUser*, PubnubUser);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubUsersStreamUpdateOnReceived, const TArray<UPubnubUser*>&, PubnubUsers);

USTRUCT(BlueprintType)
struct FPubnubMembershipsResponseWrapper
{
	GENERATED_BODY();
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) TArray<UPubnubMembership*> Memberships;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FPubnubPage Page;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) int Total;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Status;

	FPubnubMembershipsResponseWrapper() = default;
	FPubnubMembershipsResponseWrapper(Pubnub::MembershipsResponseWrapper& Wrapper);
	
};

USTRUCT(BlueprintType)
struct FPubnubChannelsRestrictionsWrapper
{
	GENERATED_BODY();
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) TArray<FPubnubChannelRestriction> Restrictions;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FPubnubPage Page;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) int Total;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Status;

	FPubnubChannelsRestrictionsWrapper() = default;
	FPubnubChannelsRestrictionsWrapper(Pubnub::ChannelsRestrictionsWrapper& Wrapper);
	
};

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubUser : public UObject
{
	GENERATED_BODY()
public:
	static UPubnubUser* Create(Pubnub::User User);
	~UPubnubUser(){delete InternalUser;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub User")
	FString GetUserID();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub User")
	FPubnubChatUserData GetUserData();

	UFUNCTION(BlueprintCallable, Category="Pubnub User")
	UPubnubUser* Update(FPubnubChatUserData UserData);

	UFUNCTION(BlueprintCallable, Category="Pubnub User")
	void DeleteUser();

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	TArray<FString> WherePresent();

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	bool IsPresentOn(FString ChannelID);

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	void SetRestrictions(FString ChannelID, FPubnubRestriction Restrictions);

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	FPubnubRestriction GetChannelRestrictions(UPubnubChannel* Channel);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	FPubnubChannelsRestrictionsWrapper GetChannelsRestrictions(FString Sort = "", int Limit = 0, FPubnubPage Page = FPubnubPage());

	//Deprecated in JS chat
	//UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	//void Report(FString Reason);

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	FPubnubMembershipsResponseWrapper GetMemberships(FString Filter = "", FString Sort = "", int Limit = 0, FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	UPubnubCallbackStop* StreamUpdates(FOnPubnubUserStreamUpdateReceived UserUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub User")
	UPubnubCallbackStop* StreamUpdatesOn(TArray<UPubnubUser*> Users, FOnPubnubUsersStreamUpdateOnReceived UserUpdateCallback);
	
	//Internal usage only
	Pubnub::User* GetInternalUser(){return InternalUser;};


private:
	Pubnub::User* InternalUser;

	bool IsInternalUserValid();
};
