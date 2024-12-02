// Copyright 2024 PubNub Inc. All Rights Reserved.

#include "PubnubChatStructLibrary.h"
#include "PubnubMessage.h"

Pubnub::SendTextParams FSendTextParams::GetCppSendTextParams()
{
	Pubnub::SendTextParams FinalParams;

	FinalParams.store_in_history = StoreInHistory;
	FinalParams.send_by_post = SendByPost;
	FinalParams.meta = UPubnubChatUtilities::FStringToPubnubString(Meta);

	if(!MentionedUsers.IsEmpty())
	{
		std::map<int, Pubnub::MentionedUser> CppMentionedUsers;
		for(auto It : MentionedUsers)
		{
			CppMentionedUsers[It.Key] = It.Value.GetCppMentionedUser();
		}
		FinalParams.mentioned_users = CppMentionedUsers;
	}

	if(QuotedMessage)
	{
		FinalParams.quoted_message = *QuotedMessage->GetInternalMessage();
	}
		
	return FinalParams;

}
