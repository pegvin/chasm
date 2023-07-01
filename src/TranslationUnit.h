#ifndef CHASM_TRANSLATION_UNIT_H_INCLUDED_
#define CHASM_TRANSLATION_UNIT_H_INCLUDED_ 1
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* TranslationUnit2Binary(const char* source, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif // CHASM_TRANSLATION_UNIT_H_INCLUDED_
