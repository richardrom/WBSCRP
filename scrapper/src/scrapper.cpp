/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "scrapper.hpp"

scrp::fixed_memory_pool<scrp::EOFToken> *scrp::_pool_eof_token             = nullptr;
scrp::fixed_memory_pool<scrp::CommentToken> *scrp::_pool_comment_token     = nullptr;
scrp::fixed_memory_pool<scrp::DOCTYPEToken> *scrp::_pool_doctype_token     = nullptr;
scrp::fixed_memory_pool<scrp::CDATAToken> *scrp::_pool_cdata_token         = nullptr;
scrp::fixed_memory_pool<scrp::CharacterToken> *scrp::_pool_character_token = nullptr;
scrp::fixed_memory_pool<scrp::TagToken> *scrp::_pool_tag_token             = nullptr;

namespace
{
    bool _initialize                                      = false;
    scrp::pool_allocator<scrp::char_type> *undefined_pool = nullptr;

    void _terminate_scrp()
    {
        delete undefined_pool;

        delete scrp::_pool_eof_token;
        delete scrp::_pool_comment_token;
        delete scrp::_pool_doctype_token;
        delete scrp::_pool_cdata_token;
        delete scrp::_pool_character_token;
        delete scrp::_pool_tag_token;
    }


} // namespace

template <typename T>
static void initialize_token_pool(T *&t)
{
    t = new scrp::fixed_memory_pool<T>(4096, sizeof(T));
}

bool scrp::initialize()
{
    if (_initialize)
        return _initialize;

    if (std::atexit(_terminate_scrp))
        return false;

#ifndef USE_STL_ALLOCATOR
    undefined_pool = new scrp::pool_allocator<scrp::char_type>;

    // This code will initialize general pools
    // Since the global_allocator will initialize pools of void * with chunks of n bytes
    // The type don't really matter what we want is

    undefined_pool->create_pool(8); // This pool will be used to all basic types
    undefined_pool->create_pool(16);
    undefined_pool->create_pool(32);
    undefined_pool->create_pool(64);
    undefined_pool->create_pool(128);


    static const auto _p0 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(EOFToken));
    static const auto _p1 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(CommentToken));
    static const auto _p2 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(DOCTYPEToken));
    static const auto _p3 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(CDATAToken));
    static const auto _p4 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(CharacterToken));
    static const auto _p6 = scrp::pool_allocator<scrp::char_type>::global_allocator().adjust_chunk_size(sizeof(TagToken));

#else

   auto adjust_chunk_size = [](std::size_t chunkSize) noexcept -> std::size_t
    {
        std::size_t chunk_size;
        if (chunkSize < 8)
            chunk_size = 8;
        else
        {
            const auto bw = std::bit_width(chunkSize);
            chunk_size    = static_cast<std::size_t>(static_cast<int>(2 << (bw - 1)));
        }

        return chunk_size;
    };
    static const auto _p0 = adjust_chunk_size(sizeof(EOFToken));
    static const auto _p1 = adjust_chunk_size(sizeof(CommentToken));
    static const auto _p2 = adjust_chunk_size(sizeof(DOCTYPEToken));
    static const auto _p3 = adjust_chunk_size(sizeof(CDATAToken));
    static const auto _p4 = adjust_chunk_size(sizeof(CharacterToken));
    static const auto _p5 = adjust_chunk_size(sizeof(EndTagToken));
    static const auto _p6 = adjust_chunk_size(sizeof(TagToken));

#endif /*USE_STL_ALLOCATOR*/

    _pool_eof_token       = new scrp::fixed_memory_pool<EOFToken>(_p0*5,  _p0);
    _pool_comment_token   = new scrp::fixed_memory_pool<CommentToken>(_p1*1000,  _p1);
    _pool_doctype_token   = new scrp::fixed_memory_pool<DOCTYPEToken>(_p2*5,  _p2);
    _pool_cdata_token     = new scrp::fixed_memory_pool<CDATAToken>(_p3*1000,  _p3);
    _pool_character_token = new scrp::fixed_memory_pool<CharacterToken>(_p4*50000,  _p4);
    _pool_tag_token       = new scrp::fixed_memory_pool<TagToken>(_p6*30000,  _p6);

    _initialize = true;
    return _initialize;
}

bool scrp::is_initialized()
{
    return _initialize;
}
