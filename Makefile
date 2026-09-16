CC := gcc
CFLAGS := -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude
SRC := src/rbtree.c
TSRC := tests/test_rbtree.c
BIN := build/test_rbtree
FUZZBIN := build/fuzz

all: $(BIN) $(FUZZBIN)

$(BIN): $(SRC) $(TSRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(TSRC) -o $@

$(FUZZBIN): $(SRC) tests/fuzz.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) tests/fuzz.c -o $@

test: $(BIN) $(FUZZBIN)
	./$(BIN) && ./$(FUZZBIN) 100000

asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean test

memcheck: clean all
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(BIN)
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(FUZZBIN) 20000

clean:
	rm -rf build

PKG_NAME := rbtree-lab
PKG_DIR := build/$(PKG_NAME)
CLAUDE_LOG_DIR := $(HOME)/.claude/projects/-home-tyler-Documents-CS370-rbtree-lab

package: 
	@rm -rf $(PKG_DIR)
	@mkdir -p $(PKG_DIR)/src $(PKG_DIR)/tests $(PKG_DIR)/include $(PKG_DIR)/claude-logs
	cp Makefile $(PKG_DIR)/
	cp src/rbtree.c $(PKG_DIR)/src/
	cp tests/test_rbtree.c tests/fuzz.c $(PKG_DIR)/tests/
	cp include/rbtree.h $(PKG_DIR)/include/
	cp CLAUDE.md PROMPTLOG.md REFLECTION.md $(PKG_DIR)/
	cp $(CLAUDE_LOG_DIR)/*.jsonl $(PKG_DIR)/claude-logs/
	cp -r .git $(PKG_DIR)/
	cd build && zip -r ../$(PKG_NAME).zip $(PKG_NAME)
	rm -rf $(PKG_DIR)

.PHONY: all test asan memcheck clean package