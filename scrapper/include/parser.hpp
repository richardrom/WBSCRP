/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#pragma once

#ifndef __cplusplus
#error "C++ compiler needed"
#endif /*__cplusplus*/

#ifndef WBSCRP_PARSER_HPP
#define WBSCRP_PARSER_HPP

#include "scrapper.hpp"

namespace scrp
{


    class parser
    {
    public:
        parser();
        ~parser();

    public:
        auto consume_token( Token *tok ) -> void;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}

#endif // WBSCRP_PARSER_HPP
