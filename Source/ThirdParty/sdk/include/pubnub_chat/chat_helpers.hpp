#ifndef CHAT_HELPERS_HPP
#define CHAT_HELPERS_HPP

#include "string.hpp"
#include "helpers/extern.hpp"
#include "helpers/export.hpp"
#include "enums.hpp"
#include <vector>
#include <string>
#include <map>

namespace Pubnub
{


    //Creates Json object required to use set_memberships function.
    Pubnub::String create_set_memberships_object(Pubnub::String channel_id, Pubnub::String custom_params_json = "");

    //Creates Json object required to use set_members function.
    Pubnub::String create_set_members_object(Pubnub::String user_id, Pubnub::String custom_params_json = "");

    //Creates Json object required to use set_members function. Overload for multiple users
    Pubnub::String create_set_members_object(std::vector<Pubnub::String> users_ids, Pubnub::String custom_params_json = "");


    //TODO: Probably move this to separate namespace and helper file
    //Get all keys from std::map as a std::vector
    template<typename K, typename V, typename C = std::less<K>>
    std::vector<K> getKeys(const std::map<K, V, C>& m) 
    {
        std::vector<K> keys;
        keys.reserve(m.size());
        for (const auto& pair : m) {
            keys.push_back(pair.first);
        }
        return keys;
    };

    //TODO: Probably move this to separate namespace and helper file
    //Get all values from std::map as a std::vector
    template<typename K, typename V, typename C = std::less<K>>
    std::vector<K> getValues(const std::map<K, V, C>& m) 
    {
        std::vector<K> values;
        values.reserve(m.size());
        for (const auto& pair : m) {
            values.push_back(pair.second);
        }
        return values;
    };

    template<typename K, typename V>
    std::vector<K> getValues(const std::map<K, V>& m) 
    {
        std::vector<K> values;
        values.reserve(m.size());
        for (const auto& pair : m) {
            values.push_back(pair.second);
        }
        return values;
    };

    //Checks if string starts with provided prefix
    bool string_starts_with(const Pubnub::String& string, const Pubnub::String& prefix);

    inline Pubnub::String const bool_to_string(bool b)
    {
        return b ? "true" : "false";
    };

    Pubnub::String chat_message_to_publish_string(Pubnub::String message, Pubnub::pubnub_chat_message_type message_type);

    Pubnub::String get_now_timetoken();

}

#endif /* CHAT_HELPERS_HPP */
