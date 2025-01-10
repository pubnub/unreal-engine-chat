#ifndef PN_CHAT_THREAD_MESSAGE_HPP
#define PN_CHAT_THREAD_MESSAGE_HPP

#include "callback_stop.hpp"
#include "helpers/export.hpp"
#include "pubnub_chat/message.hpp"
#include "vector.hpp"

class CallbackService;

namespace Pubnub 
{
    class Channel;

    class ThreadMessage : public Message
    {
        public:
        PN_CHAT_EXPORT ~ThreadMessage();
        PN_CHAT_EXPORT ThreadMessage& operator=(const ThreadMessage& other);

        PN_CHAT_EXPORT Pubnub::String parent_channel_id() const {return parent_channel_id_internal;};

        PN_CHAT_EXPORT Pubnub::Channel pin_to_parent_channel() const;
        PN_CHAT_EXPORT Pubnub::Channel unpin_from_parent_channel() const;

        PN_CHAT_EXPORT CallbackStop stream_updates(std::function<void(const ThreadMessage&)> message_callback) const;
        PN_CHAT_EXPORT CallbackStop stream_updates_on(Pubnub::Vector<Pubnub::ThreadMessage>, std::function<void(Pubnub::Vector<Pubnub::ThreadMessage>)> callback) const;

        private:
            PN_CHAT_EXPORT ThreadMessage(
                    Pubnub::String timetoken, 
                    std::shared_ptr<ChatService> chat_service, 
                    std::shared_ptr<MessageService> message_service, 
                    std::shared_ptr<ChannelService> channel_service,
                    std::shared_ptr<RestrictionsService> restrictions_service, 
                    std::unique_ptr<MessageDAO> data, 
                    Pubnub::String parent_channel_id);

#ifndef PN_CHAT_C_ABI
            PN_CHAT_EXPORT ThreadMessage(Pubnub::Message base_message, Pubnub::String parent_channel_id);
#endif

            Pubnub::String parent_channel_id_internal;
        friend class ::MessageService;
        friend class ::CallbackService;
#ifdef PN_CHAT_C_ABI
        public:
        Pubnub::ThreadMessage update_with_thread_base(const Pubnub::ThreadMessage& base_message) const;
        ThreadMessage(Pubnub::Message base_message, Pubnub::String parent_channel_id);
#endif
      
    };
};

#endif // PN_CHAT_THREAD_MESSAGE_HPP
