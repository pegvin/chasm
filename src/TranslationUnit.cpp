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
		} else if (Token == "jp" || Token == "call") { // JP addr OR CALL addr
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
		} else if (
			Token == "se"   || // SE  Vx, byte
			Token == "sne"  || // SNE Vx, byte
			Token == "ld"   || // LD  Vx, byte
			Token == "add"  || // ADD Vx, byte
			Token == "or"   || // OR  Vx, Vy
			Token == "and"  || // AND Vx, Vy
			Token == "xor"  || // XOR Vx, Vy
			Token == "sub"  || // AND Vx, Vy
			Token == "shr"  || // SHR Vx {, Vy}
			Token == "subn" || // SUBN Vx, Vy
			Token == "shl"     // SHL Vx {, Vy}
		) {
			if (Token == "se") OpCode = 0x3000;
			else if (Token == "sne") OpCode = 0x4000;
			else if (Token == "ld") OpCode = 0x6000;
			else if (Token == "add") OpCode = 0x7000;

			const String& Arg1 = Tokens[i + 1];
			const String& Arg2 = Tokens[i + 2];

			/* Handles:
				SE   Vx, Vy
				LD   Vx, Vy
				OR   Vx, Vy
				AND  Vx, Vy
				XOR  Vx, Vy
				ADD  Vx, Vy
				SUB  Vx, Vy
				SHR  Vx {, Vy}
				SUBN Vx, Vy
				SHL  Vx {, Vy}
				SNE  Vx, Vy
			*/
			if (Arg1[0] == 'v' && Arg2[0] == 'v') {
				if      (Token == "se")   OpCode = 0x5000;
				else if (Token == "ld")   OpCode = 0x8000;
				else if (Token == "or")   OpCode = 0x8001;
				else if (Token == "and")  OpCode = 0x8002;
				else if (Token == "xor")  OpCode = 0x8003;
				else if (Token == "add")  OpCode = 0x8004;
				else if (Token == "sub")  OpCode = 0x8005;
				else if (Token == "shr")  OpCode = 0x8006;
				else if (Token == "subn") OpCode = 0x8007;
				else if (Token == "shl")  OpCode = 0x800E;
				else if (Token == "sne")  OpCode = 0x9000;
				else {
					printf("Invalid Token: '%s'\n", Token.c_str());
					exit(1);
				}

				u8 Register1 = strtol(Arg1.substr(1, 1).c_str(), NULL, 16);
				u8 Register2 = strtol(Arg2.substr(1, 1).c_str(), NULL, 16);
				OpCode = ((OpCode >> 8) | (Register1 & 0x0F)) << 8;
				OpCode = OpCode | ((Register2 & 0x0F) << 4);
				if      (Token == "or")   OpCode = OpCode | (1   & 0x0F);
				else if (Token == "and")  OpCode = OpCode | (2   & 0x0F);
				else if (Token == "xor")  OpCode = OpCode | (3   & 0x0F);
				else if (Token == "add")  OpCode = OpCode | (4   & 0x0F);
				else if (Token == "sub")  OpCode = OpCode | (5   & 0x0F);
				else if (Token == "shr")  OpCode = OpCode | (6   & 0x0F);
				else if (Token == "subn") OpCode = OpCode | (7   & 0x0F);
				else if (Token == "shl")  OpCode = OpCode | (0xE & 0x0F);
			} else { // SE/SNE Vx, byte
				u8 RegisterIndex = strtol(Arg1.substr(1, 1).c_str(), NULL, 16);
				u8 ByteVal = 0;

				if (Arg2.rfind("0x", 0) == 0) {
					ByteVal = strtol(Arg2.substr(2, 2).c_str(), NULL, 16);
				} else if (std::isdigit(Arg2[0])) {
					for (char c : Arg2) {
						if (!std::isdigit(c)) {
							printf("Invalid Number Notation: '%s'\n", Arg2.c_str());
							exit(1);
						}
					}
					ByteVal = strtol(Arg2.c_str(), NULL, 10);
				} else {
					printf("Invalid Number Notation: '%s'\n", Arg2.c_str());
					exit(1);
				}

				OpCode = ((OpCode >> 8) | (RegisterIndex & 0x0F)) << 8;
				OpCode = OpCode | ByteVal;
			}

			Binary[Index] = OpCode >> 8;
			Binary[Index + 1] = OpCode & 0x00FF;
			Index += 2;
			i += 2;
		} else {
			printf("Invalid Token: '%s'\n", Token.c_str());
			exit(1);
		}
		printf("OpCode: %04X\n", OpCode);
	}

	return Binary;
}
