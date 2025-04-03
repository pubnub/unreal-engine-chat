#ifndef PN_CHAT_EXPORT_HPP
#define PN_CHAT_EXPORT_HPP

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(__MINGW32__)
#ifdef PN_CHAT_STATIC_DEFINE
#  define PN_CHAT_EXPORT
#  define PN_CHAT_NO_EXPORT
#else
#  ifndef PN_CHAT_EXPORT
#    ifdef pubnub_chat_EXPORTS
        /* We are building this library */
#      define PN_CHAT_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define PN_CHAT_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef PN_CHAT_NO_EXPORT
#    define PN_CHAT_NO_EXPORT 
#  endif
#endif

#ifndef PN_CHAT_DEPRECATED
#  define PN_CHAT_DEPRECATED __declspec(deprecated)
#endif

#ifndef PN_CHAT_DEPRECATED_EXPORT
#  define PN_CHAT_DEPRECATED_EXPORT PN_CHAT_EXPORT PN_CHAT_DEPRECATED
#endif

#ifndef PN_CHAT_DEPRECATED_NO_EXPORT
#  define PN_CHAT_DEPRECATED_NO_EXPORT PN_CHAT_NO_EXPORT PN_CHAT_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef PN_CHAT_NO_DEPRECATED
#    define PN_CHAT_NO_DEPRECATED
#  endif
#endif
#else
#  define PN_CHAT_EXPORT
#endif

#endif /* PN_CHAT_EXPORT_HPP */

