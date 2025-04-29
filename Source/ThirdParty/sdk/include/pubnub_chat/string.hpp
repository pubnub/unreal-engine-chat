#ifndef PN_STRING_H
#define PN_STRING_H

#include "helpers/export.hpp"
#include <cstddef>
#include <ostream>
#include <string>
#include <cstring>

namespace Pubnub {
    /**
     * The String class to handle strings in the Pubnub SDK.
     *
     * It wraps the const char* string and provides methods to manipulate it 
     * without the need to worry about memory management and dll boundaries.
     *
     * The String is designed to be used as a value type and it is not meant to be used as a pointer.
     *
     * Example:
     *  String string = "Hello, World!";
     *  string += " How are you?";
     *
     *  std::cout << string << std::endl;
     *
     *  // Output: Hello, World! How are you?
     */
    PN_CHAT_EXPORT class String 
    {
        //TODO: check FString from Unreal Engine
    public:
        /**
         * Default constructor that leaves the string empty.
         */
        PN_CHAT_EXPORT String() = default;
        PN_CHAT_EXPORT ~String();

        /**
         * Constructor that initializes the string with a const char* string.
         * 
         * @param string The const char* string to initialize the String with.
         */
        PN_CHAT_EXPORT String(const char* string);

        /**
         * Constructor that initializes the string with a std::string.
         * 
         * @param string The char* to initialize the String with.
         */
        PN_CHAT_EXPORT String(char* string);

        /**
         * Constructor that initializes the string with a const char* string 
         * and the length of the string.
         *
         * @param string The const char* string to initialize the String with.
         * @param length The length of the string.
         */
        PN_CHAT_EXPORT String(const char* string, std::size_t length);

        /**
         * Constructor that initializes the string with a std::string.
         * 
         * @param string The std::string to initialize the String with.
         */
        PN_CHAT_EXPORT String(std::string string);

        /**
         * Copy constructor that initializes the string with another String.
         * 
         * @param string The std::string const reference to initialize the String with.
         */
        PN_CHAT_EXPORT String(const String& string);

        /**
         * Move constructor that initializes the string with another String.
         * 
         * @param string The std::string rvalue reference to initialize the String with.
         */
        PN_CHAT_EXPORT String(String&& string);

        /**
         * Implicit conversion operator that converts the String to a std::string.
         * 
         * @return The std::string string.
         */
        PN_CHAT_EXPORT operator std::string() const;

        /**
         * Implicit conversion operator that converts the String to a const char*.
         *
         * Keep in mind that the pointer is only valid as long as the String is not modified.
         * 
         * @return The const char* string.
         */
        PN_CHAT_EXPORT operator const char*() const;

        /**
         * Copy assignment operator that assigns the string with another String.
         * 
         * @param string The std::string const reference to assign the String with.
         */
        PN_CHAT_EXPORT String& operator=(const String& string);

        /**
         * Move assignment operator that assigns the string with another String.
         * 
         * @param string The std::string rvalue reference to assign the String with.
         */
        PN_CHAT_EXPORT String& operator=(String&& string);

        /**
         * Copy assignment operator that assigns the string with a const char* string.
         * 
         * @param string The const char* to assign the String with.
         */
        PN_CHAT_EXPORT String& operator=(const char* string);

        /**
         * Copy assignment operator that assigns the string with a char* string.
         * 
         * @param string The char* to assign the String with.
         */
        PN_CHAT_EXPORT String& operator=(char* string);

        /**
         * Add assignment operator that appends the string with another String.
         *
         * Usage of this operator means that the String is considered as a mutable object
         * and the memory management expects that the operation will be done more than once.
         * That means that the String will allocate more memory than it needs to store the string 
         * to avoid reallocation on every append operation.
         *
         * @see capacity
         * @see reserve
         *
         * @param string The std::string const reference to append the String with.
         *
         * Example:
         *  String string = "Hello, ";
         *  String world = "World!";
         *  string += world;
         *  
         *  // String { "Hello, World!" } 
         */
        PN_CHAT_EXPORT String& operator+=(const String& string);

        /**
         * Add assignment operator that appends the string with another String.
         *
         * Usage of this operator means that the String is considered as a mutable object
         * and the memory management expects that the operation will be done more than once.
         * That means that the String will allocate more memory than it needs to store the string 
         * to avoid reallocation on every append operation.
         *
         * @see capacity 
         * @see reserve
         *
         * @param string The std::string const reference to append the String with.
         *
         * Example:
         *  String string = "Hello, ";
         *  String world = "World!";
         *  string += world;
         *
         *  // String { "Hello, World!" }
         */
        PN_CHAT_EXPORT String& operator+=(String& string);

        /**
         * Add assignment operator that appends the string with a const char* string.
         *
         * Usage of this operator means that the String is considered as a mutable object
         * and the memory management expects that the operation will be done more than once.
         * That means that the String will allocate more memory than it needs to store the string 
         * to avoid reallocation on every append operation.
         *
         * @see capacity 
         * @see reserve
         *
         * @param string The const char* to append the String with.
         *
         * Example:
         *  String string = "Hello, ";
         *  string += "World!";
         *
         *  // String { "Hello, World!" }
         */
        PN_CHAT_EXPORT String& operator+=(const char* string);

        /**
         * Add assignment operator that appends the string with a std::string.
         *
         * Usage of this operator means that the String is considered as a mutable object
         * and the memory management expects that the operation will be done more than once.
         * That means that the String will allocate more memory than it needs to store the string 
         * to avoid reallocation on every append operation.
         *
         * @see capacity 
         * @see reserve
         *
         * @param string The std::string to append the String with.
         *
         * Example:
         *  String string = "Hello, ";
         *  std::string world = "World!";
         *  string += world;
         *
         *  // String { "Hello, World!" }
         */
        PN_CHAT_EXPORT String& operator+=(std::string string);

        /**
         * Copy assignment operator that assigns the string with a std::string.
         * 
         * @param string The std::string to assign the String with.
         */
        PN_CHAT_EXPORT String& operator=(std::string string);

        /**
         * Returns raw const char* pointer to the string that the String holds.
         *
         * Keep in mind that the pointer is only valid as long as the String is not modified.
         *
         * @return The const char* string.
         */
       PN_CHAT_EXPORT const char* c_str() const;

       /**
        * Returns the raw char* pointer to the string that the String holds.
        *
        * Keep in mind that the pointer is only valid as long as the String is not modified.
        * 
        * @note You should not edit the string directly through the pointer unless you know what you are doing.
        *
        * @return The char* string.
        */
       PN_CHAT_EXPORT char* c_str();

        /**
         * Returns std version of the string that the String holds.
         *
         * @return The std::string string.
         */
       PN_CHAT_EXPORT std::string to_std_string() const;

        /**
         * Returns the length of the string that the String holds.
         *
         * @return The length of the string.
         */
       PN_CHAT_EXPORT std::size_t length() const;

        /**
         * Returns the capacity of the string that the String holds.
         * 
         * Capacity is the size of the allocated storage for the string, a value that is at least as large as length.
         *
         * It is a good practice to reserve the memory for the string if the append operation is expected to be done
         * more than once. The memory will be reallocated only if the capacity is not enough to store the new character.
         *
         * @see reserve
         *
         * @return The capacity of the string.
         */
       PN_CHAT_EXPORT std::size_t capacity() const;

       /**
        * Checks if the string is empty.
        *
        * @return True if the string is empty, false otherwise.
        */
       PN_CHAT_EXPORT bool empty() const;

       /**
        * Clears the string.
        *
        * This method will not reallocate the memory.
        * It is used to clear the content of the string and set the length of the string to zero
        * without freeing the memory. It is useful when the memory is needed to be reused.
        *
        * To completely free the memory, the String should be destroyed.
        *
        * @see erase
        */
       PN_CHAT_EXPORT void clear();

       /**
        * Erases the character(s) at the specified position.
        *
        * This method will not reallocate the memory.
        *
        * @see capacity 
        *
        * @param pos The position of the character to erase. When the position is out of bounds, the method will do nothing.
        * @param count The number of characters to erase.
        *
        * Example:
        *   String string = "Hello, World!";
        *   string.erase(5, 8);
        *
        *   // String { "Hello!" }
        */
       PN_CHAT_EXPORT void erase(std::size_t pos, std::size_t count = 1);

       /**
        * Inserts the character at the specified position.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character.
        * The memory will be reallocated to the new size that is the length of the string plus one.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity
        * @see reserve
        *
        * @param pos The position to insert the character at. When the position is out of bounds, the method will do nothing.
        * @param character The character to insert.
        *
        * Example:
        *  String string = "Hello, World!";
        *  string.insert(5, '!');
        *
        * // String { "Hello!, World!" }
        */
       PN_CHAT_EXPORT void insert(std::size_t pos, char character);

       /**
        * Shrinks the capacity of the string to fit its length.
        *
        * This method will reallocate the memory to the size of the length of the string.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity 
        *
        * Example:
        *  String string;
        *  string.reserve(100);
        *  string += "Hello, ";
        *  string += "World!";
        *  string.shrink();
        *
        *  // String { string: "Hello, World!", capacity: 13 }
        */
       PN_CHAT_EXPORT void shrink();

       /**
        * Finds the first occurrence of the character or sequence of characters in the string.
        *
        * @param string The character or sequence of characters to find.
        * @param std::size_t The position to start the search from.
        *
        * @return The position of the first occurrence of the character or sequence of characters
        * or String::npos if the character or sequence of characters is not found.
        *
        * @see String::npos
        *
        * Example:
        *  String string = "Hello, World!";
        *  auto pos = string.find("World!");
        *
        *  // 7
        */
       PN_CHAT_EXPORT std::size_t find(const char* string, std::size_t pos = 0) const;

       /**
        * Replaces the character or sequence of characters with another character or sequence of characters.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity
        * @see reserve
        *
        * @param pos The position to start the replacement from.
        * @param count The number of characters to replace.
        * @param strign The character or sequence of characters to replace with.
        *
        * Example:
        *  String string = "Hello, World!";
        *  string.replace(7, 6, "Universe!");
        *
        *  // String { "Hello, Universe!" }
        */
       PN_CHAT_EXPORT void replace(std::size_t pos, std::size_t count, const char* string);

       /**
        * Replaces the character or sequence of characters with another character or sequence of characters.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character 
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity
        * @see reserve
        *
        * @param pos The position to start the replacement from.
        * @param count The number of characters to replace.
        * @param string The character or sequence of characters to replace with.
        *
        * Example:
        *  String string = "Hello, World!";
        *  std::string universe = "Universe!";
        *  string.replace(7, 6, universe);
        *
        *  // String { "Hello, Universe!" }
        */
       PN_CHAT_EXPORT void replace(std::size_t pos, std::size_t count, const std::string string);

       /**
        * Replaces the character or sequence of characters with another character or sequence of characters. 
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character 
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity 
        * @see reserve
        *
        * @param pos The position to start the replacement from.
        * @param count The number of characters to replace.
        * @param string The character or sequence of characters to replace with.
        *
        * Example:
        *  String string = "Hello, World!";
        *  String universe = "Universe!";
        *  string.replace(7, 6, universe);
        *
        *  // String { "Hello, Universe!" }
        */
       PN_CHAT_EXPORT void replace(std::size_t pos, std::size_t count, const String& string);

       /**
        * Replaces all the occurrences of the character or sequence of characters with another character or sequence of characters.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character 
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity 
        * @see reserve 
        *
        * @param string The character or sequence of characters to replace.
        * @param replacement The character or sequence of characters to replace with.
        *
        * Example:
        * String string = "Hello, World!";
        * string.replace_all("World!", "Universe!");
        *
        * // String { "Hello, Universe!" }
        */
       PN_CHAT_EXPORT void replace_all(const char* string, const char* replacement);

       /**
        * Replaces all the occurrences of the character or sequence of characters with another character or sequence of characters.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character 
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity 
        * @see reserve 
        *
        * @param string The character or sequence of characters to replace.
        * @param replacement The character or sequence of characters to replace with.
        *
        * Example:
        * String string = "Hello, World!";
        * string.replace_all("World!", "Universe!");
        *
        * // String { "Hello, Universe!" }
        */       
       PN_CHAT_EXPORT void replace_all(const std::string& string, const std::string& replacement);

       /**
        * Replaces all the occurrences of the character or sequence of characters with another character or sequence of characters.
        *
        * This method will reallocate the memory if the capacity is not enough to store the new character 
        * or sequence of characters. The memory will be reallocated to the new size that is the length
        * of the string plus the length of the new character or sequence of characters.
        * The previous content of the string will be copied to the new memory.
        *
        * @see capacity 
        * @see reserve 
        *
        * @param string The character or sequence of characters to replace.
        * @param replacement The character or sequence of characters to replace with.
        *
        * Example:
        * String string = "Hello, World!";
        * String universe = "Universe!";
        * string.replace_all("World!", universe);
        *
        * // String { "Hello, Universe!" }
        */
       PN_CHAT_EXPORT void replace_all(const String& string, const String& replacement);

       /**
        * Substring method that returns the substring of the string.
        *
        * This function creates a new String object that holds the substring of the string.
        * The memory for the new String object is allocated and the content of the substring is copied to the new memory.
        *
        * @param pos The position to start the substring from.
        * @param count The number of characters to include in the substring.
        *
        * @return The substring of the string.
        *
        * Example:
        *  String string = "Hello, World!";
        *  auto substring = string.substring(7, 6);
        *
        *  // String { "World!" }
        */
       PN_CHAT_EXPORT String substring(std::size_t pos, std::size_t count) const;

        // TODO: reconsider the iterator design
       /**
        * Begins the read-only iteration over the string.
        *
        * @return The const char* pointer to the beginning of the string.
        */
       PN_CHAT_EXPORT const char* begin() const;

       /**
        * Ends the read-only iteration over the string.
        *
        * @return The const char* pointer to the end of the string.
        */
       PN_CHAT_EXPORT const char* end() const;

       /**
        * Begins the read-write iteration over the string.
        *
        * @return The char* pointer to the beginning of the string.
        */
       PN_CHAT_EXPORT char* begin();

       /**
        * Ends the read-write iteration over the string.
        *
        * @return The char* pointer to the end of the string.
        */
       PN_CHAT_EXPORT char* end();

       /**
        * Returns the first character of the string.
        *
        * @return The first character of the string.
        */
       PN_CHAT_EXPORT const char& front() const;

       /**
        * Returns the last character of the string.
        *
        * @return The last character of the string.
        */
       PN_CHAT_EXPORT const char& back() const;

       /**
        * Returns the first character of the string.
        *
        * @return The first character of the string.
        */
       PN_CHAT_EXPORT char& front();

       /**
        * Returns the last character of the string.
        *
        * @return The last character of the string.
        */
       PN_CHAT_EXPORT char& back();

       /**
        * Returns the const char* pointer to the string and invalidates the String.
        * It is meant to be used when the String is passed to a function that takes const char*
        * and the String is not needed anymore or the existence of the String can apply undefined behavior
        * e.g. when the String is passed through a dll boundary over the C ABI.
        *
        * This function will return the pointer to the string and set the internal pointer to nullptr
        * so the memory is not freed when the String is destroyed.
        *
        * @return The const char* pointer to the string.
        *
        * @see c_str
        *
        * Example:
        *  String string = "Hello, World!";
        *  auto c_str = string.into_c_str();
        *
        *  // c_str is valid 
        *  // string is invalid and should not be used anymore
        */
       PN_CHAT_EXPORT char* into_c_str();

       /**
        * Allocates the memory for the string.
        *
        * It can be used to preallocate the memory for the string to avoid reallocation on every append operation.
        * The capacity of the string will be set to the given size and the memory will be reallocated to the new size.
        * The previous content of the string will be copied to the new memory.
        *
        * If the given size is smaller than the current length of the string, the extra characters will shrinked!
        *
        * @see shrink
        *
        * The need to reserve memory should be considered when the String is used as a mutable object and the append operation
        * is expected to be done more than once.
        *
        * @param size The size of the memory to allocate.
        *
        * Example:
        *   String string;
        *   string.reserve(100);
        *   string += "Hello, ";
        *   string += "World!";
        *   // ...
        *   // The memory for the string is allocated only once and the append operation is done in the allocated memory.
        */
       PN_CHAT_EXPORT void reserve(std::size_t size);

       /**
        * The constant that represents the position that is not found.
        *
        * @see find 
        *
        * @note The value is set to -1 whereat the std::size_t is an unsigned integer so depending on the compiler
        * can be different. In most cases, the value is the maximum value of the std::size_t.
        *
        * Example:
        *  String string = "Hello, World!";
        *  auto pos = string.find("Universe!");
        *
        *  if (pos == String::npos) {
        *    // The string is not found
        *  }
        */
       static const std::size_t npos = -1;

       // TODO: think about rust-like iterators

    private:
        std::size_t calculate_capacity(std::size_t size) const;
        void grow_if_needed(std::size_t size);
        void init(const char* string, std::size_t length);

        char* string = nullptr;
        unsigned int len = 0;
        unsigned int cap = 0;
    };

    struct StringComparer
    {
        bool operator()(const String& lhs, const String& rhs) const
        {
            auto comparison_bounds = std::min(lhs.length(), rhs.length());

            for (std::size_t i = 0; i < comparison_bounds; i++) {
                if (lhs[i] < rhs[i]) {
                    return true;
                } else if (lhs[i] > rhs[i]) {
                    return false;
                }
            }

            return lhs.length() < rhs.length();
        }
    };
}

/**
 * Add operator that appends two Strings.
 *
 * This operator creates a new String object that holds the result of the append operation.
 * The memory for the new String object is allocated and the content of the append operation
 * is copied to the new memory.
 *
 * @param lhs The left hand side String to append.
 * @param rhs The right hand side String to append.
 * @return The String that is the result of the append operation.
 *
 * Example:
 *  String string = "Hello, ";
 *  String world = "World!";
 *  auto result = string + world;
 *
 *  // String { "Hello, World!" }
 */
PN_CHAT_EXPORT Pubnub::String operator+(const Pubnub::String& lhs, const Pubnub::String& rhs);


/**
 * Add operator that appends a String with a const char* string.
 *
 * This operator creates a new String object that holds the result of the append operation.
 * The memory for the new String object is allocated and the content of the append operation
 * is copied to the new memory.
 *
 * @param lhs The left hand side String to append.
 * @param rhs The right hand side const char* string to append.
 * @return The String that is the result of the append operation.
 *
 * Example:
 *  String string = "Hello, ";
 *  auto result = string + "World!";
 *
 *  // String { "Hello, World!" }
 */
//PN_CHAT_EXPORT Pubnub::String operator+(const Pubnub::String& lhs, const char* rhs);

/**
 * Add operator that appends a String with a std::string.
 *
 * This operator creates a new String object that holds the result of the append operation.
 * The memory for the new String object is allocated and the content of the append operation
 * is copied to the new memory.
 *
 * @param lhs The left hand side String to append.
 * @param rhs The right hand side std::string to append.
 * @return The String that is the result of the append operation.
 *
 * Example:
 *  String string = "Hello, ";
 *  std::string world = "World!";
 *  auto result = string + world;
 *
 *  // String { "Hello, World!" }
 */
//PN_CHAT_EXPORT Pubnub::String operator+(const Pubnub::String& lhs, std::string rhs);

/**
 * Equality operator that compares two Strings over their values.
 * 
 * @param lhs The left hand side String to compare.
 * @param rhs The right hand side String to compare.
 * @return True if the String values are equal (ptr do not need to be the same), false otherwise.
 */
PN_CHAT_EXPORT bool operator==(const Pubnub::String& lhs, const Pubnub::String& rhs);

/**
 * Inequality operator that compares two Strings over their values.
 * 
 * @param lhs The left hand side String to compare.
 * @param rhs The right hand side String to compare.
 * @return True if the String values are not equal, false otherwise.
 */
PN_CHAT_EXPORT bool operator!=(const Pubnub::String& lhs, const Pubnub::String& rhs);

#endif /* PN_STRING_H */
