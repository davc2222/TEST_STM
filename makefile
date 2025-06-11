# שם הקובץ הבינארי
TARGET = app

# תקיית קבצי המקור
SRC_DIR = src

# תקיית הקבצים המוכללים (headers)
INC_DIR = ./inc

# מקומפלצי לכל קובץ .c תחת src ותקיות המשנה
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:.c=.o)

# הקומפיילר והדגלים
CC = gcc
CFLAGS = -Wall -Wextra -I$(INC_DIR) -g

# פקודת ברירת מחדל
all: $(TARGET)

# יצירת הקובץ הבינארי
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# קומפילציה של כל קובץ .c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ניקוי קבצים זמניים
clean:
	rm -f $(TARGET) $(OBJS)

# הרצה (אם רוצים)
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
