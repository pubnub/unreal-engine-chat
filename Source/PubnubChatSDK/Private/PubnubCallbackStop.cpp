// Copyright 2024 PubNub Inc. All Rights Reserved.


#include "PubnubCallbackStop.h"

UPubnubCallbackStop* UPubnubCallbackStop::Create(Pubnub::CallbackStop CallbackStop)
{
	UPubnubCallbackStop* NewCallbackStop = NewObject<UPubnubCallbackStop>();
	NewCallbackStop->InternalCallbackStop = new Pubnub::CallbackStop(CallbackStop);
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
		InternalCallbackStop->operator()();
	}
}

