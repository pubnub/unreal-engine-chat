// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "PubnubChatMessageDraftUtilities.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatMessageDraftUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateUserMentionTarget(const FString UserID);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateChannelMentionTarget(const FString ChannelID);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateUrlMentionTarget(const FString Url);
	

};
