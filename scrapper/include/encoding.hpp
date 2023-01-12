/////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ricardo Romero on 22/12/22.
// Copyright (c) 2022 Ricardo Romero.  All rights reserved.
//

#pragma once

#ifndef __cplusplus
#error "C++ compiler needed"
#endif /*__cplusplus*/

#ifndef WBSCRP_ENCODING_HPP
#define WBSCRP_ENCODING_HPP

#include "scrapper.hpp"


namespace scrp::encoding
{
    using utf8_pair = std::pair<uint32_t, uint32_t>;


    template <uint32_t utf16>
    constexpr auto to_utf8() -> utf8_pair
    {
        if constexpr (utf16 < 0x007F)
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

    template <utf8_pair _utf8_pair>
    struct to_view
    {
        constexpr to_view()
        {
            if constexpr (_utf8_pair.second == 1)
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
        }

        const std::size_t _size { _utf8_pair.second + 1 };
        char _data[_utf8_pair.second + 1] { 0 };
    };

    template <utf8_pair _utf8_pair_0, utf8_pair _utf8_pair_1>
    struct to_d_view
    {
        constexpr to_d_view()
        {
            if constexpr (_utf8_pair_0.second == 1)
            {
                _data[0] = static_cast<char>(_utf8_pair_0.first);
                _data[1] = 0;
            }
            else if (_utf8_pair_0.second == 2)
            {
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[0] = (0xFF00 & _utf8_pair_0.first) >> 8;
                    _data[1] = 0x00FF & _utf8_pair_0.first;
                    _data[2] = 0;
                }
                else
                {
                    _data[0] = 0x00FF & _utf8_pair_0.first;
                    _data[1] = (0xFF00 & _utf8_pair_0.first) >> 8;
                    _data[2] = 0;
                }
            }
            else if (_utf8_pair_0.second == 3)
            {
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[0] = (0xFF0000 & _utf8_pair_0.first) >> 16;
                    _data[1] = (0x00FF00 & _utf8_pair_0.first) >> 8;
                    _data[2] = (0x0000FF & _utf8_pair_0.first);
                    _data[3] = 0;
                }
                else
                {
                    _data[0] = (0x0000FF & _utf8_pair_0.first);
                    _data[1] = (0x00FF00 & _utf8_pair_0.first) >> 8;
                    _data[2] = (0xFF0000 & _utf8_pair_0.first) >> 16;
                    _data[3] = 0;
                }
            }
            else if (_utf8_pair_0.second == 4)
            {
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[0] = (0xFF000000 & _utf8_pair_0.first) >> 24;
                    _data[1] = (0x00FF0000 & _utf8_pair_0.first) >> 16;
                    _data[2] = (0x0000FF00 & _utf8_pair_0.first) >> 8;
                    _data[3] = (0x000000FF & _utf8_pair_0.first);
                    _data[4] = 0;
                }
                else
                {
                    _data[0] = (0x000000FF & _utf8_pair_0.first);
                    _data[1] = (0x0000FF00 & _utf8_pair_0.first) >> 8;
                    _data[2] = (0x00FF0000 & _utf8_pair_0.first) >> 16;
                    _data[3] = (0xFF000000 & _utf8_pair_0.first) >> 24;
                    _data[4] = 0;
                }
            }

            if constexpr (_utf8_pair_1.second == 1)
            {
                _data[_utf8_pair_1.second + 0] = static_cast<char>(_utf8_pair_1.first);
                _data[_utf8_pair_1.second + 1] = 0;
            }
            else if (_utf8_pair_1.second == 2)
            {
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[_utf8_pair_0.second + 0] = (0xFF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 1] = 0x00FF & _utf8_pair_1.first;
                    _data[_utf8_pair_0.second + 2] = 0;
                }
                else
                {
                    _data[_utf8_pair_0.second + 0] = 0x00FF & _utf8_pair_1.first;
                    _data[_utf8_pair_0.second + 1] = (0xFF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 2] = 0;
                }
            }
            else if (_utf8_pair_1.second == 3)
            {
                // E2 82 AC
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[_utf8_pair_0.second + 0] = (0xFF0000 & _utf8_pair_1.first) >> 16;
                    _data[_utf8_pair_0.second + 1] = (0x00FF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 2] = (0x0000FF & _utf8_pair_1.first);
                    _data[_utf8_pair_0.second + 3] = 0;
                }
                else
                {
                    _data[_utf8_pair_0.second + 0] = (0x0000FF & _utf8_pair_1.first);
                    _data[_utf8_pair_0.second + 1] = (0x00FF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 2] = (0xFF0000 & _utf8_pair_1.first) >> 16;
                    _data[_utf8_pair_0.second + 3] = 0;
                }
            }
            else if (_utf8_pair_1.second == 4)
            {
                if constexpr (std::endian::native == std::endian::little)
                {
                    _data[_utf8_pair_0.second + 0] = (0xFF000000 & _utf8_pair_1.first) >> 24;
                    _data[_utf8_pair_0.second + 1] = (0x00FF0000 & _utf8_pair_1.first) >> 16;
                    _data[_utf8_pair_0.second + 2] = (0x0000FF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 3] = (0x000000FF & _utf8_pair_1.first);
                    _data[_utf8_pair_0.second + 4] = 0;
                }
                else
                {
                    _data[_utf8_pair_0.second + 0] = (0x000000FF & _utf8_pair_1.first);
                    _data[_utf8_pair_0.second + 1] = (0x0000FF00 & _utf8_pair_1.first) >> 8;
                    _data[_utf8_pair_0.second + 2] = (0x00FF0000 & _utf8_pair_1.first) >> 16;
                    _data[_utf8_pair_0.second + 3] = (0xFF000000 & _utf8_pair_1.first) >> 24;
                    _data[_utf8_pair_0.second + 4] = 0;
                }
            }
        }

        const std::size_t _size { _utf8_pair_0.second + _utf8_pair_1.second + 1 };
        char _data[_utf8_pair_0.second + _utf8_pair_1.second + 1] { 0 };
    };

}

#endif // WBSCRP_ENCODING_HPP
