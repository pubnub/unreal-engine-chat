// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <pubnub_chat/message.hpp>
#include "PubnubChatStructLibrary.h"
#include "PubnubMessage.generated.h"

class UPubnubMessage;
class UPubnubThreadChannel;
class UPubnubCallbackStop;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubMessageStreamUpdateReceived, UPubnubMessage*, PubnubMessage);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubMessagesStreamUpdateOnReceived, const TArray<UPubnubMessage*>&, PubnubMessages);


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMessage : public UObject
{
	GENERATED_BODY()
public:
	static UPubnubMessage* Create(Pubnub::Message Message);
	~UPubnubMessage();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message")
	FString GetTimetoken();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message")
	FPubnubChatMessageData GetMessageData();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	void EditText(FString NewText);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message")
	FString Text();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubMessage* DeleteMessage();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	bool DeleteMessageHard();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	bool Deleted();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubMessage* Restore();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	EPubnubChatMessageType Type();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	void Pin();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	void Unpin();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubMessage* ToggleReaction(FString Reaction);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	TArray<FPubnubMessageAction> Reactions();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	bool HasUserReaction(FString Reaction);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	void Forward(FString ChannelID);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	void Report(FString Reason);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubCallbackStop* StreamUpdates(FOnPubnubMessageStreamUpdateReceived MessageUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubCallbackStop* StreamUpdatesOn(TArray<UPubnubMessage*> Messages, FOnPubnubMessagesStreamUpdateOnReceived MessageUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message | Threads")
	UPubnubThreadChannel* CreateThread();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message | Threads")
	UPubnubThreadChannel* GetThread();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message | Threads")
	bool HasThread();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message | Threads")
	void RemoveThread();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	TArray<FPubnubMentionedUser> MentionedUsers();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	TArray<FPubnubReferencedChannel> ReferencedChannels();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubMessage* QuotedMessage();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	TArray<FPubnubTextLink> TextLinks();
	


	
	//Internal usage only
	Pubnub::Message* GetInternalMessage(){return InternalMessage;};

protected:
	Pubnub::Message* InternalMessage;

	bool IsInternalMessageValid();
	bool IsThreadMessage = false;
};
