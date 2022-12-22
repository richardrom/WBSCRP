/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 20/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//


#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <tokenizer.hpp>


TEST_CASE("Scrapper Tokenizer")
{
    scrp::initialize();
    scrp::Tokenizer tok("");
    CHECK(tok.tokenize() == false);
}
