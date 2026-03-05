// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessageDraft.h"
#include "PubnubChatChannel.h"
#include "Internationalization/Regex.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatMembership.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "PubnubChatUser.h"
#include "Threads/PubnubFunctionThread.h"

// Schema constants for markdown link rendering
static const FString SCHEMA_USER = TEXT("pn-user://");
static const FString SCHEMA_CHANNEL = TEXT("pn-channel://");

FString UPubnubChatMessageDraft::GetCurrentText() const
{
	FString CurrentText;
	for (const FPubnubChatMessageElement& Element : MessageElements)
	{
		CurrentText.Append(Element.Text);
	}
	return CurrentText;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::InsertText(int Position, const FString Text)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Text);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position >= 0), TEXT("Position cannot be negative"));
	
	FPubnubChatOperationResult FinalResult;
	
	//If there are no any elements yet, just add inserted text as new element
	if (MessageElements.IsEmpty())
	{
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position == 0), TEXT("Position is too big - above all MessageElements"));
		
		TriggerTypingIndicator();
		
		FPubnubChatMessageElement NewElement = FPubnubChatMessageElement({.Text = Text, .Start = 0, .Length = Text.Len()});
		MessageElements.Add(NewElement);
		FireMessageDraftChangedDelegate();
		return FinalResult;
	}
	
	//Check if position is not above the last element as we shouldn't have gaps between elements
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position <= MessageElements.Last().Start + MessageElements.Last().Length), TEXT("Position is too big - above all MessageElements"));
	
	// If insert position is strictly inside a mention (not at its start or end): convert that mention to plain text, insert the new text, and shift later elements.
	// Inserting at the start or end of a mention keeps the mention and uses the existing "insert before/after" logic below.
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		FPubnubChatMessageElement& Element = MessageElements[i];
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{
			continue;
		}
		int32 ElementStart = Element.Start;
		int32 ElementEnd = ElementStart + Element.Length;
		if (Position > ElementStart && Position < ElementEnd)
		{
			int32 PositionInElement = Position - ElementStart;
			Element.MentionTarget = FPubnubChatMentionTarget();
			Element.InsertText(PositionInElement, Text);
			MoveMessageElementsAfterPosition(ElementEnd, Text.Len(), true);
			TriggerTypingIndicator();
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
	}
	
	TriggerTypingIndicator();
	
	int MessageElementIndex = -1;
	
	//Find the closes MessageElements to insert Position
	for (int i = 0; i < MessageElements.Num(); i++)
	{
		//Check if text will be inserted directly before another MessageElement
		if (Position == MessageElements[i].Start)
		{
			MessageElementIndex = i;
			break;
		}
		//Check if text will be inserted in the middle of another MessageElement
		else if (Position > MessageElements[i].Start && Position < MessageElements[i].GetEndPosition())
		{
			//Add inserted text into previous Text MessageElement
			MessageElements[i].InsertText(Position - MessageElements[i].Start, Text);
			//Move Start in all further MessageElements by the length of inserted text
			MoveMessageElementsAfterPosition(Position, Text.Len(), false);
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
	}
	
	//If we found that inserted text starts at the same index as one of current MessageElements
	if (MessageElementIndex >= 0)
	{
		//Insert it to this MessageElement if it's a text one
		if (MessageElements[MessageElementIndex].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{
			//Insert Text at the beginning of this text MessageElement
			MessageElements[MessageElementIndex].InsertText(0, Text);
			//Move Start in all further MessageElements by the length of inserted text
			MoveMessageElementsAfterPosition(Position, Text.Len(), false);
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
		//Insert it to the previous MessageElement if it's a text one
		if (MessageElementIndex > 0 && MessageElements[MessageElementIndex - 1].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{
			//Insert Text at the end of this text MessageElement
			MessageElements[MessageElementIndex - 1].InsertTextAtTheEnd(Text);
			//Move Start in all further MessageElements by the length of inserted text (including one that Starts with exact Position)
			MoveMessageElementsAfterPosition(Position, Text.Len(), true);
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
		
		//Neither of those are text ones, so create new text MessageElement - reorder existing ones first
		MoveMessageElementsAfterPosition(Position, Text.Len(), true);
		FPubnubChatMessageElement NewElement = FPubnubChatMessageElement({.Text = Text, .Start = Position, .Length = Text.Len()});
		MessageElements.Insert(NewElement, MessageElementIndex);
		FireMessageDraftChangedDelegate();
		return FinalResult;
	}
	
	//If logic reached this place MessageElementIndex was not set
	//Since elements are contiguous, this means Position is at the end of the last element (Position == last element's end)
	
	//Insert it to the last MessageElement if it's a text one
	if (MessageElements.Last().MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
	{
		MessageElements.Last().InsertTextAtTheEnd(Text);
		FireMessageDraftChangedDelegate();
		return FinalResult;
	}
	
	// Or if last MessageElement is not a text one, just create new one and add it at the end
	FPubnubChatMessageElement NewElement = FPubnubChatMessageElement({.Text = Text, .Start = Position, .Length = Text.Len()});
	MessageElements.Add(NewElement);
	FireMessageDraftChangedDelegate();
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::AppendText(const FString Text)
{
	return InsertText(GetCurrentText().Len(), Text);
}

FPubnubChatOperationResult UPubnubChatMessageDraft::RemoveText(int Position, int Length)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position >= 0), TEXT("Position cannot be negative"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Length > 0), TEXT("Length has to be positive"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!MessageElements.IsEmpty(), TEXT("This MessageDraft is empty - there is nothing to remove"));
	
	//Validate bounds - check if removal range is within bounds
	int32 LastElementEnd = MessageElements.Last().Start + MessageElements.Last().Length;
	int32 RemoveEnd = Position + Length;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position <= LastElementEnd), TEXT("Position is too big - above all MessageElements"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((RemoveEnd <= LastElementEnd), TEXT("Removal range extends beyond all MessageElements"));
	
	//Check if removal would affect any MentionTarget elements
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!IsPositionWithinMentionTarget(Position, Length), TEXT("Cannot remove text that overlaps with a MentionTarget"));
	
	FPubnubChatOperationResult FinalResult;
	
	TriggerTypingIndicator();
	
	//Find and process the affected text element
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		//Check if removal overlaps with this element and it's a text element
		if (Position >= MessageElements[i].Start && Position < MessageElements[i].Start + MessageElements[i].Length)
		{
			//If remove length matches this MessageElement length, just remove the whole MessageElement
			if (MessageElements[i].Length == Length)
			{
				MessageElements.RemoveAt(i);
				MoveMessageElementsAfterPosition(Position, -Length, false);
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}
			
			//Or remove just some part of the text
			MessageElements[i].RemoveText(Position - MessageElements[i].Start, Length);
			MoveMessageElementsAfterPosition(Position, -Length, false);
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
	}
	
	//Generally logic should never reach this place
	FinalResult.Error = true;
	FinalResult.ErrorMessage = TEXT("Failed to Remove Text - MessageElement to RemoveText was not found");
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::AddMention(int Position, int Length, const FPubnubChatMentionTarget MentionTarget)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position >= 0), TEXT("Position cannot be negative"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Length > 0), TEXT("Length has to be positive"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!MessageElements.IsEmpty(), TEXT("This MessageDraft is empty - InsertText first to be able to AddMention"));
	
	//Validate bounds - check if AddMention range is within bounds
	int32 LastElementEnd = MessageElements.Last().Start + MessageElements.Last().Length;
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position <= LastElementEnd), TEXT("Position is too big - above all MessageElements"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position + Length <= LastElementEnd), TEXT("Removal range extends beyond all MessageElements"));
	
	//Check if removal would affect any existing MentionTarget elements
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!IsPositionWithinMentionTarget(Position, Length), TEXT("Cannot remove text that overlaps with a MentionTarget"));
	
	FPubnubChatOperationResult FinalResult;
	
	TriggerTypingIndicator();
	
	//Find and process the affected text element
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		//Check if AddMention overlaps with this element and it's a text element
		if (Position >= MessageElements[i].Start && Position < MessageElements[i].Start + MessageElements[i].Length)
		{
			//If AddMention length matches this MessageElement length, just set it's MentionTarget
			if (MessageElements[i].Length == Length)
			{
				MessageElements[i].MentionTarget = MentionTarget; 
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}
			
			//If length doesn't match we have 3 possible cases:
			
			FPubnubChatMessageElement& Element = MessageElements[i];
			int32 ElementStart = Element.Start;
			int32 ElementEnd = Element.Start + Element.Length;
			int32 MentionEnd = Position + Length;
			int32 PositionInElement = Position - ElementStart;
			
			//Extract the text that will become the mention
			FString MentionText = Element.Text.Mid(PositionInElement, Length);
			
			//Case 1: Position starts at the same position as MessageElement
			if (Position == ElementStart)
			{
				//Create new Mention element at the start
				FPubnubChatMessageElement MentionElement = FPubnubChatMessageElement({.MentionTarget = MentionTarget, .Text = MentionText, .Start = Position, .Length = Length});
				
				//Remove the mention part from original element
				Element.RemoveText(0, Length);
				Element.Start = Position + Length;
				
				//Insert the mention element before the updated original element
				MessageElements.Insert(MentionElement, i);
				
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}
			
			//Case 2: AddMention is in the middle of MessageElement
			if (Position > ElementStart && MentionEnd < ElementEnd)
			{
				//Extract text parts
				FString TextBefore = Element.Text.Mid(0, PositionInElement);
				FString TextAfter = Element.Text.Mid(PositionInElement + Length);
				
				//Create three elements: text before, mention, text after
				FPubnubChatMessageElement TextBeforeElement = FPubnubChatMessageElement({.MentionTarget = FPubnubChatMentionTarget(), .Text = TextBefore, .Start = ElementStart, .Length = PositionInElement});
				FPubnubChatMessageElement MentionElement = FPubnubChatMessageElement({.MentionTarget = MentionTarget, .Text = MentionText, .Start = Position, .Length = Length});
				FPubnubChatMessageElement TextAfterElement = FPubnubChatMessageElement({.MentionTarget = FPubnubChatMentionTarget(), .Text = TextAfter, .Start = Position + Length, .Length = ElementEnd - MentionEnd});
				
				//Remove original element and insert the three new elements
				MessageElements.RemoveAt(i);
				MessageElements.Insert(TextBeforeElement, i);
				MessageElements.Insert(MentionElement, i + 1);
				MessageElements.Insert(TextAfterElement, i + 2);
				
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}
			
			//Case 3: AddMention is at the end of MessageElement
			if (MentionEnd == ElementEnd)
			{
				//Remove the mention part from original element
				Element.RemoveText(PositionInElement, Length);
				
				//Create new Mention element at the end
				FPubnubChatMessageElement MentionElement = FPubnubChatMessageElement({.MentionTarget = MentionTarget, .Text = MentionText, .Start = Position, .Length = Length});
				
				//Insert the mention element after the updated original element
				MessageElements.Insert(MentionElement, i + 1);
				
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}

			return FinalResult;
		}
	}
	
	//Generally logic should never reach this place
	FinalResult.Error = true;
	FinalResult.ErrorMessage = TEXT("Failed to Remove Text - MessageElement to RemoveText was not found");
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::RemoveMention(int Position)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Position >= 0), TEXT("Position cannot be negative"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!MessageElements.IsEmpty(), TEXT("This MessageDraft is empty - there is nothing to remove"));
	
	FPubnubChatOperationResult FinalResult;
	
	//Find the MessageElement that contains Position (Position is within the element)
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		const FPubnubChatMessageElement& Element = MessageElements[i];
		int32 ElementStart = Element.Start;
		int32 ElementEnd = Element.Start + Element.Length;
		
		//Check if Position is within this element (Position >= Start && Position < End)
		//Note: Position == End means it's at the start of the next element, so we don't include it
		if (Position >= ElementStart && Position < ElementEnd)
		{
			//Check if it's a valid MentionTarget
			if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
			{
				FinalResult.Error = true;
				FinalResult.ErrorMessage = TEXT("Element at Position is not a MentionTarget");
				return FinalResult;
			}
			
			//Found the mention element - remove it
			int32 MentionLength = Element.Length;
			MessageElements.RemoveAt(i);
			
			//Move all elements after the removed mention forward (subtract MentionLength from their Start)
			MoveMessageElementsAfterPosition(ElementStart, -MentionLength, false);
			
			TriggerTypingIndicator();
			FireMessageDraftChangedDelegate();
			return FinalResult;
		}
	}
	
	//No element found containing Position
	FinalResult.Error = true;
	FinalResult.ErrorMessage = TEXT("No MessageElement with MentionTarget found at the specified Position");
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::InsertSuggestedMention(const FPubnubChatSuggestedMention SuggestedMention)
{
	FPubnubChatOperationResult FinalResult;
	
	// Validate input
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((SuggestedMention.Offset >= 0), TEXT("Offset cannot be negative"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!SuggestedMention.ReplaceFrom.IsEmpty(), TEXT("ReplaceFrom cannot be empty"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!SuggestedMention.ReplaceTo.IsEmpty(), TEXT("ReplaceTo cannot be empty"));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((SuggestedMention.Target.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None), TEXT("Target must be a valid mention target (User or Channel)"));
	
	// Get current text to validate
	FString CurrentText = GetCurrentText();
	
	// Validate that the text at Offset matches ReplaceFrom (similar to KMP validation)
	int32 ReplaceFromLength = SuggestedMention.ReplaceFrom.Len();
	int32 TextEnd = SuggestedMention.Offset + ReplaceFromLength;
	
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((TextEnd <= CurrentText.Len()), TEXT("ReplaceFrom extends beyond current text"));
	
	FString TextAtOffset = CurrentText.Mid(SuggestedMention.Offset, ReplaceFromLength);
	if (TextAtOffset != SuggestedMention.ReplaceFrom)
	{
		FinalResult.Error = true;
		FinalResult.ErrorMessage = FString::Printf(TEXT("Mention suggestion invalid: expected '%s' at offset %d, but found '%s'"), 
			*SuggestedMention.ReplaceFrom, SuggestedMention.Offset, *TextAtOffset);
		return FinalResult;
	}
	
	// Suppress delegate and typing indicator calls during batch operations
	bSuppressDelegateAndTyping = true;
	
	// Step 1: Remove the ReplaceFrom text
	FPubnubChatOperationResult RemoveResult = RemoveText(SuggestedMention.Offset, ReplaceFromLength);
	if (RemoveResult.Error)
	{
		bSuppressDelegateAndTyping = false;
		return RemoveResult;
	}
	
	// Step 2: Insert the ReplaceTo text at the same position
	FPubnubChatOperationResult InsertResult = InsertText(SuggestedMention.Offset, SuggestedMention.ReplaceTo);
	if (InsertResult.Error)
	{
		bSuppressDelegateAndTyping = false;
		return InsertResult;
	}
	
	// Step 3: Add the mention at the position where ReplaceTo was inserted
	FPubnubChatOperationResult AddMentionResult = AddMention(SuggestedMention.Offset, SuggestedMention.ReplaceTo.Len(), SuggestedMention.Target);
	if (AddMentionResult.Error)
	{
		bSuppressDelegateAndTyping = false;
		return AddMentionResult;
	}
	
	// Re-enable delegate and typing indicator calls
	bSuppressDelegateAndTyping = false;
	
	// Now fire delegate and typing indicator once at the end (similar to KMP's fireMessageElementsChanged)
	TriggerTypingIndicator();
	FireMessageDraftChangedDelegate();
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::Send(FPubnubChatSendTextParams SendTextParams)
{
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(Channel, TEXT("Channel for this MessageDraft is invalid."));
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!GetCurrentText().IsEmpty(), TEXT("Can't send empty message draft."));
	
	FString DraftMessage = GetDraftTextToSend();
	return Channel->SendText(DraftMessage);
}

void UPubnubChatMessageDraft::SendAsync(FOnPubnubChatOperationResponse OnOperationResponse, FPubnubChatSendTextParams SendTextParams)
{
	FOnPubnubChatOperationResponseNative NativeCallback;
	NativeCallback.BindLambda([OnOperationResponse](const FPubnubChatOperationResult& OperationResult)
	{
		OnOperationResponse.ExecuteIfBound(OperationResult);
	});

	SendAsync(NativeCallback, SendTextParams);
}

void UPubnubChatMessageDraft::SendAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative, FPubnubChatSendTextParams SendTextParams)
{
	if (!Channel)
	{
		FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Channel for this MessageDraft is invalid."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, FPubnubChatOperationResult::CreateError(ErrorLogMessage));
		return;
	}
	if (GetCurrentText().IsEmpty())
	{
		FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Can't send empty message draft."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage);
		UPubnubUtilities::CallPubnubDelegate(OnOperationResponseNative, FPubnubChatOperationResult::CreateError(ErrorLogMessage));
		return;
	}
	
	FString DraftMessage = GetDraftTextToSend();
	Channel->SendTextAsync(DraftMessage, OnOperationResponseNative, SendTextParams);
}

FString UPubnubChatMessageDraft::GetDraftTextToSend()
{
	FString FinalText;
	
	for (const FPubnubChatMessageElement& Element : MessageElements)
	{
		// If element has no mention target, just append the text
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{
			FinalText.Append(Element.Text);
		}
		else
		{
			// Element has a mention target - create markdown link
			FString EscapedText = EscapeLinkText(Element.Text);
			FString LinkUrl;
			
			switch (Element.MentionTarget.MentionTargetType)
			{
				case EPubnubChatMentionTargetType::PCMTT_User:
					LinkUrl = SCHEMA_USER + EscapeLinkUrl(Element.MentionTarget.Target);
					break;
				case EPubnubChatMentionTargetType::PCMTT_Channel:
					LinkUrl = SCHEMA_CHANNEL + EscapeLinkUrl(Element.MentionTarget.Target);
					break;
				case EPubnubChatMentionTargetType::PCMTT_Url:
					LinkUrl = EscapeLinkUrl(Element.MentionTarget.Target);
					break;
				default:
					// Unknown type, just append text
					FinalText.Append(Element.Text);
					continue;
			}
			
			// Format: [text](url)
			FinalText.Append(FString::Printf(TEXT("[%s](%s)"), *EscapedText, *LinkUrl));
		}
	}
	
	return FinalText;
}

FString UPubnubChatMessageDraft::EscapeLinkText(const FString& Text)
{
	// Escape backslashes first (so we don't double-escape)
	FString Escaped = Text.Replace(TEXT("\\"), TEXT("\\\\"));
	// Escape closing brackets
	Escaped = Escaped.Replace(TEXT("]"), TEXT("\\]"));
	return Escaped;
}

FString UPubnubChatMessageDraft::EscapeLinkUrl(const FString& Url)
{
	// Escape backslashes first (so we don't double-escape)
	FString Escaped = Url.Replace(TEXT("\\"), TEXT("\\\\"));
	// Escape closing parentheses
	Escaped = Escaped.Replace(TEXT(")"), TEXT("\\)"));
	return Escaped;
}

void UPubnubChatMessageDraft::InitMessageDraft(UPubnubChatChannel* InChannel, const FPubnubChatMessageDraftConfig& InMessageDraftConfig)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChannel, TEXT("Can't create MessageDraft on invalid channel"));
	
	Channel = InChannel;
	MessageDraftConfig = InMessageDraftConfig;
}

void UPubnubChatMessageDraft::FireMessageDraftChangedDelegate()
{
	// Skip if suppressed (e.g., during batch operations)
	if (bSuppressDelegateAndTyping)
	{
		return;
	}
	
	// Always fire basic update
	OnMessageDraftUpdated.Broadcast(MessageElements);
	OnMessageDraftUpdatedNative.Broadcast(MessageElements);
	
	// Only get suggestions if there are listeners AND potential mentions exist
	TArray<FPubnubChatSuggestedMention> SuggestedMentions;
	if (HasListenersForSuggestions())
	{
		SuggestedMentions = GetSuggestedMentions();
	}
	
	OnMessageDraftUpdatedWithSuggestions.Broadcast(MessageElements, SuggestedMentions);
	OnMessageDraftUpdatedWithSuggestionsNative.Broadcast(MessageElements, SuggestedMentions);
}

bool UPubnubChatMessageDraft::HasListenersForSuggestions() const
{
	// Check if there are any listeners registered for suggestions
	return OnMessageDraftUpdatedWithSuggestions.IsBound() || 
		   OnMessageDraftUpdatedWithSuggestionsNative.IsBound();
}

TArray<FPubnubChatSuggestedMention> UPubnubChatMessageDraft::GetSuggestedMentions()
{
	TArray<FPubnubChatSuggestedMention> SuggestedMentions;
	
	if (!Channel || !Channel->Chat)
	{
		return SuggestedMentions;
	}
	
	FString CurrentText = GetCurrentText();
	if (CurrentText.IsEmpty())
	{
		return SuggestedMentions;
	}
	
	// Find potential user mentions (@username)
	TArray<FMentionMatch> UserMatches = FindUserMentionMatches(CurrentText);
	
	// Find potential channel mentions (#channelname)
	TArray<FMentionMatch> ChannelMatches = FindChannelMentionMatches(CurrentText);
	
	// Filter out matches that are already actual mentions
	TArray<FMentionMatch> UserMatchesNeeded;
	for (const FMentionMatch& Match : UserMatches)
	{
		bool bIsAlreadyMention = false;
		for (const FPubnubChatMessageElement& Element : MessageElements)
		{
			if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_User)
			{
				// Check if this match position overlaps with an existing user mention
				int32 ElementEnd = Element.Start + Element.Length;
				if (Match.Offset >= Element.Start && Match.Offset < ElementEnd)
				{
					bIsAlreadyMention = true;
					break;
				}
			}
		}
		if (!bIsAlreadyMention)
		{
			UserMatchesNeeded.Add(Match);
		}
	}
	
	TArray<FMentionMatch> ChannelMatchesNeeded;
	for (const FMentionMatch& Match : ChannelMatches)
	{
		bool bIsAlreadyMention = false;
		for (const FPubnubChatMessageElement& Element : MessageElements)
		{
			if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_Channel)
			{
				// Check if this match position overlaps with an existing channel mention
				int32 ElementEnd = Element.Start + Element.Length;
				if (Match.Offset >= Element.Start && Match.Offset < ElementEnd)
				{
					bIsAlreadyMention = true;
					break;
				}
			}
		}
		if (!bIsAlreadyMention)
		{
			ChannelMatchesNeeded.Add(Match);
		}
	}
	
	// Resolve user suggestions
	TArray<FPubnubChatSuggestedMention> UserSuggestions = ResolveUserSuggestions(UserMatchesNeeded);
	SuggestedMentions.Append(UserSuggestions);
	
	// Resolve channel suggestions
	TArray<FPubnubChatSuggestedMention> ChannelSuggestions = ResolveChannelSuggestions(ChannelMatchesNeeded);
	SuggestedMentions.Append(ChannelSuggestions);
	
	return SuggestedMentions;
}

TArray<UPubnubChatMessageDraft::FMentionMatch> UPubnubChatMessageDraft::FindUserMentionMatches(const FString& Text) const
{
	TArray<FMentionMatch> Matches;
	
	// Regex pattern: @ followed by alphanumeric characters and underscores
	// Pattern: @[a-zA-Z0-9_]+
	FString Pattern = TEXT("@[a-zA-Z0-9_]+");
	FRegexPattern RegexPattern(Pattern);
	FRegexMatcher Matcher(RegexPattern, Text);
	
	while (Matcher.FindNext())
	{
		int32 MatchStart = Matcher.GetMatchBeginning();
		int32 MatchEnd = Matcher.GetMatchEnding();
		FString MatchText = Text.Mid(MatchStart, MatchEnd - MatchStart);
		
		// Only include if the mention text (after @) is at least 3 characters
		// This matches KMP behavior: filter { matchResult -> matchResult.value.length > 3 }
		if (MatchText.Len() > 3) // @ + at least 3 characters
		{
			FMentionMatch Match;
			Match.Offset = MatchStart;
			Match.Text = MatchText;
			Match.bIsUserMention = true;
			Matches.Add(Match);
		}
	}
	
	return Matches;
}

TArray<UPubnubChatMessageDraft::FMentionMatch> UPubnubChatMessageDraft::FindChannelMentionMatches(const FString& Text) const
{
	TArray<FMentionMatch> Matches;
	
	// Regex pattern: # followed by alphanumeric characters and underscores
	// Pattern: #[a-zA-Z0-9_]+
	FString Pattern = TEXT("#[a-zA-Z0-9_]+");
	FRegexPattern RegexPattern(Pattern);
	FRegexMatcher Matcher(RegexPattern, Text);
	
	while (Matcher.FindNext())
	{
		int32 MatchStart = Matcher.GetMatchBeginning();
		int32 MatchEnd = Matcher.GetMatchEnding();
		FString MatchText = Text.Mid(MatchStart, MatchEnd - MatchStart);
		
		// Only include if the mention text (after #) is at least 3 characters
		if (MatchText.Len() > 3) // # + at least 3 characters
		{
			FMentionMatch Match;
			Match.Offset = MatchStart;
			Match.Text = MatchText;
			Match.bIsUserMention = false;
			Matches.Add(Match);
		}
	}
	
	return Matches;
}

TArray<FPubnubChatSuggestedMention> UPubnubChatMessageDraft::ResolveUserSuggestions(const TArray<FMentionMatch>& Matches)
{
	TArray<FPubnubChatSuggestedMention> Suggestions;
	
	if (!Channel || !Channel->Chat || Matches.IsEmpty())
	{
		return Suggestions;
	}
	
	// Group matches by search text for caching
	TMap<FString, TArray<FMentionMatch>> MatchesBySearchText;
	for (const FMentionMatch& Match : Matches)
	{
		// Extract search text (remove @ prefix)
		FString SearchText = Match.Text.Mid(1); // Remove '@'
		MatchesBySearchText.FindOrAdd(SearchText).Add(Match);
	}
	
	// Process each unique search text
	for (const auto& Pair : MatchesBySearchText)
	{
		const FString& SearchText = Pair.Key;
		const TArray<FMentionMatch>& MatchesForText = Pair.Value;
		
		// Check cache first
		FString CacheKey = FString::Printf(TEXT("user:%s"), *SearchText);
		TArray<FPubnubChatSuggestedMention>* CachedSuggestions = SuggestionsCache.Find(CacheKey);
		
		if (CachedSuggestions)
		{
			// Use cached suggestions - create suggestions for each match position
			for (const FMentionMatch& Match : MatchesForText)
			{
				for (const FPubnubChatSuggestedMention& CachedSuggestion : *CachedSuggestions)
				{
					// Create a new suggestion for each match position (offset may differ)
					FPubnubChatSuggestedMention Suggestion;
					Suggestion.Offset = Match.Offset;
					Suggestion.ReplaceFrom = Match.Text;
					Suggestion.ReplaceTo = CachedSuggestion.ReplaceTo;
					Suggestion.Target = CachedSuggestion.Target;
					Suggestions.Add(Suggestion);
				}
			}
		}
		else
		{
			// Query API for suggestions
			TArray<UPubnubChatUser*> SuggestedUsers;
			
			if (MessageDraftConfig.UserSuggestionSource == EPubnubChatMessageDraftSuggestionSource::PCMDSS_Channel)
			{
				// Get user suggestions from channel members
				FString Filter = FString::Printf(TEXT(R"(uuid.name LIKE "%s*")"), *SearchText);
				FPubnubChatMembershipsResult GetMembersResult = Channel->GetMembers(MessageDraftConfig.UserLimit, Filter);
				
				if (!GetMembersResult.Result.Error)
				{
					for (UPubnubChatMembership* Membership : GetMembersResult.Memberships)
					{
						if (UPubnubChatUser* User = Membership->GetUser())
						{
							SuggestedUsers.Add(User);
						}
					}
				}
			}
			else
			{
				// Get user suggestions globally
				FPubnubChatGetUserSuggestionsResult GetUserSuggestionsResult = Channel->Chat->GetUserSuggestions(SearchText, MessageDraftConfig.UserLimit);
				
				if (!GetUserSuggestionsResult.Result.Error)
				{
					SuggestedUsers = GetUserSuggestionsResult.Users;
				}
			}
			
			// Create base suggestions (without offset) for caching
			// We'll use the first match's ReplaceFrom for the cache, but create suggestions for all matches
			TArray<FPubnubChatSuggestedMention> BaseSuggestionsForCache;
			
			for (UPubnubChatUser* User : SuggestedUsers)
			{
				if (!User)
				{
					continue;
				}
				
				FPubnubChatSuggestedMention BaseSuggestion;
				BaseSuggestion.Offset = 0; // Offset doesn't matter for cache
				BaseSuggestion.ReplaceFrom = MatchesForText[0].Text; // Use first match's text for cache
				
				// Use user name if available, otherwise user ID
				FString UserName = User->GetUserData().UserName;
				BaseSuggestion.ReplaceTo = !UserName.IsEmpty() ? UserName : User->GetUserID();
				
				BaseSuggestion.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
				BaseSuggestion.Target.Target = User->GetUserID();
				
				BaseSuggestionsForCache.Add(BaseSuggestion);
				
				// Create suggestions for each match position
				for (const FMentionMatch& Match : MatchesForText)
				{
					FPubnubChatSuggestedMention Suggestion;
					Suggestion.Offset = Match.Offset;
					Suggestion.ReplaceFrom = Match.Text;
					Suggestion.ReplaceTo = BaseSuggestion.ReplaceTo;
					Suggestion.Target = BaseSuggestion.Target;
					Suggestions.Add(Suggestion);
				}
			}
			
			// Cache the base suggestions (without specific offsets)
			if (!BaseSuggestionsForCache.IsEmpty())
			{
				SuggestionsCache.Add(CacheKey, BaseSuggestionsForCache);
			}
		}
	}
	
	return Suggestions;
}

TArray<FPubnubChatSuggestedMention> UPubnubChatMessageDraft::ResolveChannelSuggestions(const TArray<FMentionMatch>& Matches)
{
	TArray<FPubnubChatSuggestedMention> Suggestions;
	
	if (!Channel || !Channel->Chat || Matches.IsEmpty())
	{
		return Suggestions;
	}
	
	// Group matches by search text for caching
	TMap<FString, TArray<FMentionMatch>> MatchesBySearchText;
	for (const FMentionMatch& Match : Matches)
	{
		// Extract search text (remove # prefix)
		FString SearchText = Match.Text.Mid(1); // Remove '#'
		MatchesBySearchText.FindOrAdd(SearchText).Add(Match);
	}
	
	// Process each unique search text
	for (const auto& Pair : MatchesBySearchText)
	{
		const FString& SearchText = Pair.Key;
		const TArray<FMentionMatch>& MatchesForText = Pair.Value;
		
		// Check cache first
		FString CacheKey = FString::Printf(TEXT("channel:%s"), *SearchText);
		TArray<FPubnubChatSuggestedMention>* CachedSuggestions = SuggestionsCache.Find(CacheKey);
		
		if (CachedSuggestions)
		{
			// Use cached suggestions - create suggestions for each match position
			for (const FMentionMatch& Match : MatchesForText)
			{
				for (const FPubnubChatSuggestedMention& CachedSuggestion : *CachedSuggestions)
				{
					// Create a new suggestion for each match position (offset may differ)
					FPubnubChatSuggestedMention Suggestion;
					Suggestion.Offset = Match.Offset;
					Suggestion.ReplaceFrom = Match.Text;
					Suggestion.ReplaceTo = CachedSuggestion.ReplaceTo;
					Suggestion.Target = CachedSuggestion.Target;
					Suggestions.Add(Suggestion);
				}
			}
		}
		else
		{
			// Query API for channel suggestions
			FPubnubChatGetChannelSuggestionsResult GetChannelSuggestionsResult = Channel->Chat->GetChannelSuggestions(SearchText, MessageDraftConfig.ChannelLimit);
			
			if (!GetChannelSuggestionsResult.Result.Error)
			{
				// Create base suggestions (without offset) for caching
				TArray<FPubnubChatSuggestedMention> BaseSuggestionsForCache;
				
				for (UPubnubChatChannel* SuggestedChannel : GetChannelSuggestionsResult.Channels)
				{
					if (!SuggestedChannel)
					{
						continue;
					}
					
					FPubnubChatSuggestedMention BaseSuggestion;
					BaseSuggestion.Offset = 0; // Offset doesn't matter for cache
					BaseSuggestion.ReplaceFrom = MatchesForText[0].Text; // Use first match's text for cache
					
					// Use channel name if available, otherwise channel ID
					FString ChannelName = SuggestedChannel->GetChannelData().ChannelName;
					BaseSuggestion.ReplaceTo = !ChannelName.IsEmpty() ? ChannelName : SuggestedChannel->GetChannelID();
					
					BaseSuggestion.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
					BaseSuggestion.Target.Target = SuggestedChannel->GetChannelID();
					
					BaseSuggestionsForCache.Add(BaseSuggestion);
					
					// Create suggestions for each match position
					for (const FMentionMatch& Match : MatchesForText)
					{
						FPubnubChatSuggestedMention Suggestion;
						Suggestion.Offset = Match.Offset;
						Suggestion.ReplaceFrom = Match.Text;
						Suggestion.ReplaceTo = BaseSuggestion.ReplaceTo;
						Suggestion.Target = BaseSuggestion.Target;
						Suggestions.Add(Suggestion);
					}
				}
				
				// Cache the base suggestions (without specific offsets)
				if (!BaseSuggestionsForCache.IsEmpty())
				{
					SuggestionsCache.Add(CacheKey, BaseSuggestionsForCache);
				}
			}
		}
	}
	
	return Suggestions;
}

void UPubnubChatMessageDraft::TriggerTypingIndicator()
{
	// Skip if suppressed (e.g., during batch operations)
	if (bSuppressDelegateAndTyping)
	{
		return;
	}
	
	if (!MessageDraftConfig.IsTypingIndicatorTriggered)
	{ return;}
	
	if (!Channel)
	{ return; }
	
	if (MessageElements.IsEmpty())
	{
		Channel->StopTyping(); 
	}
	else
	{
		Channel->StartTyping(); 
	}
}

void UPubnubChatMessageDraft::MoveMessageElementsAfterPosition(int Position, int Length, bool IncludeEqualPosition)
{
	for (auto& MessageElement: MessageElements)
	{
		if (MessageElement.Start > Position )
		{
			MessageElement.Start += Length;
		}
		else if (IncludeEqualPosition && MessageElement.Start == Position)
		{
			MessageElement.Start += Length;
		}
	}
}

bool UPubnubChatMessageDraft::IsPositionWithinMentionTarget(int Position, int Length)
{
	//Check if position would overlap with any MentionTarget
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		//If it's element without MentionTarget, just skip it
		if (MessageElements[i].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{ continue; }
		
		//Check if Position with given Length overlaps with this element
		int32 ElementEnd = MessageElements[i].Start + MessageElements[i].Length;
		if (Position + Length > MessageElements[i].Start && Position < ElementEnd)
		{
			return true;
		}
	}
	
	return false;
}

FPubnubChatOperationResult UPubnubChatMessageDraft::Update(const FString& NewText)
{
	FPubnubChatOperationResult FinalResult;
	
	//Reconstruct current text from MessageElements
	FString CurrentText = GetCurrentText();
	
	//If texts are identical, no changes needed
	if (CurrentText == NewText)
	{
		return FinalResult;
	}
	
	//Calculate diff operations using a simple but effective algorithm
	//Process from end to start to avoid position shifting issues
	int32 CurrentPos = CurrentText.Len();
	int32 NewPos = NewText.Len();
	
	//Track which mentions will be affected by changes
	TArray<int32> AffectedMentionIndices;
	
	//First, identify mentions that will be affected by any text changes
	//We'll check this by comparing the texts and finding changed regions
	int32 CommonPrefix = 0;
	int32 CurrentLen = CurrentText.Len();
	int32 NewLen = NewText.Len();
	
	//Find common prefix
	while (CommonPrefix < CurrentLen && CommonPrefix < NewLen && 
		   CurrentText[CommonPrefix] == NewText[CommonPrefix])
	{
		CommonPrefix++;
	}
	
	//Find common suffix
	int32 CommonSuffix = 0;
	while (CommonSuffix < CurrentLen - CommonPrefix && CommonSuffix < NewLen - CommonPrefix &&
		   CurrentText[CurrentLen - 1 - CommonSuffix] == NewText[NewLen - 1 - CommonSuffix])
	{
		CommonSuffix++;
	}
	
	//Calculate the changed region in current text
	int32 ChangeStart = CommonPrefix;
	int32 ChangeEnd = CurrentLen - CommonSuffix;
	
	//Find all mentions that overlap with the changed region
	for (int32 i = 0; i < MessageElements.Num(); ++i)
	{
		const FPubnubChatMessageElement& Element = MessageElements[i];
		
		//If it's a mention and overlaps with changed region, mark for removal
		if (Element.MentionTarget.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None)
		{
			int32 ElementStart = Element.Start;
			int32 ElementEnd = Element.Start + Element.Length;
			
			//Check if mention overlaps with changed region
			if (ChangeEnd > ElementStart && ChangeStart < ElementEnd)
			{
				AffectedMentionIndices.Add(i);
			}
		}
	}
	
	//Remove affected mentions first (in reverse order to maintain indices)
	for (int32 i = AffectedMentionIndices.Num() - 1; i >= 0; --i)
	{
		int32 MentionIndex = AffectedMentionIndices[i];
		int32 MentionStart = MessageElements[MentionIndex].Start;
		int32 MentionLength = MessageElements[MentionIndex].Length;
		
		//Remove the mention element
		MessageElements.RemoveAt(MentionIndex);
		
		//Adjust positions of elements after the removed mention
		MoveMessageElementsAfterPosition(MentionStart, -MentionLength, false);
		
		//Recalculate change region after mention removal
		CurrentText = GetCurrentText();
		CurrentLen = CurrentText.Len();
		
		//Recalculate common prefix and suffix
		CommonPrefix = 0;
		while (CommonPrefix < CurrentLen && CommonPrefix < NewLen && 
			   CurrentText[CommonPrefix] == NewText[CommonPrefix])
		{
			CommonPrefix++;
		}
		
		CommonSuffix = 0;
		while (CommonSuffix < CurrentLen - CommonPrefix && CommonSuffix < NewLen - CommonPrefix &&
			   CurrentText[CurrentLen - 1 - CommonSuffix] == NewText[NewLen - 1 - CommonSuffix])
		{
			CommonSuffix++;
		}
		
		ChangeStart = CommonPrefix;
		ChangeEnd = CurrentLen - CommonSuffix;
	}
	
	//Now apply text changes: remove old text, insert new text
	//Remove the changed portion from current text
	if (ChangeEnd > ChangeStart)
	{
		//Use internal removal without delegate firing
		CurrentText = GetCurrentText();
		for (int32 i = MessageElements.Num() - 1; i >= 0; --i)
		{
			FPubnubChatMessageElement& Element = MessageElements[i];
			int32 ElementStart = Element.Start;
			int32 ElementEnd = Element.Start + Element.Length;
			
			if (ChangeEnd > ElementStart && ChangeStart < ElementEnd)
			{
				int32 OverlapStart = FMath::Max(ChangeStart, ElementStart);
				int32 OverlapEnd = FMath::Min(ChangeEnd, ElementEnd);
				int32 OverlapLength = OverlapEnd - OverlapStart;
				
				if (OverlapLength > 0)
				{
					int32 RemoveStartInElement = OverlapStart - ElementStart;
					Element.Text.RemoveAt(RemoveStartInElement, OverlapLength);
					Element.Length -= OverlapLength;
					
					if (Element.Length <= 0)
					{
						MessageElements.RemoveAt(i);
					}
				}
			}
		}
		
		//Move elements after change region
		MoveMessageElementsAfterPosition(ChangeStart, -(ChangeEnd - ChangeStart), false);
	}
	
	//Insert new text at ChangeStart position
	FString TextToInsert = NewText.Mid(CommonPrefix, NewLen - CommonPrefix - CommonSuffix);
	if (!TextToInsert.IsEmpty())
	{
		//Use internal insertion without delegate firing
		if (MessageElements.IsEmpty())
		{
			FPubnubChatMessageElement NewElement = FPubnubChatMessageElement({.Text = TextToInsert, .Start = 0, .Length = TextToInsert.Len()});
			MessageElements.Add(NewElement);
		}
		else
		{
			//Find element at or after ChangeStart
			int32 InsertIndex = MessageElements.Num();
			for (int32 i = 0; i < MessageElements.Num(); ++i)
			{
				if (MessageElements[i].Start >= ChangeStart)
				{
					InsertIndex = i;
					break;
				}
			}
			
			//Check if we can merge with previous element
			if (InsertIndex > 0 && MessageElements[InsertIndex - 1].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
			{
				int32 PrevElementEnd = MessageElements[InsertIndex - 1].Start + MessageElements[InsertIndex - 1].Length;
				if (PrevElementEnd == ChangeStart)
				{
					MessageElements[InsertIndex - 1].InsertTextAtTheEnd(TextToInsert);
					MoveMessageElementsAfterPosition(ChangeStart, TextToInsert.Len(), false);
					TriggerTypingIndicator();
					FireMessageDraftChangedDelegate();
					return FinalResult;
				}
			}
			
			//Check if we can merge with element at ChangeStart
			if (InsertIndex < MessageElements.Num() && MessageElements[InsertIndex].Start == ChangeStart &&
				MessageElements[InsertIndex].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
			{
				MessageElements[InsertIndex].InsertText(0, TextToInsert);
				MoveMessageElementsAfterPosition(ChangeStart, TextToInsert.Len(), false);
				TriggerTypingIndicator();
				FireMessageDraftChangedDelegate();
				return FinalResult;
			}
			
			//Create new element
			FPubnubChatMessageElement NewElement = FPubnubChatMessageElement({.Text = TextToInsert, .Start = ChangeStart, .Length = TextToInsert.Len()});
			MessageElements.Insert(NewElement, InsertIndex);
			MoveMessageElementsAfterPosition(ChangeStart, TextToInsert.Len(), false);
		}
	}
	
	//Post-process: merge adjacent text elements and ensure no gaps
	//This maintains the design constraint: no two adjacent text elements, no gaps
	MergeAdjacentTextElements();
	
	TriggerTypingIndicator();
	FireMessageDraftChangedDelegate();
	return FinalResult;
}

void UPubnubChatMessageDraft::MergeAdjacentTextElements()
{
	if (MessageElements.Num() < 2)
	{
		return;
	}
	
	//Merge adjacent text elements (process from end to start to avoid index shifting)
	for (int32 i = MessageElements.Num() - 2; i >= 0; --i)
	{
		FPubnubChatMessageElement& CurrentElement = MessageElements[i];
		FPubnubChatMessageElement& NextElement = MessageElements[i + 1];
		
		//Check if both are text elements (no MentionTarget)
		if (CurrentElement.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None &&
			NextElement.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
		{
			//Check if they're adjacent (no gap)
			int32 CurrentElementEnd = CurrentElement.Start + CurrentElement.Length;
			if (CurrentElementEnd == NextElement.Start)
			{
				//Merge: append next element's text to current element
				CurrentElement.Text.Append(NextElement.Text);
				CurrentElement.Length += NextElement.Length;
				
				//Remove the next element
				MessageElements.RemoveAt(i + 1);
			}
		}
	}
	
	//Ensure no gaps: recalculate Start positions to be contiguous
	int32 CurrentPosition = 0;
	for (auto& Element : MessageElements)
	{
		Element.Start = CurrentPosition;
		CurrentPosition += Element.Length;
	}
}
