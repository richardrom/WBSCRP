/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 21/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "parser_error.hpp"

scrp::parser_error::parser_error(scrp::parser_error_type type, std::size_t position, std::size_t offset, std::size_t line)
{
}

scrp::parser_error::~parser_error() = default;

const char *scrp::parser_error::what() const noexcept
{
    return nullptr;
}

auto scrp::parser_error::type() const noexcept -> scrp::parser_error_type
{
    scrp::parser_error_type result;
    return result;
}

auto scrp::parser_error::pos() const noexcept -> std::size_t
{
    return 0;
}

auto scrp::parser_error::line_offset() const noexcept -> std::size_t
{
    return 0;
}
auto scrp::parser_error::line() const noexcept -> std::size_t
{
    return 0;
}
