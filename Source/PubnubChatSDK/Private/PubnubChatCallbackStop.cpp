// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "PubnubChatCallbackStop.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "PubnubChatSubsystem.h"

void UPubnubChatCallbackStop::BeginDestroy()
{
	if(StopCallback && !IsStopped)
	{
		StopCallback();
	}
		
	UObject::BeginDestroy();
}

FPubnubChatOperationResult UPubnubChatCallbackStop::Stop()
{
	if(IsStopped)
	{
		FString WarningMessage = FString::Printf(TEXT("[%s], This callback was already stopped."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *WarningMessage);
		return FPubnubChatOperationResult::CreateError(WarningMessage);
	}
	
	if(!StopCallback)
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s], Can't stop callback, the callback is invalid"), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		return FPubnubChatOperationResult::CreateError(ErrorMessage);
	}

	FPubnubChatOperationResult StopResult = StopCallback();
	
	//Only set to stopped if there was no error on the way, so user can try again in case of failure
	if(!StopResult.Error)
	{
		IsStopped = true;
	}

	return StopResult;
}

void UPubnubChatCallbackStop::InitCallbackStop(TFunction<FPubnubChatOperationResult()> InStopCallback)
{
	StopCallback = InStopCallback;
}
