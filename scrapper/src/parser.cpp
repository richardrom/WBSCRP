/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 13/01/23.
// Copyright (c) 2023 Ricardo Romero.  All rights reserved.
//
#include "parser.hpp"

namespace scrp
{
    struct scrp::parser::Impl
    {
    };
} // namespace scrp

scrp::parser::parser() :
    _impl { new Impl }
{
}


scrp::parser::~parser() = default;


auto scrp::parser::consume_token(scrp::Token *tok) -> void
{

}

