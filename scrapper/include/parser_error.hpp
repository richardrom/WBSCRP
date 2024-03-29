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
        abrupt_closing_of_empty_comment,
        abrupt_doctype_public_identifier,
        abrupt_doctype_system_identifier,
        absence_of_digits_in_numeric_character_reference,
        cdata_in_html_content,
        character_reference_outside_unicode_range,
        control_character_in_input_stream,
        control_character_reference,
        end_tag_with_attributes,
        duplicate_attribute,
        end_tag_with_trailing_solidus,
        eof_before_tag_name,
        eof_in_cdata,
        eof_in_comment,
        eof_in_doctype,
        eof_in_script_html_comment_like_text,
        eof_in_tag,
        incorrectly_closed_comment,
        incorrectly_opened_comment,
        invalid_character_sequence_after_doctype_name,
        invalid_first_character_of_tag_name,
        missing_attribute_value,
        missing_doctype_name,
        missing_doctype_public_identifier,
        missing_doctype_system_identifier,
        missing_end_tag_name,
        missing_quote_before_doctype_public_identifier,
        missing_quote_before_doctype_system_identifier,
        missing_semicolon_after_character_reference,
        missing_whitespace_after_doctype_public_keyword,
        missing_whitespace_after_doctype_system_keyword,
        missing_whitespace_before_doctype_name,
        missing_whitespace_between_attributes,
        missing_whitespace_between_doctype_public_and_system_identifiers,
        nested_comment,
        noncharacter_character_reference,
        noncharacter_in_input_stream,
        non_void_html_element_start_tag_with_trailing_solidus,
        null_character_reference,
        surrogate_character_reference,
        surrogate_in_input_stream,
        unexpected_character_after_doctype_system_identifier,
        unexpected_character_in_attribute_name,
        unexpected_character_in_unquoted_attribute_value,
        unexpected_equals_sign_before_attribute_name,
        unexpected_null_character,
        unexpected_question_mark_instead_of_tag_name,
        unexpected_solidus_in_tag,
        unknown_named_character_reference,
    };

    struct parser_error final
    {
        parser_error(parser_error_type type, std::size_t position, std::size_t offset, std::size_t line);
        ~parser_error();

        const char *what() const noexcept;
        auto type() const noexcept -> parser_error_type;
        auto pos() const noexcept -> std::size_t;
        auto line_offset() const noexcept -> std::size_t;
        auto line() const noexcept -> std::size_t;

    private:
        parser_error_type _type;
        std::size_t _pos;
        std::size_t _offset;
        std::size_t _line;
    };
} // namespace scrp

#endif // WBSCRP_PARSER_ERROR_HPP
