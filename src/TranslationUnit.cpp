#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "types.hpp"
#include "TranslationUnit.h"

unsigned char* TranslationUnit2Binary(const char* source, u32 len) {
	u8* Binary = new u8[len];

	Vector<String> Instructions;
	for (u32 i = 0; i < len; i++) {
		if (source[i] == '\n') { // process instruction here.
			for (String& instruction : Instructions) {
				printf("Instruction: %s\n", instruction.c_str());
			}
		}
		if (source[i] == '\0') break;
		if (std::isspace(source[i])) continue;

		u32 symbolLen = 0;
		while (!std::isspace(source[i + symbolLen])) {
			symbolLen++;
		}

		if (symbolLen > 0) {
			Instructions.push_back(String(source + i, symbolLen));
			i += symbolLen;
		}
	}

	return Binary;
}
