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

enum TokenType : u8 {
	TOKEN = 0, // Token
	END   = 1  // End Of Instruction
};

struct Token {
	TokenType type;
	String    val;
	u32       line;
	u32       cols;
};

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
		u16 result = std::strtol(str.substr(2, 3).c_str(), NULL, 16);
		/*                                     ^                  ^
		                               3 Hex Digits (12 Bits)   Base 16 */
		if (errno == EINVAL && result == 0 && _errFlag != nullptr) *_errFlag = true;
	} else if (str.rfind("0b", 0) == 0) { // try to parse as binary
		u16 result = std::strtol(str.substr(2, 12).c_str(), NULL, 2);
		/*                                     ^                  ^
		                                     12 Bits            Base 2  */
		if (errno == EINVAL && result == 0 && _errFlag != nullptr) *_errFlag = true;
	} else { // try to parse as base-10 number
		u16 result = std::strtol(str.c_str(), NULL, 10);
		/*                                          ^
		                                         Base 10 */
		if (errno == EINVAL && result == 0 && _errFlag != nullptr) *_errFlag = true;
	}
	return value;
}

Vector<u16>* TranslationUnit2Binary(const char* fileName, const char* source, u32 len) {
	// Vector<String> Tokens;
	Vector<String> Lines;
	Vector<Token>  Tokens;

	if (source != NULL) {
		std::stringstream ss(source);
		std::string line;
		while (std::getline(ss, line, '\n')) {
			Lines.emplace_back(line);
		}
	}

	if (Lines.size() > 0) {
		for (u32 lIdx = 0; lIdx < Lines.size(); lIdx++) {
			String& Line = Lines[lIdx];

			// First Filter Out All Of The Comments
			for (u32 i = 0; i < Line.size(); i++) {
				if (
					Line[i] == ';' ||
					/* this weird hack checks if previous character is a ';' i.e. comment,
					   because the while loop that checks for 'symbols' stops when it sees ';',
					   and does the processing with symbols, and increases 'i' by the number of
					   characters in the symbol, which becomes the problem since in the next
					   iteration of the loop i is again increased by 1 thus if there was a ';'
					   just after a symbol it is missed. */
					(i > 0 && Line[i - 1] == ';')
				) {
					if (i > 0 && Line[i - 1] == ';') i--; // if previous character is a ';' set 'i' to it.
					u32 commentLen = 0;
					while ((Line.begin() + i + commentLen) != Line.end()) {
						commentLen++;
					}
					if (commentLen > 0) {
						Line.erase(i, commentLen);
						Tokens.push_back({ TokenType::END, "", lIdx, i });
					}
				}

				u32 symbolLen = 0;
				while (i + symbolLen < Line.size() && !std::isspace(Line[i + symbolLen])) {
					if (Line[i + symbolLen] == ';') {
						break;
					}
					symbolLen++;
				}

				if (symbolLen > 0) {
					Tokens.push_back({
						.type = TokenType::TOKEN,
						.val  = Line.substr(i, symbolLen),
						.line = lIdx,
						.cols = i
					});

					if (Line.begin() + i + symbolLen == Line.end()) {
						Tokens.push_back({ TokenType::END, "", lIdx, i });
					}
					i += symbolLen;
				}

				// Handle Trailing Spaces And Shit
				if (Tokens.back().type != TokenType::END) {
					bool symbolsLeft = false;
					for (u32 u = symbolLen; i + u < Line.size(); u++) {
						if (!std::isspace(Line[i + u])) symbolsLeft = true;
					}
					if (!symbolsLeft) {
						Tokens.push_back({ TokenType::END, "", lIdx, (u32)Line.size() - 1 });
						i += Line.size() - symbolLen - 1;
					}
				}
			}
		}
	}

	for (Token& Token : Tokens) {
		// printf("Token: %6s - Position: %d:%d\n", Token.val.c_str(), Token.line, Token.cols);
		std::transform(
			Token.val.begin(), Token.val.end(), Token.val.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);
	}

	Vector<u16>* Binary = new Vector<u16>();

	#define CHECK_TOO_MANY_ARGS() if (TokenI < Tokens.size() && Tokens[TokenI + 1].type != TokenType::END) goto TooManyArgsError
	#define CHECK_TOO_FEW_ARGS(numExpected) \
		ArgsExpected = numExpected, ArgsProvided = 0; \
		if (TokenI + ArgsExpected >= Tokens.size()) { \
			goto TooFewArgsError; \
		} else { \
			for (u32 i = 0; i < ArgsExpected; i++) { \
				if (Tokens[TokenI + i + 1].type == TokenType::END) goto TooFewArgsError; \
				ArgsProvided++; \
			} \
		}

	for (u32 TokenI = 0; TokenI < Tokens.size(); TokenI++) {
		Token& Token = Tokens[TokenI];

		// Read-Only, Don't Modify It's Value Below It Will Cause Undefined Behavior.
		u16 ArgsExpected = 0, ArgsProvided = 0;

		if (Token.type == TokenType::END) continue;

		u16 OpCode;

		if (Token.val == "cls") {
			CHECK_TOO_FEW_ARGS(0);
			Binary->push_back(0x00E0);
		} else if (Token.val == "ret") {
			CHECK_TOO_FEW_ARGS(0);
			Binary->push_back(0x00EE);
		} else if (Token.val == "jp" || Token.val == "call") {
			CHECK_TOO_FEW_ARGS(1);
			auto& nnn = Tokens[TokenI + 1];
			if (nnn.type == TokenType::END) goto TooFewArgsError;
			u16 nnnParsed = 0;
			nnnParsed = strtol(nnn.val.substr(2, 3).c_str(), NULL, 16);
			nnnParsed = nnnParsed & 0x0FFF;

			if (Token.val == "jp") OpCode = 0x1000 + nnnParsed;
			else if (Token.val == "call") OpCode = 0x2000 + nnnParsed;

			Binary->push_back(OpCode);
			TokenI++; // increment index by 1 since we used that as our argument.
		} else if (
			Token.val == "se"
		) {
			CHECK_TOO_FEW_ARGS(2);
			auto& Arg1 = Tokens[TokenI + 1];
			auto& Arg2 = Tokens[TokenI + 2];

			bool errorOccurred1 = false;
			u16 RegisterX = ParseRegisterNotation(Arg1.val, &errorOccurred1);

			bool errorOccurred2 = false;
			u16 RegisterY = ParseRegisterNotation(Arg2.val, &errorOccurred2);

			bool errorOccurred3 = false;
			u16 Byte = ParseNumberNotation(Arg2.val, &errorOccurred1);

			// if arguments were indeed register indexes
			if (!errorOccurred1 && !errorOccurred2) {
			} else if (!errorOccurred1 && !errorOccurred3) {
			} else {
				if (errorOccurred1) TokenI += 1;
				else if (errorOccurred2 || errorOccurred3) TokenI += 2;
				goto UnknownIdentifierError;
			}

			TokenI += 2; // consumed 2 arguments
		} else {
			goto UnknownIdentifierError;
		}

		CHECK_TOO_MANY_ARGS();

		continue; // by default skip the below code since we only want to run it via 'goto'.

		// shows an unknown identifier error for the symbol in at "TokenI" index
		UnknownIdentifierError: {
			const auto& Tok = Tokens[TokenI];
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

		// shows too many arguments error for symbols from Tokens[TokenI + 1] until it is .type if not equal to "END"
		TooManyArgsError: {
			for (std::size_t idx = TokenI + 1; idx < Tokens.size() && Tokens[idx].type != TokenType::END; idx++) {
				const auto& NextTok = Tokens[idx];
				u32 NextTokLineNumChars = std::to_string(NextTok.line).size();

				printf("%s:%d:%d: error: too many arguments for instruction '%s'\n", fileName, NextTok.line, NextTok.cols, Token.val.c_str());
				printf("   %d | %s\n", NextTok.line, Lines[NextTok.line].c_str());
				printf("   %*c |", NextTokLineNumChars, ' ');
				printf(" %*c", NextTok.cols + 1, '^');
				for (u32 numTilde = 1; numTilde < NextTok.val.size(); numTilde++)
					printf("~");

				printf("\n");
				printf("   %*c |\n", NextTokLineNumChars, ' ');
				if (Tokens[idx + 1].type == TokenType::END) exit(1);
			}
		}

		/* shows to few arguments error, where expected number of arguments is "ArgsExpected",
		   and number of arguments provided is "ArgsProvided"
		*/
		TooFewArgsError: {
			u32 TokenLineNumChars = std::to_string(Token.line).size();
			printf("%s:%d:%d: error: too few arguments for instruction '%s', expected %d got %d\n", fileName, Token.line, Token.cols, Token.val.c_str(), ArgsExpected, ArgsProvided);
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

#if 0
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
#endif

	return Binary;
}


