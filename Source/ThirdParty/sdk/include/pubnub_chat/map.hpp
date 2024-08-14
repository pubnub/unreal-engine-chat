#ifndef PN_CHAT_MAP_HPP
#define PN_CHAT_MAP_HPP

#include "vector.hpp"
#include <map>
#include <iostream>

namespace Pubnub {

    template <typename T, typename K, typename C = std::less<K>>
    struct Map
    {
        Map(){};
        Map(std::map<T, K> in_map) {
            for(auto it = in_map.begin(); it != in_map.end(); it++)
            {
                keys.push_back(it->first);
                values.push_back(it->second);
            }
        }

        Map(std::map<T, K, C> in_map) {
            for(auto it = in_map.begin(); it != in_map.end(); it++)
            {
                keys.push_back(it->first);
                values.push_back(it->second);
            }
        }

        Map& operator=(const Map& other) {
            keys = other.keys;
            values = other.values;
            return *this;
        }

        Map& operator=(const std::map<T, K>&& other) {
            for(auto it = other.begin(); it != other.end(); it++)
            {
                keys.push_back(it->first);
                values.push_back(it->second);
            }
            return *this;
        }

        Map& operator=(const std::map<T, K, C>&& other) {
            for(auto it = other.begin(); it != other.end(); it++)
            {
                keys.push_back(it->first);
                values.push_back(it->second);
            }
            return *this;
        }

        Pubnub::Vector<T> keys;
        Pubnub::Vector<K> values;

        int size(){
            return keys.size();
        }

        std::map<T, K> into_std_map(){
            if(keys.size() != values.size())
            {
                throw std::runtime_error("Map is broken, can't convert into std::map");
            }

            std::vector<T> std_keys = keys.into_std_vector();
            std::vector<K> std_values = values.into_std_vector();

            std::map<T, K> final_map;
            for(int i = 0; i < std_keys.size(); i++)
            {
                final_map[std_keys[i]] = std_values[i];
            }

            return final_map;
        }

    };
}
#endif // PN_CHAT_VECTOR_HPP
