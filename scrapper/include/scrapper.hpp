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
#include <stack>
#include <deque>

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

    template<typename T>
    using sc_vector = std::vector<T, pool_allocator<T>>;

    template<typename T>
    using sc_stack = std::stack<T, std::deque<T, pool_allocator<T>>>;


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
        EndTagOpen,
        AttributeValueDoubleQuoted,
        AttributeValueSingleQuoted,
        AttributeValueUnquoted,
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
        CDATA,
    };

    enum class TokenType
    {
        Comment,
        EndOfFile
    };
    struct Token
    {
        sc_string data;
        TokenType type;
    };


} // namespace scrp

#endif /*CHREF_TOOL*/


#endif // WBSCRP_SCRAPPER_HPP
