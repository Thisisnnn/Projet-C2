CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = -lcurl

BUILD_DIR = build
SRCS = main.c client.c tasks.c base64.c network.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET = client

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true