// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"

#include "PubnubChatUser.generated.h"

class UPubnubClient;
class UPubnubChat;

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
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatUserData GetUserData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetUserID() const { return UserID; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Update(FPubnubChatUserData UserData);

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

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString UserID = "";
	

	bool IsInitialized = false;

	void InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID);
};
