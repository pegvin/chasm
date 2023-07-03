#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <cctype>
#include <string>

#include "types.hpp"
#include "TranslationUnit.h"

unsigned char* TranslationUnit2Binary(const char* source, u32 len) {
	Vector<String> Tokens;

	// Converts Source Code Into Tokens, And Removes Any
	for (u32 i = 0; i < len; i++) {
		if (source[i] == '\0') break;
		if (std::isspace(source[i])) continue;
		if (source[i] == ';') {
			u32 commentLen = 0;
			while (source[i + commentLen] != '\n') {
				commentLen++;
			}
			i += commentLen;
		}

		u32 symbolLen = 0;
		while (!std::isspace(source[i + symbolLen])) {
			symbolLen++;
		}

		if (symbolLen > 0) {
			auto Tok = String(source + i, symbolLen);
			std::transform(Tok.begin(), Tok.end(), Tok.begin(), [](unsigned char c){ return std::tolower(c); });
			Tokens.push_back(Tok);
			i += symbolLen;
		}
	}

	u8* Binary = new u8[len];
	u32 Index = 0;
	for (std::size_t i = 0; i < Tokens.size(); i++) {
		String& Token = Tokens[i];
		u16 OpCode;
		if (Token == "cls") {
			OpCode = 0x00E0;
			Binary[Index] = OpCode >> 8;
			Binary[Index + 1] = OpCode & 0x00FF;
			Index += 2;
		} else if (Token == "ret") {
			OpCode = 0x00EE;
			Binary[Index] = OpCode >> 8;
			Binary[Index + 1] = OpCode & 0x00FF;
			Index += 2;
		} else if (Token == "jp" || Token == "call") {
			OpCode = Token == "jp" ? 0x1000 : 0x2000; // 000 is the address to jump to and will be parsed now.
			String AddrTok = Tokens[i + 1];
			if (AddrTok.empty() || AddrTok.rfind("0x", 0) != 0) {
				printf("Invalid Address Notation: '%s'\n", AddrTok.c_str());
				exit(1);
			}
			u16 Addr = strtol(AddrTok.substr(2, 3).c_str(), NULL, 16);
			OpCode += Addr;
			Binary[Index] = OpCode >> 8;
			Binary[Index + 1] = OpCode & 0x00FF;
			Index += 2;
			i += 1;
		} else if (Token == "se" || Token == "sne" || Token == "ld" || Token == "add") {
			if (Token == "se") OpCode = 0x3000;
			else if (Token == "sne") OpCode = 0x4000;
			else if (Token == "ld") OpCode = 0x6000;
			else if (Token == "add") OpCode = 0x7000;

			String Register = Tokens[i + 1];
			String Byte = Tokens[i + 2];

			if (Byte[0] == 'v') { // SE Vx, Vy
				OpCode = 0x5000;
				u8 Register1 = strtol(Register.substr(1, 1).c_str(), NULL, 16);
				u8 Register2 = strtol(Byte.substr(1, 1).c_str(), NULL, 16);
				OpCode = ((OpCode >> 8) | (Register1 & 0x0F)) << 8;
				OpCode = OpCode | ((Register2 & 0x0F) << 4);
			} else { // SE/SNE Vx, byte
				u8 RegisterIndex = strtol(Register.substr(1, 1).c_str(), NULL, 16);
				u8 ByteVal = 0;

				if (Byte.rfind("0x", 0) == 0) {
					ByteVal = strtol(Byte.substr(2, 2).c_str(), NULL, 16);
				} else if (std::isdigit(Byte[0])) {
					for (char c : Byte) {
						if (!std::isdigit(c)) {
							printf("Invalid Number Notation: '%s'\n", Byte.c_str());
							exit(1);
						}
					}
					ByteVal = strtol(Byte.c_str(), NULL, 10);
				} else {
					printf("Invalid Number Notation: '%s'\n", Byte.c_str());
					exit(1);
				}

				OpCode = ((OpCode >> 8) | (RegisterIndex & 0x0F)) << 8;
				OpCode = OpCode | ByteVal;
			}

			Binary[Index] = OpCode >> 8;
			Binary[Index + 1] = OpCode & 0x00FF;
			Index += 2;
			i += 2;
			printf("OpCode: %X\n", OpCode);
		} else {
			printf("Invalid Token: %s\n", Token.c_str());
		}
	}

	return Binary;
}
