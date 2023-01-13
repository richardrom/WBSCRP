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
        attributes.reserve(20);
    }

public:
    sc_stack<States> state;
    sc_vector<Token> tokens;
    sc_vector<parser_error> errors;
    sc_unordered_map<sc_string, sc_string> attributes;
    sc_string data;
    sc_string current_token_data;
    sc_string extra_token_data_0;
    sc_string extra_token_data_1;
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
                    doctype(dataIterator, currentState);
                    break;
                case States::BeforeDOCTYPEName:
                    before_doctype_name(dataIterator, currentState);
                    break;
                case States::DOCTYPEName:
                    doctype_name(dataIterator, currentState);
                    break;
                case States::AfterDOCTYPEName:
                    after_doctype_name(dataIterator, currentState);
                    break;
                case States::AfterDOCTYPEPublicKeyword:
                    after_doctype_public_keyword(dataIterator, currentState);
                    break;
                case States::BeforeDOCTYPEPublicIdentifier:
                    before_doctype_public_identifier(dataIterator, currentState);
                    break;
                case States::DOCTYPEPublicIdentifierDQ:
                    doctype_public_identifier_dq(dataIterator, currentState);
                    break;
                case States::DOCTYPEPublicIdentifierSQ:
                    doctype_public_identifier_sq(dataIterator, currentState);
                    break;
                case States::AfterDOCTYPEPublicIdentifier:
                    after_doctype_public_identifier(dataIterator, currentState);
                    break;
                case States::BetweenDOCTYPEPublicAndSystemIdentifiers:
                    between_doctype_public_and_system_identifiers(dataIterator, currentState);
                    break;
                case States::AfterDOCTYPESystemKeyword:
                    after_doctype_system_keyword(dataIterator, currentState);
                    break;
                case States::BeforeDOCTYPESystemIdentifier:
                    before_doctype_system_identifier(dataIterator, currentState);
                    break;
                case States::DOCTYPESystemIdentifierDQ:
                    doctype_system_identifier_dq(dataIterator, currentState);
                    break;
                case States::DOCTYPESystemIdentifierSQ:
                    doctype_system_identifier_sq(dataIterator, currentState);
                    break;
                case States::AfterDOCTYPESystemIdentifier:
                    after_doctype_system_identifier(dataIterator, currentState);
                    break;
                case States::BogusDOCTYPE:
                    bogus_doctype(dataIterator, currentState);
                    break;
                case States::CDATASection:
                    cdata_section(dataIterator, currentState);
                    break;
                case States::CDATASectionBracket:
                    cdata_section_bracket(dataIterator, currentState);
                    break;
                case States::CDATASectionEnd:
                    cdata_section_end(dataIterator, currentState);
                    break;
                case States::EndTagOpen:
                    end_tag_open(dataIterator, currentState);
                    break;
                case States::TagName:
                    tag_name(dataIterator, currentState);
                    break;
                case States::BeforeAttributeName:
                    before_attribute_name(dataIterator, currentState);
                    break;
                case States::SelfClosingStartTag:
                    self_closing_start_tag(dataIterator, currentState);
                    break;
                case States::AttributeName:
                    attribute_name(dataIterator, currentState);
                    break;
                case States::AfterAttributeName:
                    after_attribute_name(dataIterator, currentState);
                    break;
                case States::BeforeAttributeValue:
                    before_attribute_value(dataIterator, currentState);
                    break;
                case States::AttributeValueDQ:
                    attribute_value_dq(dataIterator, currentState);
                    break;
                case States::AttributeValueSQ:
                    attribute_value_sq(dataIterator, currentState);
                    break;
                case States::AttributeValueUnquoted:
                    attribute_value_unquoted(dataIterator, currentState);
                    break;
                case States::AfterAttributeValueQuoted:
                    after_attribute_value_quoted(dataIterator, currentState);
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
        if (state == States::AttributeValueDQ
            || state == States::AttributeValueSQ
            || state == States::AttributeValueUnquoted)
            return true;
    }
    return false;
}

auto scrp::Tokenizer::insert_attribute(const sc_string &name, const sc_string &value) -> void
{
    auto iter = _impl->attributes.find(name);
    if (iter == _impl->attributes.end())
    {
        _impl->attributes[name] = value;
    }
    else
    {
        emit_error(parser_error_type::duplicate_attribute);
        // discard the new attribute
    }
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

auto scrp::Tokenizer::emit_token(const Token &token) noexcept -> void
{
    _impl->current_token_data.clear();
    _impl->extra_token_data_0.clear();
    _impl->extra_token_data_1.clear();
    _impl->attributes.clear();
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
            emit_token(CommentToken { _impl->current_token_data });
            emit_token(EOFToken {});
            break;
        case States::DOCTYPE:
            [[fallthrough]];
        case States::BeforeDOCTYPEName:
            [[fallthrough]];
        case States::DOCTYPEName:
            [[fallthrough]];
        case States::AfterDOCTYPEName:
            [[fallthrough]];
        case States::AfterDOCTYPEPublicKeyword:
            [[fallthrough]];
        case States::BeforeDOCTYPEPublicIdentifier:
            [[fallthrough]];
        case States::AfterDOCTYPESystemKeyword:
            [[fallthrough]];
        case States::BeforeDOCTYPESystemIdentifier:
            [[fallthrough]];
        case States::BogusDOCTYPE:
            emit_error(parser_error_type::eof_in_doctype);
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
            emit_token(EOFToken {});
            break;
    }
}

auto scrp::Tokenizer::data_state(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '&':
            stateChange = States::CharacterReference;
            _impl->state.push(States::Data);
            return;
        case '<':
            stateChange = States::TagOpen;
            return;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            break;
        default:
            emit_token(CharacterToken { sc_string { *pos } });
            break;
    }

    if (is_next_char_eof(pos))
    {
        emit_token(EOFToken {});
    }
}

auto scrp::Tokenizer::tag_open_state(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '!':
            stateChange = States::MarkupDeclarationOpen;
            return;
        case '/':
            stateChange = States::EndTagOpen;
            break;
        case '?':
            emit_error(parser_error_type::unexpected_question_mark_instead_of_tag_name);
            stateChange = States::BogusComment;
            _impl->current_token_data.clear();
            --pos;
            break;
        default:
            if (scrp::Tokenizer::is_char_alpha(*pos))
            {
                stateChange = States::TagName;
                --pos;
            }
            else
            {
                emit_error(parser_error_type::invalid_first_character_of_tag_name);
                emit_token(CharacterToken { "<" });
                stateChange = States::Data;
                --pos;
            }
            break;
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_before_tag_name);
        emit_token(CharacterToken { ">" });
        emit_token(EOFToken {});
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
                    stateChange = States::CDATASection;
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
            emit_token(CommentToken { _impl->current_token_data });
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
            emit_token(CommentToken { _impl->current_token_data });
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
            emit_token(CommentToken { _impl->current_token_data });
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
        emit_token(CommentToken { _impl->current_token_data });
        emit_token(EOFToken {});
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
            emit_token(CommentToken { _impl->current_token_data });
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
            emit_token(CommentToken { _impl->current_token_data });
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
    switch (*pos)
    {
        default:
            emit_error(parser_error_type::missing_whitespace_before_doctype_name);
            [[fallthrough]];
        case '>':
            --pos;
            [[fallthrough]];
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeDOCTYPEName;
    }
}

auto scrp::Tokenizer::before_doctype_name(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // ignore
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->current_token_data += encoding::_sv_invalid;
            stateChange = States::DOCTYPEName;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_name);
            emit_token(DOCTYPEToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        default:
            auto ch = *pos;
            if (is_char_upper_alpha(ch))
                ch = to_lower(ch);
            _impl->current_token_data += ch;
            stateChange = States::DOCTYPEName;
    }
}

auto scrp::Tokenizer::doctype_name(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::AfterDOCTYPEName;
            break;
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->current_token_data += encoding::_sv_invalid;
            break;
        default:
            _impl->current_token_data += *pos;
    }
}

auto scrp::Tokenizer::after_doctype_name(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    static constexpr std::string_view _public { "public" };
    static constexpr std::string_view _system { "system" };

    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // ignore
            break;
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        default:
            _impl->extra_token_data_0 += *pos;
            if (_impl->extra_token_data_0.size() <= _public.size() && string_to_lower(_impl->extra_token_data_0) == _public.substr(0, _impl->extra_token_data_0.size()))
            {
                if (string_to_lower(_impl->extra_token_data_0) == _public)
                {
                    _impl->extra_token_data_0.clear();
                    _impl->extra_token_data_1.clear();
                    stateChange = States::AfterDOCTYPEPublicKeyword;
                }
            }
            else if (_impl->extra_token_data_0.size() <= _system.size() && string_to_lower(_impl->extra_token_data_0) == _system.substr(0, _impl->extra_token_data_0.size()))
            {
                if (string_to_lower(_impl->extra_token_data_0) == _system)
                {
                    _impl->extra_token_data_0.clear();
                    _impl->extra_token_data_1.clear();
                    stateChange = States::AfterDOCTYPESystemKeyword;
                }
            }
            else
            {
                emit_error(parser_error_type::invalid_character_sequence_after_doctype_name);
                --pos; // Reconsume
                stateChange = States::BogusDOCTYPE;
            }
            break;
    }
}

auto scrp::Tokenizer::after_doctype_public_keyword(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeDOCTYPEPublicIdentifier;
            break;
        case '\"':
            emit_error(parser_error_type::missing_whitespace_after_doctype_public_keyword);
            stateChange = States::DOCTYPEPublicIdentifierDQ;
            break;
        case '\'':
            emit_error(parser_error_type::missing_whitespace_after_doctype_public_keyword);
            stateChange = States::DOCTYPEPublicIdentifierSQ;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_public_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_public_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }
}

auto scrp::Tokenizer::before_doctype_public_identifier(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // Ignore
            break;
        case '\"':
            stateChange = States::DOCTYPEPublicIdentifierDQ;
            break;
        case '\'':
            stateChange = States::DOCTYPEPublicIdentifierSQ;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_public_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_public_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }
}

auto scrp::Tokenizer::doctype_public_identifier_dq(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '\"':
            stateChange = States::AfterDOCTYPEPublicIdentifier;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_0 += encoding::_sv_invalid;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_public_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
            stateChange = States::Data;
            break;
        default:
            _impl->extra_token_data_0 += *pos;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::doctype_public_identifier_sq(sc_string::iterator &pos, scrp::States &stateChange) -> void
{

    switch (*pos)
    {
        case '\'':
            stateChange = States::AfterDOCTYPEPublicIdentifier;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_0 += encoding::_sv_invalid;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_public_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
            stateChange = States::Data;
            break;
        default:
            _impl->extra_token_data_0 += *pos;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::after_doctype_public_identifier(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BetweenDOCTYPEPublicAndSystemIdentifiers;
            break;
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
            stateChange = States::Data;
            break;
        case '\"':
            emit_error(parser_error_type::missing_whitespace_between_doctype_public_and_system_identifiers);
            stateChange = States::DOCTYPESystemIdentifierDQ;
            break;
        case '\'':
            emit_error(parser_error_type::missing_whitespace_between_doctype_public_and_system_identifiers);
            stateChange = States::DOCTYPESystemIdentifierSQ;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::between_doctype_public_and_system_identifiers(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // Ignore
            break;
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
            stateChange = States::Data;
            break;
        case '\"':
            stateChange = States::DOCTYPESystemIdentifierDQ;
            break;
        case '\'':
            stateChange = States::DOCTYPESystemIdentifierSQ;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::after_doctype_system_keyword(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeDOCTYPESystemIdentifier;
            break;
        case '\"':
            emit_error(parser_error_type::missing_whitespace_after_doctype_system_keyword);
            stateChange = States::DOCTYPESystemIdentifierDQ;
            break;
        case '\'':
            emit_error(parser_error_type::missing_whitespace_after_doctype_system_keyword);
            stateChange = States::DOCTYPESystemIdentifierSQ;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_system_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }
}

auto scrp::Tokenizer::before_doctype_system_identifier(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // Ignore
            break;
        case '\"':
            stateChange = States::DOCTYPESystemIdentifierDQ;
            break;
        case '\'':
            stateChange = States::DOCTYPESystemIdentifierSQ;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_system_identifier);
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_quote_before_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }
}

auto scrp::Tokenizer::doctype_system_identifier_dq(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '\"':
            stateChange = States::AfterDOCTYPESystemIdentifier;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_1 += encoding::_sv_invalid;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_system_identifier);
            stateChange = States::Data;
            break;
        default:
            _impl->extra_token_data_1 += *pos;
    }
    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::doctype_system_identifier_sq(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '\'':
            stateChange = States::AfterDOCTYPESystemIdentifier;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_1 += encoding::_sv_invalid;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_system_identifier);
            stateChange = States::Data;
            break;
        default:
            _impl->extra_token_data_1 += *pos;
    }
    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::after_doctype_system_identifier(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // Ignore
            break;
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::unexpected_character_after_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
        emit_token(EOFToken {});
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::bogus_doctype(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_token(DOCTYPEToken { _impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1 });
            stateChange = States::Data;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            break;
        default:;
    }
}

auto scrp::Tokenizer::cdata_section(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case ']':
            stateChange = States::CDATASectionBracket;
            break;
        default:
            _impl->current_token_data += *pos;
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_cdata);
        emit_token(CDATAToken { _impl->current_token_data });
        return;
    }
}

auto scrp::Tokenizer::cdata_section_bracket(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case ']':
            stateChange = States::CDATASectionEnd;
            break;
        default:
            _impl->current_token_data += ']';
            stateChange = States::CDATASection;
            --pos;
    }
}

auto scrp::Tokenizer::cdata_section_end(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case ']':
            _impl->current_token_data += ']';
            break;
        case '>':
            emit_token(CDATAToken { _impl->current_token_data });
            stateChange = States::Data;
            break;
        default:
            _impl->current_token_data += "]]";
            stateChange = States::CDATASection;
            --pos;
    }
}

auto scrp::Tokenizer::end_tag_open(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_error(parser_error_type::missing_end_tag_name);
            stateChange = States::Data;
            break;
        default:
            if (is_char_alpha(*pos))
            {
                emit_token(EndTagToken {});
                stateChange = States::TagName;
                --pos;
            }
            else
            {
                emit_error(parser_error_type::invalid_first_character_of_tag_name);
                stateChange = States::BogusComment;
                --pos;
            }
    }

    if (is_next_char_eof(pos))
    {
        emit_token(CharacterToken { "<" });
        emit_token(CharacterToken("/"));
        emit_token(EOFToken {});
    }
}

auto scrp::Tokenizer::tag_name(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeAttributeName;
            break;
        case '/':
            stateChange = States::SelfClosingStartTag;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->current_token_data += encoding::_sv_invalid;
        default:
            {
                const auto ch = to_lower(*pos);
                _impl->current_token_data += ch;
            }
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken {});
    }
}

auto scrp::Tokenizer::before_attribute_name(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // ignore
            break;
        case '/':
            [[fallthrough]];
        case '>':
            --pos;
            stateChange = States::AfterAttributeName;
            break;
        case '=':
            emit_error(parser_error_type::unexpected_equals_sign_before_attribute_name);
            _impl->extra_token_data_0 = *pos;
            stateChange               = States::AttributeName;
            break;
        default:
            _impl->extra_token_data_0.clear();
            stateChange = States::AttributeName;
    }

    if (is_next_char_eof(pos))
    {
        --pos;
        stateChange = States::AfterAttributeName;
    }
}

auto scrp::Tokenizer::attribute_name(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            [[fallthrough]];
        case '/':
            [[fallthrough]];
        case '>':
            --pos;
            stateChange = States::AfterAttributeName;
            break;
        case '=':
            stateChange = States::BeforeAttributeValue;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_0 += encoding::_sv_invalid;
            break;
        case '\"':
            [[fallthrough]];
        case '\'':
            [[fallthrough]];
        case '<':
            emit_error(parser_error_type::unexpected_character_in_attribute_name);
            [[fallthrough]];
        default:
            const auto ch = to_lower(*pos);
            _impl->extra_token_data_0 += ch;
    }

    if (is_next_char_eof(pos))
    {
        --pos;
        stateChange = States::AfterAttributeName;
    }
}

auto scrp::Tokenizer::after_attribute_name(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // ignore
            break;
        case '/':
            stateChange = States::SelfClosingStartTag;
            break;
        case '=':
            stateChange = States::BeforeAttributeValue;
            break;
        case '>':
            emit_token(TagToken { _impl->current_token_data, _impl->attributes });
            stateChange = States::Data;
            break;
        default:
            insert_attribute(_impl->extra_token_data_0, _impl->extra_token_data_1);
            _impl->extra_token_data_0.clear();
            _impl->extra_token_data_1.clear();
            stateChange = States::AttributeName;
            --pos; // Reconsume
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken {});
    }
}

auto scrp::Tokenizer::before_attribute_value(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            // ignore
            break;
        case '\"':
            stateChange = States::AttributeValueDQ;
            break;
        case '\'':
            stateChange = States::AttributeValueSQ;
            break;
        case '>':
            emit_error(parser_error_type::missing_attribute_value);
            emit_token(TagToken { _impl->current_token_data, _impl->attributes });
            stateChange = States::Data;
            break;
        default:
            stateChange = States::AttributeValueUnquoted;
            --pos;
    }
}

auto scrp::Tokenizer::attribute_value_dq(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '\"':
            stateChange = States::AfterAttributeValueQuoted;
            break;
        case '&':
            _impl->state.push(States::AttributeValueDQ);
            stateChange = States::CharacterReference;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_1 += encoding::_sv_invalid;
            break;
        default:
            _impl->extra_token_data_1 += *pos;
    }

    if(is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken{});
    }
}

auto scrp::Tokenizer::attribute_value_sq(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '\'':
            stateChange = States::AfterAttributeValueQuoted;
            break;
        case '&':
            _impl->state.push(States::AttributeValueSQ);
            stateChange = States::CharacterReference;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_1 += encoding::_sv_invalid;
            break;
        default:
            _impl->extra_token_data_1 += *pos;
    }

    if(is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken{});
    }
}

auto scrp::Tokenizer::attribute_value_unquoted(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeAttributeName;
            break;
        case '&':
            _impl->state.push(States::AttributeValueUnquoted);
            stateChange = States::CharacterReference;
            break;
        case '>':
            insert_attribute(_impl->extra_token_data_0, _impl->extra_token_data_1);
            _impl->extra_token_data_0.clear();
            _impl->extra_token_data_1.clear();
            emit_token(TagToken { _impl->current_token_data, _impl->attributes });
            stateChange = States::Data;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            _impl->extra_token_data_1 += encoding::_sv_invalid;
            break;
        case '\"':
            [[fallthrough]];
        case '\'':
            [[fallthrough]];
        case '<':
            [[fallthrough]];
        case '=':
            [[fallthrough]];
        case '`':
            emit_error(parser_error_type::unexpected_character_in_unquoted_attribute_value);
            [[fallthrough]];
        default:
            _impl->extra_token_data_1 += *pos;
    }

    if(is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken{});
    }
}

auto scrp::Tokenizer::after_attribute_value_quoted(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 0x09:
            [[fallthrough]];
        case 0x0A:
            [[fallthrough]];
        case 0x0C:
            [[fallthrough]];
        case 0x20:
            stateChange = States::BeforeAttributeName;
            break;
        case '/':
            stateChange = States::SelfClosingStartTag;
            break;
        case '>':
            insert_attribute(_impl->extra_token_data_0, _impl->extra_token_data_1);
            _impl->extra_token_data_0.clear();
            _impl->extra_token_data_1.clear();
            emit_token(TagToken { _impl->current_token_data, _impl->attributes });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_whitespace_between_attributes);
            stateChange = States::BeforeAttributeName;
            --pos;

    }

    if(is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken{});
    }
}

auto scrp::Tokenizer::self_closing_start_tag(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_token(TagToken { _impl->current_token_data, _impl->attributes, true });
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::unexpected_solidus_in_tag);
            stateChange = States::BeforeAttributeName;
            --pos;
    }

    if(is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_token(EOFToken{});
    }
}
