// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"

#include "PubnubChatMessage.generated.h"

class UPubnubClient;
class UPubnubChat;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatMessage : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatMessageData GetMessageData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetMessageTimetoken() const { return Timetoken; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetCurrentText();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetType() { return "text"; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult EditText(const FString NewText);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Restore();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatIsDeletedResult IsDeleted();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Pin();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Unpin();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult ToggleReaction(const FString Reaction);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatGetReactionsResult GetReactions();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatHasReactionResult HasUserReaction(const FString Reaction);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Forward(UPubnubChatChannel* Channel);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Report(const FString Reason = "");
	

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString Timetoken = "";
	UPROPERTY()
	FString ChannelID = "";
	

	bool IsInitialized = false;

	void InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken);

	/**
	 * Gets the internal composite message ID used for repository operations.
	 * Format: [ChannelID].[Timetoken]
	 * @return Composite message identifier
	 */
	FString GetInternalMessageID() const;
};

