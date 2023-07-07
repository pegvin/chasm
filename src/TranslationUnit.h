#ifndef CHASM_TRANSLATION_UNIT_H_INCLUDED_
#define CHASM_TRANSLATION_UNIT_H_INCLUDED_ 1
#pragma once

#include "types.hpp"

Vector<u16>* TranslationUnit2Binary(const char* source, unsigned int len);

#endif // CHASM_TRANSLATION_UNIT_H_INCLUDED_
