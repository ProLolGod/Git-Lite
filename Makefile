CXX = g++
CXXFLAGS = -std=c++20 -Wall -Iinclude
LDFLAGS = -lssl -lcrypto -lz

SRCS = \
	src/utils.cpp \
	src/ObjectStore.cpp \
	src/init_repo.cpp \
	src/write_tree.cpp \
	src/commit_tree.cpp \
	src/mktag.cpp \
	src/log.cpp \
	src/branch.cpp \
	src/checkout.cpp \
	src/add.cpp \
	src/commit.cpp \
	main.cpp \
	src/status.cpp

TARGET = git-lite

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)