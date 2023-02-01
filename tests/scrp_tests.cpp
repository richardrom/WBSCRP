/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <crc64.hpp>
#include <encoding.hpp>
#include <encoding_character_reference.hpp>

#include <fmt/core.h>
#include <parser.hpp>
#include <parser_error.hpp>
#include <tokenizer.hpp>

#include <random>

TEST_CASE("Scrapper Tokenizer")
{
    scrp::initialize();
    scrp::Tokenizer tok("");
    CHECK(tok.tokenize() == false);
}

TEST_CASE("UTF16 to UTF8 constexpr encoding")
{
    constexpr auto p8    = scrp::encoding::to_utf8<'d'>();
    constexpr auto p16   = scrp::encoding::to_utf8<0x00A3>();
    constexpr auto p24_0 = scrp::encoding::to_utf8<0x0939>();
    constexpr auto p24_1 = scrp::encoding::to_utf8<0x20AC>();
    constexpr auto p24_2 = scrp::encoding::to_utf8<0xD55C>();
    constexpr auto p32_0 = scrp::encoding::to_utf8<0x10348>();

    SECTION("Pair check")
    {
        CHECK(p8 == scrp::encoding::utf8_pair { 'd', 1 });
        CHECK(p16 == scrp::encoding::utf8_pair { 0xC2A3u, 2 });
        CHECK(p24_0 == scrp::encoding::utf8_pair { 0xE0A4B9u, 3 });
        CHECK(p24_1 == scrp::encoding::utf8_pair { 0xE282ACu, 3 });
        CHECK(p24_2 == scrp::encoding::utf8_pair { 0xED959Cu, 3 });
        CHECK(p32_0 == scrp::encoding::utf8_pair { 0xF0908D88u, 4 });
    }

    SECTION("View check")
    {
        constexpr auto v_p8    = scrp::encoding::to_view<scrp::encoding::to_utf8<'d'>()>();
        constexpr auto v_p16   = scrp::encoding::to_view<scrp::encoding::to_utf8<0x00A3>()>();
        constexpr auto v_p24_0 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x0939>()>();
        constexpr auto v_p24_1 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x20AC>()>();
        constexpr auto v_p24_2 = scrp::encoding::to_view<scrp::encoding::to_utf8<0xD55C>()>();
        constexpr auto v_p32_0 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x10348>()>();

        CHECK(strcmp("d", v_p8._data) == 0);
        CHECK(strcmp("\xC2\xA3", v_p16._data) == 0);
        CHECK(strcmp("\xE0\xA4\xB9", v_p24_0._data) == 0);
        CHECK(strcmp("\xE2\x82\xAC", v_p24_1._data) == 0);
        CHECK(strcmp("\xED\x95\x9C", v_p24_2._data) == 0);
        CHECK(strcmp("\xF0\x90\x8D\x88", v_p32_0._data) == 0);
    }

    static constexpr auto s_v_p8    = scrp::encoding::to_view<scrp::encoding::to_utf8<'d'>()>();
    static constexpr auto s_v_p16   = scrp::encoding::to_view<scrp::encoding::to_utf8<0x00A3>()>();
    static constexpr auto s_v_p24_0 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x0939>()>();
    static constexpr auto s_v_p24_1 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x20AC>()>();
    static constexpr auto s_v_p24_2 = scrp::encoding::to_view<scrp::encoding::to_utf8<0xD55C>()>();
    static constexpr auto s_v_p32_0 = scrp::encoding::to_view<scrp::encoding::to_utf8<0x10348>()>();

    static constexpr std::string_view ss_p8 { s_v_p8._data };
    static constexpr std::string_view ss_p16 { s_v_p16._data };
    static constexpr std::string_view ss_p24_0 { s_v_p24_0._data };
    static constexpr std::string_view ss_p24_1 { s_v_p24_1._data };
    static constexpr std::string_view ss_p24_2 { s_v_p24_2._data };
    static constexpr std::string_view ss_p32_0 { s_v_p32_0._data };

    SECTION("constexpr string_view check")
    {
        CHECK(ss_p8 == "d");
        CHECK(ss_p16 == "\xC2\xA3");
        CHECK(ss_p24_0 == "\xE0\xA4\xB9");
        CHECK(ss_p24_1 == "\xE2\x82\xAC");
        CHECK(ss_p24_2 == "\xED\x95\x9C");
        CHECK(ss_p32_0 == "\xF0\x90\x8D\x88");
    }

    static constexpr auto s_v0_0 = scrp::encoding::to_d_view<scrp::encoding::to_utf8<0x022DA>(), scrp::encoding::to_utf8<0x0FE00>()>();
    static constexpr auto s_v1_0 = scrp::encoding::to_d_view<scrp::encoding::to_utf8<0x0228B>(), scrp::encoding::to_utf8<0x0FE00>()>();
    static constexpr auto s_v2_0 = scrp::encoding::to_d_view<scrp::encoding::to_utf8<0x0003D>(), scrp::encoding::to_utf8<0x020E5>()>();

    static constexpr std::string_view ss_v0 { s_v0_0._data };
    static constexpr std::string_view ss_v1 { s_v1_0._data };
    static constexpr std::string_view ss_v2 { s_v2_0._data };

    SECTION("constexpr d_view check")
    {
        // e2 8b 9a ef b8 80
        CHECK(ss_v0 == "\xE2\x8B\x9A\xEF\xB8\x80");
        CHECK(ss_v1 == "\xE2\x8A\x8B\xEF\xB8\x80");
        CHECK(ss_v2 == "\x3D\xE2\x83\xA5");
    }
}

TEST_CASE("Find character reference")
{
    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<std::size_t> rd(0, scrp::encoding::chref_table.size() - 1);
    std::vector<std::size_t> ref_index;
    std::vector<std::string> search_strings;

    ref_index.reserve(60000);
    for (int i = 0; i < 60000; ++i)
    {
        ref_index.emplace_back(rd(gen));
    }

    search_strings.reserve(120000);
    for (const auto &p : ref_index)
    {
        std::string scrmp { scrp::encoding::chref_table[p].data };
        scrmp += "$%$"; // This will make sure scrmp is not equal to any of the characters even when shuffled

        std::shuffle(scrmp.begin(), scrmp.end(), gen);

        search_strings.emplace_back(scrmp);
        search_strings.emplace_back(scrp::encoding::chref_table[p].data);
    }

    int found = 0, not_found = 0;

    for (const auto &p : search_strings)
    {
        auto f = scrp::encoding::find_reference(p);
        if (f.crc64 == 0)
            ++not_found;
        else
            ++found;
    }

    CHECK(found == 60000);
    CHECK(not_found == 60000);
}

TEST_CASE("Find reference benchmarks")
{
    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<std::size_t> rd(0, scrp::encoding::chref_table.size() - 1);
    std::vector<std::size_t> ref_index;

    ref_index.reserve(3000);
    for (int i = 0; i < 3000; ++i)
    {
        ref_index.emplace_back(rd(gen));
    }

    int sFound = 0;
    BENCHMARK("Search by string")
    {

        for (const auto &p : ref_index)
        {
            const std::string_view str = scrp::encoding::chref_table[p].data;

            for (const auto &d : scrp::encoding::chref_table)
            {
                if (d.data == str)
                    ++sFound;
            }
        }
    };

    CHECK(sFound > ref_index.size());

    int lFound = 0;
    BENCHMARK("Search by CRC")
    {
        for (const auto &p : ref_index)
        {
            const std::string_view str = scrp::encoding::chref_table[p].data;
            uint64_t crc               = scrp::crc64::compute(str.data(), str.size());

            for (const auto &d : scrp::encoding::chref_table)
            {
                if (d.crc64 == crc)
                    ++lFound;
            }
        }
    };

    CHECK(lFound > ref_index.size());

    int fFound = 0;
    BENCHMARK("Search with find_reference()")
    {
        for (const auto &p : ref_index)
        {
            const std::string_view str = scrp::encoding::chref_table[p].data;

            auto f = scrp::encoding::find_reference(str);
            if (f.crc64 != 0)
                ++fFound;
        }
    };

    CHECK(fFound > ref_index.size());
}

#define CHECK_DOCTYPE(tok, name_, pb_, sy_, ssv_)   \
    CHECK((tok)->type == scrp::TokenType::DOCTYPE); \
    CHECK((tok)->name == scrp::sc_string((name_))); \
    CHECK((tok)->public_identifier_name == (pb_));  \
    CHECK((tok)->system_identifier_name == (sy_));  \
    CHECK((tok)->force_quirks_flag == (ssv_));

#define CHECK_COMMENT(tok, com_)                    \
    CHECK((tok)->type == scrp::TokenType::Comment); \
    CHECK((tok)->comment == (com_));

#define CHECK_CHARACTER(tok, char_)                   \
    CHECK((tok)->type == scrp::TokenType::Character); \
    CHECK((tok)->code_point == (char_));

#define CHECK_EOF(tok) \
    CHECK((tok)->type == scrp::TokenType::EndOfFile);

#define CHECK_TAG(tok, char_, self_)            \
    CHECK((tok)->type == scrp::TokenType::Tag); \
    CHECK((tok)->tag_name == (char_));          \
    CHECK((tok)->self_closing == (self_));

#define CHECK_EMPTY_ATTRIBUTES(tok) \
    CHECK((tok)->attributes.empty());

#define CHECK_ATTRIBUTES_SIZE(tok, size_) \
    CHECK((tok)->attributes.size() == (size_));

#define CHECK_END_TAG(tok, char_)                  \
    CHECK((tok)->type == scrp::TokenType::EndTag); \
    CHECK((tok)->tag_name == (char_));             \
    CHECK((tok)->self_closing == false);

TEST_CASE("DOCTYPE Test")
{

    scrp::initialize();

    scrp::parser test_parser;

    SECTION("Correct Doctype lowercase: <!DOCTYPE html>")
    {
        scrp::Tokenizer tok("<!DOCTYPE html>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().empty());

        CHECK(tok.tokens().size() == 1);
        // auto t = );
        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", false);
    }

    SECTION("Correct Doctype uppercase: <!DOCTYPE HTML>")
    {
        scrp::Tokenizer tok("<!DOCTYPE HTML>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().empty());

        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", false);
    }

    SECTION("Correct Doctype mixed case: <!DOCTYPE HtMl>")
    {
        scrp::Tokenizer tok("<!DOCTYPE HtMl>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().empty());
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", false);
    }

    SECTION("Correct Doctype case with EOF: <!DOCTYPE HtMl")
    {
        scrp::Tokenizer tok("<!DOCTYPE HtMl");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", true);
        CHECK(tok.tokens()[1]->type == scrp::TokenType::EndOfFile);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("Truncated doctype start: <!DOC>")
    {
        scrp::Tokenizer tok("<!DOC>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "DOC");
        CHECK(tok.tokens()[1]->type == scrp::TokenType::EndOfFile);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::incorrectly_opened_comment);
    }

    SECTION("Doctype in error: <!DOCTYPE foo>")
    {
        scrp::Tokenizer tok("<!DOCTYPE foo>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.get_parse_errors().empty());
        CHECK(tok.tokenize() == true);
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "foo", "", "", false);
    }

    SECTION("DOCTYPE without name")
    {
        scrp::Tokenizer tok("<!DOCTYPE>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().size() == 1);
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "", "", "", true);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_doctype_name);
    }

    SECTION("DOCTYPE without space before name")
    {
        scrp::Tokenizer tok("<!DOCTYPEhtml>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().size() == 1);
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", false);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
    }

    SECTION("Incorrect DOCTYPE without a space before name")
    {
        scrp::Tokenizer tok("<!DOCTYPEfoo>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().size() == 1);
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "foo", "", "", false);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
    }

    SECTION("DOCTYPE with publicId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML Transitional 4.01//EN\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().size() == 0);
        CHECK(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "-//W3C//DTD HTML Transitional 4.01//EN", "", false);
    }

    SECTION("DOCTYPE with EOF after PUBLIC")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        CHECK(tok.get_parse_errors().size() == 1);
        CHECK(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", true);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("DOCTYPE with EOF after PUBLIC '")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC '");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", true);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("DOCTYPE with EOF after PUBLIC 'x")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC 'x");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "x", "", false);
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("DOCTYPE with systemId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html SYSTEM \"-//W3C//DTD HTML Transitional 4.01//EN\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "-//W3C//DTD HTML Transitional 4.01//EN", false);
    }

    SECTION("DOCTYPE with single-quoted systemId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC '-//W3C//DTD HTML Transitional 4.01//EN'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "-//W3C//DTD HTML Transitional 4.01//EN", "", false);
    }

    SECTION("DOCTYPE with publicId and systemId")
    {
        scrp::Tokenizer tok(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML Transitional 4.01//EN" "-//W3C//DTD HTML Transitional 4.01//EN">)");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "-//W3C//DTD HTML Transitional 4.01//EN", "-//W3C//DTD HTML Transitional 4.01//EN", false);
    }

    SECTION("DOCTYPE with > in double-quoted publicId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC \">x");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        CHECK(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 3);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", true);
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[1]), "x");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[2]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_doctype_public_identifier);
    }

    SECTION("DOCTYPE with > in single-quoted publicId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC '>x");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 3);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "", true);
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[1]), "x");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[2]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_doctype_public_identifier);
    }

    SECTION("DOCTYPE with > in double-quoted systemId\"")
    {
        scrp::Tokenizer tok(R"(<!DOCTYPE html PUBLIC "foo" ">x)");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 3);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "foo", "", true);
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[1]), "x");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[2]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_doctype_system_identifier);
    }

    SECTION("DOCTYPE with > in single-quoted systemId")
    {
        scrp::Tokenizer tok("<!DOCTYPE html PUBLIC 'foo' '>x");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 3);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "foo", "", true);
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[1]), "x");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[2]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_doctype_system_identifier);
    }

    SECTION("<!DOCTYPE \\u0008")
    {
        scrp::Tokenizer tok("<!DOCTYPE \u0008");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "\u0008", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::control_character_in_input_stream);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE \\u0009")
    {
        scrp::Tokenizer tok("<!DOCTYPE \u0009");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE \\u000B")
    {
        scrp::Tokenizer tok("<!DOCTYPE \u000B");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "\u000B", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::control_character_in_input_stream);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE \\u001F")
    {
        scrp::Tokenizer tok("<!DOCTYPE \u001F");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "\u001F", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::control_character_in_input_stream);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE  ")
    {
        scrp::Tokenizer tok("<!DOCTYPE  ");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE !")
    {
        scrp::Tokenizer tok("<!DOCTYPE !");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "!", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE \"")
    {
        scrp::Tokenizer tok("<!DOCTYPE \"");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "\"", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE &")
    {
        scrp::Tokenizer tok("<!DOCTYPE &");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "&", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE \'")
    {
        scrp::Tokenizer tok("<!DOCTYPE \'");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "\'", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE -")
    {
        scrp::Tokenizer tok("<!DOCTYPE -");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "-", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE /")
    {
        scrp::Tokenizer tok("<!DOCTYPE /");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "/", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE 0")
    {
        scrp::Tokenizer tok("<!DOCTYPE 0");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "0", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE 1")
    {
        scrp::Tokenizer tok("<!DOCTYPE 1");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "1", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a =")
    {
        scrp::Tokenizer tok("<!DOCTYPE a =");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "=", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a ?")
    {
        scrp::Tokenizer tok("<!DOCTYPE a ?");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "?", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a @")
    {
        scrp::Tokenizer tok("<!DOCTYPE a @");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "@", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a A")
    {
        scrp::Tokenizer tok("<!DOCTYPE a A");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "A", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a B")
    {
        scrp::Tokenizer tok("<!DOCTYPE a B");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "B", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''\\u001F")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''\u001F");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 4);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::control_character_in_input_stream);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[3].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC'' ")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC'' ");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''!")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''!");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''\"")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''\"");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_between_doctype_public_and_system_identifiers);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''#")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''#");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''&")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''&");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC'''")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC'''");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_between_doctype_public_and_system_identifiers);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''(")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''(");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a PUBLIC''-")
    {
        scrp::Tokenizer tok("<!DOCTYPE a PUBLIC''-");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_quote_before_doctype_system_identifier);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a SYSTEM\"!")
    {
        scrp::Tokenizer tok("<!DOCTYPE a SYSTEM\"!");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "!", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_system_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a SYSTEM\"\"")
    {
        scrp::Tokenizer tok("<!DOCTYPE a SYSTEM\"\"");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_system_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a SYSTEM\"#")
    {
        scrp::Tokenizer tok("<!DOCTYPE a SYSTEM\"#");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "#", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_system_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a SYSTEM\"&")
    {
        scrp::Tokenizer tok("<!DOCTYPE a SYSTEM\"&");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "&", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_system_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPE a SYSTEM\"'")
    {
        scrp::Tokenizer tok("<!DOCTYPE a SYSTEM\"'");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "'", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_after_doctype_system_keyword);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPEa PUBLIC'>")
    {
        scrp::Tokenizer tok("<!DOCTYPEa PUBLIC'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 4);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::abrupt_doctype_public_identifier);
        CHECK(tok.get_parse_errors()[3].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPEa PUBLIC'?")
    {
        scrp::Tokenizer tok("<!DOCTYPEa PUBLIC'?");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "?", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPEa PUBLIC'@")
    {
        scrp::Tokenizer tok("<!DOCTYPEa PUBLIC'@");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "@", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPEa PUBLIC'A")
    {
        scrp::Tokenizer tok("<!DOCTYPEa PUBLIC'A");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "A", "", false);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::missing_whitespace_after_doctype_public_keyword);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("<!DOCTYPEa a")
    {
        scrp::Tokenizer tok("<!DOCTYPEa a");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 3);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "a", "a", "", true);
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_before_doctype_name);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::invalid_character_sequence_after_doctype_name);
        CHECK(tok.get_parse_errors()[2].type() == scrp::parser_error_type::eof_in_doctype);
    }

    SECTION("Doctype public case-sensitivity (1)")
    {
        scrp::Tokenizer tok(R"(<!DoCtYpE HtMl PuBlIc "AbC" "XyZ">)");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "AbC", "XyZ", false);
    }

    SECTION("Doctype public case-sensitivity (2)")
    {
        scrp::Tokenizer tok(R"(<!dOcTyPe hTmL pUbLiC "aBc" "xYz">)");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "aBc", "xYz", false);
    }

    SECTION("Doctype system case-sensitivity (1)")
    {
        scrp::Tokenizer tok("<!DoCtYpE HtMl SyStEm \"XyZ\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "XyZ", false);
    }

    SECTION("Doctype system case-sensitivity (2)")
    {
        scrp::Tokenizer tok("<!dOcTyPe hTmL sYsTeM \"xYz\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_DOCTYPE(scrp::Tokenizer::doctype_token_cast(tok.tokens()[0]), "html", "", "xYz", false);
    }
}

TEST_CASE("CDATA Test")
{
    scrp::initialize();
    scrp::parser test_parser;

    SECTION("CDATA")
    {
        scrp::Tokenizer tok("<![CDATA[<>abcdef:']]]>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        auto *cdata = scrp::Tokenizer::cdata_token_cast(tok.tokens()[0]);

        CHECK(cdata->cdata == "<>abcdef:']");
    }

    SECTION("CDATA with EOF")
    {
        scrp::Tokenizer tok("<![CDATA[<>abcdef:");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        auto *cdata = scrp::Tokenizer::cdata_token_cast(tok.tokens()[0]);

        CHECK(cdata->cdata == "<>abcdef:");
        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_cdata);
    }
}

TEST_CASE("HTML Comments")
{
    scrp::initialize();
    scrp::parser test_parser;

    SECTION("Simple comment")
    {
        scrp::Tokenizer tok("<!--comment-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "comment");

        /*{"description":"Simple comment",
"input":"<!--comment-->",
"output":[["Comment", "comment"]]},
*/
    }

    SECTION("Comment, Central dash no space")
    {
        scrp::Tokenizer tok("<!----->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "-");
        /*
           {"description":"Comment, Central dash no space",
               "input":"<!----->",
               "output":[["Comment", "-"]]},
               */
    }

    SECTION("Comment, two central dashes")
    {
        scrp::Tokenizer tok("<!-- --comment -->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " --comment ");

        /*
       {"description":"Comment, two central dashes",
           "input":"<!-- --comment -->",
           "output":[["Comment", " --comment "]]},
           */
    }

    SECTION("Comment, central less-than bang")
    {
        scrp::Tokenizer tok("<!--<!-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "<!");

        /*
       {"description":"Comment, central less-than bang",
           "input":"<!--<!-->",
           "output":[["Comment", "<!"]]},*/
    }

    SECTION("Unfinished comment")
    {
        scrp::Tokenizer tok("<!--comment");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "comment");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_comment);
        /*
            { "description" : "Unfinished comment",
                "input" : "<!--comment",
                "output" : [[ "Comment", "comment" ]],
                "errors" : [
                    { "code" : "eof-in-comment", "line" : 1, "col" : 12 }
                ] },*/
    }

    SECTION("Unfinished comment after start of nested comment")
    {
        scrp::Tokenizer tok("<!-- <!--");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <!-");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_comment);
        /*
                { "description" : "Unfinished comment after start of nested comment",
                    "input" : "<!-- <!--",
                    "output" : [[ "Comment", " <!" ]],
                    "errors" : [
                        { "code" : "eof-in-comment", "line" : 1, "col" : 10 }
                    ] },*/
    }

    SECTION("Start of a comment")
    {
        scrp::Tokenizer tok("<!-");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 2);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "-");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::incorrectly_opened_comment);
        CHECK(tok.get_parse_errors()[1].type() == scrp::parser_error_type::eof_in_comment);
        /*
                { "description" : "Start of a comment",
                    "input" : "<!-",
                    "output" : [[ "Comment", "-" ]],
                    "errors" : [
                        { "code" : "incorrectly-opened-comment", "line" : 1, "col" : 3 }
                    ] },*/
    }

    SECTION("Short comment")
    {
        scrp::Tokenizer tok("<!-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "");

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_closing_of_empty_comment);
        /*
                { "description" : "Short comment",
                    "input" : "<!-->",
                    "output" : [[ "Comment", "" ]],
                    "errors" : [
                        { "code" : "abrupt-closing-of-empty-comment", "line" : 1, "col" : 5 }
                    ] },*/
    }

    SECTION("Short comment two")
    {
        scrp::Tokenizer tok("<!--->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "");

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::abrupt_closing_of_empty_comment);
        /*
                { "description" : "Short comment two",
                    "input" : "<!--->",
                    "output" : [[ "Comment", "" ]],
                    "errors" : [
                        { "code" : "abrupt-closing-of-empty-comment", "line" : 1, "col" : 6 }
                    ] },*/
    }

    SECTION("Short comment three")
    {
        scrp::Tokenizer tok("<!---->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "");

        /*
                { "description" : "Short comment three",
                    "input" : "<!---->",
                    "output" : [[ "Comment", "" ]] },*/
    }

    SECTION("< in comment")
    {
        scrp::Tokenizer tok("<!-- <test-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <test");
        /*
                { "description" : "< in comment",
                    "input" : "<!-- <test-->",
                    "output" : [[ "Comment", " <test" ]] },*/
    }

    SECTION("<< in comment")
    {
        scrp::Tokenizer tok("<!--<<-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), "<<");

        /*
                { "description" : "<< in comment",
                    "input" : "<!--<<-->",
                    "output" : [[ "Comment", "<<" ]] },*/
    }

    SECTION("<! in comment")
    {
        scrp::Tokenizer tok("<!-- <!test-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <!test");

        /*
                { "description" : "<! in comment",
                    "input" : "<!-- <!test-->",
                    "output" : [[ "Comment", " <!test" ]] },*/
    }

    SECTION("<!- in comment")
    {
        scrp::Tokenizer tok("<!-- <!-test-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <!test");

        /*
                { "description" : "<!- in comment",
                    "input" : "<!-- <!-test-->",
                    "output" : [[ "Comment", " <!-test" ]] },*/
    }

    SECTION("Nested comment")
    {
        scrp::Tokenizer tok("<!-- <!--test-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <!--test");

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::nested_comment);
    }

    SECTION("Nested comment with extra <")
    {
        scrp::Tokenizer tok("<!-- <<!--test-->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[0]), " <<!--test");

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::nested_comment);
    }
}

TEST_CASE("Tag tests")
{
    auto CHECK_ATTRIBUTES = [&](scrp::TagToken *tok, scrp::sc_unordered_map<scrp::sc_string, scrp::sc_string> &&data) -> void {
        CHECK(tok->attributes == data);
    };

    scrp::initialize();
    scrp::parser test_parser;

    SECTION("Single Start Tag")
    {
        scrp::Tokenizer tok("<h>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
    }

    SECTION("Start/End Tag")
    {
        scrp::Tokenizer tok("<h></h>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 2);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
        CHECK_END_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), "h");
    }

    SECTION("Empty end tag")
    {
        scrp::Tokenizer tok("</>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_end_tag_name);
    }

    SECTION("Empty start tag")
    {
        scrp::Tokenizer tok("<>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[0]), "<>");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::invalid_first_character_of_tag_name);
    }

    SECTION("Start Tag w/attribute: <t a='b'>")
    {
        scrp::Tokenizer tok("<t a='b'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "t", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a", "b"}
        });
    }

    SECTION("Start Tag w/attribute: <t a=\"b\">")
    {
        scrp::Tokenizer tok("<t a=\"b\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "t", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a", "b"}
        });
    }

    SECTION("Start Tag w/attribute: <tg at='bb'>")
    {
        scrp::Tokenizer tok("<tg at='bb'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tg", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"at", "bb"}
        });
    }

    SECTION("Start Tag w/attribute: <tg at=\"bb\">")
    {
        scrp::Tokenizer tok("<tg at=\"bb\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tg", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"at", "bb"}
        });
    }

    SECTION("Start Tag w/attribute: <tag attr='ibute'>")
    {
        scrp::Tokenizer tok("<tag attr='ibute'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tag", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"attr", "ibute"}
        });
    }

    SECTION("Start Tag w/attribute: <tag attr=\"ibute\">")
    {
        scrp::Tokenizer tok("<tag attr=\"ibute\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tag", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"attr", "ibute"}
        });
    }

    SECTION("Start Tag w/attribute: <tag attr=\"ibute\">")
    {
        scrp::Tokenizer tok("<tag attr=\"ibute\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tag", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"attr", "ibute"}
        });
    }

    SECTION("Start Tag w/attribute: <tag a0=\"b0\" a1='b1' a2=\"b2\">")
    {
        scrp::Tokenizer tok(R"(<tag a0="b0" a1='b1' a2="b2">)");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "tag", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 3);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a0", "b0"},
                                                                              {"a1", "b1"},
                                                                              {"a2", "b2"}
        });
    }

    SECTION("Start Tag w/attribute no quote")
    {
        scrp::Tokenizer tok("<h a=b>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a", "b"}
        });
    }

    SECTION("Start Tag w/attributes no quote")
    {
        scrp::Tokenizer tok("<h a=b c=d>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a", "b"}, {"c", "d"}
        });
    }


    SECTION("Two unclosed start tags")
    {
        scrp::Tokenizer tok("<p>One<p>Two");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 5);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "p", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[1]), "One");
        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[2]), "p", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[2]));
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[3]), "Two");
        CHECK_EOF(scrp::Tokenizer::character_token_cast(tok.tokens()[4]));

    }

    SECTION("End Tag w/attribute")
    {
        scrp::Tokenizer tok("<h></h a='b'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
        CHECK_END_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), "h");

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::end_tag_with_attributes);

    }

    SECTION("Multiple atts no space")
    {
        scrp::Tokenizer tok("<h a='b'c='d'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);

        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {
                                                                              {"a", "b"},
                                                                              {"c", "d"}
        });

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_whitespace_between_attributes);
    }

    SECTION("Repeated attr")
    {
        scrp::Tokenizer tok("<h a='b' a='d'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);

        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", "b"}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::duplicate_attribute);
    }


    SECTION("Close attribute/Open attribute")
    {
        scrp::Tokenizer tok("</h><d a=b>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 2);

        CHECK_END_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h");
        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), "d", false);

        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), {{"a", "b"}});
    }

    SECTION("Start tag with no attributes but space before the greater-than sign")
    {
        scrp::Tokenizer tok("<h >");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 0);
    }

    SECTION("Empty attribute followed by uppercase attribute")
    {
        scrp::Tokenizer tok("<h a B=''>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}, {"b", ""}});
    }

    SECTION("Double-quote after attribute name")
    {
        scrp::Tokenizer tok("<h a \\\">");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}, {"\\\"", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_character_in_attribute_name);
    }

    SECTION("Single-quote after attribute name")
    {
        scrp::Tokenizer tok("<h a '>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "h", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}, {"\'", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_character_in_attribute_name);
    }

    SECTION("Empty end tag with following characters")
    {
        scrp::Tokenizer tok("a</>bc");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 2);

        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[0]), "abc");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[1]));


        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_end_tag_name);

    }

    SECTION("Empty end tag with following tag")
    {
        scrp::Tokenizer tok("a</><b>c");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 4);

        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[0]), "a");
        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), "b", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]));
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[2]), "c");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[3]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_end_tag_name);
    }


    SECTION("Empty end tag with following comment")
    {
        scrp::Tokenizer tok("a</><!--b-->c");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 4);

        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[0]), "a");
        CHECK_COMMENT(scrp::Tokenizer::comment_token_cast(tok.tokens()[1]), "b");
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[2]), "c");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[3]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_end_tag_name);
    }

    SECTION("Empty end tag with following end tag")
    {
        scrp::Tokenizer tok("a</></b>c");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 4);

        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[0]), "a");
        CHECK_END_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), "b");
        CHECK_CHARACTER(scrp::Tokenizer::character_token_cast(tok.tokens()[2]), "c");
        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[3]));

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_end_tag_name);
    }

    SECTION("<a \\u000C>")
    {
        scrp::Tokenizer tok("<a \u000C>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
    }

    SECTION("<a \\u000D>")
    {
        scrp::Tokenizer tok("<a \u000D>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_EMPTY_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]));
    }

    SECTION("<a \\u001F>")
    {
        scrp::Tokenizer tok("<a \u001F>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"\u001F", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::control_character_in_input_stream);
    }

    SECTION("<a (>")
    {
        scrp::Tokenizer tok("<a (>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), {{"(", ""}});
    }

    SECTION("<a ->")
    {
        scrp::Tokenizer tok("<a ->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), {{"-", ""}});
    }

    SECTION("<a .>")
    {
        scrp::Tokenizer tok("<a .>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[1]), {{".", ""}});
    }

     SECTION("<a />")
    {
       scrp::Tokenizer tok("<a />");
       tok.keep_tokens();
       tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", true);
    }

    SECTION("<a 0>")
    {
        scrp::Tokenizer tok("<a 0>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"0", ""}});

    }

    SECTION("<a 1>")
    {
        scrp::Tokenizer tok("<a 1>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"1", ""}});

    }

    SECTION("<a 2>")
    {
        scrp::Tokenizer tok("<a 2>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"2", ""}});

    }

    SECTION("<a <>")
    {
        scrp::Tokenizer tok("<a <>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"<", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_character_in_attribute_name);
    }

    SECTION("<a a #>")
    {
        scrp::Tokenizer tok("<a a #>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 2);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}, {"#", ""}});
    }

    SECTION("<a a#>")
    {
        scrp::Tokenizer tok("<a a#>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a#", ""}});
    }

    SECTION("<a a(>")
    {
        scrp::Tokenizer tok("<a a(>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a(", ""}});
    }

    SECTION("<a a->")
    {
        scrp::Tokenizer tok("<a a->");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a-", ""}});
    }


    SECTION("<a a'>")
    {
        scrp::Tokenizer tok("<a a'>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a'", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_character_in_attribute_name);
    }

    SECTION("<a a.>")
    {
        scrp::Tokenizer tok("<a a.>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a.", ""}});
    }

    SECTION("<a a/>")
    {
        scrp::Tokenizer tok("<a a/>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", true);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}});
    }

    SECTION("<a a0>")
    {
        scrp::Tokenizer tok("<a a0>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a0", ""}});
    }

    SECTION("<a a1>")
    {
        scrp::Tokenizer tok("<a a1>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a1", ""}});
    }

    SECTION("<a a9>")
    {
        scrp::Tokenizer tok("<a a9>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a9", ""}});
    }

    SECTION("<a a&>")
    {
        scrp::Tokenizer tok("<a a&>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().empty());
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a&", ""}});
    }


    SECTION("<a a<>")
    {
        scrp::Tokenizer tok("<a a<>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a<", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_character_in_attribute_name);
    }

    SECTION("<a a=>")
    {
        scrp::Tokenizer tok("<a a=>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::missing_attribute_value);
    }

    SECTION("<a a=\\u0008>")
    {
        scrp::Tokenizer tok("<a a=\u0008>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", "\u0008"}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::control_character_in_input_stream);
    }

    SECTION("<a/@>")
    {
        scrp::Tokenizer tok("<a/@>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"@", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_solidus_in_tag);
    }

    SECTION("<a/A>")
    {
        scrp::Tokenizer tok("<a/A>");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_TAG(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), "a", false);
        CHECK_ATTRIBUTES_SIZE(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), 1);
        CHECK_ATTRIBUTES(scrp::Tokenizer::tag_token_cast(tok.tokens()[0]), {{"a", ""}});

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::unexpected_solidus_in_tag);
    }

    SECTION("CR EOF in tag name")
    {
        scrp::Tokenizer tok("<z\\r");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("Slash EOF in tag name")
    {
        scrp::Tokenizer tok("<z/");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }



    SECTION("EOF in attribute value (single quoted) state")
    {
        scrp::Tokenizer tok("<a a ='a");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("EOF in attribute value (double quoted) state")
    {
        scrp::Tokenizer tok("<a a =\"a");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("EOF in after attribute value state")
    {
        scrp::Tokenizer tok("<a a ='a'");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("EOF in after attribute name state")
    {
        scrp::Tokenizer tok("<a a =");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("EOF in attribute name state")
    {
        scrp::Tokenizer tok("<a a");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }

    SECTION("EOF in before attribute name state")
    {
        scrp::Tokenizer tok("<a ");
        tok.keep_tokens();
        tok.set_parser(&test_parser);

        REQUIRE(tok.tokenize() == true);
        REQUIRE(tok.get_parse_errors().size() == 1);
        REQUIRE(tok.tokens().size() == 1);

        CHECK_EOF(scrp::Tokenizer::eof_token_cast(tok.tokens()[0]))

        CHECK(tok.get_parse_errors()[0].type() == scrp::parser_error_type::eof_in_tag);
    }



}
