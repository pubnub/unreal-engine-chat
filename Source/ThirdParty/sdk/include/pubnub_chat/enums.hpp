#ifndef PN_ENUMS_HPP
#define PN_ENUMS_HPP

#include "string.hpp"

namespace Pubnub
{
enum pubnub_message_action_type : uint8_t
{
    PMAT_Reaction,
    PMAT_Receipt,
    PMAT_Custom,
	PMAT_Edited,
	PMAT_Deleted,
	PMAT_ThreadRootId
};

enum pubnub_chat_event_type : uint8_t
{
    PCET_TYPING,
    PCET_REPORT,
    PCET_RECEPIT,
    PCET_MENTION,
    PCET_INVITE,
    PCET_CUSTOM,
    PCET_MODERATION
};

enum pubnub_chat_message_type : uint8_t
{
    PCMT_TEXT
};

enum EventMethod {
    Default,
    Publish,
    Signal
};

static inline EventMethod event_method_from_event_type(pubnub_chat_event_type type) {
    switch(type) {
        case pubnub_chat_event_type::PCET_TYPING:
        case pubnub_chat_event_type::PCET_RECEPIT:
            return EventMethod::Signal;
        default:
            return EventMethod::Publish;
    }
};

static inline Pubnub::String message_action_type_to_string(pubnub_message_action_type message_action_type)
{
    switch(message_action_type)
	{
	case pubnub_message_action_type::PMAT_Reaction:
		return "\"reaction\"";
	case pubnub_message_action_type::PMAT_Receipt:
		return "\"receipt\"";
	case pubnub_message_action_type::PMAT_Custom:
		return "\"custom\"";
	case pubnub_message_action_type::PMAT_Edited:
		return "\"edited\"";
	case pubnub_message_action_type::PMAT_Deleted:
		return "\"deleted\"";
	case pubnub_message_action_type::PMAT_ThreadRootId:
		return "\"threadRootId\"";
	}
	return "incorrect_chat_event_type";
};

static inline Pubnub::String message_action_type_to_non_quoted_string(pubnub_message_action_type message_action_type)
{
	switch (message_action_type)
	{
	case pubnub_message_action_type::PMAT_Reaction:
		return "reaction";
	case pubnub_message_action_type::PMAT_Receipt:
		return "receipt";
	case pubnub_message_action_type::PMAT_Custom:
		return "custom";
	case pubnub_message_action_type::PMAT_Edited:
		return "edited";
	case pubnub_message_action_type::PMAT_Deleted:
		return "deleted";
	case pubnub_message_action_type::PMAT_ThreadRootId:
		return "threadRootId";
	}
	return "incorrect_chat_event_type";
};

static inline Pubnub::String chat_event_type_to_string(pubnub_chat_event_type chat_event_type)
{
    switch(chat_event_type)
	{
	case pubnub_chat_event_type::PCET_TYPING:
		return "typing";
	case pubnub_chat_event_type::PCET_REPORT:
		return "report";
	case pubnub_chat_event_type::PCET_RECEPIT:
		return "receipt";
	case pubnub_chat_event_type::PCET_MENTION:
		return "mention";
	case pubnub_chat_event_type::PCET_INVITE:
		return "invite";
	case pubnub_chat_event_type::PCET_CUSTOM:
		return "custom";
	case pubnub_chat_event_type::PCET_MODERATION:
		return "moderation";
	}
	return "incorrect_chat_event_type";
};

static inline Pubnub::String chat_message_type_to_string(pubnub_chat_message_type chat_message_type)
{
    switch(chat_message_type)
	{
    case pubnub_chat_message_type::PCMT_TEXT:
        return "text";
    }
    return "incorrect_chat_message_type";
}

static inline pubnub_chat_message_type chat_message_type_from_string(Pubnub::String chat_message_type_string)
{
	if(chat_message_type_string == Pubnub::String("text") || chat_message_type_string == Pubnub::String("\"text\"")) return pubnub_chat_message_type::PCMT_TEXT;

	throw std::invalid_argument("can't convert chat_message_type_string to pubnub_chat_message_type");
}

static inline pubnub_message_action_type message_action_type_from_string(Pubnub::String message_action_type_string)
{
	if(message_action_type_string == Pubnub::String("reaction") || message_action_type_string == Pubnub::String("\"reaction\"")) 
		return pubnub_message_action_type::PMAT_Reaction;
	else if(message_action_type_string == Pubnub::String("receipt") || message_action_type_string == Pubnub::String("\"receipt\"")) 
		return pubnub_message_action_type::PMAT_Receipt;
	else if(message_action_type_string == Pubnub::String("custom") || message_action_type_string == Pubnub::String("\"custom\"")) 
		return pubnub_message_action_type::PMAT_Custom;
	else if(message_action_type_string == Pubnub::String("edited") || message_action_type_string == Pubnub::String("\"edited\"")) 
		return pubnub_message_action_type::PMAT_Edited;
	else if(message_action_type_string == Pubnub::String("deleted") || message_action_type_string == Pubnub::String("\"deleted\"")) 
		return pubnub_message_action_type::PMAT_Deleted;
	else if(message_action_type_string == Pubnub::String("threadRootId") || message_action_type_string == Pubnub::String("\"threadRootId\"")) 
		return pubnub_message_action_type::PMAT_ThreadRootId;
	
	throw std::invalid_argument("can't convert message_action_type_string to pubnub_message_action_type");
}

static inline pubnub_chat_event_type chat_event_type_from_string(Pubnub::String chat_event_type_string)
{
	if(chat_event_type_string == Pubnub::String("typing") || chat_event_type_string == Pubnub::String("\"typing\"")) 
		return pubnub_chat_event_type::PCET_TYPING;
	else if(chat_event_type_string == Pubnub::String("report") || chat_event_type_string == Pubnub::String("\"report\"")) 
		return pubnub_chat_event_type::PCET_REPORT;
	else if(chat_event_type_string == Pubnub::String("receipt") || chat_event_type_string == Pubnub::String("\"receipt\"")) 
		return pubnub_chat_event_type::PCET_RECEPIT;
	else if(chat_event_type_string == Pubnub::String("mention") || chat_event_type_string == Pubnub::String("\"mention\"")) 
		return pubnub_chat_event_type::PCET_MENTION;
	else if(chat_event_type_string == Pubnub::String("invite") || chat_event_type_string == Pubnub::String("\"invite\"")) 
		return pubnub_chat_event_type::PCET_INVITE;
	else if(chat_event_type_string == Pubnub::String("custom") || chat_event_type_string == Pubnub::String("\"custom\"")) 
		return pubnub_chat_event_type::PCET_CUSTOM;
	else if(chat_event_type_string == Pubnub::String("moderation") || chat_event_type_string == Pubnub::String("\"moderation\"")) 
		return pubnub_chat_event_type::PCET_MODERATION;
	
	throw std::invalid_argument("can't convert chat_event_type_string to pubnub_chat_event_type");
}

}

#endif // PN_ENUMS_HPP
