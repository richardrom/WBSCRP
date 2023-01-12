/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 11/01/23.
// Copyright (c) 2023 Ricardo Romero.  All rights reserved.
//

#include "encoding_character_reference.hpp"
#include "crc64.hpp"

const scrp::encoding::character_reference &scrp::encoding::find_reference(std::string_view reference)
{
    if (reference.size() < min_reference_size || chref_table.empty())
        return null_chref;


    if (chref_table.size() == 1)
    {
        // Case were chref_table might be changed by error
        // Otherwise, this code will never be reached
        if ( chref_table[0].data == reference )
            return chref_table[0];
        else
            return null_chref;
    }

    uint64_t crc = crc64::compute(reference.data(), reference.size());
    // Do a binary search, the precondition is that chref_table is sorted by the crc field
    int64_t lw = 0, hg = chref_table.size() - 1;
    while( lw <= hg )
    {
        int64_t partition = lw + ( hg - lw) / 2;

        if( chref_table[partition].crc64 == crc )
            return chref_table[partition];
        else
        {
            if( chref_table[partition].crc64 < crc )
                lw = partition + 1;
            else
                hg = partition - 1;
        }
    }

    return null_chref;
}