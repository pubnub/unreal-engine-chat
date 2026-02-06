// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "PubnubChatMessageDraftUtilities.generated.h"


/**
 * Blueprint function library for building mention targets used with message drafts (AddMention, InsertSuggestedMention) and suggested mentions.
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatMessageDraftUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	 * Creates a mention target for a user (e.g. for @mentions in a message draft).
	 *
	 * @param UserID The user ID to mention. Must be non-empty.
	 * @return Mention target with type User and the given UserID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateUserMentionTarget(const FString UserID);
	
	/**
	 * Creates a mention target for a channel (e.g. for #channel mentions in a message draft).
	 *
	 * @param ChannelID The channel ID to mention. Must be non-empty.
	 * @return Mention target with type Channel and the given ChannelID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateChannelMentionTarget(const FString ChannelID);
	
	/**
	 * Creates a mention target for a URL (link in the message draft).
	 *
	 * @param Url The URL to link. Must be non-empty.
	 * @return Mention target with type Url and the given URL.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Mention Target")
	static FPubnubChatMentionTarget CreateUrlMentionTarget(const FString Url);
	

};
