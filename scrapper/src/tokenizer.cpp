/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 17/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "tokenizer.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "encoding_character_reference.hpp"

struct scrp::Tokenizer::Impl
{
    explicit Impl(scrp::sc_string src) :
        data { std::move(src) }
    {
        tokens.reserve(1000);
    }

public:
    sc_stack<States> state;
    sc_vector<Token> tokens;
    sc_vector<parser_error> errors;
    sc_string data;
    sc_string current_token_data;
    sc_string ambiguous_character_reference;
    std::size_t current_position { 0 };
    std::size_t current_line { 1 };
    std::size_t line_offset { 0 };
};

scrp::Tokenizer::Tokenizer(sc_string source) :
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
    sc_string tokenName;
    tokenName.reserve(32);

    bool new_line_state = false;
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
                case States::CharacterReference:
                    character_reference(dataIterator, currentState);
                    break;
                case States::TagOpen:
                    tag_open_state(dataIterator, currentState);
                    break;
                case States::AmbiguousAmpersand:
                    ambiguous_ampersand(dataIterator, currentState);
                    break;
                case States::MarkupDeclarationOpen:
                    markup_declaration_open(dataIterator, currentState);
                    break;
                case States::CommentStart:
                    comment_start(dataIterator, currentState);
                    break;
                case States::BogusComment:
                    bogus_comment(dataIterator, currentState);
                    break;
                case States::CommentStartDash:
                    comment_start_dash(dataIterator, currentState);
                    break;
                case States::Comment:
                    comment(dataIterator, currentState);
                    break;
                case States::CommentEnd:
                    comment_end(dataIterator, currentState);
                    break;
                case States::CommentLessThanSign:
                    comment_less_than_sign(dataIterator, currentState);
                    break;
                case States::CommentLessThanSignBang:
                    comment_less_than_sign_bang(dataIterator, currentState);
                    break;
                case States::CommentLessThanSignBangDash:
                    comment_less_than_sign_bang_dash(dataIterator, currentState);
                    break;
                case States::CommentLessThanSignBangDashDash:
                    comment_less_than_sign_bang_dash_dash(dataIterator, currentState);
                    break;
                case States::CommentEndDash:
                    comment_end_dash(dataIterator, currentState);
                    break;
                case States::CommentEndBang:
                    comment_end_bang(dataIterator, currentState);
                    break;
                case States::DOCTYPE:
                    break;
                case States::CDATA:
                    break;
            }

            if (dataIterator == _impl->data.end())
                break;

            ++_impl->line_offset;
            ++_impl->current_position;

        } catch (...)
        {
            throw;
        }
    }

    handle_eof_error(currentState);

    return true;
}

auto scrp::Tokenizer::is_return_state_attribute() -> bool
{
    if (_impl->state.empty())
        return false;
    else
    {
        const auto &state = _impl->state.top();
        if (state == States::AttributeValueDoubleQuoted
            || state == States::AttributeValueSingleQuoted
            || state == States::AttributeValueUnquoted)
            return true;
    }
    return false;
}

auto scrp::Tokenizer::is_next_char_eof(const sc_string::iterator &pos) const -> bool
{
    return _impl->data.end() == std::next(pos);
}

auto scrp::Tokenizer::is_char_lower_alpha(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _llp = static_cast<scrp::char_type>(0x61);
    constexpr auto _lup = static_cast<scrp::char_type>(0x7A);

    return (ch >= _llp && ch <= _lup);
}

auto scrp::Tokenizer::is_char_upper_alpha(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _ulp = static_cast<scrp::char_type>(0x41);
    constexpr auto _uup = static_cast<scrp::char_type>(0x5A);

    return (ch >= _ulp && ch <= _uup);
}

auto scrp::Tokenizer::is_char_alpha(scrp::char_type ch) noexcept -> bool
{
    return scrp::Tokenizer::is_char_lower_alpha(ch) || scrp::Tokenizer::is_char_upper_alpha(ch);
}

auto scrp::Tokenizer::is_char_digit(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _dlp = static_cast<scrp::char_type>(0x30);
    constexpr auto _dup = static_cast<scrp::char_type>(0x39);

    return (ch >= _dlp && ch <= _dup);
}

auto scrp::Tokenizer::is_char_alphanumeric(scrp::char_type ch) noexcept -> bool
{
    return scrp::Tokenizer::is_char_alpha(ch) || scrp::Tokenizer::is_char_digit(ch);
}

auto scrp::Tokenizer::is_char_lower_hex_digit(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _hlp = static_cast<scrp::char_type>(0x61);
    constexpr auto _hup = static_cast<scrp::char_type>(0x66);

    return (ch >= _hlp && ch <= _hup);
}

auto scrp::Tokenizer::is_char_upper_hex_digit(scrp::char_type ch) noexcept -> bool
{
    constexpr auto _hlp = static_cast<scrp::char_type>(0x41);
    constexpr auto _hup = static_cast<scrp::char_type>(0x46);

    return (ch >= _hlp && ch <= _hup);
}

auto scrp::Tokenizer::is_char_hex_digit(scrp::char_type ch) noexcept -> bool
{
    return scrp::Tokenizer::is_char_lower_hex_digit(ch) || scrp::Tokenizer::is_char_upper_hex_digit(ch);
}

auto scrp::Tokenizer::is_noncharacter(int64_t codepoint) noexcept -> bool
{
    /*A noncharacter is a code point that is in the range U+FDD0 to U+FDEF, inclusive, or */
    if (codepoint >= 0xFDD0 && codepoint <= 0xFDEF)
        return true;
    switch (codepoint)
    {
        case 0xFFFE:
        case 0xFFFF:
        case 0x1FFFE:
        case 0x1FFFF:
        case 0x2FFFE:
        case 0x2FFFF:
        case 0x3FFFE:
        case 0x3FFFF:
        case 0x4FFFE:
        case 0x4FFFF:
        case 0x5FFFE:
        case 0x5FFFF:
        case 0x6FFFE:
        case 0x6FFFF:
        case 0x7FFFE:
        case 0x7FFFF:
        case 0x8FFFE:
        case 0x8FFFF:
        case 0x9FFFE:
        case 0x9FFFF:
        case 0xAFFFE:
        case 0xAFFFF:
        case 0xBFFFE:
        case 0xBFFFF:
        case 0xCFFFE:
        case 0xCFFFF:
        case 0xDFFFE:
        case 0xDFFFF:
        case 0xEFFFE:
        case 0xEFFFF:
        case 0xFFFFE:
        case 0xFFFFF:
        case 0x10FFFE:
        case 0x10FFFF:
            return true;
        default:
            return false;
    }
}

auto scrp::Tokenizer::is_control_character(int64_t codepoint) noexcept -> bool
{
    if ((codepoint >= 0x0 && codepoint <= 0x1F)
        || (codepoint >= 0x7F && codepoint <= 0x9F))
        return true;
    return false;
}

auto scrp::Tokenizer::to_lower(scrp::char_type ch) noexcept -> scrp::char_type
{
    constexpr auto _hlp = static_cast<scrp::char_type>(0x41);
    constexpr auto _llp = static_cast<scrp::char_type>(0x61);

    if (scrp::Tokenizer::is_char_upper_alpha(ch))
        return static_cast<scrp::char_type>(static_cast<int>(ch) - _hlp + _llp);
    return ch;
}

auto scrp::Tokenizer::string_to_lower(sc_string &str) noexcept -> sc_string
{
    sc_string lower;
    lower.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(lower), scrp::Tokenizer::to_lower);
    return lower;
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
auto scrp::Tokenizer::get_parse_errors() const noexcept -> scrp::sc_vector<parser_error>
{
    return _impl->errors;
}

auto scrp::Tokenizer::emit_error(parser_error_type type) noexcept -> void
{
    _impl->errors.emplace_back(type, _impl->current_position, _impl->line_offset, _impl->current_line);
}

auto scrp::Tokenizer::emit_token(Token token) noexcept -> void
{
    _impl->current_token_data.clear();
}

auto scrp::Tokenizer::handle_eof_error(scrp::States stateChange) -> void
{
    switch (stateChange)
    {
        case States::CommentStartDash:
            [[fallthrough]];
        case States::Comment:
            [[fallthrough]];
        case States::BogusComment:
            [[fallthrough]];
        case States::CommentEnd:
            [[fallthrough]];
        case States::CommentEndDash:
            emit_error(parser_error_type::eof_in_comment);
            emit_token({ _impl->current_token_data, TokenType::Comment });
            emit_token({ "", TokenType::EndOfFile });
            break;
    }
}

auto scrp::Tokenizer::data_state(sc_string::iterator &pos, States &stateChange) -> void
{
    auto currentChar = *pos;

    switch (currentChar)
    {
        case '<':
            stateChange = States::TagOpen;
            return;
        case '&':
            stateChange = States::CharacterReference;
            _impl->state.push(States::Data);
            return;
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

auto scrp::Tokenizer::tag_open_state(sc_string::iterator &pos, States &stateChange) -> void
{
    auto currentChar = *pos;

    switch (currentChar)
    {
        case '!':
            stateChange = States::MarkupDeclarationOpen;
            return;
        case '/':
            stateChange = States::EndTagOpen;
            break;
        case '?':
            emit_error(parser_error_type::unexpected_question_mark_instead_of_tag_name);
            // TODO: Create a comment token whose data is the empty string. Reconsume in the bogus comment state.
            break;
        default:
            if (scrp::Tokenizer::is_char_alpha(currentChar))
            {
            }
            break;
    }

    if (is_next_char_eof(pos))
    {
    }
}

auto scrp::Tokenizer::character_reference(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    sc_string named_reference;
    named_reference.reserve(32);
    auto last_reference = encoding::null_chref;
    _impl->ambiguous_character_reference.clear();
    int64_t numeric_reference = 0;
    for (; pos != _impl->data.end(); ++pos)
    {
        const auto &ch = *pos;

        if (stateChange == States::CharacterReference)
        {
            if (is_char_alphanumeric(ch))
            {
                named_reference += ch;
                stateChange = States::NamedCharacterReference;
            }
            else if (ch == '#')
            {
                stateChange = States::NumericCharacterReference;
            }
            else
            {
                // TODO: Flush code points consumed as a character reference. Reconsume in the return state.
            }
        }
        else if (stateChange == States::NamedCharacterReference)
        {
            if (ch == ';')
            {
                named_reference += ch;
                const auto &current_ref = encoding::find_reference(named_reference);
                if (current_ref == encoding::null_chref)
                {
                    stateChange                          = States::AmbiguousAmpersand;
                    _impl->ambiguous_character_reference = named_reference;
                    return;
                }
                else
                {
                    // TODO: Emit code point
                    int i = 0;
                }
            }
            else
            {
                named_reference += ch;

                if (!is_return_state_attribute())
                {
                    const auto &current_ref = encoding::find_reference(named_reference);
                    if (current_ref != encoding::null_chref)
                    {
                        last_reference = current_ref;
                    }
                    else if (current_ref == encoding::null_chref && last_reference != encoding::null_chref)
                    {
                        emit_error(parser_error_type::missing_semicolon_after_character_reference);
                        // TODO: Emit code point

                        // Switch to the return state
                        stateChange = _impl->state.top();
                        _impl->state.pop();
                        return;
                    }
                }
                else
                {
                    // TODO: Handle attribute return state
                }
            }
        }
        else if (stateChange == States::NumericCharacterReference)
        {
            if (ch == 'x' || ch == 'X')
            {
                stateChange = States::HexadecimalCharacterReferenceStart;
            }
            else
            {
                stateChange = States::DecimalCharacterReferenceStart;
                --pos; // Reconsume
            }
        }
        else if (stateChange == States::HexadecimalCharacterReferenceStart)
        {
            if (is_char_hex_digit(ch))
            {
                stateChange = States::HexadecimalCharacterReference;
                --pos; // Reconsume
            }
            else
            {
                emit_error(parser_error_type::absence_of_digits_in_numeric_character_reference);

                stateChange = _impl->state.top();
                _impl->state.pop();

                --pos; // Reconsume in the return state
            }
        }
        else if (stateChange == States::DecimalCharacterReferenceStart)
        {
            if (is_char_digit(ch))
            {
                stateChange = States::DecimalCharacterReference;
                --pos; // Reconsume
            }
            else
            {
                emit_error(parser_error_type::absence_of_digits_in_numeric_character_reference);

                stateChange = _impl->state.top();
                _impl->state.pop();

                --pos; // Reconsume in the return state
            }
        }
        else if (stateChange == States::HexadecimalCharacterReference)
        {
            if (is_char_digit(ch))
            {
                numeric_reference *= 16;
                numeric_reference += static_cast<int64_t>(ch) - 0x30ll;
            }
            else if (is_char_upper_hex_digit(ch))
            {
                numeric_reference *= 16;
                numeric_reference += static_cast<int64_t>(ch) - 0x37ll;
            }
            else if (is_char_lower_hex_digit(ch))
            {
                numeric_reference *= 16;
                numeric_reference += static_cast<int64_t>(ch) - 0x57ll;
            }
            else if (ch == ';')
            {
                stateChange = States::NumericCharacterReferenceEnd;
            }
            else
            {
                emit_error(parser_error_type::missing_semicolon_after_character_reference);
                stateChange = States::NumericCharacterReferenceEnd;
                --pos;
            }
        }
        else if (stateChange == States::DecimalCharacterReference)
        {
            if (is_char_digit(ch))
            {
                numeric_reference *= 10;
                numeric_reference += static_cast<int64_t>(ch) - 0x30ll;
            }
            else if (ch == ';')
            {
                stateChange = States::NumericCharacterReferenceEnd;
                return;
            }
            else
            {
                emit_error(parser_error_type::missing_semicolon_after_character_reference);
                stateChange = States::NumericCharacterReferenceEnd;
                --pos;
                return;
            }
        }

        if (stateChange == States::NumericCharacterReferenceEnd)
        {
            if (numeric_reference == 0)
            {
                emit_error(parser_error_type::null_character_reference);
                numeric_reference = 0xFFFD;
            }
            else if (numeric_reference > 0x10FFFF)
            {
                emit_error(parser_error_type::character_reference_outside_unicode_range);
                numeric_reference = 0xFFFD;
            }
            else if (numeric_reference >= 0xD800 && numeric_reference <= 0xDFFF)
            {
                emit_error(parser_error_type::surrogate_character_reference);
                numeric_reference = 0xFFFD;
            }
            else if (is_noncharacter(numeric_reference))
            {
                emit_error(parser_error_type::noncharacter_character_reference);
                numeric_reference = -1; // non-compliant: use to avoid emitting the character
            }
            else if (numeric_reference == 0x0D || is_control_character(numeric_reference))
            {
                emit_error(parser_error_type::control_character_reference);
                switch (numeric_reference)
                {
                    case 0x80: numeric_reference = 0x20AC; break;
                    case 0x82: numeric_reference = 0x201A; break;
                    case 0x83: numeric_reference = 0x0192; break;
                    case 0x84: numeric_reference = 0x201E; break;
                    case 0x85: numeric_reference = 0x2026; break;
                    case 0x86: numeric_reference = 0x2020; break;
                    case 0x87: numeric_reference = 0x2021; break;
                    case 0x88: numeric_reference = 0x02C6; break;
                    case 0x89: numeric_reference = 0x2030; break;
                    case 0x8A: numeric_reference = 0x0160; break;
                    case 0x8B: numeric_reference = 0x2039; break;
                    case 0x8C: numeric_reference = 0x0152; break;
                    case 0x8E: numeric_reference = 0x017D; break;
                    case 0x91: numeric_reference = 0x2018; break;
                    case 0x92: numeric_reference = 0x2019; break;
                    case 0x93: numeric_reference = 0x201C; break;
                    case 0x94: numeric_reference = 0x201D; break;
                    case 0x95: numeric_reference = 0x2022; break;
                    case 0x96: numeric_reference = 0x2013; break;
                    case 0x97: numeric_reference = 0x2014; break;
                    case 0x98: numeric_reference = 0x02DC; break;
                    case 0x99: numeric_reference = 0x2122; break;
                    case 0x9A: numeric_reference = 0x0161; break;
                    case 0x9B: numeric_reference = 0x203A; break;
                    case 0x9C: numeric_reference = 0x0153; break;
                    case 0x9E: numeric_reference = 0x017E; break;
                    case 0x9F: numeric_reference = 0x0178; break;
                    default:;
                }
            }

            // TODO: emit the numeric reference

            // Return to the return state
            stateChange = _impl->state.top();
            _impl->state.pop();
        }
    }

    pos = _impl->data.end();
}

auto scrp::Tokenizer::ambiguous_ampersand(sc_string::iterator &pos, States &stateChange) -> void
{
    if (!is_return_state_attribute())
    {
        if (!_impl->ambiguous_character_reference.empty())
        {
            emit_error(parser_error_type::unknown_named_character_reference);
        }
        /*
        else
        {
            NOT AN ERROR
        }
        */
    }
    else
    {
        // TODO: Handle attribute value
    }

    stateChange = _impl->state.top();
    _impl->state.pop();
}

auto scrp::Tokenizer::markup_declaration_open(sc_string::iterator &pos, States &stateChange) -> void
{
    sc_string markup_name;
    markup_name.reserve(32);
    bool test_for_comment = false;

    static constexpr std::string_view _doctype { "doctype" };
    static constexpr std::string_view _cdata { "[CDATA[" };

    for (; pos != _impl->data.end(); ++pos)
    {
        const auto &ch = *pos;
        if (ch == '-' && !test_for_comment)
            test_for_comment = true;
        else if (ch == '-' && test_for_comment)
        {
            stateChange = States::CommentStart;
            return;
        }
        else
        {
            markup_name += ch;

            auto lower = string_to_lower(markup_name);
            auto ads   = _cdata.substr(0, markup_name.size());
            if (markup_name.size() <= _doctype.size() && string_to_lower(markup_name) == _doctype.substr(0, markup_name.size()))
            {
                if (string_to_lower(markup_name) == _doctype)
                {
                    stateChange = States::DOCTYPE;
                    return;
                }
            }
            else if (markup_name.size() <= _cdata.size() && markup_name == _cdata.substr(0, markup_name.size()))
            {
                if (markup_name == _cdata)
                {
                    // TODO: CHECK CDATA BOGUS COMMENT
                    stateChange = States::CDATA;
                    return;
                }
            }
            else
            {
                emit_error(parser_error_type::incorrectly_opened_comment);
                stateChange = States::BogusComment;
            }
        }
    }
}

auto scrp::Tokenizer::bogus_comment(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_token({ _impl->current_token_data, TokenType::Comment });
            stateChange = States::Data;
            break;
        case '0':
            emit_error(parser_error_type::unexpected_null_character);
            _impl->current_token_data += encoding::_sv_invalid;
            break;
        default:
            _impl->current_token_data += *pos;
    }
}

auto scrp::Tokenizer::comment_start(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '-':
            stateChange = States::CommentStartDash;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_closing_of_empty_comment);
            emit_token({ _impl->current_token_data, TokenType::Comment });
            stateChange = States::Data;
            break;
        default:
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_start_dash(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '-':
            stateChange = States::CommentEnd;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_closing_of_empty_comment);
            emit_token({ _impl->current_token_data, TokenType::Comment });
            stateChange = States::Data;
            break;
        default:
            --pos;
            _impl->current_token_data += "-";
            stateChange = States::Comment;
            break;
    }
}

auto scrp::Tokenizer::comment(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '<':
            _impl->current_token_data += *pos;
            stateChange = States::CommentLessThanSign;
            break;
        case '-':
            stateChange = States::CommentEndDash;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->current_token_data += encoding::_sv_invalid;
            break;
        default:
            _impl->current_token_data += *pos;
    }
}

auto scrp::Tokenizer::comment_less_than_sign(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '!':
            _impl->current_token_data += *pos;
            stateChange = States::CommentLessThanSignBang;
            break;
        case '<':
            _impl->current_token_data += *pos;
            break;
        default:
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_less_than_sign_bang(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '-':
            stateChange = States::CommentLessThanSignBangDash;
            break;
        default:
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_less_than_sign_bang_dash(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '-':
            stateChange = States::CommentLessThanSignBangDashDash;
            break;
        default:
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_less_than_sign_bang_dash_dash(sc_string::iterator &pos, States &stateChange) -> void
{
    if (is_next_char_eof(pos))
    {
        comment_end(pos, stateChange);
        return;
    }

    switch (*pos)
    {
        case '>':
            --pos;
            stateChange = States::CommentEnd;
            break;
        default:
            emit_error(parser_error_type::nested_comment);
            --pos;
            stateChange = States::CommentEnd;
    }
}

auto scrp::Tokenizer::comment_end_dash(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_comment);
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        emit_token({ _impl->current_token_data, TokenType::Comment });
        emit_token({ "", TokenType::EndOfFile });
    }

    switch (*pos)
    {
        case '-':
            stateChange = States::CommentEnd;
            break;
        default:
            _impl->current_token_data += *pos;
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_end(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_token({ _impl->current_token_data, TokenType::Comment });
            stateChange = States::Data;
            break;
        case '!':
            stateChange = States::CommentEndBang;
            break;
        case '-':
            _impl->current_token_data += *pos;
            break;
        default:
            --pos;
            _impl->current_token_data += "--";
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_end_bang(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '-':
            _impl->current_token_data += "--!";
            stateChange = States::CommentEndDash;
            break;
        case '>':
            emit_error(parser_error_type::incorrectly_closed_comment);
            emit_token({ _impl->current_token_data, TokenType::Comment });
            stateChange = States::Data;
            break;
        default:
            _impl->current_token_data += "--!";
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::doctype(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
}
