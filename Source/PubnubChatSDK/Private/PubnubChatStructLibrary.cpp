// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatStructLibrary.h"
#include "PubnubMessage.h"

Pubnub::SendTextParams FPubnubSendTextParams::GetCppSendTextParams()
{
	Pubnub::SendTextParams FinalParams;

	FinalParams.store_in_history = StoreInHistory;
	FinalParams.send_by_post = SendByPost;
	FinalParams.meta = UPubnubChatUtilities::FStringToPubnubString(Meta);

	if(QuotedMessage)
	{
		FinalParams.quoted_message = *QuotedMessage->GetInternalMessage();
	}
		
	return FinalParams;

}
