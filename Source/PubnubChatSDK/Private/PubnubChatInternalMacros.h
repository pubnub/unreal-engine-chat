// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


#define PUBNUB_CHAT_RETURN_WRAPPER_IF_NOT_INITIALIZED(ReturnWrapper) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!ObjectsRepository) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Objects Repository is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)


#define PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED() \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
		if (!ObjectsRepository) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Objects Repository is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
	} while (false)

#define PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(ReturnWrapper) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
		if (!Chat) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Chat is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)


#define PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED() \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
		if (!PubnubClient) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
		if (!Chat) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Chat is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
	} while (false)

#define PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(...) \
	do { \
		if (!IsInitialized) \
		{ \
			UE_LOG(PubnubChatLog, Error, TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			return __VA_ARGS__; \
		} \
		if (!PubnubClient) \
		{ \
			UE_LOG(PubnubChatLog, Error, TEXT("[%s]: Pubnub Client is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			return __VA_ARGS__; \
		} \
		if (!Chat) \
		{ \
			UE_LOG(PubnubChatLog, Error, TEXT("[%s]: Chat is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			return __VA_ARGS__; \
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
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Field) \
	do { \
		if (Field.IsEmpty()) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), TEXT(#Field)); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(ReturnWrapper, Object) \
	do { \
	if (!Object) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s has to be a valid object. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), TEXT(#Object)); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Object) \
	do { \
		if (!Object) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s has to be a valid object. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), TEXT(#Object)); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(ReturnWrapper, Condition, Message) \
	do { \
		if (!Condition) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), Message); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result.ErrorMessage = ErrorLogMessage; \
			ReturnWrapper.Result.Error = true; \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED(Condition, ErrorMessage) \
	do { \
		if (!Condition) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: %s. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), ErrorMessage); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			return FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
		} \
	} while (false)


#define PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(Condition, ErrorMessage, ...) \
	do { \
		if (!Condition) \
		{ \
			UE_LOG(PubnubChatLog, Error, TEXT("[%s]: %s. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)), ErrorMessage); \
			return __VA_ARGS__; \
		} \
	} while (false)

#define PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(ReturnWrapper, PubnubResult, MethodName) \
	do { \
		ReturnWrapper.Result.AddStep(MethodName, PubnubResult); \
		if (PubnubResult.Error) \
		{ \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(OperationResult, PubnubResult, MethodName) \
	do { \
		OperationResult.AddStep(MethodName, PubnubResult); \
		if (PubnubResult.Error) \
		{ \
			return OperationResult; \
		} \
	} while (false)

#define PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(ReturnWrapper, ChatOperationResult) \
	do { \
		ReturnWrapper.Result.Merge(ChatOperationResult); \
		if (ChatOperationResult.Error) \
		{ \
			return ReturnWrapper; \
		} \
	} while (false)

#define PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, ChatOperationResult) \
	do { \
		FinalResult.Merge(ChatOperationResult); \
		if (ChatOperationResult.Error) \
		{ \
			return FinalResult; \
		} \
	} while (false)



#define PUBNUB_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_WRAPPER(Delegate, ReturnWrapper) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			UPubnubUtilities::CallPubnubDelegate(Delegate, ReturnWrapper); \
			return; \
		} \
		if (!AsyncFunctionsThread) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: AsyncFunctionsThread is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			ReturnWrapper.Result = FPubnubChatOperationResult::CreateError(ErrorLogMessage); \
			UPubnubUtilities::CallPubnubDelegate(Delegate, ReturnWrapper); \
			return; \
		} \
	} while (false)

#define PUBNUB_RETURN_WITH_DELEGATE_IF_NOT_INITIALIZED_OPERATION_RESULT(Delegate) \
	do { \
		if (!IsInitialized) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: Not initialized. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			UPubnubUtilities::CallPubnubDelegate(Delegate, FPubnubChatOperationResult::CreateError(ErrorLogMessage)); \
			return; \
		} \
		if (!AsyncFunctionsThread) \
		{ \
			FString ErrorLogMessage = FString::Printf(TEXT("[%s]: AsyncFunctionsThread is invalid. Aborting operation. This object was already destroyed or was not initialized correctly."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__))); \
			UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorLogMessage); \
			UPubnubUtilities::CallPubnubDelegate(Delegate, FPubnubChatOperationResult::CreateError(ErrorLogMessage)); \
			return; \
		} \
	} while (false)