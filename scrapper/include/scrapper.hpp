/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#pragma once

#ifndef __cplusplus
#error "C++ compiler needed"
#endif /*__cplusplus*/

#ifndef WBSCRP_SCRAPPER_HPP
#define WBSCRP_SCRAPPER_HPP

#ifndef CHREF_TOOL
#include "allocator.hpp"
#include "pool_reporter.hpp"
#include <deque>
#include <map>
#include <stack>
#include <unordered_map>
#include <utility>

namespace scrp
{
#ifdef USE_UTF16
    using char_type = wchar_t;
#else
    using char_type = char;
#endif /*USE_UTF16*/

#if defined(REPORT_ALLOCATIONS) && defined(CHECK_MEMORY_LEAK)
    template <typename T>
    using pool_allocator = pool::pool_allocator<T, pool::allocator_iostream_reporter, pool::pool_iostream_reporter>;

    template <typename T>
    using fixed_memory_pool = pool::memory_pool<T, pool::pool_iostream_reporter>;
#elif !defined(REPORT_ALLOCATIONS) && defined(CHECK_MEMORY_LEAK)
    template <typename T>
    using pool_allocator = pool::pool_allocator<T, pool::pool_iostream_reporter>;

    template <typename T>
    using fixed_memory_pool = pool::memory_pool<T, pool::pool_iostream_reporter>;
#else

#ifdef USE_STL_ALLOCATOR
    template <typename T>
    using pool_allocator = std::allocator<T>;

    template <typename T>
    using fixed_memory_pool = pool::memory_pool<T>;

#else
    template <typename T>
    using pool_allocator = pool::pool_allocator<T>;
    template <typename T>
    using fixed_memory_pool = pool::memory_pool<T>;
#endif /*USE_STL_ALLOCATOR*/

#endif /*REPORT_ALLOCATIONS*/

#ifdef USE_STL_ALLOCATOR
    using sc_string = std::string;
#else
    using sc_string         = std::basic_string<char_type, std::char_traits<char_type>, pool_allocator<char_type>>;
#endif

    template <typename T>
    using sc_vector = std::vector<T, pool_allocator<T>>;

    template <typename T>
    using sc_stack = std::stack<T, std::deque<T, pool_allocator<T>>>;

    template <typename T>
    using sc_deque = std::deque<T, pool_allocator<T>>;

    template <typename Key, typename T>
    using sc_map = std::map<Key, T, std::less<Key>, pool_allocator<std::pair<const Key, T>>>;

    template <typename Key, typename T>
    using sc_unordered_map = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, pool_allocator<std::pair<const Key, T>>>;

    // Call before any calls to the library
    extern bool initialize();
    extern bool is_initialized();

    enum class TokenType
    {
        Comment,
        EndOfFile,
        DOCTYPE,
        CDATA,
        Character,
        EndTag,
        Tag
    };

    struct Token
    {
        virtual ~Token() = default;

        explicit Token(TokenType tp) :
            type { tp } { }

        TokenType type;
        bool consumed { false };
    };

    struct EOFToken : public Token
    {
        inline EOFToken() :
            Token(TokenType::EndOfFile) { }
    };

    struct CommentToken : public Token
    {
        inline explicit CommentToken(sc_string cmt) :
            Token(TokenType::Comment),
            comment { std::move(cmt) } { }
        sc_string comment;
    };

    struct DOCTYPEToken : public Token
    {

        inline explicit DOCTYPEToken(sc_string nm) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) } { }
        inline DOCTYPEToken(sc_string nm, bool quirks_flag) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) },
            force_quirks_flag { quirks_flag } { }
        inline DOCTYPEToken(sc_string nm, sc_string public_id_name) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) },
            public_identifier_name { std::move(public_id_name) } { }
        inline DOCTYPEToken(sc_string nm, sc_string public_id_name, bool quirks_flag) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) },
            public_identifier_name { std::move(public_id_name) },
            force_quirks_flag { quirks_flag } { }
        inline DOCTYPEToken(sc_string nm, sc_string public_id_name, sc_string system_id_name) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) },
            public_identifier_name { std::move(public_id_name) },
            system_identifier_name { std::move(system_id_name) } { }
        inline DOCTYPEToken(sc_string nm, sc_string public_id_name, sc_string system_id_name, bool quirks_flag) :
            Token(TokenType::DOCTYPE),
            name { std::move(nm) },
            public_identifier_name { std::move(public_id_name) },
            system_identifier_name { std::move(system_id_name) },
            force_quirks_flag { quirks_flag } { }

        sc_string name;
        sc_string public_identifier_name;
        sc_string system_identifier_name;
        bool force_quirks_flag { false };
    };

    struct CDATAToken : public Token
    {
        inline explicit CDATAToken(sc_string data) :
            Token(TokenType::CDATA),
            cdata { std::move(data) } { }

        sc_string cdata;
    };

    struct CharacterToken : public Token
    {
        inline explicit CharacterToken(sc_string cp) :
            Token(TokenType::Character),
            code_point { std::move(cp) } { }

        sc_string code_point;
    };

    struct TagToken : public Token
    {
        TagToken() :
            Token(TokenType::EndTag) { }

        explicit TagToken(sc_string name) :
            Token(TokenType::Tag),
            tag_name { std::move(name) } { }

        explicit TagToken(sc_string name, const sc_unordered_map<sc_string, sc_string> &attrs) :
            Token(TokenType::Tag),
            tag_name { std::move(name) },
            attributes { attrs } { }

        explicit TagToken(sc_string name, bool self_close) :
            Token(TokenType::Tag),
            tag_name { std::move(name) },
            attributes {},
            self_closing { self_close } { }

        explicit TagToken(sc_string name, const sc_unordered_map<sc_string, sc_string> &attrs, bool self_close) :
            Token(TokenType::Tag),
            tag_name { std::move(name) },
            attributes { attrs },
            self_closing { self_close } { }

        inline auto set_end_tag() -> void
        {
            type = TokenType::EndTag;
        }

        sc_unordered_map<sc_string, sc_string> attributes;
        sc_string tag_name;
        bool self_closing { false };
    };

    extern fixed_memory_pool<EOFToken> *_pool_eof_token;
    extern fixed_memory_pool<CommentToken> *_pool_comment_token;
    extern fixed_memory_pool<DOCTYPEToken> *_pool_doctype_token;
    extern fixed_memory_pool<CDATAToken> *_pool_cdata_token;
    extern fixed_memory_pool<CharacterToken> *_pool_character_token;
    extern fixed_memory_pool<TagToken> *_pool_tag_token;

    template <typename T>
    inline auto get_token_pool() -> auto
    {
        return nullptr;
    }

    template <>
    inline auto get_token_pool<EOFToken>() -> auto
    {
        return _pool_eof_token;
    }
    template <>
    inline auto get_token_pool<CommentToken>() -> auto
    {
        return _pool_comment_token;
    }

    template <>
    inline auto get_token_pool<DOCTYPEToken>() -> auto
    {
        return _pool_doctype_token;
    }

    template <>
    inline auto get_token_pool<CDATAToken>() -> auto
    {
        return _pool_cdata_token;
    }

    template <>
    inline auto get_token_pool<CharacterToken>() -> auto
    {
        return _pool_character_token;
    }

    template <>
    inline auto get_token_pool<TagToken>() -> auto
    {
        return _pool_tag_token;
    }

} // namespace scrp

#endif /*CHREF_TOOL*/

#endif // WBSCRP_SCRAPPER_HPP
