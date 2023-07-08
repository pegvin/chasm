#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>

#include "TranslationUnit.h"

// converts 'Vx' to a register index.
u8  ParseRegisterNotation(const String& str, bool* _errFlag = nullptr) {
	u8 result = 0;
	if (std::tolower(str[0]) != 'v') {
		*_errFlag = true;
	} else {
		result = std::strtol(str.substr(1, 1).c_str(), NULL, 16);
		if (errno == EINVAL && result == 0 && _errFlag != nullptr) *_errFlag = true;
	}

	return result;
}

// parses '0xXXX' '124' '0b101100' etc.
u16 ParseNumberNotation(const String& str, bool* _errFlag = nullptr) {
	u16 value = 0;
	if (str.rfind("0x", 0) == 0) { // try to parse as hexadecimal
		value = std::strtol(str.substr(2, 3).c_str(), NULL, 16);
		/*                                     ^                  ^
		                               3 Hex Digits (12 Bits)   Base 16 */
		if (errno == EINVAL && value == 0 && _errFlag != nullptr) *_errFlag = true;
	} else if (str.rfind("0b", 0) == 0) { // try to parse as binary
		value = std::strtol(str.substr(2, 12).c_str(), NULL, 2);
		/*                                     ^                  ^
		                                     12 Bits            Base 2  */
		if (errno == EINVAL && value == 0 && _errFlag != nullptr) *_errFlag = true;
	} else { // try to parse as base-10 number
		value = std::strtol(str.c_str(), NULL, 10);
		/*                                          ^
		                                         Base 10 */
		if (errno == EINVAL && value == 0 && _errFlag != nullptr) *_errFlag = true;
	}
	return value;
}

struct Token {
	String val  = "";
	u32    line = 0;
	u32    cols = 0;
};

typedef Vector<Token> LineTokenized;

Vector<u16>* TranslationUnit2Binary(const char* fileName, const char* source, u32 len) {
	Vector<String> Lines;
	Vector<LineTokenized> LinesParsed;

	if (source != NULL) {
		std::stringstream ss(source);
		std::string line;
		while (std::getline(ss, line, '\n')) {
			Lines.emplace_back(line);
		}
	}

	for (u32 LineIndex = 0; LineIndex < Lines.size(); LineIndex++) {
		String& Line = Lines[LineIndex];
		LineTokenized LineParsed;

		for (u32 CharacterIndex = 0; CharacterIndex < Line.size(); CharacterIndex++) {
			if (Line[CharacterIndex] == ';') {
				Line.erase(Line.begin() + CharacterIndex, Line.end());
				break;
			}
		}

		if (Line.empty()) continue;

		for (u32 CharacterIndex = 0; CharacterIndex < Line.size(); CharacterIndex++) {
			u32 SubStrPos = CharacterIndex;
			while (SubStrPos < Line.size() && !std::isspace(Line[SubStrPos])) {
				SubStrPos++;
			}

			if (SubStrPos > CharacterIndex) {
				struct Token t;
				t.val = Line.substr(CharacterIndex, SubStrPos - CharacterIndex),
				t.line = LineIndex;
				t.cols = CharacterIndex;
				LineParsed.push_back(t);
				CharacterIndex = SubStrPos;
			}
		}

		if (LineParsed.size() > 0) {
			LinesParsed.push_back(LineParsed);
		}
	}

	// printf("-----\n");
	for (LineTokenized& LineParsed : LinesParsed) {
		for (Token& Token : LineParsed) {
			// printf("Token: %6s - Position: %d:%d\n", Token.val.c_str(), Token.line, Token.cols);
			std::transform(
				Token.val.begin(), Token.val.end(), Token.val.begin(),
				[](unsigned char c) { return std::tolower(c); }
			);
		}
		// printf("-----\n");
	}

	Vector<u16>* Binary = new Vector<u16>();

	#define CHECK_TOO_MANY_ARGS() do { if (TokenI != (LineParsed.size() - 1)) goto TooManyArgsError; } while (0)
	#define CHECK_TOO_FEW_ARGS(numExpected) do { \
		ArgsExpected = numExpected; \
		if (ArgsExpected > LineParsed.size() - 1) goto TooFewArgsError; \
	} while(0)

	for (const LineTokenized& LineParsed : LinesParsed) {
		for (u32 TokenI = 0; TokenI < LineParsed.size(); TokenI++) {
			u16 ArgsExpected = 0;
			u16 OpCode;
			const struct Token& Token = LineParsed[TokenI];

			if (Token.val == "cls") {
				CHECK_TOO_FEW_ARGS(0);
				Binary->push_back(0x00E0);
			} else if (Token.val == "ret") {
				CHECK_TOO_FEW_ARGS(0);
				Binary->push_back(0x00EE);
			} else if (
				Token.val == "sys"  ||
				Token.val == "jp"   ||
				Token.val == "call" ||
				Token.val == "ldi"  ||
				Token.val == "jp0"  ||
				Token.val == "skp"  ||
				Token.val == "sknp" ||
				Token.val == "dtld" ||
				Token.val == "ldk"  ||
				Token.val == "lddt" ||
				Token.val == "ldst" ||
				Token.val == "addi" ||
				Token.val == "ldf"  ||
				Token.val == "ldb"  ||
				Token.val == "ld[i]"||
				Token.val == "[i]ld"||
				false
			) {
				CHECK_TOO_FEW_ARGS(1);
				const struct Token& Arg1 = LineParsed[TokenI + 1];

				bool errorOccurred1 = false;
				u16 Arg1Num = ParseNumberNotation(Arg1.val, &errorOccurred1);

				bool errorOccurred2 = false;
				u16 Arg1Register = ParseRegisterNotation(Arg1.val, &errorOccurred2);

				if (!errorOccurred1 && errorOccurred2) { // if first arg is 'nnn'
					if      (Token.val == "sys")  OpCode = 0x0000;
					else if (Token.val == "jp")   OpCode = 0x1000;
					else if (Token.val == "call") OpCode = 0x2000;
					else if (Token.val == "ldi")  OpCode = 0xA000;
					else if (Token.val == "jp0")  OpCode = 0xB000;
					else goto InvalidArgumentError;

					OpCode = OpCode | (Arg1Num & 0x0FFF);
				} else if (!errorOccurred2) {
					if      (Token.val == "skp")  OpCode = 0xE09E;
					else if (Token.val == "sknp") OpCode = 0xE0A1;
					else if (Token.val == "dtld") OpCode = 0xF007;
					else if (Token.val == "ldk")  OpCode = 0xF00A;
					else if (Token.val == "lddt") OpCode = 0xF015;
					else if (Token.val == "ldst") OpCode = 0xF018;
					else if (Token.val == "addi") OpCode = 0xF01E;
					else if (Token.val == "ldf")  OpCode = 0xF029;
					else if (Token.val == "ldb")  OpCode = 0xF033;
					else if (Token.val == "ld[i]")OpCode = 0xF055;
					else if (Token.val == "[i]ld")OpCode = 0xF065;
					else goto InvalidArgumentError;

					OpCode = OpCode | (Arg1Register << 8) | (OpCode & 0x00FF);
				} else goto UnknownIdentifierError;

				Binary->push_back(OpCode);
				TokenI++; // increment index by 1 since we used that as our argument.
			} else if (
				Token.val == "se"   ||
				Token.val == "sne"  ||
				Token.val == "ld"   ||
				Token.val == "add"  ||
				Token.val == "or"   ||
				Token.val == "and"  ||
				Token.val == "xor"  ||
				Token.val == "sub"  ||
				Token.val == "shr"  ||
				Token.val == "subn" ||
				Token.val == "shl"  ||
				Token.val == "rnd"  ||
				false
			) {
				CHECK_TOO_FEW_ARGS(2);
				const struct Token& Arg1 = LineParsed[TokenI + 1];
				const struct Token& Arg2 = LineParsed[TokenI + 2];

				bool errorOccurred1 = false;
				u16 RegisterX = ParseRegisterNotation(Arg1.val, &errorOccurred1);

				bool errorOccurred2 = false;
				u16 RegisterY = ParseRegisterNotation(Arg2.val, &errorOccurred2);

				bool errorOccurred3 = false;
				u16 Arg2Num = ParseNumberNotation(Arg2.val, &errorOccurred3);

				if (!errorOccurred1 && !errorOccurred2) { // if arguments are Vx, Vy
					if (Token.val == "se")        OpCode = 0x5000;
					else if (Token.val == "sne")  OpCode = 0x9000;
					else if (Token.val == "ld")   OpCode = 0x8000;
					else if (Token.val == "add")  OpCode = 0x8004;
					else if (Token.val == "or")   OpCode = 0x8001;
					else if (Token.val == "and")  OpCode = 0x8002;
					else if (Token.val == "xor")  OpCode = 0x8003;
					else if (Token.val == "sub")  OpCode = 0x8005;
					else if (Token.val == "shr")  OpCode = 0x8006;
					else if (Token.val == "subn") OpCode = 0x8007;
					else if (Token.val == "shl")  OpCode = 0x800E;
					else goto UnknownIdentifierError;

					OpCode = (((OpCode >> 8) | (RegisterX & 0x0F)) << 8) | (OpCode & 0x000F);
					OpCode = OpCode | (RegisterY << 4);
				} else if (!errorOccurred1 && !errorOccurred3) { // arguments are Vx, byte
					if (Token.val == "se")       OpCode = 0x3000;
					else if (Token.val == "sne") OpCode = 0x4000;
					else if (Token.val == "ld")  OpCode = 0x6000;
					else if (Token.val == "add") OpCode = 0x7000;
					else if (Token.val == "rnd") OpCode = 0xC000;
					else goto UnknownIdentifierError;

					OpCode = ((OpCode >> 8) | (RegisterX & 0x0F)) << 8;
					OpCode = OpCode | (Arg2Num & 0x00FF);
				} else {
					if (errorOccurred1) TokenI += 1;
					else if (errorOccurred2 || errorOccurred3) TokenI += 2;
					goto UnknownIdentifierError;
				}

				Binary->push_back(OpCode);
				TokenI += 2; // consumed 2 arguments
			} else if (Token.val == "drw") {
				CHECK_TOO_FEW_ARGS(3);
				OpCode = 0xD000;

				const struct Token& Arg1 = LineParsed[TokenI + 1];
				const struct Token& Arg2 = LineParsed[TokenI + 2];
				const struct Token& Arg3 = LineParsed[TokenI + 3];

				bool errorOccurred1 = false;
				u16 RegisterX = ParseRegisterNotation(Arg1.val, &errorOccurred1);

				bool errorOccurred2 = false;
				u16 RegisterY = ParseRegisterNotation(Arg2.val, &errorOccurred2);

				bool errorOccurred3 = false;
				u16 Arg3Num = ParseNumberNotation(Arg3.val, &errorOccurred3);

				if (!errorOccurred1 && !errorOccurred2 && !errorOccurred3) {
					OpCode = OpCode | ((RegisterX & 0x000F) << 8);
					OpCode = OpCode | ((RegisterY & 0x000F) << 4);
					OpCode = OpCode | ((Arg3Num & 0x000F) << 0);
				}

				Binary->push_back(OpCode);
				TokenI += 3;
			} else {
				goto UnknownIdentifierError;
			}

			CHECK_TOO_MANY_ARGS();

			continue; // by default skip the below code since we only want to run it via 'goto'.

			// shows an unknown identifier error for the symbol in at "TokenI" index
			UnknownIdentifierError: {
				const auto& Tok = LineParsed[TokenI];
				u32 TokLineNumChars = std::to_string(Tok.line).size();
				printf("%s:%d:%d: error: unknown identifier '%s'\n", fileName, Tok.line, Tok.cols, Tok.val.c_str());
				printf("   %d | %s\n", Tok.line, Lines[Tok.line].c_str());
				printf("   %*c |", TokLineNumChars, ' ');
				printf(" %*c", Tok.cols + 1, '^');
				for (u32 numTilde = 1; numTilde < Tok.val.size(); numTilde++) {
					printf("~");
				}

				printf("\n");
				printf("   %*c |\n", TokLineNumChars, ' ');
				exit(1);
			}

			// shows an invalid argument error for the symbol in at "TokenI" index
			InvalidArgumentError: {
				const auto& Tok = LineParsed[TokenI];
				u32 TokLineNumChars = std::to_string(Tok.line).size();
				printf("%s:%d:%d: error: invalid arguments to instruction '%s'\n", fileName, Tok.line, Tok.cols, Tok.val.c_str());
				printf("   %d | %s\n", Tok.line, Lines[Tok.line].c_str());
				printf("   %*c |", TokLineNumChars, ' ');
				printf(" %*c", Tok.cols + 1, '^');
				for (u32 numTilde = 1; numTilde < Tok.val.size(); numTilde++) {
					printf("~");
				}

				printf("\n");
				printf("   %*c |\n", TokLineNumChars, ' ');
				exit(1);
			}

			// shows too many arguments error for symbols from LineParsed[TokenI + 1] until it is .type if not equal to "END"
			TooManyArgsError: {
				const auto& NextTok = LineParsed[TokenI + 1];
				const auto& EndTok = LineParsed.back();
				u32 NextTokLineNumChars = std::to_string(NextTok.line).size();
				u32 LineToHighlightLen = 0;

				LineToHighlightLen = Lines[NextTok.line].substr(NextTok.cols, EndTok.cols).size();

				printf("%s:%d:%d: error: too many arguments for instruction '%s'\n", fileName, NextTok.line, NextTok.cols, Token.val.c_str());
				printf("   %d | %s\n", NextTok.line, Lines[NextTok.line].c_str());
				printf("   %*c |", NextTokLineNumChars, ' ');
				printf(" %*c", NextTok.cols + 1, '^');
				for (u32 numTilde = 1; numTilde < LineToHighlightLen; numTilde++)
					printf("~");

				printf("\n");
				printf("   %*c |\n", NextTokLineNumChars, ' ');
				exit(1);
			}

			/* shows to few arguments error, where expected number of arguments is "ArgsExpected",
			and number of arguments provided is "ArgsProvided"
			*/
			TooFewArgsError: {
				u32 TokenLineNumChars = std::to_string(Token.line).size();
				printf("%s:%d:%d: error: too few arguments for instruction '%s', expected %d got %ld\n", fileName, Token.line, Token.cols, Token.val.c_str(), ArgsExpected, LineParsed.size() - 1);
				printf("   %d | %s\n", Token.line, Lines[Token.line].c_str());
				printf("   %*c |", TokenLineNumChars, ' ');
				printf(" %*c", Token.cols + 1, '^');
				for (u32 numTilde = 1; numTilde < Token.val.size(); numTilde++) {
					printf("~");
				}

				printf("\n");
				printf("   %*c |\n", TokenLineNumChars, ' ');
				exit(1);
			}
		}
	}

	return Binary;
}

