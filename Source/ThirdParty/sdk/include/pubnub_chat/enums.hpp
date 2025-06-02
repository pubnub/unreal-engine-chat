#ifndef PN_ENUMS_HPP
#define PN_ENUMS_HPP

#include "string.hpp"
#include <cstdint>

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

enum pn_log_level {
    None,
    Error,
    Warning,
    Info,
    Debug,
    Trace
};

enum pn_connection_status {
    PCS_CONNECTION_ONLINE,
    PCS_CONNECTION_OFFLINE,
    PCS_CONNECTION_ERROR
};

}

#endif // PN_ENUMS_HPP
