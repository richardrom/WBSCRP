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

#include "parser_error.hpp"
#include "scrapper.hpp"

namespace scrp
{
    class parser;
    enum class States;
    class Tokenizer
    {
    public:
        explicit Tokenizer(sc_string source);
        ~Tokenizer();

    public:
        /// \return true if no errors were encounter; false if recoverable parsing errors were encounter. Retrieve this errors with get_parse_errors();
        /// \throws scrp::parser_error if an unrecoverable parse_error is encounter
        [[nodiscard]] auto tokenize() -> bool;

        [[nodiscard]] auto get_parse_errors() const noexcept -> scrp::sc_vector<parser_error>;

        auto set_parser(parser *parser) -> void;

        /// \brief Once a token is passed to the parser is discarded.
        /// \note This is default behavior
        /// \note If this flag is set while the tokenizer is running, it will incur in undefined behavior
        auto release_tokens() -> void;

        /// \brief Don't discard the tokens once they are passed to the parser
        /// \note If this flag is set while the tokenizer is running, it will incur in undefined behavior
        auto keep_tokens() -> void;

        [[nodiscard]] auto tokens() const -> const sc_vector<Token *> &;

    protected:
        auto handle_eof_error(States stateChange) -> void;
        auto data_state(sc_string::iterator &pos, States &stateChange) -> void;
        auto character_reference(sc_string::iterator &pos, States &stateChange) -> void;
        auto named_character_reference(sc_string::iterator &pos, States &stateChange) -> void;
        auto numeric_character_reference(sc_string::iterator &pos, States &stateChange) -> void;
        auto hexadecimal_character_reference_start(sc_string::iterator &pos, States &stateChange) -> void;
        auto decimal_character_reference_start(sc_string::iterator &pos, States &stateChange) -> void;
        auto hexadecimal_character_reference(sc_string::iterator &pos, States &stateChange) -> void;
        auto decimal_character_reference(sc_string::iterator &pos, States &stateChange) -> void;
        auto numeric_character_reference_end(sc_string::iterator &pos, States &stateChange) -> void;
        auto ambiguous_ampersand(sc_string::iterator &pos, States &stateChange) -> void;
        auto tag_open_state(sc_string::iterator &pos, States &stateChange) -> void;
        auto markup_declaration_open(sc_string::iterator &pos, States &stateChange) -> void;
        auto bogus_comment(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_start(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_start_dash(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_end(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_less_than_sign(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_end_dash(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_less_than_sign_bang(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_less_than_sign_bang_dash(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_less_than_sign_bang_dash_dash(sc_string::iterator &pos, States &stateChange) -> void;
        auto comment_end_bang(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype(sc_string::iterator &pos, States &stateChange) -> void;
        auto before_doctype_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_doctype_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_doctype_public_keyword(sc_string::iterator &pos, States &stateChange) -> void;
        auto before_doctype_public_identifier(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype_public_identifier_dq(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype_public_identifier_sq(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_doctype_public_identifier(sc_string::iterator &pos, States &stateChange) -> void;
        auto between_doctype_public_and_system_identifiers(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_doctype_system_keyword(sc_string::iterator &pos, States &stateChange) -> void;
        auto before_doctype_system_identifier(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype_system_identifier_dq(sc_string::iterator &pos, States &stateChange) -> void;
        auto doctype_system_identifier_sq(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_doctype_system_identifier(sc_string::iterator &pos, States &stateChange) -> void;
        auto bogus_doctype(sc_string::iterator &pos, States &stateChange) -> void;
        auto cdata_section(sc_string::iterator &pos, States &stateChange) -> void;
        auto cdata_section_bracket(sc_string::iterator &pos, States &stateChange) -> void;
        auto cdata_section_end(sc_string::iterator &pos, States &stateChange) -> void;
        auto end_tag_open(sc_string::iterator &pos, States &stateChange) -> void;
        auto tag_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto before_attribute_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto attribute_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_attribute_name(sc_string::iterator &pos, States &stateChange) -> void;
        auto before_attribute_value(sc_string::iterator &pos, States &stateChange) -> void;
        auto attribute_value_dq(sc_string::iterator &pos, States &stateChange) -> void;
        auto attribute_value_sq(sc_string::iterator &pos, States &stateChange) -> void;
        auto attribute_value_unquoted(sc_string::iterator &pos, States &stateChange) -> void;
        auto after_attribute_value_quoted(sc_string::iterator &pos, States &stateChange) -> void;
        auto self_closing_start_tag(sc_string::iterator &pos, States &stateChange) -> void;

    protected:
        auto insert_attribute(const sc_string &name, const sc_string &value) -> void;
        inline auto insert_character_to_string(sc_string &name, const sc_string &str) -> void;
        inline auto insert_character_to_string(sc_string &name, std::string_view str) -> void;
        inline auto insert_character_to_string(sc_string &name, char_type ch) -> void;
        [[nodiscard]] auto is_next_char_eof(const sc_string::iterator &pos) const -> bool;
        [[nodiscard]] static auto is_char_alpha(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_lower_alpha(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_upper_alpha(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_lower_hex_digit(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_upper_hex_digit(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_hex_digit(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_digit(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_char_alphanumeric(scrp::char_type ch) noexcept -> bool;
        [[nodiscard]] static auto is_noncharacter(int64_t codepoint) noexcept -> bool;
        [[nodiscard]] static auto is_control_character(int64_t codepoint) noexcept -> bool;
        [[nodiscard]] static auto to_lower(scrp::char_type ch) noexcept -> scrp::char_type;
        [[nodiscard]] static auto string_to_lower(sc_string &str) noexcept -> sc_string;


    protected:
        [[nodiscard]] auto is_return_state_attribute() -> bool;

    protected:
        [[nodiscard]] auto current_position() const noexcept -> std::size_t;
        [[nodiscard]] auto current_line() const noexcept -> std::size_t;
        [[nodiscard]] auto current_line_offset() const noexcept -> std::size_t;

    protected:
        auto emit_token(Token *token) noexcept -> void;
        auto emit_error(parser_error_type type) noexcept -> void;

        template <typename T, typename... Args>
        auto emit_token(Args... args) -> void
        {
            emit_token( get_token_pool<T>()->alloc(args...) );
        }

        auto emit_eof_token()
        {
            emit_token<EOFToken>();
        }

        auto emit_end_tag_token()
        {
            emit_token<EndTagToken>();
        }

        template <typename... Args>
        auto emit_comment_token(Args... args)
        {
            emit_token<CommentToken>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto emit_character_token(Args... args)
        {
            emit_token<CharacterToken>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto emit_doctype_token(Args... args)
        {
            emit_token<DOCTYPEToken>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto emit_cdata_token(Args... args)
        {
            emit_token<CDATAToken>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto emit_tag_token(Args... args)
        {
            emit_token<TagToken>(std::forward<Args>(args)...);
        }

    public:
        template<typename T>
        static auto token_cast(Token *tok) -> T *
        {
            return static_cast<T *>(tok);
        }

        static inline auto doctype_token_cast(Token *tok) -> DOCTYPEToken *
        {
            return token_cast<DOCTYPEToken>(tok);
        }

        static inline auto eof_token_cast(Token *tok) -> EOFToken *
        {
            return token_cast<EOFToken>(tok);
        }

        static inline auto comment_token_cast(Token *tok) -> CommentToken *
        {
            return token_cast<CommentToken>(tok);
        }

        static inline auto character_token_cast(Token *tok) -> CharacterToken *
        {
            return token_cast<CharacterToken>(tok);
        }

        static inline auto cdata_token_cast(Token *tok) -> CDATAToken *
        {
            return token_cast<CDATAToken>(tok);
        }


    private:
        auto release_token(Token *&) -> void;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
} // namespace scrp

#endif // WBSCRP_TOKENIZER_HPP
