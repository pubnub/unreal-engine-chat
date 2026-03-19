// Copyright 2026 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatMessageDraftUtilities.h"
#include "PubnubChatConst.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatEnumLibrary.h"

FPubnubChatMentionTarget UPubnubChatMessageDraftUtilities::CreateUserMentionTarget(const FString UserID)
{
	FPubnubChatMentionTarget NewTarget;
	NewTarget.Target = UserID;
	NewTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
	return NewTarget;
}

FPubnubChatMentionTarget UPubnubChatMessageDraftUtilities::CreateChannelMentionTarget(const FString ChannelID)
{
	FPubnubChatMentionTarget NewTarget;
	NewTarget.Target = ChannelID;
	NewTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
	return NewTarget;
}

FPubnubChatMentionTarget UPubnubChatMessageDraftUtilities::CreateUrlMentionTarget(const FString Url)
{
	FPubnubChatMentionTarget NewTarget;
	NewTarget.Target = Url;
	NewTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
	return NewTarget;
}

FString UPubnubChatMessageDraftUtilities::UnescapeLinkText(const FString& EscapedLinkText)
{
	FString Result;
	Result.Reserve(EscapedLinkText.Len());
	for (int32 i = 0; i < EscapedLinkText.Len(); ++i)
	{
		if (EscapedLinkText[i] == TCHAR('\\') && i + 1 < EscapedLinkText.Len())
		{
			TCHAR Next = EscapedLinkText[i + 1];
			if (Next == TCHAR('\\') || Next == TCHAR(']'))
			{
				Result.AppendChar(Next);
				++i;
				continue;
			}
		}
		Result.AppendChar(EscapedLinkText[i]);
	}
	return Result;
}

FString UPubnubChatMessageDraftUtilities::UnescapeLinkUrl(const FString& EscapedLinkUrl)
{
	FString Result;
	Result.Reserve(EscapedLinkUrl.Len());
	for (int32 i = 0; i < EscapedLinkUrl.Len(); ++i)
	{
		if (EscapedLinkUrl[i] == TCHAR('\\') && i + 1 < EscapedLinkUrl.Len())
		{
			TCHAR Next = EscapedLinkUrl[i + 1];
			if (Next == TCHAR('\\') || Next == TCHAR(')'))
			{
				Result.AppendChar(Next);
				++i;
				continue;
			}
		}
		Result.AppendChar(EscapedLinkUrl[i]);
	}
	return Result;
}

static FPubnubChatMentionTarget MentionTargetFromUrl(const FString& Url)
{
	FPubnubChatMentionTarget Target;
	if (Url.StartsWith(Pubnub_Schema_User))
	{
		Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_User;
		Target.Target = Url.Mid(Pubnub_Schema_User.Len());
	}
	else if (Url.StartsWith(Pubnub_Schema_Channel))
	{
		Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Channel;
		Target.Target = Url.Mid(Pubnub_Schema_Channel.Len());
	}
	else
	{
		Target.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_Url;
		Target.Target = Url;
	}
	return Target;
}

TArray<FPubnubChatMessageElement> UPubnubChatMessageDraftUtilities::ParseMessageMarkdownToElements(const FString& MarkdownText)
{
	TArray<FPubnubChatMessageElement> Elements;
	const int32 Len = MarkdownText.Len();
	int32 DisplayStart = 0;
	int32 i = 0;

	while (i < Len)
	{
		int32 OpenBracket = MarkdownText.Find(TEXT("["), ESearchCase::CaseSensitive, ESearchDir::FromStart, i);
		if (OpenBracket == INDEX_NONE)
		{
			if (i < Len)
			{
				FPubnubChatMessageElement Plain;
				Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
				Plain.Text = MarkdownText.Mid(i);
				Plain.Start = DisplayStart;
				Plain.Length = Plain.Text.Len();
				Elements.Add(Plain);
			}
			break;
		}

		// Skip '[' that is escaped (part of literal \ [ in plain text or inside link text)
		if (OpenBracket > 0 && MarkdownText[OpenBracket - 1] == TCHAR('\\'))
		{
			FPubnubChatMessageElement Plain;
			Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
			Plain.Text = MarkdownText.Mid(i, OpenBracket + 1 - i);
			Plain.Start = DisplayStart;
			Plain.Length = Plain.Text.Len();
			Elements.Add(Plain);
			DisplayStart += Plain.Length;
			i = OpenBracket + 1;
			continue;
		}
		if (OpenBracket > i)
		{
			FPubnubChatMessageElement Plain;
			Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
			Plain.Text = MarkdownText.Mid(i, OpenBracket - i);
			Plain.Start = DisplayStart;
			Plain.Length = Plain.Text.Len();
			Elements.Add(Plain);
			DisplayStart += Plain.Length;
		}

		// Parse link text: from OpenBracket+1 until unescaped ']'
		int32 p = OpenBracket + 1;
		FString RawLinkText;
		while (p < Len)
		{
			if (MarkdownText[p] == TCHAR('\\') && p + 1 < Len)
			{
				RawLinkText.AppendChar(MarkdownText[p]);
				RawLinkText.AppendChar(MarkdownText[p + 1]);
				p += 2;
				continue;
			}
			if (MarkdownText[p] == TCHAR(']'))
			{
				break;
			}
			RawLinkText.AppendChar(MarkdownText[p]);
			++p;
		}
		if (p >= Len)
		{
			FPubnubChatMessageElement Plain;
			Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
			Plain.Text = MarkdownText.Mid(OpenBracket);
			Plain.Start = DisplayStart;
			Plain.Length = Plain.Text.Len();
			Elements.Add(Plain);
			break;
		}
		int32 CloseBracket = p;
		FString LinkText = UnescapeLinkText(RawLinkText);

		++p;
		if (p >= Len || MarkdownText[p] != TCHAR('('))
		{
			FPubnubChatMessageElement Plain;
			Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
			Plain.Text = MarkdownText.Mid(OpenBracket, CloseBracket - OpenBracket + 1);
			Plain.Start = DisplayStart;
			Plain.Length = Plain.Text.Len();
			Elements.Add(Plain);
			DisplayStart += Plain.Length;
			i = CloseBracket + 1;
			continue;
		}
		++p;
		FString RawUrl;
		while (p < Len)
		{
			if (MarkdownText[p] == TCHAR('\\') && p + 1 < Len)
			{
				RawUrl.AppendChar(MarkdownText[p]);
				RawUrl.AppendChar(MarkdownText[p + 1]);
				p += 2;
				continue;
			}
			if (MarkdownText[p] == TCHAR(')'))
			{
				break;
			}
			RawUrl.AppendChar(MarkdownText[p]);
			++p;
		}
		if (p >= Len)
		{
			FPubnubChatMessageElement Plain;
			Plain.MentionTarget.MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
			Plain.Text = MarkdownText.Mid(OpenBracket);
			Plain.Start = DisplayStart;
			Plain.Length = Plain.Text.Len();
			Elements.Add(Plain);
			break;
		}
		FString Url = UnescapeLinkUrl(RawUrl);

		FPubnubChatMessageElement MentionElement;
		MentionElement.MentionTarget = MentionTargetFromUrl(Url);
		MentionElement.Text = LinkText;
		MentionElement.Start = DisplayStart;
		MentionElement.Length = LinkText.Len();
		Elements.Add(MentionElement);
		DisplayStart += MentionElement.Length;

		i = p + 1;
	}

	return Elements;
}