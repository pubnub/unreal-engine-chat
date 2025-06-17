// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "PubnubMessageDraft.h"
#include "PubnubChatSubsystem.h"
#include "PubnubCallbackStop.h"
#include "PubnubChat.h"
#include "Async/Async.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


/* MENTION TARGET */

UPubnubMentionTarget* UPubnubMentionTarget::Create(Pubnub::MentionTarget MentionTarget)
{
	UPubnubMentionTarget* NewMentionTarget = NewObject<UPubnubMentionTarget>();
	NewMentionTarget->InternalMentionTarget = new Pubnub::MentionTarget(MentionTarget);
	return NewMentionTarget;
}

UPubnubMentionTarget::~UPubnubMentionTarget()
{
	delete InternalMentionTarget;
}

UPubnubMentionTarget* UPubnubMentionTarget::CreateUserMentionTarget(const FString UserID)
{
	return Create(Pubnub::MentionTarget::user(UPubnubChatUtilities::FStringToPubnubString(UserID)));
}

UPubnubMentionTarget* UPubnubMentionTarget::CreateChannelMentionTarget(const FString Channel)
{
	return Create(Pubnub::MentionTarget::channel(UPubnubChatUtilities::FStringToPubnubString(Channel)));
}

UPubnubMentionTarget* UPubnubMentionTarget::CreateUrlMentionTarget(const FString Url)
{
	return Create(Pubnub::MentionTarget::url(UPubnubChatUtilities::FStringToPubnubString(Url)));
}

FString UPubnubMentionTarget::GetTarget()
{
	if(!IsInternalMentionTargetValid()) {return "";}

	return UPubnubChatUtilities::PubnubStringToFString(InternalMentionTarget->get_target());
}

EPubnubMentionTargetType UPubnubMentionTarget::GetType()
{
	if(!IsInternalMentionTargetValid()) {return EPubnubMentionTargetType::PMTT_User;}

	return (EPubnubMentionTargetType)InternalMentionTarget->get_type();
}

bool UPubnubMentionTarget::IsInternalMentionTargetValid()
{
	if(InternalMentionTarget == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("This PubnubMentionTarget is invalid"));
		return false;
	}
	return true;
}

/* MESSAGE ELEMENT */

UPubnubMessageElement* UPubnubMessageElement::Create(Pubnub::MessageElement MessageElement)
{
	UPubnubMessageElement* NewMessageElement = NewObject<UPubnubMessageElement>();
	NewMessageElement->InternalMessageElement = new Pubnub::MessageElement(MessageElement);
	return NewMessageElement;
}

UPubnubMessageElement::~UPubnubMessageElement()
{
	delete InternalMessageElement;
}


FString UPubnubMessageElement::GetText()
{
	if(!IsInternalMessageElementValid()) {return "";}

	return UPubnubChatUtilities::PubnubStringToFString(InternalMessageElement->text);
}

UPubnubMentionTarget* UPubnubMessageElement::GetTarget()
{
	if(!IsInternalMessageElementValid()) {return nullptr;}
	if(!InternalMessageElement->target.has_value()) {return nullptr;}

	return UPubnubMentionTarget::Create(InternalMessageElement->target.value());
}

bool UPubnubMessageElement::IsInternalMessageElementValid()
{
	if(InternalMessageElement == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("This PubnubMessageElement is invalid"));
		return false;
	}
	return true;
}


/* MESSAGE DRAFT */


UPubnubMessageDraft* UPubnubMessageDraft::Create(Pubnub::MessageDraft MessageDraft)
{
	UPubnubMessageDraft* NewMessage = NewObject<UPubnubMessageDraft>();
	NewMessage->InternalMessageDraft = new Pubnub::MessageDraft(MessageDraft);
	return NewMessage;
}

UPubnubMessageDraft::~UPubnubMessageDraft()
{
	delete InternalMessageDraft;
}

void UPubnubMessageDraft::InsertText(int Position, const FString Text)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->insert_text(Position, UPubnubChatUtilities::FStringToPubnubString(Text));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft InsertText Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::RemoveText(int Position, int Length)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->remove_text(Position, Length);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft InsertText Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::InsertSuggestedMention(FPubnubSuggestedMention SuggestedMention, const FString Text)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->insert_suggested_mention(SuggestedMention.GetCppSuggestedMention(), UPubnubChatUtilities::FStringToPubnubString(Text));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft InsertSuggestedMention Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::AddMention(int Position, int Length, UPubnubMentionTarget* MentionTarget)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->add_mention(Position, Length, *MentionTarget->GetInternalMentionTarget());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft AddMention Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::RemoveMention(int Position)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->remove_mention(Position);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft RemoveMention Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::Update(FString Text)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->update(UPubnubChatUtilities::FStringToPubnubString(Text));
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft Update Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::Send(FPubnubSendTextParams SendTextParams)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		InternalMessageDraft->send(SendTextParams.GetCppSendTextParams());
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft Send Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::AddChangeListener(FOnPubnubDraftUpdated DraftUpdateCallback)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		auto lambda = [DraftUpdateCallback](Pubnub::Vector<Pubnub::MessageElement> MessageElements)
		{
			auto StdMessageElements = MessageElements.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [DraftUpdateCallback, StdMessageElements]()
			{
				TArray<UPubnubMessageElement*> FinalMessageElements;
			
				for(auto CppMessageElement : StdMessageElements)
				{
					UPubnubMessageElement* NewMessageElement = UPubnubMessageElement::Create(CppMessageElement);
					FinalMessageElements.Add(NewMessageElement);
				}
			
				DraftUpdateCallback.ExecuteIfBound(FinalMessageElements);
			});
		};
		InternalMessageDraft->add_change_listener(lambda);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft Add Change Listener Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}

void UPubnubMessageDraft::AddChangeListenerWithSuggestions(FOnPubnubDraftUpdatedWithSuggestions DraftUpdateCallback)
{
	if(!IsInternalMessageDraftValid()) {return;}

	try
	{
		auto lambda = [DraftUpdateCallback](Pubnub::Vector<Pubnub::MessageElement> MessageElements , Pubnub::Vector<Pubnub::SuggestedMention> SuggestedMentions)
		{
			auto StdMessageElements = MessageElements.into_std_vector();
			auto StdSuggestedMentions = SuggestedMentions.into_std_vector();
			AsyncTask(ENamedThreads::GameThread, [DraftUpdateCallback, StdMessageElements, StdSuggestedMentions]()
			{
				TArray<UPubnubMessageElement*> FinalMessageElements;
			
				for(auto CppMessageElement : StdMessageElements)
				{
					UPubnubMessageElement* NewMessageElement = UPubnubMessageElement::Create(CppMessageElement);
					FinalMessageElements.Add(NewMessageElement);
				}

				TArray<FPubnubSuggestedMention> FinalSuggestedMentions;
			
				for(auto CppSuggestedMention : StdSuggestedMentions)
				{
					FinalSuggestedMentions.Add(FPubnubSuggestedMention(CppSuggestedMention));
				}
			
				DraftUpdateCallback.ExecuteIfBound(FinalMessageElements, FinalSuggestedMentions);
			});
		};
		InternalMessageDraft->add_change_listener(lambda);
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("Message Draft Add Change Listener With Suggestions Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
}


bool UPubnubMessageDraft::IsInternalMessageDraftValid()
{
	if(InternalMessageDraft == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("This PubnubMessageDraft is invalid"));
		return false;
	}
	return true;
}
