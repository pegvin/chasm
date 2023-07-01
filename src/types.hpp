#ifndef CHASM_TYPES_HPP_INCLUDED_
#define CHASM_TYPES_HPP_INCLUDED_ 1
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <limits>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;

using f32 = float;
using f64 = double;

using String = std::string;

template<typename Type>
using Vector = std::vector<Type>;

#endif // CHASM_TYPES_HPP_INCLUDED_
