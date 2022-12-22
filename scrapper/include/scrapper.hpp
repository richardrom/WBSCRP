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

#include "allocator.hpp"
#include "pool_reporter.hpp"

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
    using wbstring = std::basic_string<char_type, std::char_traits<char_type>, pool_allocator<char_type>>;

    template<typename T>
    using wbvector = std::vector<T, pool_allocator<T>>;

    // Call before any calls to the library
    extern bool initialize();
    extern bool is_initialized();

    // HTML5 Conformant states
    enum class States
    {
        Data,
        TagOpen,
    };

    struct Token
    {
        wbstring name;
    };


} // namespace scrp


#endif // WBSCRP_SCRAPPER_HPP
