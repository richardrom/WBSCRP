/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 21/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include "parser_error.hpp"


scrp::parser_error::parser_error(scrp::parser_error_type type, std::size_t position, std::size_t offset, std::size_t line) :
    _type { type },
    _pos { position },
    _offset { offset },
    _line { line }
{
}

scrp::parser_error::~parser_error() = default;

const char *scrp::parser_error::what() const noexcept
{
    switch(_type)
    {

        case parser_error_type::abrupt_closing_of_empty_comment: break;
        case parser_error_type::abrupt_doctype_public_identifier: break;
        case parser_error_type::abrupt_doctype_system_identifier: break;
        case parser_error_type::absence_of_digits_in_numeric_character_reference: break;
        case parser_error_type::cdata_in_html_content: break;
        case parser_error_type::character_reference_outside_unicode_range: break;
        case parser_error_type::control_character_in_input_stream: break;
        case parser_error_type::control_character_reference: break;
        case parser_error_type::end_tag_with_attributes: break;
        case parser_error_type::duplicate_attribute: break;
        case parser_error_type::end_tag_with_trailing_solidus: break;
        case parser_error_type::eof_before_tag_name: break;
        case parser_error_type::eof_in_cdata: break;
        case parser_error_type::eof_in_comment: break;
        case parser_error_type::eof_in_doctype: break;
        case parser_error_type::eof_in_script_html_comment_like_text: break;
        case parser_error_type::eof_in_tag: break;
        case parser_error_type::incorrectly_closed_comment: break;
        case parser_error_type::incorrectly_opened_comment: break;
        case parser_error_type::invalid_character_sequence_after_doctype_name: break;
        case parser_error_type::invalid_first_character_of_tag_name: break;
        case parser_error_type::missing_attribute_value: break;
        case parser_error_type::missing_doctype_name: break;
        case parser_error_type::missing_doctype_public_identifier: break;
        case parser_error_type::missing_doctype_system_identifier: break;
        case parser_error_type::missing_end_tag_name: break;
        case parser_error_type::missing_quote_before_doctype_public_identifier: break;
        case parser_error_type::missing_quote_before_doctype_system_identifier: break;
        case parser_error_type::missing_semicolon_after_character_reference: break;
        case parser_error_type::missing_whitespace_after_doctype_public_keyword: break;
        case parser_error_type::missing_whitespace_after_doctype_system_keyword: break;
        case parser_error_type::missing_whitespace_before_doctype_name: break;
        case parser_error_type::missing_whitespace_between_attributes: break;
        case parser_error_type::missing_whitespace_between_doctype_public_and_system_identifiers: break;
        case parser_error_type::nested_comment: break;
        case parser_error_type::noncharacter_character_reference: break;
        case parser_error_type::noncharacter_in_input_stream: break;
        case parser_error_type::non_void_html_element_start_tag_with_trailing_solidus: break;
        case parser_error_type::null_character_reference: break;
        case parser_error_type::surrogate_character_reference: break;
        case parser_error_type::surrogate_in_input_stream: break;
        case parser_error_type::unexpected_character_after_doctype_system_identifier: break;
        case parser_error_type::unexpected_character_in_attribute_name: break;
        case parser_error_type::unexpected_character_in_unquoted_attribute_value: break;
        case parser_error_type::unexpected_equals_sign_before_attribute_name: break;
        case parser_error_type::unexpected_null_character: break;
        case parser_error_type::unexpected_question_mark_instead_of_tag_name: break;
        case parser_error_type::unexpected_solidus_in_tag: break;
        case parser_error_type::unknown_named_character_reference: break;
    }

    return nullptr;
}

auto scrp::parser_error::type() const noexcept -> scrp::parser_error_type
{
    return _type;
}

auto scrp::parser_error::pos() const noexcept -> std::size_t
{
    return _pos;
}

auto scrp::parser_error::line_offset() const noexcept -> std::size_t
{
    return _offset;
}
auto scrp::parser_error::line() const noexcept -> std::size_t
{
    return _line;
}
