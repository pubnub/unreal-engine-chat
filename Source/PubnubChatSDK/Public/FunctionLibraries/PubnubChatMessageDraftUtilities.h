// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "PubnubChatMessageDraftUtilities.generated.h"


/**
 * Blueprint function library for building mention targets used with message drafts (AddMention, InsertSuggestedMention) and suggested mentions.
 * Also provides utilities to parse message markdown (as sent by MessageDraft) back into message elements.
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

	/**
	 * Unescapes link text as stored in markdown [text](url). Reverses the escaping used when sending (\\ -> \, \] -> ]).
	 *
	 * @param EscapedLinkText Escaped link text (e.g. from between '[' and ']' in the markdown).
	 * @return Unescaped link text suitable for FPubnubChatMessageElement::Text.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	static FString UnescapeLinkText(const FString& EscapedLinkText);

	/**
	 * Unescapes link URL as stored in markdown [text](url). Reverses the escaping used when sending (\\ -> \, \) -> )).
	 *
	 * @param EscapedLinkUrl Escaped URL (e.g. from between '(' and ')' in the markdown).
	 * @return Unescaped URL.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	static FString UnescapeLinkUrl(const FString& EscapedLinkUrl);

	/**
	 * Parses message text containing markdown links (as produced by MessageDraft Send) into message elements.
	 * Plain text and [text](url) links are converted to FPubnubChatMessageElement with Start/Length in display order.
	 * Supports pn-user://, pn-channel:// and plain URLs. Escaped sequences in link text and URL are handled.
	 *
	 * @param MarkdownText The message text (e.g. from UPubnubChatMessage::GetCurrentText()) which may contain [text](url) links.
	 * @return Array of message elements; concatenating element.Text yields the display text; Start/Length match draft-style indices.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	static TArray<FPubnubChatMessageElement> ParseMessageMarkdownToElements(const FString& MarkdownText);
	

};
