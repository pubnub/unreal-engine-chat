// Fill out your copyright notice in the Description page of Project Settings.


#include "StructLibraries/PubnubChatStructLibrary.h"
#include "kismet/KismetMathLibrary.h"
#include "PubnubChatConst.h"


void FPubnubChatConfig::ValidateConfig()
{
	TypingTimeout = UKismetMathLibrary::Max(TypingTimeout, Pubnub_Chat_Min_Typing_Indicator_Timeout);
	TypingTimeoutDifference = UKismetMathLibrary::Max(TypingTimeoutDifference, 0);
}

FPubnubChatOperationResult& FPubnubChatOperationResult::MarkSuccess()
{
	Error = false;
	return *this;
}

FPubnubChatOperationResult FPubnubChatOperationResult::CreateError(FString InErrorMessage)
{
	FPubnubChatOperationResult Result;
	Result.Error = true;
	Result.ErrorMessage = InErrorMessage;
	return Result;
}

FPubnubChatOperationResult FPubnubChatOperationResult::FromSingleStep(const FString& StepName, const FPubnubOperationResult& OperationResult)
{
	FPubnubChatOperationResult Result;
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
	FPubnubChatOperationStepResult StepResult;
	StepResult.StepName = StepName;
	StepResult.OperationResult = OperationResult;
	StepResults.Add(StepResult);

	// If this step failed, mark overall result as error
	if (OperationResult.Error)
	{
		Error = true;

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

	return *this;
}

FPubnubChatOperationResult& FPubnubChatOperationResult::Merge(const FPubnubChatOperationResult& OtherResult)
{
	// Add all step results from the other result
	StepResults.Append(OtherResult.StepResults);

	// If the other result had errors, mark overall as error
	if (OtherResult.Error)
	{
		Error = true;

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

	return *this;
}

FPubnubMembershipInputData FPubnubChatMembershipData::ToPubnubMembershipInputData(const FString ChannelID) const
{
	FPubnubMembershipInputData MembershipInputData;
	MembershipInputData.Channel = ChannelID;
	MembershipInputData.Custom = Custom;
	MembershipInputData.Status = Status;
	MembershipInputData.Type = Type;
	MembershipInputData.ForceSetAllFields();

	return MembershipInputData;
}

FPubnubChannelMemberInputData FPubnubChatMembershipData::ToPubnubChannelMemberInputData(const FString UserID) const
{
	FPubnubChannelMemberInputData ChannelMemberInputData;
	ChannelMemberInputData.User = UserID;
	ChannelMemberInputData.Custom = Custom;
	ChannelMemberInputData.Status = Status;
	ChannelMemberInputData.Type = Type;
	ChannelMemberInputData.ForceSetAllFields();

	return ChannelMemberInputData;
}

FPubnubChatMembershipData FPubnubChatMembershipData::FromPubnubMembershipData(const FPubnubMembershipData& PubnubMembershipData)
{
	FPubnubChatMembershipData ChatMembershipData;

	ChatMembershipData.Custom = PubnubMembershipData.Custom;
	ChatMembershipData.Type = PubnubMembershipData.Type;
	ChatMembershipData.Status = PubnubMembershipData.Status;

	return ChatMembershipData;
}

FPubnubChatMembershipData FPubnubChatMembershipData::FromPubnubChannelMemberData(const FPubnubChannelMemberData& PubnubChannelMemberData)
{
	FPubnubChatMembershipData ChatMembershipData;

	ChatMembershipData.Custom = PubnubChannelMemberData.Custom;
	ChatMembershipData.Type = PubnubChannelMemberData.Type;
	ChatMembershipData.Status = PubnubChannelMemberData.Status;

	return ChatMembershipData;
}

FPubnubMembershipInputData FPubnubChatUpdateMembershipInputData::ToPubnubMembershipInputData(const FString ChannelID) const
{
	FPubnubMembershipInputData MembershipInputData;
	MembershipInputData.Channel = ChannelID;
	MembershipInputData.Custom = Custom;
	MembershipInputData.Status = Status;
	MembershipInputData.Type = Type;
	MembershipInputData.ForceSetCustom = ForceSetCustom;
	MembershipInputData.ForceSetStatus = ForceSetStatus;
	MembershipInputData.ForceSetType = ForceSetType;

	return MembershipInputData;
}

FPubnubChannelMemberInputData FPubnubChatUpdateMembershipInputData::ToPubnubChannelMemberInputData(const FString UserID) const
{
	FPubnubChannelMemberInputData ChannelMemberInputData;
	ChannelMemberInputData.User = UserID;
	ChannelMemberInputData.Custom = Custom;
	ChannelMemberInputData.Status = Status;
	ChannelMemberInputData.Type = Type;
	ChannelMemberInputData.ForceSetAllFields();

	return ChannelMemberInputData;
}

FPubnubChatUpdateMembershipInputData FPubnubChatUpdateMembershipInputData::FromChatMembershipData(const FPubnubChatMembershipData& PubnubMembershipData)
{
	FPubnubChatUpdateMembershipInputData UpdateMembershipInputData;
	UpdateMembershipInputData.Custom = PubnubMembershipData.Custom;
	UpdateMembershipInputData.Status = PubnubMembershipData.Status;
	UpdateMembershipInputData.Type = PubnubMembershipData.Type;
	
	return UpdateMembershipInputData;
}
