// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatMessageDraftUtilities.h"
#include "PubnubChatSubsystem.h"

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