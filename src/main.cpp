#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.hpp"
#include "TranslationUnit.h"

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No Input Files Given!\n");
		return 1;
	}

	const char* InputFilePath = argv[1];

	FILE* f = fopen(InputFilePath, "r");
	fseek(f, 0, SEEK_END);
	size_t fSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* _SourceCode = new char[fSize + 1];
	memset(_SourceCode, '\0', fSize + 1);
	if (fread(_SourceCode, 1, fSize, f) != fSize) {
		printf("Failed to read source file!\n");
		delete[] _SourceCode;
		return 1;
	}

	Vector<u16>* bin = TranslationUnit2Binary(InputFilePath, _SourceCode, fSize + 1);

	if (bin) {
		for (u16 instruction : *bin) {
			printf("Instruction: %04X\n", instruction);
		}
	}

	if (f) fclose(f);
	if (_SourceCode) delete[] _SourceCode;
	if (bin) delete bin;
	return 0;
}
