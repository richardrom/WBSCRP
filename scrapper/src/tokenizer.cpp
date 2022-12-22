/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 17/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "tokenizer.hpp"

#include <algorithm>
#include <array>
#include <utility>

struct scrp::Tokenizer::Impl
{
    explicit Impl(scrp::wbstring src) :
        data { std::move(src) }
    {
        tokens.reserve(1000);
    }

public:
    wbvector<Token> tokens;
    wbvector<parser_error> errors;
    wbstring data;
    std::size_t current_position { 0 };
    std::size_t current_line { 1 };
    std::size_t line_offset { 0 };
};

namespace
{
    constexpr auto _breakers = std::to_array<const scrp::char_type>("<> -=/\"\t\r\n\b");
}

scrp::Tokenizer::Tokenizer(wbstring source) :
    _impl { new Impl(std::move(source)) }
{
    assert(scrp::is_initialized());
}
scrp::Tokenizer::~Tokenizer() = default;

auto scrp::Tokenizer::tokenize() -> bool
{
    if (_impl->data.empty())
        return false;

    States currentState = States::Data; // Initial state
    wbstring tokenName;
    tokenName.reserve(32);

    bool new_line_state = true;
    for (auto dataIterator = _impl->data.begin(); dataIterator != _impl->data.end(); ++dataIterator)
    {
        if (*dataIterator == '\r')
        {
            ++_impl->current_position;
            ++_impl->line_offset;
            new_line_state = true;
            continue;
        }

        if (*dataIterator == '\n')
        {
            // if new_line_state == true this is the CR/LF case
            // if new_line_state == false this is the LF case
            // in either case the parser will ignore the flag
            _impl->line_offset = 0;
            ++_impl->current_position;
            ++_impl->current_line;
            new_line_state = false;
            continue;
        }

        if (new_line_state && *dataIterator != '\n')
        {
            // This is the old CR new line case; current_position must not be updated
            _impl->line_offset = 0;
            ++_impl->current_line;
            new_line_state = false;
        }

        try
        {
            switch (currentState)
            {
                case States::Data:
                    data_state(dataIterator, currentState);
                    break;
                case States::TagOpen:
                    tag_open_state(dataIterator);
                    break;
            }
        } catch (...)
        {
            throw;
        }

        ++_impl->line_offset;
        ++_impl->current_position;
    }

    return true;
}

auto scrp::Tokenizer::is_next_char_eof(const wbstring::iterator &pos) const -> bool
{
    return _impl->data.end() == std::next(pos);
}

auto scrp::Tokenizer::is_char_alpha(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _llp = static_cast<scrp::char_type>(0x41);
    constexpr auto _lup = static_cast<scrp::char_type>(0x5A);
    constexpr auto _ulp = static_cast<scrp::char_type>(0x61);
    constexpr auto _uup = static_cast<scrp::char_type>(0x7A);

    return (ch >= _llp && ch <= _lup) || (ch >= _ulp && ch <= _uup);
}

auto scrp::Tokenizer::current_position() const noexcept -> std::size_t
{
    return _impl->current_position;
}
auto scrp::Tokenizer::current_line() const noexcept -> std::size_t
{
    return _impl->current_line;
}
auto scrp::Tokenizer::current_line_offset() const noexcept -> std::size_t
{
    return _impl->current_line;
}

auto scrp::Tokenizer::data_state(wbstring::iterator &pos, States &stateChange) -> void
{
    auto currentChar = *pos;

    switch (currentChar)
    {
        case '<':
            stateChange = States::TagOpen;
            return;
        case '&':
            break;
        case 0:
            // according to the standard this is not an unrecoverable error
            emit_error(parser_error_type::unexpected_null_character);
            break;
        default:
            break;
    }

    if (is_next_char_eof(pos))
    {

    }
}

auto scrp::Tokenizer::tag_open_state(wbstring::iterator &pos) -> void
{
    auto currentChar = *pos;

    switch (currentChar)
    {
        case '!':
            break;
        case '/':
            break;
        case '?':
            break;
    }

    if (scrp::Tokenizer::is_char_alpha(currentChar))
    {
    }

    if (is_next_char_eof(pos))
    {
    }
}

auto scrp::Tokenizer::get_parse_errors() const noexcept -> scrp::wbvector<parser_error>
{
    return _impl->errors;
}

auto scrp::Tokenizer::emit_error(parser_error_type type) noexcept -> void
{
    _impl->errors.emplace_back(type, _impl->current_position, _impl->line_offset, _impl->current_line);
}

auto scrp::Tokenizer::emit_token(Token token) noexcept -> void
{

}
