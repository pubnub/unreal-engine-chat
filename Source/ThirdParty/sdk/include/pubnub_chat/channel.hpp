#ifndef PN_CHAT_CHANNEL_HPP
#define PN_CHAT_CHANNEL_HPP

#include "callback_handle.hpp"
#include "event.hpp"
#include "string.hpp"
#include "helpers/export.hpp"
#include "restrictions.hpp"
#include "message_elements.hpp"
#include "message.hpp"
#include "enums.hpp"
#include "vector.hpp"
#include "callback_stop.hpp"
#include "page.hpp"
#include "map.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include "send_text_params.hpp"

#include "message_draft_config.hpp"

#ifdef PN_CHAT_C_ABI
#include "pubnub_helper.h"
#endif

class ChannelService; 
class ChatService;
class PresenceService;
class RestrictionsService;
class MessageService;
class MembershipService;
class ChannelDAO;

namespace Pubnub 
{
    class Message;
    class Membership;
    class User;
    class MessageDraft;
    struct EventsHistoryWrapper;
    struct ChatMembershipData;

    struct ChatChannelData
    {
        Pubnub::String channel_name = "";
        Pubnub::String description = "";
        Pubnub::String custom_data_json = "";
        Pubnub::String updated = "";
        Pubnub::String status = "";
        Pubnub::String type = "";
    };

    struct MembersResponseWrapper
    {
        Pubnub::Vector<Pubnub::Membership> memberships;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };

    struct UsersRestrictionsWrapper
    {
        Pubnub::Vector<Pubnub::UserRestriction> restrictions;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };

    class PN_CHAT_EXPORT Channel {
        public:
            Channel();
            Channel(const Channel& other);
            virtual ~Channel();

            Pubnub::Channel& operator =(const Pubnub::Channel& other);

            Pubnub::String channel_id() const;
            Pubnub::ChatChannelData channel_data() const;

            Pubnub::Channel update(const ChatChannelData& in_additional_channel_data) const;
            Pubnub::CallbackHandle connect(std::function<void(Message)> message_callback) const;
            Pubnub::CallbackHandle join(std::function<void(Message)> message_callback, const Pubnub::String& additional_params = "") const;
            Pubnub::CallbackHandle join(std::function<void(Message)> message_callback, const Pubnub::ChatMembershipData& membership_data) const;
            void disconnect() const;
            void leave() const;
            void delete_channel() const;

            virtual void send_text(const Pubnub::String& message, SendTextParams text_params = SendTextParams());
            Pubnub::Vector<Pubnub::String> who_is_present() const;
            bool is_present(const Pubnub::String& user_id) const;
            void set_restrictions(const Pubnub::String& user_id, Pubnub::Restriction restrictions) const;
            Pubnub::Restriction get_user_restrictions(const Pubnub::User& user) const;
            UsersRestrictionsWrapper get_users_restrictions(const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            Pubnub::Vector<Pubnub::Message> get_history(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 25) const;
            Pubnub::Message get_message(const Pubnub::String& timetoken) const;
            MembersResponseWrapper get_members(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            Pubnub::Membership invite(const Pubnub::User& user) const;
            Pubnub::Vector<Pubnub::Membership> invite_multiple(Pubnub::Vector<Pubnub::User> users) const;
            void start_typing() const;
            void stop_typing() const;
            CallbackHandle get_typing(std::function<void(Pubnub::Vector<Pubnub::String>)> typing_callback) const;
            Pubnub::Channel pin_message(const Pubnub::Message& message) const;
            Pubnub::Channel unpin_message() const;
            Pubnub::Message get_pinned_message() const;
            void forward_message(const Pubnub::Message& message) const;
            virtual void emit_user_mention(const Pubnub::String& user_id, const Pubnub::String& timetoken, const Pubnub::String& text) const;
            Pubnub::Vector<Pubnub::Membership> get_user_suggestions(Pubnub::String text, int limit = 10) const;

            Pubnub::CallbackHandle stream_updates(std::function<void(const Pubnub::Channel&)> channel_callback) const;
            Pubnub::CallbackHandle stream_updates_on(Pubnub::Vector<Pubnub::Channel> channels, std::function<void(Pubnub::Vector<Pubnub::Channel>)> channel_callback);
            CallbackHandle stream_presence(std::function<void(Pubnub::Vector<Pubnub::String>)> presence_callback) const;
            CallbackHandle stream_read_receipts(std::function<void(Pubnub::Map<Pubnub::String, Pubnub::Vector<Pubnub::String>, Pubnub::StringComparer>)> read_receipts_callback) const;

            Pubnub::EventsHistoryWrapper get_messsage_reports_history(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 100) const;
            CallbackHandle stream_message_reports(std::function<void(const Pubnub::Event&)> event_callback) const;
            Pubnub::MessageDraft create_message_draft(Pubnub::MessageDraftConfig message_draft_config = Pubnub::MessageDraftConfig()) const;

        protected:
            Channel(
                    Pubnub::String channel_id,
                    std::shared_ptr<const ChatService> chat_service,
                    std::shared_ptr<const ChannelService> channel_service,
                    std::shared_ptr<const PresenceService> presence_service,
                    std::shared_ptr<const RestrictionsService> restrictions_service,
                    std::shared_ptr<const MessageService> message_service,
                    std::shared_ptr<const MembershipService> membership_service,
                    std::unique_ptr<ChannelDAO> data);
            
            Pubnub::String channel_id_internal;
            std::unique_ptr<ChannelDAO> data;
            std::shared_ptr<const ChatService> chat_service;
            std::shared_ptr<const PresenceService> presence_service;
            std::shared_ptr<const RestrictionsService> restrictions_service;
            std::shared_ptr<const MessageService> message_service;
            std::shared_ptr<const MembershipService> membership_service;
            std::shared_ptr<const ChannelService> channel_service;

        friend class ::ChannelService;

#ifdef PN_CHAT_C_ABI
        public:
        // TODO: probably not needed
        Pubnub::Channel update_with_base(const Pubnub::Channel& base_channel) const;

        std::shared_ptr<const ChatService> shared_chat_service() const;
#endif
    };
};

#endif // PN_CHAT_CHANNEL_HPP
