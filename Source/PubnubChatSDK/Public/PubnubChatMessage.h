// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"

#include "PubnubChatMessage.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubSubscription;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageUpdateReceived, FString, Timetoken, FPubnubChatMessageData, MessageData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageUpdateReceivedNative, FString Timetoken, const FPubnubChatMessageData& MessageData);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMessage : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChatThreadChannel;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageUpdateReceived OnMessageUpdateReceived;
	FOnPubnubChatMessageUpdateReceivedNative OnMessageUpdateReceivedNative;
	
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
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult CreateThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult GetThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatHasThreadResult HasThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult RemoveThread();

protected:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString Timetoken = "";
	UPROPERTY()
	FString ChannelID = "";
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;

	void InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken);
	void UpdateMessageData(const FPubnubChatMessageData& NewMessageData);

	/**
	 * Gets the internal composite message ID used for repository operations.
	 * Format: [ChannelID].[Timetoken]
	 * @return Composite message identifier
	 */
	FString GetInternalMessageID() const;
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

