CC:=gcc
CXX:=g++
CFLAGS:=-std=c99 -Wall -MMD -MP
CXXFLAGS:=-std=c++11 -Wall -MMD -MP
LFLAGS:=

BIN        := chasm
BUILD      := build
BUILD_TYPE := Debug

SOURCES_C   :=
SOURCES_CPP := src/main.cpp src/TranslationUnit.cpp

OBJECTS    := $(SOURCES_C:.c=.c.o) $(SOURCES_CPP:.cpp=.cpp.o)
OBJECTS    := $(patsubst %,$(BUILD)/%,$(OBJECTS))
DEPENDS    := $(OBJECTS:.o=.d)

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS+=-O0 -g
	CXXFLAGS+=-O0 -g
else
	ifeq ($(BUILD_TYPE),Release)
		CFLAGS+=-O3
		CXXFLAGS+=-O3
	else
$(error Unknown Build Type "$(BUILD_TYPE)")
	endif
endif

-include $(DEPENDS)

all: $(BIN)

$(BUILD)/%.c.o: %.c
	@echo "CC  -" $<
	@mkdir -p "$$(dirname "$@")"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.cpp.o: %.cpp
	@echo "CXX -" $<
	@mkdir -p "$$(dirname "$@")"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN): $(OBJECTS)
	@echo Linking $@
	@$(CXX) $(LFLAGS) $(OBJECTS) -o $@

.PHONY: run
.PHONY: clean

run: $(all)
	./$(BIN)

clean:
	$(RM) -rv $(BIN) $(BUILD)

