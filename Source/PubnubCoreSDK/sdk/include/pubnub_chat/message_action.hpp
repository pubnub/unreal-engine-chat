#ifndef PN_MESSAGE_ACTION_H
#define PN_MESSAGE_ACTION_H

#include "string.hpp"
#include "enums.hpp"

namespace Pubnub
{
    struct MessageAction
    {
        Pubnub::pubnub_message_action_type type;
        Pubnub::String value;
        Pubnub::String timetoken;
        Pubnub::String user_id;
    };
}
#endif /* PN_MESSAGE_ACTION_H */
