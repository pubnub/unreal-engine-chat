// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
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
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult EditText(const FString NewText);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void EditTextAsync(const FString NewText, FOnPubnubChatOperationResponse OnOperationResponse);
	void EditTextAsync(const FString NewText, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse, bool Soft = false);
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Restore();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RestoreAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void RestoreAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatIsDeletedResult IsDeleted();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	void IsDeletedAsync(FOnPubnubChatIsDeletedResponse OnIsDeletedResponse);
	void IsDeletedAsync(FOnPubnubChatIsDeletedResponseNative OnIsDeletedResponseNative);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Pin();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void PinAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Unpin();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void UnpinAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult ToggleReaction(const FString Reaction);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ToggleReactionAsync(const FString Reaction, FOnPubnubChatOperationResponse OnOperationResponse);
	void ToggleReactionAsync(const FString Reaction, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatGetReactionsResult GetReactions();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	void GetReactionsAsync(FOnPubnubChatGetReactionsResponse OnReactionsResponse);
	void GetReactionsAsync(FOnPubnubChatGetReactionsResponseNative OnReactionsResponseNative);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatHasReactionResult HasUserReaction(const FString Reaction);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	void HasUserReactionAsync(const FString Reaction, FOnPubnubChatHasReactionResponse OnHasReactionResponse);
	void HasUserReactionAsync(const FString Reaction, FOnPubnubChatHasReactionResponseNative OnHasReactionResponseNative);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Forward(UPubnubChatChannel* Channel);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ForwardAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	void ForwardAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Report(const FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ReportAsync(FOnPubnubChatOperationResponse OnOperationResponse, const FString Reason = "");
	void ReportAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, const FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatMessage*>& Messages);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult CreateThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	void CreateThreadAsync(FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	void CreateThreadAsync(FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult GetThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	void GetThreadAsync(FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	void GetThreadAsync(FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatHasThreadResult HasThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	void HasThreadAsync(FOnPubnubChatHasThreadResponse OnHasThreadResponse);
	void HasThreadAsync(FOnPubnubChatHasThreadResponseNative OnHasThreadResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult RemoveThread();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RemoveThreadAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void RemoveThreadAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

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

