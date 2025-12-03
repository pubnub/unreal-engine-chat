// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "PubnubChatSubsystem.h"

FString UPubnubChatLogUtilities::GetNameFromFunctionMacro(FString FunctionName)
{
	if(FunctionName.IsEmpty()) {return "";}
	int Index = -1;
	FunctionName.FindLastChar(TEXT(':'), Index);
	return FunctionName.Mid(Index + 1);
}

FString UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(FString FunctionName)
{
	if(FunctionName.IsEmpty()) {return "";}
	int Index = -1;
	FunctionName.FindLastChar(TEXT(':'), Index);
	return FString::Printf(TEXT("[%s] [%s]"), *FunctionName.Left(Index - 1), *FunctionName.Mid(Index + 1));
}

void UPubnubChatLogUtilities::PrintEmptyFieldLog(FString FunctionName, FString FieldName)
{
	UE_LOG(PubnubChatLog, Warning, TEXT("Failed to: %s, %s field can't be empty."), *GetNameFromFunctionMacro(FunctionName), *FieldName);
}

void UPubnubChatLogUtilities::PrintInvalidObjectFieldLog(FString FunctionName, FString FieldName)
{
	UE_LOG(PubnubChatLog, Warning, TEXT("Failed to: %s, %s has to be a valid object."), *GetNameFromFunctionMacro(FunctionName), *FieldName);
}

void UPubnubChatLogUtilities::PrintEmptyArrayFieldLog(FString FunctionName, FString FieldName)
{
	UE_LOG(PubnubChatLog, Warning, TEXT("Failed to: %s, %s has to contain at least one valid object."), *GetNameFromFunctionMacro(FunctionName), *FieldName);
}

void UPubnubChatLogUtilities::PrintFunctionError(FString FunctionName, FString Error)
{
	UE_LOG(PubnubChatLog, Error, TEXT("Failed to: %s, Error: %s."), *GetNameFromFunctionMacro(FunctionName), *Error);
}
