#ifndef PN_CHAT_EXPORT_HPP
#define PN_CHAT_EXPORT_HPP

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(__MINGW32__)
#define PN_CHAT_EXPORT __declspec(dllexport)
#else
#define PN_CHAT_EXPORT
#endif

#endif /* PN_CHAT_EXPORT_HPP */

