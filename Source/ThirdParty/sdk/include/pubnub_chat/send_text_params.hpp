#ifndef PN_CHAT_SEND_TEXT_PARAMS_H
#define PN_CHAT_SEND_TEXT_PARAMS_H

#include "map.hpp"
#include "message.hpp"
#include "message_elements.hpp"
#include "string.hpp"

namespace Pubnub {

    struct SendTextParams {
        bool store_in_history = true;
        bool send_by_post = false;
        Pubnub::String meta = "";
        Pubnub::Map<int, Pubnub::MentionedUser> mentioned_users;
        Pubnub::Map<int, Pubnub::ReferencedChannel> referenced_channels;
        Pubnub::Vector<Pubnub::TextLink> text_links;
        Pubnub::Option<Pubnub::Message> quoted_message;
    };
}



#endif // PN_CHAT_SEND_TEXT_PARAMS_H
