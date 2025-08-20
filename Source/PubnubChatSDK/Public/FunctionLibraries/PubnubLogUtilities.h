// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubLogUtilities.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubLogUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	//This is to remove class name from __FUNCTION__ macro output
	static FString GetNameFromFunctionMacro(FString FunctionName);
	
	//Use to print PubnubChatLog that function input was empty. As FunctionName can accept __FUNCTION__ and will trim the class name
	static void PrintEmptyFieldLog(FString FunctionName, FString FieldName);
	
	//Use to print PubnubChatLog that function input was an invalid object. As FunctionName can accept __FUNCTION__ and will trim the class name
	static void PrintInvalidObjectFieldLog(FString FunctionName, FString FieldName);

	//Use to print PubnubChatLog that function array input was empty. As FunctionName can accept __FUNCTION__ and will trim the class name
	static void PrintEmptyArrayFieldLog(FString FunctionName, FString FieldName);

	//Use to print PubnubChatLog with error, usually from cpp exception
	static void PrintFunctionError(FString FunctionName, FString Error);

};
