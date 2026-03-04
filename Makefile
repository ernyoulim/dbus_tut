CC=gcc
CFLAGS=`pkg-config --cflags gio-2.0`
LIBS=`pkg-config --libs gio-2.0`
TARGET=mini-nm

all: $(TARGET)

$(TARGET): mini-nm.c
	$(CC) mini-nm.c -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
