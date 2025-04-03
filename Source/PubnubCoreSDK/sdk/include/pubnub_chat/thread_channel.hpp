#ifndef PN_CHAT_THREAD_CHANNEL_HPP
#define PN_CHAT_THREAD_CHANNEL_HPP

#include "pubnub_chat/channel.hpp"
#include "pubnub_chat/message.hpp"


namespace Pubnub 
{
    class ThreadMessage;

    class ThreadChannel : public Channel
    {
        public:

        PN_CHAT_EXPORT ~ThreadChannel();
        PN_CHAT_EXPORT ThreadChannel();
        PN_CHAT_EXPORT ThreadChannel& operator=(const ThreadChannel& other);

        PN_CHAT_EXPORT Pubnub::String parent_channel_id() const;
        PN_CHAT_EXPORT Pubnub::Message parent_message() const;

        PN_CHAT_EXPORT void send_text(const Pubnub::String& message, SendTextParams text_params = SendTextParams()) override;

        PN_CHAT_EXPORT Pubnub::Vector<Pubnub::ThreadMessage> get_thread_history(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count) const;
        PN_CHAT_EXPORT Pubnub::ThreadChannel pin_message_to_thread(const Pubnub::ThreadMessage& message) const;
        PN_CHAT_EXPORT Pubnub::ThreadChannel unpin_message_from_thread() const;
        PN_CHAT_EXPORT Pubnub::Channel pin_message_to_parent_channel(const Pubnub::ThreadMessage& message) const;
        PN_CHAT_EXPORT Pubnub::Channel unpin_message_from_parent_channel() const;

        PN_CHAT_EXPORT virtual void emit_user_mention(const Pubnub::String& user_id, const Pubnub::String& timetoken, const Pubnub::String& text) const override;

        private:
            PN_CHAT_EXPORT ThreadChannel(
                    Pubnub::String channel_id, 
                    std::shared_ptr<const ChatService> chat_service, 
                    std::shared_ptr<const ChannelService> channel_service, 
                    std::shared_ptr<const PresenceService> presence_service, 
                    std::shared_ptr<const RestrictionsService> restrictions_service, 
                    std::shared_ptr<const MessageService> message_service, 
                    std::shared_ptr<const MembershipService> membership_service,
                    std::unique_ptr<ChannelDAO> data,
                    Pubnub::String parent_channel_id, 
                    Pubnub::Message parent_message);


            Pubnub::String parent_channel_id_internal;
            Pubnub::Message parent_message_internal;

        friend class ::ChannelService;

        //Bool to track if there was at least one message sent, because the thread is created on server during sending the first message
        bool is_thread_created = true;

        void set_is_thread_created(bool is_created);

      
    };
};

#endif // PN_CHAT_THREAD_CHANNEL_HPP
