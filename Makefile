
OBJS := ltest.o
CC := clang

CFLAGS := -ggdb3 -Wall
LDFLAGS :=
LDLIBS := 

TARGET := ltest

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

	
clean:
	rm -r $(OBJS) $(TARGET)
