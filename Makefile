CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude
BUILD_DIR := build
TESTS := $(BUILD_DIR)/tests
HEADERS := include/tl/lexer.hpp include/tl/ast.hpp include/tl/parser.hpp

.PHONY: all test sanitize clean
all: $(TESTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TESTS): tests/test_main.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_main.cpp -o $@

test: $(TESTS)
	./$(TESTS)

sanitize: | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror \
		-fsanitize=address,undefined -fno-omit-frame-pointer tests/test_main.cpp \
		-o $(BUILD_DIR)/tests_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1 ./$(BUILD_DIR)/tests_asan

clean:
	rm -rf $(BUILD_DIR)
