/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 17/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#pragma once

#ifndef __cplusplus
#error "C++ compiler needed"
#endif /*__cplusplus*/

#ifndef WBSCRP_TOKENIZER_HPP
#define WBSCRP_TOKENIZER_HPP

#include "scrapper.hpp"
#include "parser_error.hpp"

namespace scrp
{
    class Tokenizer
    {
    public:
        explicit Tokenizer(wbstring source);
        ~Tokenizer();

    public:
        /// \return true if no errors were encounter; false if recoverable parsing errors were encounter. Retrieve this errors with get_parse_errors();
        /// \throws scrp::parser_error if an unrecoverable parse_error is encounter
        auto tokenize() -> bool;

        auto get_parse_errors() const noexcept -> scrp::wbvector<parser_error>;

    protected:
        auto data_state(wbstring::iterator &pos, States &stateChange) -> void;
        auto tag_open_state(wbstring::iterator &pos) -> void;

    protected:
        auto is_next_char_eof(const wbstring::iterator &pos) const -> bool;
        static auto is_char_alpha(scrp::char_type ch) noexcept -> bool;

    protected:
        auto current_position() const noexcept -> std::size_t;
        auto current_line() const noexcept -> std::size_t;
        auto current_line_offset() const noexcept -> std::size_t;

    protected:
        auto emit_token(Token token) noexcept -> void;
        auto emit_error(parser_error_type type) noexcept -> void;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
} // namespace scrp

#endif // WBSCRP_TOKENIZER_HPP
