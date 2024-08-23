#ifndef PN_CHAT_MENTIONS_HPP
#define PN_CHAT_MENTIONS_HPP

#include "event.hpp"
#include "message.hpp"

namespace Pubnub {
    struct UserMentionData {
        Pubnub::String channel_id;
        Pubnub::String user_id;
        Pubnub::Event event;
        Pubnub::Message message;

        Pubnub::Option<Pubnub::String> parent_channel_id;
        Pubnub::Option<Pubnub::String> thread_channel_id;
    };

    struct UserMentionDataList {
        Pubnub::Vector<Pubnub::UserMentionData> user_mention_data;
        bool is_more;
    };
}

#endif // PN_CHAT_MENTIONS_HPP
