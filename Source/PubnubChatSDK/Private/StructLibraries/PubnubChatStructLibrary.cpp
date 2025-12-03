// Fill out your copyright notice in the Description page of Project Settings.


#include "StructLibraries/PubnubChatStructLibrary.h"


FPubnubChatOperationResult& FPubnubChatOperationResult::MarkSuccess()
{
	Status = 200;
	Error = false;
	return *this;
}

FPubnubChatOperationResult FPubnubChatOperationResult::CreateError(int InStatus, FString InErrorMessage)
{
	return FPubnubChatOperationResult(InStatus, true, InErrorMessage);
}

FPubnubChatOperationResult FPubnubChatOperationResult::FromSingleStep(const FString& StepName, const FPubnubOperationResult& OperationResult)
{
	FPubnubChatOperationResult Result;
	Result.Status = OperationResult.Status;
	Result.Error = OperationResult.Error;
		
	FPubnubChatOperationStepResult StepResult;
	StepResult.StepName = StepName;
	StepResult.OperationResult = OperationResult;
	Result.StepResults.Add(StepResult);

	if (OperationResult.Error)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Operation '%s' failed: %s"), *StepName, *OperationResult.ErrorMessage);
	}

	return Result;
}

FPubnubChatOperationResult& FPubnubChatOperationResult::AddStep(const FString& StepName, const FPubnubOperationResult& OperationResult)
{
	{
		FPubnubChatOperationStepResult StepResult;
		StepResult.StepName = StepName;
		StepResult.OperationResult = OperationResult;
		StepResults.Add(StepResult);

		// If this step failed, mark overall result as error
		if (OperationResult.Error)
		{
			Error = true;
			// Use the first failed step's status
			if (Status == 0 || Status == 200)
			{
				Status = OperationResult.Status;
			}

			// Build aggregated error message
			if (ErrorMessage.IsEmpty())
			{
				ErrorMessage = FString::Printf(TEXT("Operation '%s' failed: %s"), *StepName, *OperationResult.ErrorMessage);
			}
			else
			{
				ErrorMessage += FString::Printf(TEXT(" | Operation '%s' failed: %s"), *StepName, *OperationResult.ErrorMessage);
			}
		}
		else if (!Error && Status == 0)
		{
			// All steps succeeded so far
			Status = 200;
		}

		return *this;
	}
}

FPubnubChatOperationResult& FPubnubChatOperationResult::Merge(const FPubnubChatOperationResult& OtherResult)
{
	// Add all step results from the other result
	StepResults.Append(OtherResult.StepResults);

	// If the other result had errors, mark overall as error
	if (OtherResult.Error)
	{
		Error = true;
		// Use the first failed step's status
		if (Status == 0 || Status == 200)
		{
			Status = OtherResult.Status;
		}

		// Merge error messages
		if (ErrorMessage.IsEmpty())
		{
			ErrorMessage = OtherResult.ErrorMessage;
		}
		else if (!OtherResult.ErrorMessage.IsEmpty())
		{
			ErrorMessage += TEXT(" | ") + OtherResult.ErrorMessage;
		}
	}
	else if (!Error && Status == 0)
	{
		// All steps succeeded so far
		Status = 200;
	}

	return *this;
}