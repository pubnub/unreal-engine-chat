// Copyright 2025 PubNUB Inc. All Rights Reserved.

#include "PubnubChatMessageDraft.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

// ============================================================================
// MESSAGE DRAFT UNIT TESTS - Direct MessageDraft Testing (No API Calls)
// ============================================================================

// Helper function to verify MessageElements are contiguous and properly structured
bool VerifyMessageElementsStructure(const TArray<FPubnubChatMessageElement>& Elements, const FString& ExpectedText)
{
	if (Elements.IsEmpty())
	{
		return ExpectedText.IsEmpty();
	}
	
	// Check contiguity and no adjacent text elements
	for (int32 i = 0; i < Elements.Num(); ++i)
	{
		const FPubnubChatMessageElement& Element = Elements[i];
		
		// Verify Start position matches expected
		int32 ExpectedStart = 0;
		for (int32 j = 0; j < i; ++j)
		{
			ExpectedStart += Elements[j].Length;
		}
		
		if (Element.Start != ExpectedStart)
		{
			return false;
		}
		
		// Check no adjacent text elements
		if (i > 0)
		{
			const FPubnubChatMessageElement& PrevElement = Elements[i - 1];
			if (PrevElement.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None &&
				Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_None)
			{
				return false; // Two adjacent text elements found
			}
		}
	}
	
	// Verify reconstructed text matches expected
	FString ReconstructedText;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		ReconstructedText.Append(Element.Text);
	}
	
	return ReconstructedText == ExpectedText;
}

// ============================================================================
// INSERT TEXT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextEmptyTest, "PubnubChat.Unit.MessageDraft.InsertText.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextEmptyTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Insert text into empty draft
	FPubnubChatOperationResult Result = Draft->InsertText(0, TEXT("Hello"));
	TestFalse("InsertText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted", CurrentText, TEXT("Hello"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have one element", Elements.Num(), 1);
	
	if (Elements.Num() == 1)
	{
		TestEqual("Element text should match", Elements[0].Text, TEXT("Hello"));
		TestEqual("Element start should be 0", Elements[0].Start, 0);
		TestEqual("Element length should match", Elements[0].Length, 5);
		TestEqual("Element should be text (no mention)", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextAtStartTest, "PubnubChat.Unit.MessageDraft.InsertText.AtStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextAtStartTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert initial text
	Draft->InsertText(0, TEXT("World"));
	
	// Insert text at start
	FPubnubChatOperationResult Result = Draft->InsertText(0, TEXT("Hello "));
	TestFalse("InsertText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted at start", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextAtEndTest, "PubnubChat.Unit.MessageDraft.InsertText.AtEnd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextAtEndTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert initial text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Insert text at end
	FPubnubChatOperationResult Result = Draft->InsertText(5, TEXT(" World"));
	TestFalse("InsertText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted at end", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextInMiddleTest, "PubnubChat.Unit.MessageDraft.InsertText.InMiddle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextInMiddleTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert initial text
	Draft->InsertText(0, TEXT("HelloWorld"));
	
	// Insert text in middle
	FPubnubChatOperationResult Result = Draft->InsertText(5, TEXT(" "));
	TestFalse("InsertText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted in middle", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextBeforeMentionTest, "PubnubChat.Unit.MessageDraft.InsertText.BeforeMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextBeforeMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("HelloWorld"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(5, 5, MentionTarget);
	
	// Insert text before mention
	FPubnubChatOperationResult Result = Draft->InsertText(5, TEXT(" "));
	TestFalse("InsertText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted before mention", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextEmptyStringTest, "PubnubChat.Unit.MessageDraft.InsertText.EmptyString", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextEmptyStringTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Insert empty string should fail/return early
	FPubnubChatOperationResult Result = Draft->InsertText(0, TEXT(""));
	TestTrue("InsertText should fail with empty string", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain empty", CurrentText, TEXT(""));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextNegativePositionTest, "PubnubChat.Unit.MessageDraft.InsertText.NegativePosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextNegativePositionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Insert with negative position should fail
	FPubnubChatOperationResult Result = Draft->InsertText(-1, TEXT("Hello"));
	TestTrue("InsertText should fail with negative position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextPositionTooBigTest, "PubnubChat.Unit.MessageDraft.InsertText.PositionTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextPositionTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Insert at position beyond end should fail
	FPubnubChatOperationResult Result = Draft->InsertText(10, TEXT("World"));
	TestTrue("InsertText should fail with position too big", Result.Error);
	
	// Insert at position exactly at end should succeed (it's allowed)
	Result = Draft->InsertText(5, TEXT(" World"));
	TestFalse("InsertText should succeed at end position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextIntoMentionTest, "PubnubChat.Unit.MessageDraft.InsertText.IntoMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextIntoMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(0, 5, MentionTarget);
	
	// Try to insert text into the middle of mention (should fail)
	FPubnubChatOperationResult Result = Draft->InsertText(2, TEXT("X"));
	TestTrue("InsertText should fail when inserting into middle of mention", Result.Error);
	
	// Try to insert text at start of mention (should succeed - creates new text element before mention)
	Result = Draft->InsertText(0, TEXT("X"));
	TestFalse("InsertText should succeed when inserting at start of mention", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted before mention", CurrentText, TEXT("XHello"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have two elements", Elements.Num(), 2);
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text should match", Elements[0].Text, TEXT("X"));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text should match", Elements[1].Text, TEXT("Hello"));
	}
	
	// Try to insert text at end of mention (should fail - position 5 is inside the mention, end is exclusive)
	// Reset: remove the "X" we added
	Draft->RemoveText(0, 1);
	Result = Draft->InsertText(4, TEXT("X"));
	TestTrue("InsertText should fail when inserting at end position of mention", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextBeforeMentionWithPreviousTextTest, "PubnubChat.Unit.MessageDraft.InsertText.BeforeMentionWithPreviousText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextBeforeMentionWithPreviousTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text, add mention, then insert more text after
	Draft->InsertText(0, TEXT("Hello"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(0, 5, MentionTarget); // Mention "Hello"
	Draft->InsertText(5, TEXT(" World")); // Text after mention
	
	// Now insert text at start of mention (position 0)
	// This should add text to the previous element if it exists, or create new one
	// In this case, there's no previous element, so it should create a new text element
	FPubnubChatOperationResult Result = Draft->InsertText(0, TEXT("Hi "));
	TestFalse("InsertText should succeed when inserting at start of mention", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be inserted before mention", CurrentText, TEXT("Hi Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify structure: text element, mention, text element
	TestEqual("Should have three elements", Elements.Num(), 3);
	if (Elements.Num() == 3)
	{
		TestEqual("First element should be text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text should match", Elements[0].Text, TEXT("Hi "));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text should match", Elements[1].Text, TEXT("Hello"));
		TestEqual("Third element should be text", Elements[2].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Third element text should match", Elements[2].Text, TEXT(" World"));
	}
	
	return true;
}

// ============================================================================
// REMOVE TEXT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextFromStartTest, "PubnubChat.Unit.MessageDraft.RemoveText.FromStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextFromStartTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Remove text from start
	FPubnubChatOperationResult Result = Draft->RemoveText(0, 6);
	TestFalse("RemoveText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be removed from start", CurrentText, TEXT("World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextFromEndTest, "PubnubChat.Unit.MessageDraft.RemoveText.FromEnd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextFromEndTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Remove text from end
	FPubnubChatOperationResult Result = Draft->RemoveText(5, 6);
	TestFalse("RemoveText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be removed from end", CurrentText, TEXT("Hello"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextFromMiddleTest, "PubnubChat.Unit.MessageDraft.RemoveText.FromMiddle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextFromMiddleTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Remove text from middle
	FPubnubChatOperationResult Result = Draft->RemoveText(5, 1);
	TestFalse("RemoveText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be removed from middle", CurrentText, TEXT("HelloWorld"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextEntireElementTest, "PubnubChat.Unit.MessageDraft.RemoveText.EntireElement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextEntireElementTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Remove entire element
	FPubnubChatOperationResult Result = Draft->RemoveText(0, 5);
	TestFalse("RemoveText should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be empty", CurrentText, TEXT(""));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have no elements", Elements.Num(), 0);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextInvalidLengthTest, "PubnubChat.Unit.MessageDraft.RemoveText.InvalidLength", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextInvalidLengthTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Remove with invalid length should fail
	FPubnubChatOperationResult Result = Draft->RemoveText(0, 0);
	TestTrue("RemoveText should fail with zero length", Result.Error);
	
	Result = Draft->RemoveText(0, -1);
	TestTrue("RemoveText should fail with negative length", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextNegativePositionTest, "PubnubChat.Unit.MessageDraft.RemoveText.NegativePosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextNegativePositionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Remove with negative position should fail
	FPubnubChatOperationResult Result = Draft->RemoveText(-1, 1);
	TestTrue("RemoveText should fail with negative position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextEmptyDraftTest, "PubnubChat.Unit.MessageDraft.RemoveText.EmptyDraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextEmptyDraftTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Remove from empty draft should fail
	FPubnubChatOperationResult Result = Draft->RemoveText(0, 1);
	TestTrue("RemoveText should fail on empty draft", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextPositionTooBigTest, "PubnubChat.Unit.MessageDraft.RemoveText.PositionTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextPositionTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Remove at position beyond end should fail
	FPubnubChatOperationResult Result = Draft->RemoveText(10, 1);
	TestTrue("RemoveText should fail with position too big", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextRangeTooBigTest, "PubnubChat.Unit.MessageDraft.RemoveText.RangeTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextRangeTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Remove range that extends beyond end should fail
	FPubnubChatOperationResult Result = Draft->RemoveText(3, 10);
	TestTrue("RemoveText should fail when range extends beyond end", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveTextOverlapsMentionTest, "PubnubChat.Unit.MessageDraft.RemoveText.OverlapsMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveTextOverlapsMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget); // Mention "World"
	
	// Try to remove text that overlaps with mention (should fail)
	FPubnubChatOperationResult Result = Draft->RemoveText(5, 3); // Overlaps with "World"
	TestTrue("RemoveText should fail when overlapping with mention", Result.Error);
	
	// Try to remove text that starts inside mention (should fail)
	Result = Draft->RemoveText(7, 2);
	TestTrue("RemoveText should fail when starting inside mention", Result.Error);
	
	// Try to remove text that covers entire mention (should fail)
	Result = Draft->RemoveText(6, 5);
	TestTrue("RemoveText should fail when covering entire mention", Result.Error);
	
	// Try to remove text that extends into mention (should fail)
	Result = Draft->RemoveText(3, 5); // From "lo " to "Wo"
	TestTrue("RemoveText should fail when extending into mention", Result.Error);
	
	return true;
}

// ============================================================================
// ADD MENTION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionEntireTextTest, "PubnubChat.Unit.MessageDraft.AddMention.EntireText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionEntireTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Add mention to entire text
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	FPubnubChatOperationResult Result = Draft->AddMention(0, 5, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have one element", Elements.Num(), 1);
	
	if (Elements.Num() == 1)
	{
		TestEqual("Element should have mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Mention target should match", Elements[0].MentionTarget.Target, TEXT("user123"));
		TestEqual("Element text should match", Elements[0].Text, TEXT("Hello"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionAtStartTest, "PubnubChat.Unit.MessageDraft.AddMention.AtStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionAtStartTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Add mention at start
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	FPubnubChatOperationResult Result = Draft->AddMention(0, 5, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have two elements", Elements.Num(), 2);
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should have mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("First element text should match", Elements[0].Text, TEXT("Hello"));
		TestEqual("Second element should be text", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Second element text should match", Elements[1].Text, TEXT(" World"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionInMiddleTest, "PubnubChat.Unit.MessageDraft.AddMention.InMiddle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionInMiddleTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Add mention in middle
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	FPubnubChatOperationResult Result = Draft->AddMention(6, 5, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	// When adding mention in middle, if there's no text after, we get 2 elements (text before + mention)
	// The empty text element after is not created or gets merged
	TestTrue("Should have 2 or 3 elements", Elements.Num() == 2 || Elements.Num() == 3);
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// When adding mention in middle, if there's no text after, we get 2 elements (text before + mention)
	// The empty text element after is not created or gets merged
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text should match", Elements[0].Text, TEXT("Hello "));
		TestEqual("Second element should have mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text should match", Elements[1].Text, TEXT("World"));
	}
	else if (Elements.Num() == 3)
	{
		// If empty text element is created, verify it
		TestEqual("First element should be text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text should match", Elements[0].Text, TEXT("Hello "));
		TestEqual("Second element should have mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text should match", Elements[1].Text, TEXT("World"));
		TestEqual("Third element should be text", Elements[2].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Third element text should be empty", Elements[2].Text, TEXT(""));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionAtEndTest, "PubnubChat.Unit.MessageDraft.AddMention.AtEnd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionAtEndTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Add mention at end
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	FPubnubChatOperationResult Result = Draft->AddMention(6, 5, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionChannelTest, "PubnubChat.Unit.MessageDraft.AddMention.Channel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionChannelTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Add channel mention
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	MentionTarget.Target = TEXT("channel123");
	
	FPubnubChatOperationResult Result = Draft->AddMention(0, 5, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	if (Elements.Num() == 1)
	{
		TestEqual("Element should have channel mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_Channel);
		TestEqual("Channel target should match", Elements[0].MentionTarget.Target, TEXT("channel123"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionUrlTest, "PubnubChat.Unit.MessageDraft.AddMention.Url", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionUrlTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Check this"));
	
	// Add URL mention
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	MentionTarget.Target = TEXT("https://example.com");
	
	FPubnubChatOperationResult Result = Draft->AddMention(6, 4, MentionTarget);
	TestFalse("AddMention should succeed", Result.Error);
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	if (Elements.Num() >= 2)
	{
		// Find the URL mention element
		bool bFoundUrlMention = false;
		for (const FPubnubChatMessageElement& Element : Elements)
		{
			if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_Url)
			{
				bFoundUrlMention = true;
				TestEqual("URL target should match", Element.MentionTarget.Target, TEXT("https://example.com"));
				break;
			}
		}
		TestTrue("Should have URL mention", bFoundUrlMention);
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionNegativePositionTest, "PubnubChat.Unit.MessageDraft.AddMention.NegativePosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionNegativePositionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	// Add mention with negative position should fail
	FPubnubChatOperationResult Result = Draft->AddMention(-1, 5, MentionTarget);
	TestTrue("AddMention should fail with negative position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionInvalidLengthTest, "PubnubChat.Unit.MessageDraft.AddMention.InvalidLength", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionInvalidLengthTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	// Add mention with zero length should fail
	FPubnubChatOperationResult Result = Draft->AddMention(0, 0, MentionTarget);
	TestTrue("AddMention should fail with zero length", Result.Error);
	
	// Add mention with negative length should fail
	Result = Draft->AddMention(0, -1, MentionTarget);
	TestTrue("AddMention should fail with negative length", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionEmptyDraftTest, "PubnubChat.Unit.MessageDraft.AddMention.EmptyDraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionEmptyDraftTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	// Add mention to empty draft should fail
	FPubnubChatOperationResult Result = Draft->AddMention(0, 5, MentionTarget);
	TestTrue("AddMention should fail on empty draft", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionPositionTooBigTest, "PubnubChat.Unit.MessageDraft.AddMention.PositionTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionPositionTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	// Add mention at position beyond end should fail
	FPubnubChatOperationResult Result = Draft->AddMention(10, 5, MentionTarget);
	TestTrue("AddMention should fail with position too big", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionRangeTooBigTest, "PubnubChat.Unit.MessageDraft.AddMention.RangeTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionRangeTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	
	// Add mention with range that extends beyond end should fail
	FPubnubChatOperationResult Result = Draft->AddMention(3, 10, MentionTarget);
	TestTrue("AddMention should fail when range extends beyond end", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddMentionOverlapsExistingMentionTest, "PubnubChat.Unit.MessageDraft.AddMention.OverlapsExistingMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddMentionOverlapsExistingMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget1;
	MentionTarget1.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget1.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget1); // Mention "World"
	
	// Try to add mention that overlaps with existing mention (should fail)
	FPubnubChatMentionTarget MentionTarget2;
	MentionTarget2.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget2.Target = TEXT("user456");
	
	// Overlap at start
	FPubnubChatOperationResult Result = Draft->AddMention(5, 3, MentionTarget2);
	TestTrue("AddMention should fail when overlapping with existing mention", Result.Error);
	
	// Overlap in middle
	Result = Draft->AddMention(7, 3, MentionTarget2);
	TestTrue("AddMention should fail when overlapping with existing mention", Result.Error);
	
	// Overlap at end
	Result = Draft->AddMention(9, 3, MentionTarget2);
	TestTrue("AddMention should fail when overlapping with existing mention", Result.Error);
	
	// Completely covers existing mention
	Result = Draft->AddMention(6, 5, MentionTarget2);
	TestTrue("AddMention should fail when covering existing mention", Result.Error);
	
	return true;
}

// ============================================================================
// REMOVE MENTION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionTest, "PubnubChat.Unit.MessageDraft.RemoveMention.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(0, 5, MentionTarget);
	
	// Remove mention - this removes the entire element including text
	FPubnubChatOperationResult Result = Draft->RemoveMention(0);
	TestFalse("RemoveMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be removed along with mention", CurrentText, TEXT(""));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have no elements", Elements.Num(), 0);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionInMiddleTest, "PubnubChat.Unit.MessageDraft.RemoveMention.InMiddle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionInMiddleTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention in middle
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	// Remove mention (position anywhere within the mention) - this removes the entire mention element including text
	FPubnubChatOperationResult Result = Draft->RemoveMention(7);
	TestFalse("RemoveMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Mention text should be removed", CurrentText, TEXT("Hello "));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify no mentions remain and text before mention is preserved
	TestEqual("Should have one element", Elements.Num(), 1);
	if (Elements.Num() == 1)
	{
		TestEqual("Element should be text (no mention)", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Element text should match", Elements[0].Text, TEXT("Hello "));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionNotAtStartTest, "PubnubChat.Unit.MessageDraft.RemoveMention.NotAtStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionNotAtStartTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(0, 5, MentionTarget);
	
	// Remove mention at position 2 (within the mention, not at start) - this removes the entire mention element including text
	FPubnubChatOperationResult Result = Draft->RemoveMention(2);
	TestFalse("RemoveMention should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be removed along with mention", CurrentText, TEXT(""));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have no elements", Elements.Num(), 0);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionInvalidPositionTest, "PubnubChat.Unit.MessageDraft.RemoveMention.InvalidPosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionInvalidPositionTest::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Try to remove mention at text position (no mention there)
	FPubnubChatOperationResult Result = Draft->RemoveMention(0);
	TestTrue("RemoveMention should fail when no mention at position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionNegativePositionTest, "PubnubChat.Unit.MessageDraft.RemoveMention.NegativePosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionNegativePositionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Remove mention with negative position should fail
	FPubnubChatOperationResult Result = Draft->RemoveMention(-1);
	TestTrue("RemoveMention should fail with negative position", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionEmptyDraftTest, "PubnubChat.Unit.MessageDraft.RemoveMention.EmptyDraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionEmptyDraftTest::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Remove mention from empty draft should fail
	FPubnubChatOperationResult Result = Draft->RemoveMention(0);
	TestTrue("RemoveMention should fail on empty draft", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionPositionTooBigTest, "PubnubChat.Unit.MessageDraft.RemoveMention.PositionTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionPositionTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Remove mention at position beyond end should fail
	FPubnubChatOperationResult Result = Draft->RemoveMention(10);
	TestTrue("RemoveMention should fail with position too big", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftRemoveMentionAtTextElementTest, "PubnubChat.Unit.MessageDraft.RemoveMention.AtTextElement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftRemoveMentionAtTextElementTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text (no mention)
	Draft->InsertText(0, TEXT("Hello World"));
	
	// Try to remove mention at text element position (should fail)
	FPubnubChatOperationResult Result = Draft->RemoveMention(0);
	TestTrue("RemoveMention should fail when position is in text element", Result.Error);
	
	Result = Draft->RemoveMention(5);
	TestTrue("RemoveMention should fail when position is in text element", Result.Error);
	
	return true;
}

// ============================================================================
// UPDATE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateNoChangeTest, "PubnubChat.Unit.MessageDraft.Update.NoChange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateNoChangeTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Update with same text
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello"));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateSimpleChangeTest, "PubnubChat.Unit.MessageDraft.Update.SimpleChange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateSimpleChangeTest::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Update text
	FPubnubChatOperationResult Result = Draft->Update(TEXT("World"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be updated", CurrentText, TEXT("World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdatePreserveMentionTest, "PubnubChat.Unit.MessageDraft.Update.PreserveMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdatePreserveMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	// Update with same text (mention should be preserved)
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello World"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should remain the same", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify mention is still there
	bool bHasMention = false;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_User &&
			Element.MentionTarget.Target == TEXT("user123"))
		{
			bHasMention = true;
			break;
		}
	}
	TestTrue("Mention should be preserved", bHasMention);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateRemoveMentionTest, "PubnubChat.Unit.MessageDraft.Update.RemoveMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateRemoveMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add mention
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	// Update text, changing the mention text (mention should be removed)
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello Universe"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be updated", CurrentText, TEXT("Hello Universe"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify mention is removed
	bool bHasMention = false;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		if (Element.MentionTarget.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None)
		{
			bHasMention = true;
			break;
		}
	}
	TestFalse("Mention should be removed when text changes", bHasMention);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateAppendTextTest, "PubnubChat.Unit.MessageDraft.Update.AppendText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateAppendTextTest::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Update by appending text
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello World"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be updated", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdatePrependTextTest, "PubnubChat.Unit.MessageDraft.Update.PrependText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdatePrependTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("World"));
	
	// Update by prepending text
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello World"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be updated", CurrentText, TEXT("Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateEmptyTextTest, "PubnubChat.Unit.MessageDraft.Update.EmptyText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateEmptyTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text
	Draft->InsertText(0, TEXT("Hello"));
	
	// Update to empty text
	FPubnubChatOperationResult Result = Draft->Update(TEXT(""));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be empty", CurrentText, TEXT(""));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have no elements", Elements.Num(), 0);
	
	return true;
}

// ============================================================================
// COMPLEX SCENARIO TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftComplexScenario1Test, "PubnubChat.Unit.MessageDraft.ComplexScenario1", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftComplexScenario1Test::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Complex scenario: Insert, add mention, insert more, remove text, remove mention
	Draft->InsertText(0, TEXT("Hello"));
	Draft->InsertText(5, TEXT(" World"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	Draft->InsertText(11, TEXT("!"));
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be correct", CurrentText, TEXT("Hello World!"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Remove text
	Draft->RemoveText(11, 1);
	CurrentText = Draft->GetCurrentText();
	TestEqual("Text after removal should be correct", CurrentText, TEXT("Hello World"));
	
	// Remove mention - this removes the entire mention element including its text ("World")
	Draft->RemoveMention(6);
	CurrentText = Draft->GetCurrentText();
	TestEqual("Text after mention removal should be correct (mention text removed)", CurrentText, TEXT("Hello "));
	
	Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured after all operations", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify we have one text element with "Hello "
	TestEqual("Should have one element", Elements.Num(), 1);
	if (Elements.Num() == 1)
	{
		TestEqual("Element should be text (no mention)", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Element text should match", Elements[0].Text, TEXT("Hello "));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftComplexScenario2Test, "PubnubChat.Unit.MessageDraft.ComplexScenario2", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftComplexScenario2Test::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Complex scenario: Multiple mentions with text between
	Draft->InsertText(0, TEXT("Hello World Test"));
	
	FPubnubChatMentionTarget MentionTarget1;
	MentionTarget1.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget1.Target = TEXT("user1");
	Draft->AddMention(0, 5, MentionTarget1);
	
	FPubnubChatMentionTarget MentionTarget2;
	MentionTarget2.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget2.Target = TEXT("user2");
	Draft->AddMention(6, 5, MentionTarget2);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be correct", CurrentText, TEXT("Hello World Test"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify we have 3 elements: mention, text, mention, text
	TestEqual("Should have correct number of elements", Elements.Num(), 4);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftUpdateComplexTest, "PubnubChat.Unit.MessageDraft.Update.Complex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftUpdateComplexTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Create text with mentions
	Draft->InsertText(0, TEXT("Hello World"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	// Update: Change text but preserve one mention's text
	FPubnubChatOperationResult Result = Draft->Update(TEXT("Hello Universe"));
	TestFalse("Update should succeed", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be updated", CurrentText, TEXT("Hello Universe"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftMultipleSequentialInsertsTest, "PubnubChat.Unit.MessageDraft.MultipleSequentialInserts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftMultipleSequentialInsertsTest::RunTest(const FString& Parameters)
{
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Multiple sequential inserts
	Draft->InsertText(0, TEXT("H"));
	Draft->InsertText(1, TEXT("e"));
	Draft->InsertText(2, TEXT("l"));
	Draft->InsertText(3, TEXT("l"));
	Draft->InsertText(4, TEXT("o"));
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be correct after sequential inserts", CurrentText, TEXT("Hello"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertRemoveInsertTest, "PubnubChat.Unit.MessageDraft.InsertRemoveInsert", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertRemoveInsertTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Insert, remove, insert again
	Draft->InsertText(0, TEXT("Hello World"));
	Draft->RemoveText(5, 6);
	Draft->InsertText(5, TEXT(" Universe"));
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be correct", CurrentText, TEXT("Hello Universe"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAddRemoveMentionTest, "PubnubChat.Unit.MessageDraft.AddRemoveMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAddRemoveMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Add and remove mention
	Draft->InsertText(0, TEXT("Hello"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(0, 5, MentionTarget);
	
	// Verify mention exists
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	bool bHasMentionBefore = false;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_User)
		{
			bHasMentionBefore = true;
			break;
		}
	}
	TestTrue("Should have mention before removal", bHasMentionBefore);
	
	// Remove mention
	Draft->RemoveMention(0);
	
	// Verify mention is removed
	Elements = Draft->GetMessageElements();
	bool bHasMentionAfter = false;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		if (Element.MentionTarget.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None)
		{
			bHasMentionAfter = true;
			break;
		}
	}
	TestFalse("Should not have mention after removal", bHasMentionAfter);
	
	FString CurrentText = Draft->GetCurrentText();
	// RemoveMention removes the entire mention element including its text
	TestEqual("Text should be removed along with mention", CurrentText, TEXT(""));
	
	TArray<FPubnubChatMessageElement> FinalElements = Draft->GetMessageElements();
	TestEqual("Should have no elements after removing mention", FinalElements.Num(), 0);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftStructureInvariantTest, "PubnubChat.Unit.MessageDraft.StructureInvariant", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftStructureInvariantTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Perform various operations and verify structure is maintained
	Draft->InsertText(0, TEXT("Hello"));
	Draft->InsertText(5, TEXT(" World"));
	Draft->InsertText(11, TEXT(" Test"));
	
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user123");
	Draft->AddMention(6, 5, MentionTarget);
	
	Draft->RemoveText(11, 5);
	Draft->InsertText(11, TEXT(" Universe"));
	
	FString CurrentText = Draft->GetCurrentText();
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	
	// Verify structure invariants after all operations
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify no gaps
	for (int32 i = 0; i < Elements.Num() - 1; ++i)
	{
		int32 CurrentEnd = Elements[i].Start + Elements[i].Length;
		int32 NextStart = Elements[i + 1].Start;
		TestEqual("Elements should be contiguous (no gaps)", CurrentEnd, NextStart);
	}
	
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
