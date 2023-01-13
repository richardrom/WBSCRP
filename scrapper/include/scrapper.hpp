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
#elif !defined(REPORT_ALLOCATIONS) && defined(CHECK_MEMORY_LEAK)
    template <typename T>
    using pool_allocator = pool::pool_allocator<T, pool::pool_iostream_reporter>;
#else
    template <typename T>
    using pool_allocator = pool::pool_allocator<T>;
#endif /*REPORT_ALLOCATIONS*/
    using sc_string = std::basic_string<char_type, std::char_traits<char_type>, pool_allocator<char_type>>;

    template <typename T>
    using sc_vector = std::vector<T, pool_allocator<T>>;

    template <typename T>
    using sc_stack = std::stack<T, std::deque<T, pool_allocator<T>>>;

    template <typename Key, typename T>
    using sc_map = std::map<Key, T, std::less<Key>, pool_allocator<std::pair<const Key, T>>>;

    template <typename Key, typename T>
    using sc_unordered_map = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, pool_allocator<std::pair<const Key, T>>>;

    // Call before any calls to the library
    extern bool initialize();
    extern bool is_initialized();

    // HTML5 Conformant states
    enum class States
    {
        Data,
        CharacterReference,
        NamedCharacterReference,
        NumericCharacterReference,
        HexadecimalCharacterReferenceStart,
        DecimalCharacterReferenceStart,
        HexadecimalCharacterReference,
        DecimalCharacterReference,
        NumericCharacterReferenceEnd,
        AmbiguousAmpersand,
        TagOpen,
        MarkupDeclarationOpen,
        CommentStart,
        CommentEnd,
        BogusComment,
        CommentStartDash,
        CommentLessThanSign,
        CommentLessThanSignBang,
        CommentLessThanSignBangDash,
        CommentLessThanSignBangDashDash,
        CommentEndDash,
        CommentEndBang,
        Comment,
        DOCTYPE,
        BeforeDOCTYPEName,
        DOCTYPEName,
        AfterDOCTYPEName,
        AfterDOCTYPEPublicKeyword,
        BeforeDOCTYPEPublicIdentifier,
        DOCTYPEPublicIdentifierDQ,
        DOCTYPEPublicIdentifierSQ,
        AfterDOCTYPEPublicIdentifier,
        BetweenDOCTYPEPublicAndSystemIdentifiers,
        AfterDOCTYPESystemKeyword,
        BeforeDOCTYPESystemIdentifier,
        DOCTYPESystemIdentifierDQ,
        DOCTYPESystemIdentifierSQ,
        AfterDOCTYPESystemIdentifier,
        BogusDOCTYPE,
        CDATASection,
        CDATASectionBracket,
        CDATASectionEnd,
        EndTagOpen,
        TagName,
        BeforeAttributeName,
        SelfClosingStartTag,
        AttributeName,
        AfterAttributeName,
        BeforeAttributeValue,
        AttributeValueDQ,
        AttributeValueSQ,
        AttributeValueUnquoted,
        AfterAttributeValueQuoted
    };

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
        explicit Token(TokenType tp) :
            type { tp } { }
        TokenType type;
    };

    struct EOFToken : public Token
    {
        inline EOFToken() :
            Token(TokenType::EndOfFile) { }
    };

    struct CommentToken : public Token
    {
        inline explicit CommentToken(const sc_string &cmt) :
            Token(TokenType::Comment),
            comment { cmt } { }
        sc_string comment;
    };

    struct DOCTYPEToken : public Token
    {

        inline explicit DOCTYPEToken(const sc_string &nm) :
            Token(TokenType::DOCTYPE),
            name { nm } { }
        inline DOCTYPEToken(const sc_string &nm, const sc_string &public_id_name) :
            Token(TokenType::DOCTYPE),
            name { nm },
            public_identifier_name { public_id_name } { }
        inline DOCTYPEToken(const sc_string &nm, const sc_string &public_id_name, const sc_string &system_id_name) :
            Token(TokenType::DOCTYPE),
            name { nm },
            public_identifier_name { public_id_name },
            system_identifier_name { system_id_name } { }

        sc_string name;
        sc_string public_identifier_name;
        sc_string system_identifier_name;
    };

    struct CDATAToken : public Token
    {
        inline explicit CDATAToken(const sc_string &data) :
            Token(TokenType::CDATA),
            cdata { data } { }
        sc_string cdata;
    };

    struct CharacterToken : public Token
    {
        inline explicit CharacterToken(const sc_string &cp) :
            Token(TokenType::Character),
            code_point { cp } { }
        sc_string code_point;
    };

    struct EndTagToken : public Token
    {
        inline EndTagToken() :
            Token(TokenType::EndTag) { }
    };

    struct TagToken : public Token
    {
        explicit TagToken(const sc_string &name) :
            Token(TokenType::Tag),
            tag_name { name } { }

        explicit TagToken(const sc_string &name, const sc_unordered_map<sc_string, sc_string> &attrs) :
            Token(TokenType::Tag),
            tag_name { name },
            attributes { attrs } { }

        explicit TagToken(const sc_string &name, bool self_close) :
            Token(TokenType::Tag),
            tag_name { name },
            attributes {},
            self_closing { self_close } { }

        explicit TagToken(const sc_string &name, const sc_unordered_map<sc_string, sc_string> &attrs, bool self_close) :
            Token(TokenType::Tag),
            tag_name { name },
            attributes { attrs },
            self_closing { self_close } { }

        sc_string tag_name;
        sc_unordered_map<sc_string, sc_string> attributes;
        bool self_closing { false };
    };

} // namespace scrp

#endif /*CHREF_TOOL*/

#endif // WBSCRP_SCRAPPER_HPP
