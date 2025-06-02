CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread

LDFLAGS = -lsqlite3 -lncurses

SERVER_DIR = server
CLIENT_DIR = client
BUILD_DIR = build

SERVER_BIN = $(SERVER_DIR)/server
CLIENT_BIN = $(CLIENT_DIR)/client

SERVER_OBJS = $(BUILD_DIR)/server.o $(BUILD_DIR)/authentication.o
CLIENT_OBJS = $(BUILD_DIR)/client.o

all: $(SERVER_BIN) $(CLIENT_BIN)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build server binary
$(SERVER_BIN): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build client binary
$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile server.o
$(BUILD_DIR)/server.o: $(SERVER_DIR)/server.cpp $(SERVER_DIR)/authentication.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile authentication.o
$(BUILD_DIR)/authentication.o: $(SERVER_DIR)/authentication.cpp $(SERVER_DIR)/authentication.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile client.o
$(BUILD_DIR)/client.o: $(CLIENT_DIR)/client.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(SERVER_BIN) $(CLIENT_BIN)
