CPP=g++
INCLUDES=#-I/usr/include/gtest
LIBS=-lSDL2
TEST_LIBS=-lgtest -lpthread
LDFLAGS=-L./lib $(LIBS)
CPPFLAGS=-c -g -Wall -Wpedantic -std=c++11 -pthread $(INCLUDES)

SRC_DIR = src
TEST_DIR = $(SRC_DIR)/tests

# Files needed to build main binary.
CLASSES_SRC = $(SRC_DIR)/Emulator.cpp \
	$(SRC_DIR)/State.cpp \
	$(SRC_DIR)/Memory.cpp \
	$(SRC_DIR)/Timer.cpp \
	$(SRC_DIR)/Interrupt.cpp \
	$(SRC_DIR)/Display.cpp \
	$(SRC_DIR)/Input.cpp
MAIN_SRC = $(SRC_DIR)/gbemu.cpp $(CLASSES_SRC)

# Files needed to build unit tests.
TEST_CLASSES_SRC = $(TEST_DIR)/EmulatorTest.cpp \
	$(TEST_DIR)/MemoryTest.cpp \
	$(TEST_DIR)/DisplayTest.cpp \
	$(TEST_DIR)/InputTest.cpp
TEST_SRC = $(TEST_DIR)/main.cpp $(CLASSES_SRC) $(TEST_CLASSES_SRC)

# Rules to build object files. Use wildcards to set foo.o depends on foo.cpp
MAIN_OBJ = $(MAIN_SRC:%.cpp=%.o)
TEST_OBJ = $(TEST_SRC:%.cpp=%.o)

MAIN_BIN = gbemu
TEST_BIN = test-gbemu

.PHONY: all clean test

# Build "all" will build main binary and test binary.
all: $(MAIN_BIN) $(TEST_BIN)

# Rule to build main binary.
$(MAIN_BIN): $(MAIN_OBJ)
	$(CPP) $(MAIN_OBJ) $(LDFLAGS) -o $(MAIN_BIN)

# Rule to build test binary.
$(TEST_BIN): $(TEST_OBJ) lib/libgtest.a
	$(CPP) $(TEST_OBJ) $(LDFLAGS) $(TEST_LIBS) -o $(TEST_BIN)

# Rule to build an object file from a .cpp file. "$<" is the *.cpp filename, $@ is the *.o filename.
%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f $(MAIN_BIN) $(TEST_BIN) $(MAIN_OBJ) $(TEST_OBJ) lib/libgtest.a

# Run tests.
test: $(TEST_BIN) $(MAIN_BIN)
	./$(TEST_BIN)
	./run_test_roms.sh

# Build libgtest library.
GTEST_SRC=/usr/src/gtest
lib/libgtest.a:
	g++ -I$(GTEST_SRC) -pthread -fPIC -c $(GTEST_SRC)/src/gtest-all.cc -o lib/gtest-all.o
	ar -rv lib/libgtest.a lib/gtest-all.o
	rm lib/gtest-all.o
