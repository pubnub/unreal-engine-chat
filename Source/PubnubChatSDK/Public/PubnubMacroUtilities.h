// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"
#include "CoreMinimal.h"

/** This macro is to check if required parameter is not empty.
 * If it is empty, log will be printed containing called function and name of empty parameter.
 * It will also return (stop function execution) if parameter is empty.
 * __VA_ARGS__ are to provide return value.
 */
#define PUBNUB_RETURN_IF_EMPTY(Field, ...) \
	if (Field.IsEmpty()) \
	{ \
		UPubnubLogUtilities::PrintEmptyFieldLog(ANSI_TO_TCHAR(__FUNCTION__), TEXT(#Field)); \
		return __VA_ARGS__; \
	}

/** This macro is to check if required object parameter is valid.
 * If not, log will be printed containing called function and name of empty parameter.
 * It will also return (stop function execution) if parameter is an invalid object.
 * __VA_ARGS__ are to provide return value.
 */
#define PUBNUB_RETURN_IF_NULL(Object, ...) \
	if (nullptr == Object) \
	{ \
		UPubnubLogUtilities::PrintInvalidObjectFieldLog(ANSI_TO_TCHAR(__FUNCTION__), TEXT(#Object)); \
		return __VA_ARGS__; \
	}

/** This macro is to check if required array is not empty. Use only with Pubnub::Vector
 * If empty, log will be printed containing called function and name of empty parameter.
 * It will also return (stop function execution) if array is empty.
 * __VA_ARGS__ are to provide return value.
 */
#define PUBNUB_RETURN_IF_EMPTY_CPP_VECTOR(VectorField, ...) \
	if (VectorField.size() == 0) \
	{ \
		UPubnubLogUtilities::PrintEmptyArrayFieldLog(ANSI_TO_TCHAR(__FUNCTION__), TEXT(#VectorField)); \
		return __VA_ARGS__; \
	}