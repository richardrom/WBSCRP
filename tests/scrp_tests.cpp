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
    for ( const auto &p : ref_index)
    {
        std::string scrmp {scrp::encoding::chref_table[p].data};
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


TEST_CASE("Character reference")
{
    scrp::initialize();
    scrp::Tokenizer tok("<,ds>");
    CHECK(tok.tokenize() == true);
}