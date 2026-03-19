// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatMessageDraft.h"
#include "PubnubChatUser.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "FunctionLibraries/PubnubChatMessageDraftUtilities.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"

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
	
	// Insert text before mention (at position 5 = start of mention "World"; mention is preserved)
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
	
	// Insert text into the middle of mention: should succeed, mention is removed and text preserved with insert
	FPubnubChatOperationResult Result = Draft->InsertText(2, TEXT("X"));
	TestFalse("InsertText should succeed when inserting into middle of mention (mention is removed)", Result.Error);
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be HeXllo (mention removed, insert applied)", CurrentText, TEXT("HeXllo"));
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have one element (former mention is now plain text)", Elements.Num(), 1);
	TestEqual("Element should be plain text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
	TestEqual("Element text should match", Elements[0].Text, TEXT("HeXllo"));
	
	// Reset for next check: replace draft with single mention "Hello"
	Draft->Update(TEXT("Hello"));
	Draft->AddMention(0, 5, MentionTarget);
	
	// Insert text at start of mention (position 0): position is at mention start, so "before" mention – mention is preserved, new text element before it
	Result = Draft->InsertText(0, TEXT("X"));
	TestFalse("InsertText should succeed when inserting at start of mention", Result.Error);
	CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be XHello", CurrentText, TEXT("XHello"));
	Elements = Draft->GetMessageElements();
	TestEqual("Should have two elements (text + mention)", Elements.Num(), 2);
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be plain text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text", Elements[0].Text, TEXT("X"));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text", Elements[1].Text, TEXT("Hello"));
	}
	
	// Reset: replace draft with single mention "Hello"
	Draft->Update(TEXT("Hello"));
	Draft->AddMention(0, 5, MentionTarget);
	
	// Insert text at position 4 (strictly inside mention, before last char): mention removed
	Result = Draft->InsertText(4, TEXT("X"));
	TestFalse("InsertText should succeed when inserting at end position of mention", Result.Error);
	CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be HellXo", CurrentText, TEXT("HellXo"));
	Elements = Draft->GetMessageElements();
	TestEqual("Should have one element", Elements.Num(), 1);
	TestEqual("Element should be plain text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertTextIntoMentionPreservesOtherMentionsTest, "PubnubChat.Unit.MessageDraft.InsertText.IntoMentionPreservesOtherMentions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertTextIntoMentionPreservesOtherMentionsTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);

	if (!Draft)
	{
		return false;
	}

	// Setup: "Hi" (mention) + " " (text) + "Tom" (mention) -> "Hi Tom"
	Draft->InsertText(0, TEXT("Hi"));
	FPubnubChatMentionTarget UserTarget;
	UserTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserTarget.Target = TEXT("user1");
	Draft->AddMention(0, 2, UserTarget);

	Draft->InsertText(2, TEXT(" "));

	Draft->InsertText(3, TEXT("Tom"));
	FPubnubChatMentionTarget TomTarget;
	TomTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	TomTarget.Target = TEXT("user_tom");
	Draft->AddMention(3, 3, TomTarget);

	// Insert "aa" inside the first mention at position 1 -> "H" + "aa" + "i" = "Haai", first mention removed
	// Second mention "Tom" must be preserved and its Start adjusted: was 3, insert length 2, so new Start = 5
	FPubnubChatOperationResult Result = Draft->InsertText(1, TEXT("aa"));
	TestFalse("InsertText should succeed", Result.Error);

	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be Haai Tom", CurrentText, TEXT("Haai Tom"));

	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have three elements: plain Haai, space, mention Tom", Elements.Num(), 3);
	if (Elements.Num() >= 3)
	{
		TestEqual("First element should be plain text (former mention)", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("First element text", Elements[0].Text, TEXT("Haai"));
		TestEqual("First element Start", Elements[0].Start, 0);

		TestEqual("Second element should be plain text", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Second element text", Elements[1].Text, TEXT(" "));
		TestEqual("Second element Start adjusted", Elements[1].Start, 4);

		TestEqual("Third element should still be mention", Elements[2].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Third element target preserved", Elements[2].MentionTarget.Target, TEXT("user_tom"));
		TestEqual("Third element text", Elements[2].Text, TEXT("Tom"));
		TestEqual("Third element Start adjusted by insert length", Elements[2].Start, 5);
	}

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
	
	// Insert text at start of mention (position 0). Position 0 is at the start of the mention, so this is "before" the mention: add text before it (mention preserved).
	FPubnubChatOperationResult Result = Draft->InsertText(0, TEXT("Hi "));
	TestFalse("InsertText should succeed when inserting at start of mention", Result.Error);
	
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be Hi Hello World", CurrentText, TEXT("Hi Hello World"));
	
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify structure: text element, mention, text element (mention preserved when inserting at its start)
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
// APPEND TEXT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAppendTextEmptyTest, "PubnubChat.Unit.MessageDraft.AppendText.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAppendTextEmptyTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);

	if (!Draft)
	{
		return false;
	}

	// Append to empty draft
	FPubnubChatOperationResult Result = Draft->AppendText(TEXT("Hello"));
	TestFalse("AppendText should succeed on empty draft", Result.Error);

	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be appended", CurrentText, TEXT("Hello"));

	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have one element", Elements.Num(), 1);
	if (Elements.Num() == 1)
	{
		TestEqual("Element should be plain text", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Element text", Elements[0].Text, TEXT("Hello"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAppendTextWithTextTest, "PubnubChat.Unit.MessageDraft.AppendText.WithText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAppendTextWithTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);

	if (!Draft)
	{
		return false;
	}

	// Setup: draft with plain text
	Draft->InsertText(0, TEXT("Hello"));

	// Append text at end
	FPubnubChatOperationResult Result = Draft->AppendText(TEXT(" World"));
	TestFalse("AppendText should succeed", Result.Error);

	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be Hello World", CurrentText, TEXT("Hello World"));

	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	TestEqual("Should have one element (appended to last text)", Elements.Num(), 1);
	if (Elements.Num() == 1)
	{
		TestEqual("Element text", Elements[0].Text, TEXT("Hello World"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftAppendTextWithMentionAtEndTest, "PubnubChat.Unit.MessageDraft.AppendText.WithMentionAtEnd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftAppendTextWithMentionAtEndTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);

	if (!Draft)
	{
		return false;
	}

	// Setup: text + mention at end ("Hi " + mention "Tom")
	Draft->InsertText(0, TEXT("Hi "));
	Draft->InsertText(3, TEXT("Tom"));
	FPubnubChatMentionTarget MentionTarget;
	MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	MentionTarget.Target = TEXT("user_tom");
	Draft->AddMention(3, 3, MentionTarget);

	// Append after the mention
	FPubnubChatOperationResult Result = Draft->AppendText(TEXT("!"));
	TestFalse("AppendText should succeed when mention is at end", Result.Error);

	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should be Hi Tom!", CurrentText, TEXT("Hi Tom!"));

	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	TestEqual("Should have three elements (text, mention, appended text)", Elements.Num(), 3);
	if (Elements.Num() == 3)
	{
		TestEqual("First element text", Elements[0].Text, TEXT("Hi "));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element text", Elements[1].Text, TEXT("Tom"));
		TestEqual("Third element should be appended text", Elements[2].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		TestEqual("Third element text", Elements[2].Text, TEXT("!"));
	}

	return true;
}

// Helper: compare parsed elements to draft elements (Text, MentionTarget type/target, Start, Length)
static bool MessageElementsEqual(const TArray<FPubnubChatMessageElement>& Expected, const TArray<FPubnubChatMessageElement>& Actual)
{
	if (Expected.Num() != Actual.Num()) return false;
	for (int32 i = 0; i < Expected.Num(); ++i)
	{
		if (Expected[i].Text != Actual[i].Text) return false;
		if (Expected[i].Start != Actual[i].Start) return false;
		if (Expected[i].Length != Actual[i].Length) return false;
		if (Expected[i].MentionTarget.MentionTargetType != Actual[i].MentionTarget.MentionTargetType) return false;
		if (Expected[i].MentionTarget.Target != Actual[i].MentionTarget.Target) return false;
	}
	return true;
}

/** Compare message elements by content only (Text, MentionTarget). Ignores Start/Length and filters out empty elements so draft vs received round-trip can match. */
static bool MessageElementsContentEqual(const TArray<FPubnubChatMessageElement>& Expected, const TArray<FPubnubChatMessageElement>& Actual)
{
	TArray<FPubnubChatMessageElement> E, A;
	for (const FPubnubChatMessageElement& El : Expected) { if (El.Text.Len() > 0 || El.MentionTarget.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None) E.Add(El); }
	for (const FPubnubChatMessageElement& El : Actual)   { if (El.Text.Len() > 0 || El.MentionTarget.MentionTargetType != EPubnubChatMentionTargetType::PCMTT_None) A.Add(El); }
	if (E.Num() != A.Num()) return false;
	for (int32 i = 0; i < E.Num(); ++i)
	{
		if (E[i].Text != A[i].Text) return false;
		if (E[i].MentionTarget.MentionTargetType != A[i].MentionTarget.MentionTargetType) return false;
		if (E[i].MentionTarget.Target != A[i].MentionTarget.Target) return false;
	}
	return true;
}

// ============================================================================
// PARSE MESSAGE MARKDOWN TESTS (round-trip: draft -> GetTextToSend -> Parse -> compare to draft elements)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownPlainTextOnlyTest, "PubnubChat.Unit.ParseMessageMarkdown.PlainTextOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownPlainTextOnlyTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("Hello World"));
	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestEqual("Markdown should be plain text", Markdown, TEXT("Hello World"));
	TestTrue("Parsed elements should match draft", MessageElementsEqual(Expected, Parsed));
	TestEqual("One element", Parsed.Num(), 1);
	if (Parsed.Num() == 1)
	{
		TestEqual("Text", Parsed[0].Text, TEXT("Hello World"));
		TestEqual("No mention", Parsed[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownEmptyTest, "PubnubChat.Unit.ParseMessageMarkdown.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownEmptyTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(TEXT(""));
	TestEqual("Empty markdown should parse to zero elements", Parsed.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownSingleUserMentionTest, "PubnubChat.Unit.ParseMessageMarkdown.SingleUserMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownSingleUserMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("Tom"));
	FPubnubChatMentionTarget UserTarget;
	UserTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserTarget.Target = TEXT("user-123");
	Draft->AddMention(0, 3, UserTarget);

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestTrue("Parsed elements should match draft", MessageElementsEqual(Expected, Parsed));
	TestEqual("One element", Parsed.Num(), 1);
	if (Parsed.Num() == 1)
	{
		TestEqual("Text", Parsed[0].Text, TEXT("Tom"));
		TestEqual("User mention", Parsed[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Target", Parsed[0].MentionTarget.Target, TEXT("user-123"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownSingleChannelMentionTest, "PubnubChat.Unit.ParseMessageMarkdown.SingleChannelMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownSingleChannelMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("general"));
	FPubnubChatMentionTarget ChannelTarget;
	ChannelTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	ChannelTarget.Target = TEXT("channel-456");
	Draft->AddMention(0, 7, ChannelTarget);

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestTrue("Parsed elements should match draft", MessageElementsEqual(Expected, Parsed));
	TestEqual("One element", Parsed.Num(), 1);
	if (Parsed.Num() == 1)
	{
		TestEqual("Text", Parsed[0].Text, TEXT("general"));
		TestEqual("Channel mention", Parsed[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_Channel);
		TestEqual("Target", Parsed[0].MentionTarget.Target, TEXT("channel-456"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownSingleUrlMentionTest, "PubnubChat.Unit.ParseMessageMarkdown.SingleUrlMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownSingleUrlMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("click here"));
	FPubnubChatMentionTarget UrlTarget;
	UrlTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	UrlTarget.Target = TEXT("https://example.com/page");
	Draft->AddMention(0, 10, UrlTarget);

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestTrue("Parsed elements should match draft", MessageElementsEqual(Expected, Parsed));
	TestEqual("One element", Parsed.Num(), 1);
	if (Parsed.Num() == 1)
	{
		TestEqual("Text", Parsed[0].Text, TEXT("click here"));
		TestEqual("Url mention", Parsed[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_Url);
		TestEqual("Target", Parsed[0].MentionTarget.Target, TEXT("https://example.com/page"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownMixedTextAndUserMentionTest, "PubnubChat.Unit.ParseMessageMarkdown.MixedTextAndUserMention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownMixedTextAndUserMentionTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("Hi "));
	Draft->InsertText(3, TEXT("Tom"));
	FPubnubChatMentionTarget UserTarget;
	UserTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserTarget.Target = TEXT("user_tom");
	Draft->AddMention(3, 3, UserTarget);
	Draft->AppendText(TEXT("!"));

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestTrue("Parsed elements should match draft", MessageElementsEqual(Expected, Parsed));
	TestEqual("Three elements", Parsed.Num(), 3);
	if (Parsed.Num() == 3)
	{
		TestEqual("First text", Parsed[0].Text, TEXT("Hi "));
		TestEqual("Second mention text", Parsed[1].Text, TEXT("Tom"));
		TestEqual("Second user", Parsed[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Third text", Parsed[2].Text, TEXT("!"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownAllMentionTypesTest, "PubnubChat.Unit.ParseMessageMarkdown.AllMentionTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownAllMentionTypesTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	// "Hi " + user "Tom" + " in " + channel "general" + " see " + url "link"
	Draft->InsertText(0, TEXT("Hi "));
	Draft->InsertText(3, TEXT("Tom"));
	FPubnubChatMentionTarget UserTarget;
	UserTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserTarget.Target = TEXT("user_tom");
	Draft->AddMention(3, 3, UserTarget);

	Draft->AppendText(TEXT(" in "));
	Draft->AppendText(TEXT("general"));
	FPubnubChatMentionTarget ChannelTarget;
	ChannelTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	ChannelTarget.Target = TEXT("ch-general");
	Draft->AddMention(10, 7, ChannelTarget);

	Draft->AppendText(TEXT(" see "));
	Draft->AppendText(TEXT("link"));
	FPubnubChatMentionTarget UrlTarget;
	UrlTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	UrlTarget.Target = TEXT("https://example.com");
	Draft->AddMention(21, 4, UrlTarget);

	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	// Draft can have 7 elements (empty TextAfter from AddMention split); parser returns 7 when markdown has 7 segments
	TestEqual("Seven elements (text, user, text, channel, text, url, optional empty)", Parsed.Num(), 7);
	if (Parsed.Num() >= 6)
	{
		TestEqual("User mention target", Parsed[1].MentionTarget.Target, TEXT("user_tom"));
		TestEqual("Channel mention target", Parsed[3].MentionTarget.Target, TEXT("ch-general"));
		// Url mention: find by type in case index varies (e.g. empty segment at 6)
		int32 UrlIdx = INDEX_NONE;
		for (int32 k = 0; k < Parsed.Num(); ++k)
			if (Parsed[k].MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_Url)
			{ UrlIdx = k; break; }
		TestTrue("Has url mention", UrlIdx != INDEX_NONE);
		if (UrlIdx != INDEX_NONE)
			TestEqual("Url mention target", Parsed[UrlIdx].MentionTarget.Target, TEXT("https://example.com"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownEscapedLinkTextTest, "PubnubChat.Unit.ParseMessageMarkdown.EscapedLinkText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownEscapedLinkTextTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	// Link text can contain ] and \ - draft escapes them. Build draft with text that would be escaped.
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("see [here]"));
	FPubnubChatMentionTarget UrlTarget;
	UrlTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	UrlTarget.Target = TEXT("https://x.com");
	Draft->AddMention(4, 6, UrlTarget); // mention "[here]" (6 chars: positions 4-9)

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestEqual("Two elements", Parsed.Num(), 2);
	if (Parsed.Num() == 2)
	{
		TestEqual("First text", Parsed[0].Text, TEXT("see "));
		TestEqual("Link text with brackets", Parsed[1].Text, TEXT("[here]"));
		TestEqual("Second is Url mention", Parsed[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_Url);
		TestEqual("Url target", Parsed[1].MentionTarget.Target, TEXT("https://x.com"));
	}
	// Round-trip: parsed display text should match draft display text
	FString ParsedText;
	for (const auto& E : Parsed) ParsedText.Append(E.Text);
	TestEqual("Parsed text matches draft", ParsedText, Draft->GetCurrentText());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownEscapedUrlTest, "PubnubChat.Unit.ParseMessageMarkdown.EscapedUrl", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownEscapedUrlTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	// URL can contain ) - draft escapes it. Use a URL with parenthesis.
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	if (!Draft) return false;

	Draft->InsertText(0, TEXT("link"));
	FPubnubChatMentionTarget UrlTarget;
	UrlTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	UrlTarget.Target = TEXT("https://example.com/path(1)");
	Draft->AddMention(0, 4, UrlTarget);

	TArray<FPubnubChatMessageElement> Expected = Draft->GetMessageElements();
	FString Markdown = Draft->GetTextToSend();
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(Markdown);

	TestTrue("Parsed elements should match draft (escaped URL)", MessageElementsEqual(Expected, Parsed));
	TestEqual("One element", Parsed.Num(), 1);
	if (Parsed.Num() == 1)
		TestEqual("URL with parenthesis", Parsed[0].MentionTarget.Target, TEXT("https://example.com/path(1)"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatParseMessageMarkdownPlainTextNoLinksTest, "PubnubChat.Unit.ParseMessageMarkdown.PlainTextNoLinks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatParseMessageMarkdownPlainTextNoLinksTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;

	// Text that looks like markdown but isn't (no closing ] for the [) parses as two plain segments
	TArray<FPubnubChatMessageElement> Parsed = UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(TEXT("Hello [world (no link)"));
	TestEqual("Malformed markdown parses as plain segments", Parsed.Num(), 2);
	if (Parsed.Num() >= 2)
	{
		TestEqual("First segment", Parsed[0].Text, TEXT("Hello "));
		TestEqual("Rest as second segment", Parsed[1].Text, TEXT("[world (no link)"));
	}
	FString Reconstructed;
	for (const auto& E : Parsed) Reconstructed.Append(E.Text);
	TestEqual("Reconstructed equals input", Reconstructed, TEXT("Hello [world (no link)"));
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

// ============================================================================
// INSERT SUGGESTED MENTION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionBasicUserTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.BasicUser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionBasicUserTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text with a potential mention
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestFalse("InsertSuggestedMention should succeed", Result.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have mention replaced", CurrentText, TEXT("Hello John Doe"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 2 elements", Elements.Num(), 2);
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be text", Elements[0].Text, TEXT("Hello "));
		TestEqual("First element should have no mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_None);
		
		TestEqual("Second element should be mention text", Elements[1].Text, TEXT("John Doe"));
		TestEqual("Second element should be user mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element should have correct user ID", Elements[1].MentionTarget.Target, TEXT("user123"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionBasicChannelTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.BasicChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionBasicChannelTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text with a potential channel mention
	Draft->InsertText(0, TEXT("Join #general"));
	
	// Create suggested mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 5;
	SuggestedMention.ReplaceFrom = TEXT("#general");
	SuggestedMention.ReplaceTo = TEXT("General Discussion");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	SuggestedMention.Target.Target = TEXT("channel456");
	
	// Insert suggested mention
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestFalse("InsertSuggestedMention should succeed", Result.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have channel mention replaced", CurrentText, TEXT("Join General Discussion"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 2 elements", Elements.Num(), 2);
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be text", Elements[0].Text, TEXT("Join "));
		TestEqual("Second element should be channel mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_Channel);
		TestEqual("Second element should have correct channel ID", Elements[1].MentionTarget.Target, TEXT("channel456"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionAtStartTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.AtStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionAtStartTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text starting with mention
	Draft->InsertText(0, TEXT("@alice Hello"));
	
	// Create suggested mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 0;
	SuggestedMention.ReplaceFrom = TEXT("@alice");
	SuggestedMention.ReplaceTo = TEXT("Alice Smith");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user789");
	
	// Insert suggested mention
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestFalse("InsertSuggestedMention should succeed", Result.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have mention at start", CurrentText, TEXT("Alice Smith Hello"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 2 elements", Elements.Num(), 2);
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Second element should be text", Elements[1].Text, TEXT(" Hello"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionInMiddleTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.InMiddle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionInMiddleTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text with mention in middle
	Draft->InsertText(0, TEXT("Hello @bob world"));
	
	// Create suggested mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@bob");
	SuggestedMention.ReplaceTo = TEXT("Bob Johnson");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user456");
	
	// Insert suggested mention
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestFalse("InsertSuggestedMention should succeed", Result.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have mention in middle", CurrentText, TEXT("Hello Bob Johnson world"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 3 elements", Elements.Num(), 3);
	
	if (Elements.Num() == 3)
	{
		TestEqual("First element should be text", Elements[0].Text, TEXT("Hello "));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Third element should be text", Elements[2].Text, TEXT(" world"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionAtEndTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.AtEnd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionAtEndTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text ending with mention
	Draft->InsertText(0, TEXT("Hello @charlie"));
	
	// Create suggested mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@charlie");
	SuggestedMention.ReplaceTo = TEXT("Charlie Brown");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user999");
	
	// Insert suggested mention
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestFalse("InsertSuggestedMention should succeed", Result.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have mention at end", CurrentText, TEXT("Hello Charlie Brown"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 2 elements", Elements.Num(), 2);
	
	if (Elements.Num() == 2)
	{
		TestEqual("First element should be text", Elements[0].Text, TEXT("Hello "));
		TestEqual("Second element should be mention", Elements[1].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionNegativeOffsetTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.NegativeOffset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionNegativeOffsetTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with negative offset
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = -1;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with negative offset", Result.Error);
	TestTrue("Error message should mention offset", Result.ErrorMessage.Contains(TEXT("Offset")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionOffsetTooBigTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.OffsetTooBig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionOffsetTooBigTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with offset beyond text
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 100;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with offset too big", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionTextMismatchTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.TextMismatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionTextMismatchTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with ReplaceFrom that doesn't match actual text
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@jane"); // Doesn't match "@john"
	SuggestedMention.ReplaceTo = TEXT("Jane Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with text mismatch", Result.Error);
	TestTrue("Error message should mention invalid", Result.ErrorMessage.Contains(TEXT("invalid")) || Result.ErrorMessage.Contains(TEXT("expected")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionEmptyReplaceFromTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.EmptyReplaceFrom", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionEmptyReplaceFromTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with empty ReplaceFrom
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT(""); // Empty
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with empty ReplaceFrom", Result.Error);
	TestTrue("Error message should mention ReplaceFrom", Result.ErrorMessage.Contains(TEXT("ReplaceFrom")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionEmptyReplaceToTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.EmptyReplaceTo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionEmptyReplaceToTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with empty ReplaceTo
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT(""); // Empty
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with empty ReplaceTo", Result.Error);
	TestTrue("Error message should mention ReplaceTo", Result.ErrorMessage.Contains(TEXT("ReplaceTo")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionInvalidTargetTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.InvalidTarget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionInvalidTargetTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	Draft->InsertText(0, TEXT("Hello @john"));
	
	// Create suggested mention with invalid target (None)
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None; // Invalid
	SuggestedMention.Target.Target = TEXT("user123");
	
	// Insert suggested mention should fail
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail with invalid target", Result.Error);
	TestTrue("Error message should mention target", Result.ErrorMessage.Contains(TEXT("Target")) || Result.ErrorMessage.Contains(TEXT("mention target")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionMultipleMentionsTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.MultipleMentions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionMultipleMentionsTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text with multiple potential mentions
	Draft->InsertText(0, TEXT("@alice and @bob"));
	
	// Insert first suggested mention
	FPubnubChatSuggestedMention SuggestedMention1;
	SuggestedMention1.Offset = 0;
	SuggestedMention1.ReplaceFrom = TEXT("@alice");
	SuggestedMention1.ReplaceTo = TEXT("Alice");
	SuggestedMention1.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention1.Target.Target = TEXT("user1");
	
	FPubnubChatOperationResult Result1 = Draft->InsertSuggestedMention(SuggestedMention1);
	TestFalse("First InsertSuggestedMention should succeed", Result1.Error);
	
	// Insert second suggested mention (offset needs to be adjusted after first insertion)
	// After first insertion: "Alice and @bob" - @bob is now at offset 10
	FPubnubChatSuggestedMention SuggestedMention2;
	SuggestedMention2.Offset = 10;
	SuggestedMention2.ReplaceFrom = TEXT("@bob");
	SuggestedMention2.ReplaceTo = TEXT("Bob");
	SuggestedMention2.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention2.Target.Target = TEXT("user2");
	
	FPubnubChatOperationResult Result2 = Draft->InsertSuggestedMention(SuggestedMention2);
	TestFalse("Second InsertSuggestedMention should succeed", Result2.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have both mentions", CurrentText, TEXT("Alice and Bob"));
	
	// Verify message elements
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestEqual("Should have 3 elements", Elements.Num(), 3);
	
	if (Elements.Num() == 3)
	{
		TestEqual("First element should be mention", Elements[0].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("First element should have user1", Elements[0].MentionTarget.Target, TEXT("user1"));
		TestEqual("Second element should be text", Elements[1].Text, TEXT(" and "));
		TestEqual("Third element should be mention", Elements[2].MentionTarget.MentionTargetType, EPubnubChatMentionTargetType::PCMTT_User);
		TestEqual("Third element should have user2", Elements[2].MentionTarget.Target, TEXT("user2"));
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionWithExistingMentionsTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.WithExistingMentions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionWithExistingMentionsTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Insert text and add an existing mention
	Draft->InsertText(0, TEXT("Hello @john world"));
	
	FPubnubChatMentionTarget ExistingMentionTarget;
	ExistingMentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	ExistingMentionTarget.Target = TEXT("user_existing");
	Draft->AddMention(6, 5, ExistingMentionTarget);
	
	// Now try to insert a suggested mention for @john (which is already a mention)
	// This should fail because we can't replace an existing mention
	FPubnubChatSuggestedMention SuggestedMention;
	SuggestedMention.Offset = 6;
	SuggestedMention.ReplaceFrom = TEXT("@john");
	SuggestedMention.ReplaceTo = TEXT("John Doe");
	SuggestedMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	SuggestedMention.Target.Target = TEXT("user123");
	
	// This should fail because RemoveText will fail (can't remove text from mention)
	FPubnubChatOperationResult Result = Draft->InsertSuggestedMention(SuggestedMention);
	TestTrue("InsertSuggestedMention should fail when trying to replace existing mention", Result.Error);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftInsertSuggestedMentionComplexScenarioTest, "PubnubChat.Unit.MessageDraft.InsertSuggestedMention.ComplexScenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftInsertSuggestedMentionComplexScenarioTest::RunTest(const FString& Parameters)
{
	bSuppressLogErrors = true;
	bSuppressLogWarnings = true;
	
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>(GetTransientPackage());
	TestNotNull("Draft should be created", Draft);
	
	if (!Draft)
	{
		return false;
	}
	
	// Setup: Complex text with multiple potential mentions
	Draft->InsertText(0, TEXT("Hey @alice, check out #general channel"));
	
	// Insert user mention
	FPubnubChatSuggestedMention UserMention;
	UserMention.Offset = 4;
	UserMention.ReplaceFrom = TEXT("@alice");
	UserMention.ReplaceTo = TEXT("Alice");
	UserMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserMention.Target.Target = TEXT("user_alice");
	
	FPubnubChatOperationResult Result1 = Draft->InsertSuggestedMention(UserMention);
	TestFalse("User mention insertion should succeed", Result1.Error);
	
	// After insertion: "Hey Alice, check out #general channel"
	// Original: "Hey @alice, check out #general channel"
	// "@alice" (6 chars) at offset 4 was replaced with "Alice" (5 chars)
	// "#general" was originally at offset 22, now shifted by (5-6) = -1
	// So "#general" is now at offset 21
	FString CurrentTextAfterFirst = Draft->GetCurrentText();
	int32 GeneralOffset = CurrentTextAfterFirst.Find(TEXT("#general"));
	TestTrue("Should find #general in text", GeneralOffset >= 0);
	
	FPubnubChatSuggestedMention ChannelMention;
	ChannelMention.Offset = GeneralOffset;
	ChannelMention.ReplaceFrom = TEXT("#general");
	ChannelMention.ReplaceTo = TEXT("General Discussion");
	ChannelMention.Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	ChannelMention.Target.Target = TEXT("channel_general");
	
	FPubnubChatOperationResult Result2 = Draft->InsertSuggestedMention(ChannelMention);
	TestFalse("Channel mention insertion should succeed", Result2.Error);
	
	// Verify final text
	FString CurrentText = Draft->GetCurrentText();
	TestEqual("Text should have both mentions", CurrentText, TEXT("Hey Alice, check out General Discussion channel"));
	
	// Verify structure
	TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
	TestTrue("Elements should be properly structured", VerifyMessageElementsStructure(Elements, CurrentText));
	
	// Verify we have user and channel mentions
	bool bFoundUserMention = false;
	bool bFoundChannelMention = false;
	for (const FPubnubChatMessageElement& Element : Elements)
	{
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_User && Element.MentionTarget.Target == TEXT("user_alice"))
		{
			bFoundUserMention = true;
		}
		if (Element.MentionTarget.MentionTargetType == EPubnubChatMentionTargetType::PCMTT_Channel && Element.MentionTarget.Target == TEXT("channel_general"))
		{
			bFoundChannelMention = true;
		}
	}
	
	TestTrue("Should have user mention", bFoundUserMention);
	TestTrue("Should have channel mention", bFoundChannelMention);
	
	return true;
}

// ============================================================================
// INTEGRATION TESTS - CreateMessageDraft and Send
// ============================================================================

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessage.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// CREATEMESSAGEDRAFT TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateMessageDraftNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.CreateMessageDraft.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateMessageDraftNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create uninitialized channel
	UPubnubChatChannel* Channel = NewObject<UPubnubChatChannel>();
	TestNotNull("Channel should be created", Channel);
	
	if(Channel)
	{
		// Try to create message draft on uninitialized channel
		FPubnubChatMessageDraftConfig Config;
		UPubnubChatMessageDraft* Draft = Channel->CreateMessageDraft(Config);
		
		TestNull("CreateMessageDraft should return nullptr when channel is not initialized", Draft);
	}

	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateMessageDraftHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.CreateMessageDraft.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateMessageDraftHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_draft_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_draft_happy";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create message draft with default config
			FPubnubChatMessageDraftConfig Config; // Default config
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("CreateMessageDraft should succeed", Draft);
			
			if(Draft)
			{
				// Verify draft is properly initialized
				FString CurrentText = Draft->GetCurrentText();
				TestEqual("Draft should start empty", CurrentText, TEXT(""));
				
				TArray<FPubnubChatMessageElement> Elements = Draft->GetMessageElements();
				TestEqual("Draft should have no elements initially", Elements.Num(), 0);
				
				// Verify draft can be used
				FPubnubChatOperationResult InsertResult = Draft->InsertText(0, TEXT("Test"));
				TestFalse("InsertText should succeed", InsertResult.Error);
				
				CurrentText = Draft->GetCurrentText();
				TestEqual("Draft text should be updated", CurrentText, TEXT("Test"));
			}
			
			// Cleanup: Delete created channel
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateMessageDraftFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.CreateMessageDraft.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateMessageDraftFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_draft_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_draft_full";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create message draft with all config parameters
			FPubnubChatMessageDraftConfig Config;
			Config.UserSuggestionSource = EPubnubChatMessageDraftSuggestionSource::PCMDSS_Channel;
			Config.IsTypingIndicatorTriggered = true;
			Config.UserLimit = 20;
			Config.ChannelLimit = 15;
			
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("CreateMessageDraft should succeed with all parameters", Draft);
			
			if(Draft)
			{
				// Verify draft is properly initialized
				FString CurrentText = Draft->GetCurrentText();
				TestEqual("Draft should start empty", CurrentText, TEXT(""));
				
				// Verify draft can be used
				FPubnubChatOperationResult InsertResult = Draft->InsertText(0, TEXT("Test"));
				TestFalse("InsertText should succeed", InsertResult.Error);
				
				CurrentText = Draft->GetCurrentText();
				TestEqual("Draft text should be updated", CurrentText, TEXT("Test"));
			}
			
			// Cleanup: Delete created channel
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

/**
 * Tests CreateMessageDraft with multiple drafts: Multiple drafts created from same channel.
 * Verifies that drafts are independent and can be used simultaneously.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCreateMessageDraftMultipleDraftsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.CreateMessageDraft.4Advanced.MultipleDrafts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCreateMessageDraftMultipleDraftsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_create_draft_multi_init";
	const FString TestChannelID = SDK_PREFIX + "test_create_draft_multi";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create multiple drafts
			FPubnubChatMessageDraftConfig Config1;
			UPubnubChatMessageDraft* Draft1 = CreateResult.Channel->CreateMessageDraft(Config1);
			
			FPubnubChatMessageDraftConfig Config2;
			UPubnubChatMessageDraft* Draft2 = CreateResult.Channel->CreateMessageDraft(Config2);
			
			TestNotNull("First draft should be created", Draft1);
			TestNotNull("Second draft should be created", Draft2);
			TestNotEqual("Drafts should be different objects", Draft1, Draft2);
			
			if(Draft1 && Draft2)
			{
				// Verify drafts are independent
				Draft1->InsertText(0, TEXT("Draft1"));
				Draft2->InsertText(0, TEXT("Draft2"));
				
				TestEqual("Draft1 text should be independent", Draft1->GetCurrentText(), TEXT("Draft1"));
				TestEqual("Draft2 text should be independent", Draft2->GetCurrentText(), TEXT("Draft2"));
			}
			
			// Cleanup: Delete created channel
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// SEND TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendInvalidChannelTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.1Validation.InvalidChannel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftSendInvalidChannelTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create draft without channel (not initialized)
	UPubnubChatMessageDraft* Draft = NewObject<UPubnubChatMessageDraft>();
	TestNotNull("Draft should be created", Draft);
	
	if(Draft)
	{
		// Add some text
		Draft->InsertText(0, TEXT("Test"));
		
		// Try to send without valid channel
		FPubnubChatSendTextParams SendParams;
		FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
		
		TestTrue("Send should fail when channel is invalid", SendResult.Error);
		TestFalse("ErrorMessage should not be empty", SendResult.ErrorMessage.IsEmpty());
	}

	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendEmptyDraftTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.1Validation.EmptyDraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatMessageDraftSendEmptyDraftTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_empty_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_empty";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Create empty draft
			FPubnubChatMessageDraftConfig Config;
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("Draft should be created", Draft);
			
			if(Draft)
			{
				// Try to send empty draft
				FPubnubChatSendTextParams SendParams;
				FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
				
				TestTrue("Send should fail when draft is empty", SendResult.Error);
				TestFalse("ErrorMessage should not be empty", SendResult.ErrorMessage.IsEmpty());
			}
			
			// Cleanup: Delete created channel
			Chat->DeleteChannel(TestChannelID);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_happy_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_happy";
	const FString TestMessageText = TEXT("Hello World");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Connect channel
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			// Create draft and add text
			FPubnubChatMessageDraftConfig Config;
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("Draft should be created", Draft);
			
			if(Draft)
			{
				FPubnubChatOperationResult InsertResult = Draft->InsertText(0, TestMessageText);
				TestFalse("InsertText should succeed", InsertResult.Error);
				
				// Set up message receiver using TSharedPtr to keep pointer alive
				TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
				TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
				auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
				{
					if(Message && !*ReceivedMessage)
					{
						*bMessageReceived = true;
						*ReceivedMessage = Message;
					}
				};
				CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
				
				// Send message with default params
				FPubnubChatSendTextParams SendParams; // Default params
				FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
				
				TestFalse("Send should succeed", SendResult.Error);
				
				// Wait for message to be received
				ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
					return *bMessageReceived;
				}, MAX_WAIT_TIME));
				
				// Verify message was received correctly
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceived, ReceivedMessage, TestMessageText, TestChannelID]()
				{
					TestTrue("Message should have been received", *bMessageReceived);
					
					if(*ReceivedMessage && IsValid(*ReceivedMessage))
					{
						TestEqual("Received message text should match", (*ReceivedMessage)->GetCurrentText(), TestMessageText);
						FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
						TestEqual("Received message channel ID should match", MessageData.ChannelID, TestChannelID);
					}
				}, 0.1f));
			}
			
			// Cleanup: Delete created channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
			{
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
				}
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.2f));
			
			return true;
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL PARAMETER TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendFullParametersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.3FullParameters.AllParameters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendFullParametersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_full_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_full";
	const FString TestMessageText = TEXT("Test message with all params");
	const FString TestMeta = TEXT("{\"key\":\"value\"}");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Connect channel
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			// Create draft and add text
			FPubnubChatMessageDraftConfig Config;
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("Draft should be created", Draft);
			
			if(Draft)
			{
				FPubnubChatOperationResult InsertResult = Draft->InsertText(0, TestMessageText);
				TestFalse("InsertText should succeed", InsertResult.Error);
				
				// Set up message receiver using TSharedPtr to keep pointer alive
				TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
				TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
				auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
				{
					if(Message && !*ReceivedMessage)
					{
						*bMessageReceived = true;
						*ReceivedMessage = Message;
					}
				};
				CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
				
				// Send message with all parameters
				FPubnubChatSendTextParams SendParams;
				SendParams.StoreInHistory = true;
				SendParams.SendByPost = false;
				SendParams.Meta = TestMeta;
				
				FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
				
				TestFalse("Send should succeed with all parameters", SendResult.Error);
				
				// Wait for message to be received
				ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
					return *bMessageReceived;
				}, MAX_WAIT_TIME));
				
				// Verify message was received correctly
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceived, ReceivedMessage, TestMessageText, TestChannelID]()
				{
					TestTrue("Message should have been received", *bMessageReceived);
					
					if(*ReceivedMessage && IsValid(*ReceivedMessage))
					{
						TestEqual("Received message text should match", (*ReceivedMessage)->GetCurrentText(), TestMessageText);
						FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
						TestEqual("Received message channel ID should match", MessageData.ChannelID, TestChannelID);
					}
				}, 0.1f));
			}
			
			// Cleanup: Delete created channel
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
			{
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
				}
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.2f));
			
			return true;
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Advanced test: Send draft with quoted message.
 * Connects to channel, sends and receives a message, creates a new draft with that message set as quoted,
 * sends the draft, then verifies the received message has Meta.quotedMessage with correct timetoken, text, userID, channelID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendWithQuotedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.WithQuotedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendWithQuotedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_quoted_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_quoted";
	const FString FirstMessageText = TEXT("First message to be quoted");
	const FString ReplyMessageText = TEXT("Reply quoting the first message");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);

	// Listen for messages
	TSharedPtr<bool> bFirstMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> FirstMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	auto OnFirstMessage = [bFirstMessageReceived, FirstMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*FirstMessage)
		{
			*bFirstMessageReceived = true;
			*FirstMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(OnFirstMessage);

	// Send first message (will be quoted later)
	FPubnubChatOperationResult SendFirstResult = CreateResult.Channel->SendText(FirstMessageText);
	TestFalse("SendText first message should succeed", SendFirstResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstMessageReceived]() -> bool { return *bFirstMessageReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, FirstMessage, FirstMessageText, ReplyMessageText, TestChannelID, InitUserID]()
	{
		if(!*FirstMessage || !IsValid(*FirstMessage))
		{
			AddError("First message was not received");
			CleanUpCurrentChatUser(Chat);
			CleanUp();
			return;
		}

		FString QuotedTimetoken = (*FirstMessage)->GetMessageTimetoken();
		FPubnubChatMessageData QuotedData = (*FirstMessage)->GetMessageData();
		TestFalse("Quoted timetoken should not be empty", QuotedTimetoken.IsEmpty());
		TestEqual("First message text should match", QuotedData.Text, FirstMessageText);

		// Create draft, set quoted message, add reply text, send
		FPubnubChatMessageDraftConfig Config;
		UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
		if(!Draft)
		{
			AddError("CreateMessageDraft failed");
			CleanUpCurrentChatUser(Chat);
			CleanUp();
			return;
		}
		Draft->SetQuotedMessage(*FirstMessage);
		Draft->InsertText(0, ReplyMessageText);

		TSharedPtr<bool> bReplyReceived = MakeShared<bool>(false);
		TSharedPtr<UPubnubChatMessage*> ReplyMessage = MakeShared<UPubnubChatMessage*>(nullptr);
		auto OnReply = [bReplyReceived, ReplyMessage](UPubnubChatMessage* Message)
		{
			if(Message && !*ReplyMessage)
			{
				*bReplyReceived = true;
				*ReplyMessage = Message;
			}
		};
		CreateResult.Channel->OnMessageReceivedNative.AddLambda(OnReply);

		FPubnubChatOperationResult SendReplyResult = Draft->Send(FPubnubChatSendTextParams());
		TestFalse("Draft Send with quoted message should succeed", SendReplyResult.Error);

		ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReplyReceived]() -> bool { return *bReplyReceived; }, MAX_WAIT_TIME));

		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReplyMessage, ReplyMessageText, TestChannelID, InitUserID, QuotedTimetoken, FirstMessageText, Chat]()
		{
			TestTrue("Reply message should have been received", *ReplyMessage && IsValid(*ReplyMessage));
			if(!*ReplyMessage || !IsValid(*ReplyMessage))
			{
				CleanUpCurrentChatUser(Chat);
				CleanUp();
				return;
			}

			FPubnubChatMessageData ReplyData = (*ReplyMessage)->GetMessageData();
			TestEqual("Reply message text should match", ReplyData.Text, ReplyMessageText);
			TestEqual("Reply channel ID should match", ReplyData.ChannelID, TestChannelID);

			// Verify Meta contains quotedMessage with correct values
			TestFalse("Reply Meta should not be empty", ReplyData.Meta.IsEmpty());
			TSharedPtr<FJsonObject> MetaJsonObject = MakeShareable(new FJsonObject);
			bool bParsed = UPubnubJsonUtilities::StringToJsonObject(ReplyData.Meta, MetaJsonObject);
			TestTrue("Reply Meta should be valid JSON", bParsed);
			if(bParsed)
			{
				const TSharedPtr<FJsonObject>* QuotedObj = nullptr;
				TestTrue("Meta should contain quotedMessage", MetaJsonObject->TryGetObjectField(ANSI_TO_TCHAR("quotedMessage"), QuotedObj) && QuotedObj && (*QuotedObj).IsValid());
				if(QuotedObj && (*QuotedObj).IsValid())
				{
					FString MetaTimetoken, MetaText, MetaUserID, MetaChannelID;
					(*QuotedObj)->TryGetStringField(ANSI_TO_TCHAR("timetoken"), MetaTimetoken);
					(*QuotedObj)->TryGetStringField(ANSI_TO_TCHAR("text"), MetaText);
					(*QuotedObj)->TryGetStringField(ANSI_TO_TCHAR("userID"), MetaUserID);
					(*QuotedObj)->TryGetStringField(ANSI_TO_TCHAR("channelID"), MetaChannelID);
					TestEqual("Quoted message timetoken in Meta should match", MetaTimetoken, QuotedTimetoken);
					TestEqual("Quoted message text in Meta should match", MetaText, FirstMessageText);
					TestEqual("Quoted message userID in Meta should match", MetaUserID, InitUserID);
					TestEqual("Quoted message channelID in Meta should match", MetaChannelID, TestChannelID);
				}
			}

			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
			{
				if(Chat) { Chat->DeleteChannel(TestChannelID); }
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.1f));
		}, 0.1f));
	}, 0.1f));

	return true;
}

/**
 * Advanced test: Send draft with quoted message and validate UPubnubChatMessage::GetQuotedMessage().
 * Same flow as WithQuotedMessage: send message, receive it, create draft with SetQuotedMessage, send draft, receive reply.
 * Validates that the reply message's GetQuotedMessage() returns FPubnubChatQuotedMessageData with correct Timetoken, Text, and UserID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendWithQuotedMessageGetQuotedMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.WithQuotedMessageGetQuotedMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendWithQuotedMessageGetQuotedMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_quoted_get_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_quoted_get";
	const FString FirstMessageText = TEXT("First message to be quoted in GetQuotedMessage test");
	const FString ReplyMessageText = TEXT("Reply that quotes the first message");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		CleanUp();
		return false;
	}

	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);

	TSharedPtr<bool> bFirstMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> FirstMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	auto OnFirstMessage = [bFirstMessageReceived, FirstMessage](UPubnubChatMessage* Message)
	{
		if(Message && !*FirstMessage)
		{
			*bFirstMessageReceived = true;
			*FirstMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(OnFirstMessage);

	FPubnubChatOperationResult SendFirstResult = CreateResult.Channel->SendText(FirstMessageText);
	TestFalse("SendText first message should succeed", SendFirstResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bFirstMessageReceived]() -> bool { return *bFirstMessageReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, CreateResult, FirstMessage, FirstMessageText, ReplyMessageText, TestChannelID, InitUserID]()
	{
		if(!*FirstMessage || !IsValid(*FirstMessage))
		{
			AddError("First message was not received");
			CleanUpCurrentChatUser(Chat);
			CleanUp();
			return;
		}

		FString QuotedTimetoken = (*FirstMessage)->GetMessageTimetoken();
		TestFalse("Quoted timetoken should not be empty", QuotedTimetoken.IsEmpty());

		FPubnubChatMessageDraftConfig Config;
		UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
		if(!Draft)
		{
			AddError("CreateMessageDraft failed");
			CleanUpCurrentChatUser(Chat);
			CleanUp();
			return;
		}
		Draft->SetQuotedMessage(*FirstMessage);
		Draft->InsertText(0, ReplyMessageText);

		TSharedPtr<bool> bReplyReceived = MakeShared<bool>(false);
		TSharedPtr<UPubnubChatMessage*> ReplyMessage = MakeShared<UPubnubChatMessage*>(nullptr);
		auto OnReply = [bReplyReceived, ReplyMessage](UPubnubChatMessage* Message)
		{
			if(Message && !*ReplyMessage)
			{
				*bReplyReceived = true;
				*ReplyMessage = Message;
			}
		};
		CreateResult.Channel->OnMessageReceivedNative.AddLambda(OnReply);

		FPubnubChatOperationResult SendReplyResult = Draft->Send(FPubnubChatSendTextParams());
		TestFalse("Draft Send with quoted message should succeed", SendReplyResult.Error);

		ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bReplyReceived]() -> bool { return *bReplyReceived; }, MAX_WAIT_TIME));

		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ReplyMessage, ReplyMessageText, TestChannelID, InitUserID, QuotedTimetoken, FirstMessageText, Chat]()
		{
			TestTrue("Reply message should have been received", *ReplyMessage && IsValid(*ReplyMessage));
			if(!*ReplyMessage || !IsValid(*ReplyMessage))
			{
				CleanUpCurrentChatUser(Chat);
				CleanUp();
				return;
			}

			FPubnubChatMessageData ReplyData = (*ReplyMessage)->GetMessageData();
			TestEqual("Reply message text should match", ReplyData.Text, ReplyMessageText);
			TestEqual("Reply channel ID should match", ReplyData.ChannelID, TestChannelID);

			// Validate GetQuotedMessage() returns correct FPubnubChatQuotedMessageData
			FPubnubChatQuotedMessageData QuotedData = (*ReplyMessage)->GetQuotedMessage();
			TestFalse("GetQuotedMessage: Timetoken should not be empty", QuotedData.Timetoken.IsEmpty());
			TestFalse("GetQuotedMessage: Text should not be empty", QuotedData.Text.IsEmpty());
			TestFalse("GetQuotedMessage: UserID should not be empty", QuotedData.UserID.IsEmpty());
			TestEqual("GetQuotedMessage: Timetoken should match quoted message", QuotedData.Timetoken, QuotedTimetoken);
			TestEqual("GetQuotedMessage: Text should match quoted message text", QuotedData.Text, FirstMessageText);
			TestEqual("GetQuotedMessage: UserID should match message author", QuotedData.UserID, InitUserID);

			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID]()
			{
				if(Chat) { Chat->DeleteChannel(TestChannelID); }
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.1f));
		}, 0.1f));
	}, 0.1f));

	return true;
}

/**
 * Advanced test: Send draft with user mentions and verify mentioned user receives via StreamMentions.
 * Two users: sender sends a draft containing a user mention; mentioned user has StreamMentions() active.
 * Verifies the mentioned user receives OnMentioned with correct MessageTimetoken, ChannelID, Text, MentionedByUserID.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendWithMentionedUsersStreamMentionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.WithMentionedUsersStreamMentions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendWithMentionedUsersStreamMentionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString SenderUserID = SDK_PREFIX + "test_send_mention_sender";
	const FString MentionedUserID = SDK_PREFIX + "test_send_mention_target";
	const FString TestChannelID = SDK_PREFIX + "test_send_mention_ch";
	const FString PlainPart = TEXT("Hello ");
	const FString MentionDisplayText = TEXT("TargetUser");
	const FString AfterPart = TEXT(", check this");
	const FString FullDraftText = PlainPart + MentionDisplayText + AfterPart;

	// Sender chat
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult SenderInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, SenderUserID, ChatConfig);
	TestFalse("InitChat for sender should succeed", SenderInitResult.Result.Error);
	UPubnubChat* SenderChat = SenderInitResult.Chat;
	if(!SenderChat)
	{
		CleanUp();
		return false;
	}

	// Create the user that will be mentioned (so they exist in the system)
	FPubnubChatUserData MentionedUserData;
	MentionedUserData.UserName = MentionDisplayText;
	FPubnubChatUserResult CreateUserResult = SenderChat->CreateUser(MentionedUserID, MentionedUserData);
	TestFalse("CreateUser for mentioned user should succeed", CreateUserResult.Result.Error);

	// Mentioned user's chat (so they can call StreamMentions)
	FPubnubChatInitChatResult MentionedInitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, MentionedUserID, ChatConfig);
	TestFalse("InitChat for mentioned user should succeed", MentionedInitResult.Result.Error);
	UPubnubChat* MentionedChat = MentionedInitResult.Chat;
	if(!MentionedChat)
	{
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
		return false;
	}

	UPubnubChatUser* MentionedUser = PubnubChatTestHelpers::GetCurrentUserFromChat(MentionedChat);
	TestNotNull("Mentioned user should be obtainable from chat", MentionedUser);
	if(!MentionedUser)
	{
		CleanUpCurrentChatUser(MentionedChat);
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
		return false;
	}

	// Start listening for mentions (as the mentioned user)
	TSharedPtr<bool> bMentionReceived = MakeShared<bool>(false);
	TSharedPtr<FPubnubChatUserMention> ReceivedMention = MakeShared<FPubnubChatUserMention>();
	MentionedUser->OnMentionedNative.AddLambda([bMentionReceived, ReceivedMention](const FPubnubChatUserMention& M)
	{
		*bMentionReceived = true;
		*ReceivedMention = M;
	});
	FPubnubChatOperationResult StreamResult = MentionedUser->StreamMentions();
	TestFalse("StreamMentions should succeed", StreamResult.Error);

	// Sender: create channel, connect, create draft with user mention, send
	FPubnubChatChannelData ChannelData;
	FPubnubChatChannelResult CreateResult = SenderChat->CreatePublicConversation(TestChannelID, ChannelData);
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);
	if(!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(MentionedChat);
		CleanUpCurrentChatUser(SenderChat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);

	FPubnubChatMessageDraftConfig Config;
	UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
	TestNotNull("Draft should be created", Draft);
	if(!Draft)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, TestChannelID, MentionedUserID]()
		{
			if(SenderChat) { SenderChat->DeleteChannel(TestChannelID); SenderChat->DeleteUser(MentionedUserID); }
			CleanUpCurrentChatUser(MentionedChat);
			CleanUpCurrentChatUser(SenderChat);
			CleanUp();
		}, 0.0f));
		return true;
	}

	Draft->InsertText(0, FullDraftText);
	int32 MentionStart = PlainPart.Len();
	int32 MentionLen = MentionDisplayText.Len();
	FPubnubChatMentionTarget UserMentionTarget;
	UserMentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserMentionTarget.Target = MentionedUserID;
	FPubnubChatOperationResult AddMentionResult = Draft->AddMention(MentionStart, MentionLen, UserMentionTarget);
	TestFalse("AddMention should succeed", AddMentionResult.Error);

	FPubnubChatOperationResult SendResult = Draft->Send(FPubnubChatSendTextParams());
	TestFalse("Send draft with mention should succeed", SendResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMentionReceived]() -> bool { return *bMentionReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMentionReceived, ReceivedMention, TestChannelID, SenderUserID, MentionedUserID, MentionDisplayText, SenderChat, MentionedChat]()
	{
		TestTrue("Mention event should have been received", *bMentionReceived);
		if(*bMentionReceived)
		{
			TestEqual("Mention ChannelID should match", ReceivedMention->ChannelID, TestChannelID);
			TestEqual("MentionedByUserID should be sender", ReceivedMention->MentionedByUserID, SenderUserID);
			TestEqual("Mention text should match display text", ReceivedMention->Text, MentionDisplayText);
			TestFalse("MessageTimetoken should not be empty", ReceivedMention->MessageTimetoken.IsEmpty());
		}

		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, SenderChat, MentionedChat, TestChannelID, MentionedUserID]()
		{
			if(SenderChat)
			{
				SenderChat->DeleteChannel(TestChannelID);
				SenderChat->DeleteUser(MentionedUserID);
			}
			CleanUpCurrentChatUser(MentionedChat);
			CleanUpCurrentChatUser(SenderChat);
			CleanUp();
		}, 0.1f));
	}, 0.1f));

	return true;
}

// ============================================================================
// ADVANCED TESTS
// ============================================================================

/**
 * Tests Send with mentions: Message draft with user and channel mentions.
 * Verifies that mentions are properly rendered as markdown links and message is sent correctly.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendWithMentionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.WithMentions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendWithMentionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_mentions_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_mentions";
	const FString TestUserID = SDK_PREFIX + "test_send_mentions_user";
	const FString TestMentionChannelID = SDK_PREFIX + "test_send_mentions_channel";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test user for mention
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("TestUser");
		FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
		
		// Create mention channel
		FPubnubChatChannelData MentionChannelData;
		FPubnubChatChannelResult CreateMentionChannelResult = Chat->CreatePublicConversation(TestMentionChannelID, MentionChannelData);
		TestFalse("CreateMentionChannel should succeed", CreateMentionChannelResult.Result.Error);
		
		// Create main channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Connect channel
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			// Create draft with mentions
			FPubnubChatMessageDraftConfig Config;
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("Draft should be created", Draft);
			
			if(Draft)
			{
				// Build message: "Hello @user, check #channel"
				// First insert all text
				Draft->InsertText(0, TEXT("Hello user, check channel"));
				
				// Add user mention to "user" (starts at position 6, length 4)
				FPubnubChatMentionTarget UserMentionTarget;
				UserMentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
				UserMentionTarget.Target = TestUserID;
				FPubnubChatOperationResult AddUserMentionResult = Draft->AddMention(6, 4, UserMentionTarget);
				TestFalse("AddMention for user should succeed", AddUserMentionResult.Error);
				
				// Add channel mention to "channel" (starts at position 18, length 7)
				FPubnubChatMentionTarget ChannelMentionTarget;
				ChannelMentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
				ChannelMentionTarget.Target = TestMentionChannelID;
				FPubnubChatOperationResult AddChannelMentionResult = Draft->AddMention(18, 7, ChannelMentionTarget);
				TestFalse("AddMention for channel should succeed", AddChannelMentionResult.Error);
				
				// Verify draft text
				FString DraftText = Draft->GetCurrentText();
				TestEqual("Draft text should be correct", DraftText, TEXT("Hello user, check channel"));
				
				// Verify markdown rendering (before sending)
				// Note: GetDraftTextToSend is private, so we'll verify through the sent message
				
				// Set up message receiver using TSharedPtr to keep pointer alive
				TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
				TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
				auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
				{
					if(Message && !*ReceivedMessage)
					{
						*bMessageReceived = true;
						*ReceivedMessage = Message;
					}
				};
				CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
				
				// Send message
				FPubnubChatSendTextParams SendParams;
				FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
				
				TestFalse("Send should succeed", SendResult.Error);
				
				// Wait for message to be received
				ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
					return *bMessageReceived;
				}, MAX_WAIT_TIME));
				
				// Verify message was received and contains markdown links
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceived, ReceivedMessage, TestChannelID, TestUserID, TestMentionChannelID]()
				{
					TestTrue("Message should have been received", *bMessageReceived);
					
					if(*ReceivedMessage && IsValid(*ReceivedMessage))
					{
						FString ReceivedText = (*ReceivedMessage)->GetCurrentText();
						FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
						TestEqual("Received message channel ID should match", MessageData.ChannelID, TestChannelID);
						
						// Verify markdown format: should contain [text](pn-user://userid) and [text](pn-channel://channelid)
						TestTrue("Message should contain user mention markdown", ReceivedText.Contains(TEXT("pn-user://")));
						TestTrue("Message should contain channel mention markdown", ReceivedText.Contains(TEXT("pn-channel://")));
						TestTrue("Message should contain user ID", ReceivedText.Contains(TestUserID));
						TestTrue("Message should contain channel ID", ReceivedText.Contains(TestMentionChannelID));
					}
				}, 0.1f));
			}
			
			// Cleanup: Delete created channels and user
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestMentionChannelID, TestUserID]()
			{
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
					Chat->DeleteChannel(TestMentionChannelID);
					Chat->DeleteUser(TestUserID);
				}
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.2f));
			
			return true;
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

/**
 * Integration test: Send a draft with mentions to a channel, receive the message, then compare
 * GetMessageElements() on the received message with the draft's elements (round-trip).
 * Category: 4Advanced.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendAndGetMessageElementsRoundTripTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.GetMessageElementsRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendAndGetMessageElementsRoundTripTest::RunTest(const FString& Parameters)
{
	if (!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_draft_roundtrip_init";
	const FString TestChannelID = SDK_PREFIX + "test_draft_roundtrip";
	const FString TestUserID = SDK_PREFIX + "test_draft_roundtrip_user";
	const FString TestMentionChannelID = SDK_PREFIX + "test_draft_roundtrip_ch";
	const FString TestUrl = TEXT("https://example.com/doc");

	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);

	TestFalse("InitChat should succeed", InitResult.Result.Error);

	UPubnubChat* Chat = InitResult.Chat;
	if (!Chat)
	{
		CleanUp();
		return false;
	}

	FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TestUserID, FPubnubChatUserData());
	TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);

	FPubnubChatChannelResult CreateMentionChannelResult = Chat->CreatePublicConversation(TestMentionChannelID, FPubnubChatChannelData());
	TestFalse("CreateMentionChannel should succeed", CreateMentionChannelResult.Result.Error);

	FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, FPubnubChatChannelData());
	TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
	TestNotNull("Channel should be created", CreateResult.Channel);

	if (!CreateResult.Channel)
	{
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}

	FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
	TestFalse("Connect should succeed", ConnectResult.Error);

	FPubnubChatMessageDraftConfig Config;
	UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
	TestNotNull("Draft should be created", Draft);

	if (!Draft)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestMentionChannelID, TestUserID]()
		{
			if (Chat)
			{
				Chat->DeleteChannel(TestChannelID);
				Chat->DeleteChannel(TestMentionChannelID);
				Chat->DeleteUser(TestUserID);
			}
			CleanUpCurrentChatUser(Chat);
			CleanUp();
		}, 0.1f));
		return true;
	}

	// Build draft: "Hi @user in #channel see link" with user, channel, and URL mentions
	Draft->InsertText(0, TEXT("Hi "));
	Draft->AppendText(TEXT("user"));
	FPubnubChatMentionTarget UserTarget;
	UserTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	UserTarget.Target = TestUserID;
	FPubnubChatOperationResult AddUserResult = Draft->AddMention(3, 4, UserTarget);
	TestFalse("AddMention user should succeed", AddUserResult.Error);

	Draft->AppendText(TEXT(" in "));
	Draft->AppendText(TEXT("channel"));
	FPubnubChatMentionTarget ChannelTarget;
	ChannelTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	ChannelTarget.Target = TestMentionChannelID;
	FPubnubChatOperationResult AddChResult = Draft->AddMention(11, 7, ChannelTarget);
	TestFalse("AddMention channel should succeed", AddChResult.Error);

	Draft->AppendText(TEXT(" see "));
	Draft->AppendText(TEXT("link"));
	FPubnubChatMentionTarget UrlTarget;
	UrlTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	UrlTarget.Target = TestUrl;
	FPubnubChatOperationResult AddUrlResult = Draft->AddMention(23, 4, UrlTarget);
	TestFalse("AddMention url should succeed", AddUrlResult.Error);

	TArray<FPubnubChatMessageElement> ExpectedElements = Draft->GetMessageElements();
	FString MarkdownSent = Draft->GetTextToSend();
	FString DraftDisplayText = Draft->GetCurrentText();

	TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
	TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
	auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
	{
		if (Message && !*ReceivedMessage)
		{
			*bMessageReceived = true;
			*ReceivedMessage = Message;
		}
	};
	CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);

	FPubnubChatSendTextParams SendParams;
	FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
	TestFalse("Send should succeed", SendResult.Error);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() { return *bMessageReceived; }, MAX_WAIT_TIME));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceived, ReceivedMessage, ExpectedElements, MarkdownSent, DraftDisplayText, TestChannelID, TestUserID, TestMentionChannelID, TestUrl]()
	{
		TestTrue("Message should have been received", *bMessageReceived);

		if (!*ReceivedMessage || !IsValid(*ReceivedMessage))
		{
			CleanUpCurrentChatUser(nullptr);
			CleanUp();
			return;
		}

		UPubnubChatMessage* Msg = *ReceivedMessage;
		FString ReceivedText = Msg->GetCurrentText();
		TestEqual("Received message text should equal sent markdown", ReceivedText, MarkdownSent);

		TArray<FPubnubChatMessageElement> ReceivedElements = Msg->GetMessageElements();
		TestTrue("GetMessageElements round-trip: content should match draft elements", MessageElementsContentEqual(ExpectedElements, ReceivedElements));

		FString Reconstructed;
		for (const auto& E : ReceivedElements) Reconstructed.Append(E.Text);
		TestEqual("Reconstructed display text should match draft display text", Reconstructed, DraftDisplayText);

		FPubnubChatMessageData MessageData = Msg->GetMessageData();
		TestEqual("Received message channel ID should match", MessageData.ChannelID, TestChannelID);
	}, 0.1f));

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestMentionChannelID, TestUserID]()
	{
		if (Chat)
		{
			Chat->DeleteChannel(TestChannelID);
			Chat->DeleteChannel(TestMentionChannelID);
			Chat->DeleteUser(TestUserID);
		}
		CleanUpCurrentChatUser(Chat);
		CleanUp();
	}, 0.2f));

	return true;
}

/**
 * Tests Send with complex message: Multiple text inserts, mentions, and text removal.
 * Verifies that complex draft operations result in correct message being sent.
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatMessageDraftSendComplexMessageTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.MessageDraft.Send.4Advanced.ComplexMessage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ClientContext);

bool FPubnubChatMessageDraftSendComplexMessageTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_send_complex_init";
	const FString TestChannelID = SDK_PREFIX + "test_send_complex";
	const FString TestUserID = SDK_PREFIX + "test_send_complex_user";
	const FString ExpectedFinalText = TEXT("Hello World!");
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		// Create test user for mention
		FPubnubChatUserData UserData;
		UserData.UserName = TEXT("TestUser");
		FPubnubChatUserResult CreateUserResult = Chat->CreateUser(TestUserID, UserData);
		TestFalse("CreateUser should succeed", CreateUserResult.Result.Error);
		
		// Create channel
		FPubnubChatChannelData ChannelData;
		FPubnubChatChannelResult CreateResult = Chat->CreatePublicConversation(TestChannelID, ChannelData);
		
		TestFalse("CreatePublicConversation should succeed", CreateResult.Result.Error);
		TestNotNull("Channel should be created", CreateResult.Channel);
		
		if(CreateResult.Channel)
		{
			// Connect channel
			FPubnubChatOperationResult ConnectResult = CreateResult.Channel->Connect();
			TestFalse("Connect should succeed", ConnectResult.Error);
			
			// Create draft and perform complex operations
			FPubnubChatMessageDraftConfig Config;
			UPubnubChatMessageDraft* Draft = CreateResult.Channel->CreateMessageDraft(Config);
			
			TestNotNull("Draft should be created", Draft);
			
			if(Draft)
			{
				// Complex sequence: Insert, add mention, insert more, remove mention, insert more
				Draft->InsertText(0, TEXT("Hello"));
				Draft->InsertText(5, TEXT(" Universe"));
				
				// Verify text before mention
				FString TextBeforeMention = Draft->GetCurrentText();
				TestEqual("Text before mention should be correct", TextBeforeMention, TEXT("Hello Universe"));
				
				// Add mention to "Universe" (starts at position 6, length 8)
				FPubnubChatMentionTarget MentionTarget;
				MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
				MentionTarget.Target = TestUserID;
				FPubnubChatOperationResult AddMentionResult = Draft->AddMention(6, 8, MentionTarget);
				TestFalse("AddMention should succeed", AddMentionResult.Error);
				
				// Remove mention (removes "Universe" text too)
				FPubnubChatOperationResult RemoveMentionResult = Draft->RemoveMention(6);
				TestFalse("RemoveMention should succeed", RemoveMentionResult.Error);
				
				// Verify text after removing mention
				FString TextAfterRemove = Draft->GetCurrentText();
				TestEqual("Text after removing mention should be 'Hello '", TextAfterRemove, TEXT("Hello "));
				
				// Insert "World" at position 6 (after "Hello ")
				FPubnubChatOperationResult InsertWorldResult = Draft->InsertText(6, TEXT("World"));
				TestFalse("InsertText 'World' should succeed", InsertWorldResult.Error);
				
				// Verify text after inserting World
				FString TextAfterWorld = Draft->GetCurrentText();
				TestEqual("Text after inserting World should be 'Hello World'", TextAfterWorld, TEXT("Hello World"));
				
				// Insert "!" at position 11 (end of "Hello World")
				FPubnubChatOperationResult InsertExclamationResult = Draft->InsertText(11, TEXT("!"));
				TestFalse("InsertText '!' should succeed", InsertExclamationResult.Error);
				
				// Verify final text
				FString FinalText = Draft->GetCurrentText();
				TestEqual("Final draft text should match", FinalText, ExpectedFinalText);
				
				// Set up message receiver using TSharedPtr to keep pointer alive
				TSharedPtr<bool> bMessageReceived = MakeShared<bool>(false);
				TSharedPtr<UPubnubChatMessage*> ReceivedMessage = MakeShared<UPubnubChatMessage*>(nullptr);
				auto MessageLambda = [bMessageReceived, ReceivedMessage](UPubnubChatMessage* Message)
				{
					if(Message && !*ReceivedMessage)
					{
						*bMessageReceived = true;
						*ReceivedMessage = Message;
					}
				};
				CreateResult.Channel->OnMessageReceivedNative.AddLambda(MessageLambda);
				
				// Send message
				FPubnubChatSendTextParams SendParams;
				FPubnubChatOperationResult SendResult = Draft->Send(SendParams);
				
				TestFalse("Send should succeed", SendResult.Error);
				
				// Wait for message to be received
				ADD_LATENT_AUTOMATION_COMMAND(FWaitUntilLatentCommand([bMessageReceived]() -> bool {
					return *bMessageReceived;
				}, MAX_WAIT_TIME));
				
				// Verify message was received correctly
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, bMessageReceived, ReceivedMessage, ExpectedFinalText, TestChannelID]()
				{
					TestTrue("Message should have been received", *bMessageReceived);
					
					if(*ReceivedMessage && IsValid(*ReceivedMessage))
					{
						TestEqual("Received message text should match expected", (*ReceivedMessage)->GetCurrentText(), ExpectedFinalText);
						FPubnubChatMessageData MessageData = (*ReceivedMessage)->GetMessageData();
						TestEqual("Received message channel ID should match", MessageData.ChannelID, TestChannelID);
					}
				}, 0.1f));
			}
			
			// Cleanup: Delete created channel and user
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Chat, TestChannelID, TestUserID]()
			{
				if(Chat)
				{
					Chat->DeleteChannel(TestChannelID);
					Chat->DeleteUser(TestUserID);
				}
				CleanUpCurrentChatUser(Chat);
				CleanUp();
			}, 0.2f));
			
			return true;
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
