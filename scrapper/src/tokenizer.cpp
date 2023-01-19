/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 17/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "tokenizer.hpp"

#include <algorithm>
#include <utility>

#include "encoding_character_reference.hpp"
#include "parser.hpp"

namespace scrp
{
    // HTML5 Conformant states
    enum class States
    {
        Data,
        CharacterReference,
        NamedCharacterReference,
        NumericCharacterReference,
        HexadecimalCharacterReferenceStart,
        DecimalCharacterReferenceStart,
        HexadecimalCharacterReference,
        DecimalCharacterReference,
        NumericCharacterReferenceEnd,
        AmbiguousAmpersand,
        TagOpen,
        MarkupDeclarationOpen,
        CommentStart,
        CommentEnd,
        BogusComment,
        CommentStartDash,
        CommentLessThanSign,
        CommentLessThanSignBang,
        CommentLessThanSignBangDash,
        CommentLessThanSignBangDashDash,
        CommentEndDash,
        CommentEndBang,
        Comment,
        DOCTYPE,
        BeforeDOCTYPEName,
        DOCTYPEName,
        AfterDOCTYPEName,
        AfterDOCTYPEPublicKeyword,
        BeforeDOCTYPEPublicIdentifier,
        DOCTYPEPublicIdentifierDQ,
        DOCTYPEPublicIdentifierSQ,
        AfterDOCTYPEPublicIdentifier,
        BetweenDOCTYPEPublicAndSystemIdentifiers,
        AfterDOCTYPESystemKeyword,
        BeforeDOCTYPESystemIdentifier,
        DOCTYPESystemIdentifierDQ,
        DOCTYPESystemIdentifierSQ,
        AfterDOCTYPESystemIdentifier,
        BogusDOCTYPE,
        CDATASection,
        CDATASectionBracket,
        CDATASectionEnd,
        EndTagOpen,
        TagName,
        BeforeAttributeName,
        SelfClosingStartTag,
        AttributeName,
        AfterAttributeName,
        BeforeAttributeValue,
        AttributeValueDQ,
        AttributeValueSQ,
        AttributeValueUnquoted,
        AfterAttributeValueQuoted
    };

    struct Tokenizer::Impl
    {
        explicit Impl(scrp::sc_string src) :
            data { std::move(src) }
        {
            tokens.reserve(5000);
            errors.reserve(100);
            attributes.reserve(20);

            current_token_data.reserve(64);
            named_reference.reserve(64);
            extra_token_data_0.reserve(64);
            extra_token_data_1.reserve(64);
            ambiguous_character_reference.reserve(64);
        }

    public:
        sc_stack<States> state;
        sc_vector<parser_error> errors;
        sc_vector<Token *> tokens;
        sc_unordered_map<sc_string, sc_string> attributes;
        sc_string data;
        sc_string current_token_data;
        sc_string named_reference;
        sc_string extra_token_data_0;
        sc_string extra_token_data_1;
        sc_string ambiguous_character_reference;
        encoding::character_reference last_char_reference;
        std::size_t current_position { 0 };
        std::size_t current_line { 1 };
        std::size_t line_offset { 0 };
        parser *parser { nullptr };
        uint32_t numeric_reference { 0 };
        bool keep_tokens { false };
        bool quirk_flag { true }; // only used in the bogus_doctype function and is set to false
                                  // when After DOCTYPE system identifier state triggers the Bogus DOCTYPE state
    };
} // namespace scrp

scrp::Tokenizer::Tokenizer(sc_string source) :
    _impl { new Impl(std::move(source)) }
{
    assert(scrp::is_initialized());
}
scrp::Tokenizer::~Tokenizer()
{
    for (auto &_tok : _impl->tokens)
    {
        release_token(_tok);
    }
}

auto scrp::Tokenizer::release_token(Token *&tok) -> void
{
    switch (tok->type)
    {

        case TokenType::Comment:
            {
                auto *t = static_cast<CommentToken *>(tok);
                get_token_pool<CommentToken>()->release(t);
            }
            break;
        case TokenType::EndOfFile:
            {
                auto *t = static_cast<EOFToken *>(tok);
                get_token_pool<EOFToken>()->release(t);
            }
            break;
        case TokenType::DOCTYPE:
            {
                auto *t = static_cast<DOCTYPEToken *>(tok);
                get_token_pool<DOCTYPEToken>()->release(t);
            }
            break;
        case TokenType::CDATA:
            {
                auto *t = static_cast<CDATAToken *>(tok);
                get_token_pool<CDATAToken>()->release(t);
            }
            break;
        case TokenType::Character:
            {
                auto *t = static_cast<CharacterToken *>(tok);
                get_token_pool<CharacterToken>()->release(t);
            }
            break;
        case TokenType::EndTag:
            {
                auto *t = static_cast<EndTagToken *>(tok);
                get_token_pool<EndTagToken>()->release(t);
            }
            break;
        case TokenType::Tag:
            {
                auto *t = static_cast<TagToken *>(tok);
                get_token_pool<TagToken>()->release(t);
            }
            break;
    }
}

auto scrp::Tokenizer::set_parser(parser *parser) -> void
{
    _impl->parser = parser;
}

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
                case States::NamedCharacterReference:
                    named_character_reference(dataIterator, currentState);
                    break;
                case States::NumericCharacterReference:
                    numeric_character_reference(dataIterator, currentState);
                    break;
                case States::HexadecimalCharacterReferenceStart:
                    hexadecimal_character_reference_start(dataIterator, currentState);
                    break;
                case States::DecimalCharacterReferenceStart:
                    decimal_character_reference_start(dataIterator, currentState);
                    break;
                case States::HexadecimalCharacterReference:
                    hexadecimal_character_reference(dataIterator, currentState);
                    break;
                case States::DecimalCharacterReference:
                    decimal_character_reference(dataIterator, currentState);
                    break;
                case States::NumericCharacterReferenceEnd:
                    numeric_character_reference_end(dataIterator, currentState);
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

auto scrp::Tokenizer::release_tokens() -> void
{
    _impl->keep_tokens = true;
}

auto scrp::Tokenizer::keep_tokens() -> void
{
    _impl->keep_tokens = true;
}

auto scrp::Tokenizer::tokens() const -> const scrp::sc_vector<scrp::Token *> &
{
    return _impl->tokens;
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

auto scrp::Tokenizer::emit_token(Token *token) noexcept -> void
{
    assert(_impl->parser != nullptr);

    _impl->current_token_data.clear();
    _impl->extra_token_data_0.clear();
    _impl->extra_token_data_1.clear();
    _impl->attributes.clear();

    if (!_impl->tokens.empty() && _impl->tokens.back()->type == TokenType::Character && token->type == TokenType::Character)
    {
        // Merge characters
        auto this_tok = dynamic_cast<CharacterToken *>(token);
        auto last_tok = dynamic_cast<CharacterToken *>(_impl->tokens.back());

        last_tok->code_point += this_tok->code_point;
        return;
    }
    else
    {
        if (token->type == TokenType::Character)
        {
            // Add the token
            _impl->tokens.push_back(token);
            // Do not consume it
            return;
        }
    }

    // Consume all not consumed tokens
    for (auto &unconsumed_tokens : _impl->tokens)
    {
        if (!unconsumed_tokens->consumed)
        {
            _impl->parser->consume_token(unconsumed_tokens);
            unconsumed_tokens->consumed = true;
        }

        if (!_impl->keep_tokens)
            release_token(unconsumed_tokens);
    }

    if (!_impl->keep_tokens)
        _impl->tokens.clear();

    _impl->parser->consume_token(token);
    token->consumed = true;

    if (_impl->keep_tokens)
    {
        _impl->tokens.push_back(token);
    }
    else
        release_token(token);
}

auto scrp::Tokenizer::insert_character_to_string(sc_string &name, const sc_string &str) -> void
{
    for (const auto &ch : str)
        insert_character_to_string(name, ch);
}

auto scrp::Tokenizer::insert_character_to_string(sc_string &name, std::string_view str) -> void
{
    for (const auto &ch : str)
        insert_character_to_string(name, ch);
}

auto scrp::Tokenizer::insert_character_to_string(sc_string &name, char_type ch) -> void
{
    if (is_control_character(ch))
        emit_error(parser_error_type::control_character_in_input_stream);

    name += ch;
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
            emit_comment_token(_impl->current_token_data);
            emit_eof_token();
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
        case States::DOCTYPEPublicIdentifierSQ:
            [[fallthrough]];
        case States::DOCTYPEPublicIdentifierDQ:
            [[fallthrough]];
        case States::DOCTYPESystemIdentifierSQ:
            [[fallthrough]];
        case States::DOCTYPESystemIdentifierDQ:
            [[fallthrough]];
        case States::BogusDOCTYPE:
            emit_error(parser_error_type::eof_in_doctype);
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
            emit_eof_token();
            break;
        case States::Data: [[fallthrough]];
        case States::CharacterReference: [[fallthrough]];
        case States::NamedCharacterReference: [[fallthrough]];
        case States::NumericCharacterReference: [[fallthrough]];
        case States::HexadecimalCharacterReferenceStart: [[fallthrough]];
        case States::DecimalCharacterReferenceStart: [[fallthrough]];
        case States::HexadecimalCharacterReference: [[fallthrough]];
        case States::DecimalCharacterReference: [[fallthrough]];
        case States::NumericCharacterReferenceEnd: [[fallthrough]];
        case States::AmbiguousAmpersand: [[fallthrough]];
        case States::TagOpen: [[fallthrough]];
        case States::MarkupDeclarationOpen: [[fallthrough]];
        case States::CommentStart: [[fallthrough]];
        case States::CommentLessThanSign: [[fallthrough]];
        case States::CommentLessThanSignBang: [[fallthrough]];
        case States::CommentLessThanSignBangDash: [[fallthrough]];
        case States::CommentLessThanSignBangDashDash: [[fallthrough]];
        case States::CommentEndBang: [[fallthrough]];
        case States::AfterDOCTYPEPublicIdentifier: [[fallthrough]];
        case States::BetweenDOCTYPEPublicAndSystemIdentifiers: [[fallthrough]];
        case States::AfterDOCTYPESystemIdentifier: [[fallthrough]];
        case States::CDATASection: [[fallthrough]];
        case States::CDATASectionBracket: [[fallthrough]];
        case States::CDATASectionEnd: [[fallthrough]];
        case States::EndTagOpen: [[fallthrough]];
        case States::TagName: [[fallthrough]];
        case States::BeforeAttributeName: [[fallthrough]];
        case States::SelfClosingStartTag: [[fallthrough]];
        case States::AttributeName: [[fallthrough]];
        case States::AfterAttributeName: [[fallthrough]];
        case States::BeforeAttributeValue: [[fallthrough]];
        case States::AttributeValueDQ: [[fallthrough]];
        case States::AttributeValueSQ: [[fallthrough]];
        case States::AttributeValueUnquoted: [[fallthrough]];
        case States::AfterAttributeValueQuoted: /* nothing to do */ break;
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
            emit_character_token(sc_string { *pos });
            break;
    }

    if (is_next_char_eof(pos))
    {
        emit_eof_token();
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
                emit_character_token("<");
                stateChange = States::Data;
                --pos;
            }
            break;
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_before_tag_name);
        emit_character_token(">");
        emit_eof_token();
    }
}

auto scrp::Tokenizer::character_reference(sc_string::iterator &pos, scrp::States &stateChange) -> void
{

    switch (*pos)
    {
        case '#':
            stateChange              = States::NumericCharacterReference;
            _impl->numeric_reference = 0;
            _impl->named_reference.clear();
            break;
        default:
            if (is_char_alphanumeric(*pos))
            {
                _impl->named_reference.clear();
                _impl->last_char_reference = encoding::null_chref;
                stateChange                = States::NamedCharacterReference;
                --pos;
            }
            else
            {
                if (!_impl->state.empty() && _impl->state.top() != States::Data)
                    stateChange = _impl->state.top();
                else
                    stateChange = States::Data;
                --pos;
            }
    }
}

auto scrp::Tokenizer::named_character_reference(sc_string::iterator &pos, States &stateChange) -> void
{
    const auto &ch = *pos;

    // NOTE: this part is not compliant with the standard, however, the output is the same

    // Keep adding the character
    _impl->named_reference += ch;

    const auto &current_ref = encoding::find_reference(_impl->named_reference);

    if (current_ref != encoding::null_chref)
    {
        // Up to this point, this code will match for example &notin;, &notinE; &notindot; ..., as &not
        // Because &not is not mandatory with a semicolon, we must change this
        if (_impl->named_reference.back() != ';')
        {
            // Keep this reference
            _impl->last_char_reference = current_ref;
            // Keep track of the position of string in case we must flush this character reference and flush
            _impl->numeric_reference = static_cast<uint32_t>(_impl->named_reference.size());
            // Keep gathering characters
            return;
        }
    }

    // Stop getting the character reference when it encounters a non alpha character except for the semicolon.

    if ((is_char_alpha(ch) || ch != ';') && current_ref == encoding::null_chref)
    {
        // Keep gathering information only if ch != ';'
        if (ch != ';')
            return;
        // In the other case keep going
    }

    if (current_ref == encoding::null_chref)
    {
        // We reach a non alpha character and find_reference didn't catch a character reference
        if (is_return_state_attribute()) // it doesn't matter if _impl->last_char_reference is not null_chref
        {
            // Not an error, just keep push the data in the
            const sc_string to_token_value = sc_string { "&" } + _impl->named_reference;
            insert_character_to_string(_impl->extra_token_data_1, to_token_value);

            // Go back to the return state
            // since is_return_state_attribute() -> true; it is certain that _impl->state is not empty
            stateChange = _impl->state.top();
        }
        else
        {
            if (_impl->last_char_reference != encoding::null_chref)
            {
                // Emit the last_char_reference
                emit_character_token(_impl->last_char_reference.utf8_encoding.data());
                emit_character_token(_impl->named_reference.substr(_impl->numeric_reference, _impl->named_reference.size()));

                stateChange = States::Data;
            }
            else
            {
                stateChange = States::AmbiguousAmpersand;
            }
        }
    }
    else
    {
        if (is_return_state_attribute())
        {
            insert_character_to_string(_impl->extra_token_data_1, current_ref.utf8_encoding);

            // Go back to the return state
            // since is_return_state_attribute() -> true; it is certain that _impl->state is not empty
            stateChange = _impl->state.top();
        }
        else
        {
            emit_character_token(_impl->last_char_reference.utf8_encoding.data());

            stateChange = States::Data;
        }
    }
}

auto scrp::Tokenizer::numeric_character_reference(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case 'x':
        case 'X':
            stateChange = States::HexadecimalCharacterReferenceStart;
            break;
        default:
            stateChange = States::HexadecimalCharacterReferenceStart;
    }
}

auto scrp::Tokenizer::hexadecimal_character_reference_start(sc_string::iterator &pos, States &stateChange) -> void
{
    if (is_char_hex_digit(*pos))
    {
        stateChange = States::HexadecimalCharacterReference;
        --pos;
    }
    else
    {
        emit_error(parser_error_type::absence_of_digits_in_numeric_character_reference);
        if (!_impl->state.empty())
            stateChange = _impl->state.top();
        else
            stateChange = States::Data;

        if (stateChange != States::Data)
            insert_character_to_string(_impl->extra_token_data_1, *pos);

        --pos;
    }
}

auto scrp::Tokenizer::decimal_character_reference_start(sc_string::iterator &pos, States &stateChange) -> void
{
    if (is_char_digit(*pos))
    {
        stateChange = States::DecimalCharacterReference;
        --pos;
    }
    else
    {
        emit_error(parser_error_type::absence_of_digits_in_numeric_character_reference);
        if (!_impl->state.empty())
            stateChange = _impl->state.top();
        else
            stateChange = States::Data;

        if (stateChange != States::Data)
            insert_character_to_string(_impl->extra_token_data_1, *pos);

        --pos;
    }
}

auto scrp::Tokenizer::hexadecimal_character_reference(sc_string::iterator &pos, States &stateChange) -> void
{
    const auto &ch = *pos;
    if (is_char_digit(ch))
    {
        _impl->numeric_reference *= 16;
        _impl->numeric_reference += static_cast<int64_t>(ch) - 0x30ll;
    }
    else if (is_char_lower_hex_digit(ch))
    {
        _impl->numeric_reference *= 16;
        _impl->numeric_reference += static_cast<int64_t>(ch) - 0x57ll;
    }
    else if (is_char_upper_hex_digit(ch))
    {
        _impl->numeric_reference *= 16;
        _impl->numeric_reference += static_cast<int64_t>(ch) - 0x37ll;
    }
    else if (ch == ';')
    {
        stateChange = States::NumericCharacterReference;
    }
    else
    {
        emit_error(parser_error_type::missing_semicolon_after_character_reference);
        stateChange = States::NumericCharacterReferenceEnd;
        --pos;
    }
}

auto scrp::Tokenizer::decimal_character_reference(sc_string::iterator &pos, States &stateChange) -> void
{
    const auto &ch = *pos;
    if (is_char_digit(ch))
    {
        _impl->numeric_reference *= 10;
        _impl->numeric_reference += static_cast<int64_t>(ch) - 0x30ll;
    }
    else if (ch == ';')
    {
        stateChange = States::NumericCharacterReference;
    }
    else
    {
        emit_error(parser_error_type::missing_semicolon_after_character_reference);
        stateChange = States::NumericCharacterReferenceEnd;
        --pos;
    }
}

auto scrp::Tokenizer::numeric_character_reference_end(sc_string::iterator &pos, States &stateChange) -> void
{
    if (_impl->numeric_reference == 0)
    {
        emit_error(parser_error_type::null_character_reference);
        _impl->numeric_reference = 0xFFFD;
    }
    else if (_impl->numeric_reference > 0x10FFFF)
    {
        emit_error(parser_error_type::character_reference_outside_unicode_range);
        _impl->numeric_reference = 0xFFFD;
    }
    else if (_impl->numeric_reference >= 0xD800 && _impl->numeric_reference <= 0xDFFF)
    {
        emit_error(parser_error_type::surrogate_character_reference);
        _impl->numeric_reference = 0xFFFD;
    }
    else if (is_noncharacter(_impl->numeric_reference))
    {
        emit_error(parser_error_type::noncharacter_character_reference);
        _impl->numeric_reference = -1; // non-compliant: use to avoid emitting the character
    }
    else if (_impl->numeric_reference == 0x0D || is_control_character(_impl->numeric_reference))
    {
        emit_error(parser_error_type::control_character_reference);
        switch (_impl->numeric_reference)
        {
            case 0x80: _impl->numeric_reference = 0x20AC; break;
            case 0x82: _impl->numeric_reference = 0x201A; break;
            case 0x83: _impl->numeric_reference = 0x0192; break;
            case 0x84: _impl->numeric_reference = 0x201E; break;
            case 0x85: _impl->numeric_reference = 0x2026; break;
            case 0x86: _impl->numeric_reference = 0x2020; break;
            case 0x87: _impl->numeric_reference = 0x2021; break;
            case 0x88: _impl->numeric_reference = 0x02C6; break;
            case 0x89: _impl->numeric_reference = 0x2030; break;
            case 0x8A: _impl->numeric_reference = 0x0160; break;
            case 0x8B: _impl->numeric_reference = 0x2039; break;
            case 0x8C: _impl->numeric_reference = 0x0152; break;
            case 0x8E: _impl->numeric_reference = 0x017D; break;
            case 0x91: _impl->numeric_reference = 0x2018; break;
            case 0x92: _impl->numeric_reference = 0x2019; break;
            case 0x93: _impl->numeric_reference = 0x201C; break;
            case 0x94: _impl->numeric_reference = 0x201D; break;
            case 0x95: _impl->numeric_reference = 0x2022; break;
            case 0x96: _impl->numeric_reference = 0x2013; break;
            case 0x97: _impl->numeric_reference = 0x2014; break;
            case 0x98: _impl->numeric_reference = 0x02DC; break;
            case 0x99: _impl->numeric_reference = 0x2122; break;
            case 0x9A: _impl->numeric_reference = 0x0161; break;
            case 0x9B: _impl->numeric_reference = 0x203A; break;
            case 0x9C: _impl->numeric_reference = 0x0153; break;
            case 0x9E: _impl->numeric_reference = 0x017E; break;
            case 0x9F: _impl->numeric_reference = 0x0178; break;
            default:;
        }
    }

    if (!_impl->state.empty())
        stateChange = _impl->state.top();
    else
        stateChange = States::Data;

    const auto utf8_character_reference = encoding::rt_to_utf8(_impl->numeric_reference);
    if (stateChange != States::Data)
    {
        insert_character_to_string(_impl->extra_token_data_1, encoding::rt_to_view(utf8_character_reference));
    }
    else
    {
        emit_character_token(encoding::rt_to_view(utf8_character_reference));
    }
}

auto scrp::Tokenizer::ambiguous_ampersand(sc_string::iterator &pos, States &stateChange) -> void
{
    if (!is_return_state_attribute())
    {
        emit_error(parser_error_type::unknown_named_character_reference);
    }
    else
    {
        const sc_string to_token_value = sc_string { "&" } + _impl->named_reference;
        insert_character_to_string(_impl->extra_token_data_1, to_token_value);
    }

    if (!_impl->state.empty() && _impl->state.top() != States::Data)
        stateChange = _impl->state.top();

    stateChange = _impl->state.top();
    _impl->state.pop();

    --pos;
}

auto scrp::Tokenizer::markup_declaration_open(sc_string::iterator &pos, States &stateChange) -> void
{
    sc_string markup_name;
    markup_name.reserve(32);
    bool test_for_comment = false;
    bool keep_testing     = true;

    static constexpr std::string_view _doctype { "doctype" };
    static constexpr std::string_view _cdata { "[CDATA[" };

    for (; pos != _impl->data.end(); ++pos)
    {
        const auto &ch = *pos;
        if (ch == '-' && !is_next_char_eof(pos) && *std::next(pos) == '-')
        {
            ++pos;
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
                markup_name.pop_back();
                _impl->current_token_data.clear();
                insert_character_to_string(_impl->current_token_data, markup_name);
                --pos;
                return;
            }
        }
    }
}

auto scrp::Tokenizer::bogus_comment(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_comment_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        case '0':
            emit_error(parser_error_type::unexpected_null_character);
            insert_character_to_string(_impl->current_token_data, encoding::_sv_invalid);
            break;
        default:
            insert_character_to_string(_impl->current_token_data, *pos);
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
            emit_comment_token(_impl->current_token_data);
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
            emit_comment_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        default:
            --pos;
            insert_character_to_string(_impl->current_token_data, '-');
            stateChange = States::Comment;
            break;
    }
}

auto scrp::Tokenizer::comment(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '<':
            insert_character_to_string(_impl->current_token_data, *pos);
            stateChange = States::CommentLessThanSign;
            break;
        case '-':
            stateChange = States::CommentEndDash;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            insert_character_to_string(_impl->current_token_data, encoding::_sv_invalid);
            break;
        default:
            insert_character_to_string(_impl->current_token_data, *pos);
    }
}

auto scrp::Tokenizer::comment_less_than_sign(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '!':
            insert_character_to_string(_impl->current_token_data, *pos);
            stateChange = States::CommentLessThanSignBang;
            break;
        case '<':
            insert_character_to_string(_impl->current_token_data, *pos);
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

    if (is_next_char_eof(pos))
    {
        comment_end(pos, stateChange);
        return;
    }
}

auto scrp::Tokenizer::comment_less_than_sign_bang_dash_dash(sc_string::iterator &pos, States &stateChange) -> void
{

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

    if (is_next_char_eof(pos))
    {
        comment_end(pos, stateChange);
        return;
    }
}

auto scrp::Tokenizer::comment_end_dash(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_comment);
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        emit_comment_token(_impl->current_token_data);
        emit_eof_token();
    }

    switch (*pos)
    {
        case '-':
            stateChange = States::CommentEnd;
            break;
        default:
            insert_character_to_string(_impl->current_token_data, *pos);
            --pos;
            stateChange = States::Comment;
    }
}

auto scrp::Tokenizer::comment_end(sc_string::iterator &pos, scrp::States &stateChange) -> void
{

    using namespace std::string_view_literals;
    switch (*pos)
    {
        case '>':
            emit_comment_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        case '!':
            stateChange = States::CommentEndBang;
            break;
        case '-':
            insert_character_to_string(_impl->current_token_data, *pos);
            break;
        default:
            --pos;
            insert_character_to_string(_impl->current_token_data, "--"sv);
            stateChange = States::Comment;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data && stateChange != States::CommentEndBang && stateChange != States::Comment)
    {
        emit_error(parser_error_type::eof_in_comment);
        emit_comment_token(_impl->current_token_data);
        emit_eof_token();
        return;
    }
}

auto scrp::Tokenizer::comment_end_bang(sc_string::iterator &pos, States &stateChange) -> void
{
    using namespace std::string_view_literals;
    switch (*pos)
    {
        case '-':
            insert_character_to_string(_impl->current_token_data, "--!"sv);
            stateChange = States::CommentEndDash;
            break;
        case '>':
            emit_error(parser_error_type::incorrectly_closed_comment);
            emit_comment_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->current_token_data, "--!"sv);
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
            insert_character_to_string(_impl->current_token_data, encoding::_sv_invalid);
            stateChange = States::DOCTYPEName;
            break;
        case '>':
            emit_error(parser_error_type::missing_doctype_name);
            emit_doctype_token(_impl->current_token_data, true);
            stateChange = States::Data;
            break;
        default:
            auto ch = *pos;
            if (is_char_upper_alpha(ch))
                ch = to_lower(ch);
            insert_character_to_string(_impl->current_token_data, ch);
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
            emit_doctype_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            insert_character_to_string(_impl->current_token_data, encoding::_sv_invalid);
            break;
        default:
            insert_character_to_string(_impl->current_token_data, to_lower(*pos));
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
            emit_doctype_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_0, *pos);
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
            emit_doctype_token(_impl->current_token_data, true);
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
            emit_doctype_token(_impl->current_token_data, true);
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
            insert_character_to_string(_impl->extra_token_data_0, encoding::_sv_invalid);
            ;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_public_identifier);
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, true);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_0, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
        emit_eof_token();
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
            insert_character_to_string(_impl->extra_token_data_0, encoding::_sv_invalid);
            ;
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_public_identifier);
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, true);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_0, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_doctype);
        if (stateChange != States::Data)
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
        emit_eof_token();
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
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
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
            {
                if (is_control_character(*pos))
                    emit_error(parser_error_type::control_character_in_input_stream);
            }
            emit_error(parser_error_type::missing_quote_before_doctype_system_identifier);
            --pos;
            stateChange = States::BogusDOCTYPE;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
        emit_eof_token();
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
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
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
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0);
        emit_eof_token();
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
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
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
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
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
            insert_character_to_string(_impl->extra_token_data_1, encoding::_sv_invalid);
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_system_identifier);
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_1, *pos);
    }
    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
        emit_eof_token();
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
            insert_character_to_string(_impl->extra_token_data_1, encoding::_sv_invalid);
            break;
        case '>':
            emit_error(parser_error_type::abrupt_doctype_system_identifier);
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_1, *pos);
    }
    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
        emit_eof_token();
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
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1);
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::unexpected_character_after_doctype_system_identifier);
            _impl->quirk_flag = false;
            --pos;
            stateChange = States::BogusDOCTYPE;
    }

    if (is_next_char_eof(pos) && stateChange != States::Data)
    {
        emit_error(parser_error_type::eof_in_doctype);
        emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, true);
        emit_eof_token();
        stateChange = States::Data; // Prevent the call to handle_eof_error()
        return;
    }
}

auto scrp::Tokenizer::bogus_doctype(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_doctype_token(_impl->current_token_data, _impl->extra_token_data_0, _impl->extra_token_data_1, _impl->quirk_flag);
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
            insert_character_to_string(_impl->current_token_data, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_cdata);
        emit_cdata_token(_impl->current_token_data);
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
            insert_character_to_string(_impl->current_token_data, ']');
            stateChange = States::CDATASection;
            --pos;
    }
}

auto scrp::Tokenizer::cdata_section_end(sc_string::iterator &pos, scrp::States &stateChange) -> void
{
    using namespace std::string_view_literals;
    switch (*pos)
    {
        case ']':
            insert_character_to_string(_impl->current_token_data, ']');
            break;
        case '>':
            emit_cdata_token(_impl->current_token_data);
            stateChange = States::Data;
            break;
        default:
            insert_character_to_string(_impl->current_token_data, "]]"sv);
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
                emit_end_tag_token();
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
        emit_character_token("<");
        emit_character_token("/");
        emit_eof_token();
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
            insert_character_to_string(_impl->current_token_data, encoding::_sv_invalid);
        default:
            {
                const auto ch = to_lower(*pos);
                insert_character_to_string(_impl->current_token_data, ch);
            }
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
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
            _impl->extra_token_data_0.clear();
            insert_character_to_string(_impl->extra_token_data_0, *pos);
            stateChange = States::AttributeName;
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
            insert_character_to_string(_impl->extra_token_data_0, encoding::_sv_invalid);
            ;
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
            insert_character_to_string(_impl->extra_token_data_0, ch);
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
            emit_tag_token(_impl->current_token_data, _impl->attributes);
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
        emit_eof_token();
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
            emit_tag_token(_impl->current_token_data, _impl->attributes);
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
            insert_character_to_string(_impl->extra_token_data_1, encoding::_sv_invalid);
            ;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_1, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
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
            insert_character_to_string(_impl->extra_token_data_1, encoding::_sv_invalid);
            ;
            break;
        default:
            insert_character_to_string(_impl->extra_token_data_1, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
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
            if (!_impl->state.empty() && _impl->state.top() != States::Data)
                _impl->state.pop();
            break;
        case '&':
            _impl->state.push(States::AttributeValueUnquoted);
            stateChange = States::CharacterReference;
            break;
        case '>':
            insert_attribute(_impl->extra_token_data_0, _impl->extra_token_data_1);
            _impl->extra_token_data_0.clear();
            _impl->extra_token_data_1.clear();
            emit_tag_token(_impl->current_token_data, _impl->attributes);
            if (!_impl->state.empty() && _impl->state.top() != States::Data)
                _impl->state.pop();
            stateChange = States::Data;
            break;
        case 0:
            emit_error(parser_error_type::unexpected_null_character);
            insert_character_to_string(_impl->extra_token_data_1, encoding::_sv_invalid);
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
            insert_character_to_string(_impl->extra_token_data_1, *pos);
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
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
            emit_tag_token(_impl->current_token_data, _impl->attributes);
            if (!_impl->state.empty() && _impl->state.top() != States::Data)
                _impl->state.pop();
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::missing_whitespace_between_attributes);
            stateChange = States::BeforeAttributeName;
            --pos;
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
    }
}

auto scrp::Tokenizer::self_closing_start_tag(sc_string::iterator &pos, States &stateChange) -> void
{
    switch (*pos)
    {
        case '>':
            emit_tag_token(_impl->current_token_data, _impl->attributes, true);
            if (!_impl->state.empty() && _impl->state.top() != States::Data)
                _impl->state.pop();
            stateChange = States::Data;
            break;
        default:
            emit_error(parser_error_type::unexpected_solidus_in_tag);
            stateChange = States::BeforeAttributeName;
            --pos;
    }

    if (is_next_char_eof(pos))
    {
        emit_error(parser_error_type::eof_in_tag);
        emit_eof_token();
    }
}
