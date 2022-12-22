/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "scrapper.hpp"


namespace
{
    bool _initialize = false;
    scrp::pool_allocator<scrp::char_type> *undefined_pool = nullptr;

    void _terminate_scrp()
    {
        delete undefined_pool;
    }

} // namespace

bool scrp::initialize()
{
    if (_initialize)
        return _initialize;

    if (std::atexit(_terminate_scrp))
        return false;

    undefined_pool = new scrp::pool_allocator<scrp::char_type>;

    // This code will initialize general pools
    // Since the global_allocator will initialize pools of void * with chunks of n bytes
    // The type don't really matter what we want is

    undefined_pool->create_pool(8); // This pool will be used to all basic types
    undefined_pool->create_pool(16);
    undefined_pool->create_pool(32);
    undefined_pool->create_pool(64);
    undefined_pool->create_pool(128);

    _initialize = true;
    return _initialize;
}

bool scrp::is_initialized()
{
    return _initialize;
}
