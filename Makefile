CC     := gcc
TARGET := bp
CFLAGS := -O2

all: $(TARGET)

bp: bp.cc
	$(CC) -o $(TARGET) bp.cc $(CFLAGS)

clean:
	rm -f $(TARGET)

