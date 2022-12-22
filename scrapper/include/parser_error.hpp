/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 21/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#pragma once

#ifndef __cplusplus
#error "C++ compiler needed"
#endif /*__cplusplus*/

#ifndef WBSCRP_PARSER_ERROR_HPP
#define WBSCRP_PARSER_ERROR_HPP

#include "scrapper.hpp"

namespace scrp
{
    enum class parser_error_type
    {
        unexpected_null_character,
        eof_before_tag_name,

    };

    struct parser_error final
    {
        parser_error( parser_error_type type, std::size_t position, std::size_t offset, std::size_t line);
        ~parser_error();


        const char* what() const noexcept;
        auto type() const noexcept -> parser_error_type;
        auto pos() const noexcept -> std::size_t;
        auto line_offset() const noexcept -> std::size_t;
        auto line() const noexcept -> std::size_t;
    };
}

#endif // WBSCRP_PARSER_ERROR_HPP
