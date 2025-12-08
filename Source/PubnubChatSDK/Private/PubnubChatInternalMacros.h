// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


#define PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(ReturnWrapper) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!ObjectsRepository) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Objects Repository is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)


#define PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED() \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
		if (!ObjectsRepository) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Objects Repository is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
	} while (false)

#define PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(ReturnWrapper) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!Chat) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Chat is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)


#define PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED() \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
		if (!Chat) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Chat is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult(0, true, ErrorLogMessage); \
		} \
	} while (false)



#define PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(ReturnWrapper, OperationResult) \
	do { \
		if (OperationResult.Result.Error) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("%s failed."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result.ErrorMessage = ErrorLogMessage; \
			ReturnWrapper.Result.Error = true; \
			ReturnWrapper.Result.Merge(OperationResult.Result); \
			return ReturnWrapper; \
		} \
	} while (false)
	
	
#define PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(ReturnWrapper, Field) \
	do { \
		if (Field.IsEmpty()) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), TEXT(#Field)); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult(0, true, ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(ReturnWrapper, Condition, ErrorMessage) \
	do { \
		if (!Condition) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), ErrorMessage); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result.ErrorMessage = ErrorLogMessage; \
			ReturnWrapper.Result.Error = true; \
			return ReturnWrapper; \
		} \
	} while (false)