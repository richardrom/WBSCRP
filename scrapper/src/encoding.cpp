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

auto scrp::encoding:: rt_to_utf8(uint32_t utf16) -> utf8_pair
{
    if (utf16 < 0x007F)
    {
        return { static_cast<uint32_t>(utf16), 1 };
    }
    else if (utf16 >= 0x0080 && utf16 <= 0x07FF)
    {
        return { static_cast<uint32_t>((0xC000 | ((utf16 & 0x7C0) << 2))
                                       | (0x80 | (utf16 & 0x3F))),
            2 };
    }
    else if (utf16 >= 0x0800 && utf16 <= 0xFFFF)
    {
        return { static_cast<uint32_t>((0xE00000 | ((utf16 & 0xF000) << 4))
                                       | (0x8000 | ((utf16 & 0xFC0) << 2))
                                       | (0x80 | (utf16 & 0x3F))),
            3 };
    }
    else if (utf16 >= 0x10000 && utf16 <= 0x10FFFF)
    {
        return { static_cast<uint32_t>((0xF0000000 | ((utf16 & 0x1C0000) << 6))
                                       | (0x800000 | ((utf16 & 0x3F000) << 4))
                                       | (0x8000 | ((utf16 & 0xFC0) << 2))
                                       | (0x80 | (utf16 & 0x3F))),
            4 };
    }
    return { static_cast<uint32_t>(utf16), 0 };
}

auto scrp::encoding::rt_to_view(const utf8_pair &_utf8_pair) -> sc_string
{
    sc_string _data;
    _data.resize(_utf8_pair.second);
    if (_utf8_pair.second == 1)
    {
        _data[0] = static_cast<char>(_utf8_pair.first);
        _data[1] = 0;
    }
    else if (_utf8_pair.second == 2)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            _data[0] = (0xFF00 & _utf8_pair.first) >> 8;
            _data[1] = 0x00FF & _utf8_pair.first;
            _data[2] = 0;
        }
        else
        {
            _data[0] = 0x00FF & _utf8_pair.first;
            _data[1] = (0xFF00 & _utf8_pair.first) >> 8;
            _data[2] = 0;
        }
    }
    else if (_utf8_pair.second == 3)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            _data[0] = (0xFF0000 & _utf8_pair.first) >> 16;
            _data[1] = (0x00FF00 & _utf8_pair.first) >> 8;
            _data[2] = (0x0000FF & _utf8_pair.first);
            _data[3] = 0;
        }
        else
        {
            _data[0] = (0x0000FF & _utf8_pair.first);
            _data[1] = (0x00FF00 & _utf8_pair.first) >> 8;
            _data[2] = (0xFF0000 & _utf8_pair.first) >> 16;
            _data[3] = 0;
        }
    }
    else if (_utf8_pair.second == 4)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            _data[0] = (0xFF000000 & _utf8_pair.first) >> 24;
            _data[1] = (0x00FF0000 & _utf8_pair.first) >> 16;
            _data[2] = (0x0000FF00 & _utf8_pair.first) >> 8;
            _data[3] = (0x000000FF & _utf8_pair.first);
            _data[4] = 0;
        }
        else
        {
            _data[0] = (0x000000FF & _utf8_pair.first);
            _data[1] = (0x0000FF00 & _utf8_pair.first) >> 8;
            _data[2] = (0x00FF0000 & _utf8_pair.first) >> 16;
            _data[3] = (0xFF000000 & _utf8_pair.first) >> 24;
            _data[4] = 0;
        }
    }
    return _data;
}