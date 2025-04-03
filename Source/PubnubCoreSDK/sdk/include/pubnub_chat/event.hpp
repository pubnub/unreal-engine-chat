#ifndef PN_EVENT_H
#define PN_EVENT_H

#include "string.hpp"
#include "enums.hpp"

namespace Pubnub
{
    struct Event
    {
        Pubnub::String timetoken = "";
        Pubnub::pubnub_chat_event_type type;
        Pubnub::String channel_id = "";
        Pubnub::String user_id = "";
        Pubnub::String payload = "";
    };

}
#endif /* PN_EVENT_H */
