// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessageDraft.h"
#include "PubnubChatChannel.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"

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
	
	//Check if insert text would affect any MentionTarget elements
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(!IsPositionWithinMentionTarget(Position, 0), TEXT("Cannot insert text into the middle of a MentionTarget"));
	
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

void UPubnubChatMessageDraft::InitMessageDraft(UPubnubChatChannel* InChannel, const FPubnubChatMessageDraftConfig& InMessageDraftConfig)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChannel, TEXT("Can't create MessageDraft on invalid channel"));
	
	Channel = InChannel;
	MessageDraftConfig = InMessageDraftConfig;
}

void UPubnubChatMessageDraft::FireMessageDraftChangedDelegate()
{
	TArray<FPubnubChatSuggestedMention> SuggestedMentions;
	
	OnMessageDraftUpdated.Broadcast(MessageElements);
	OnMessageDraftUpdatedNative.Broadcast(MessageElements);
	OnMessageDraftUpdatedWithSuggestions.Broadcast(MessageElements, SuggestedMentions);
	OnMessageDraftUpdatedWithSuggestionsNative.Broadcast(MessageElements, SuggestedMentions);
}

void UPubnubChatMessageDraft::TriggerTypingIndicator()
{
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
