// Copyright 2024 PubNub Inc. All Rights Reserved.


#include "PubnubCallbackStop.h"

UPubnubCallbackStop* UPubnubCallbackStop::Create(Pubnub::CallbackHandle CallbackStop)
{
	UPubnubCallbackStop* NewCallbackStop = NewObject<UPubnubCallbackStop>();
	NewCallbackStop->InternalCallbackStop = new Pubnub::CallbackHandle(CallbackStop);
	return NewCallbackStop;
}

UPubnubCallbackStop::~UPubnubCallbackStop()
{
	delete InternalCallbackStop;
}

void UPubnubCallbackStop::Stop()
{
	if(InternalCallbackStop)
	{
		InternalCallbackStop->close();
	}
}

