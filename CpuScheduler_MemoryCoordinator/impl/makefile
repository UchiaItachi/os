CC      = g++
CFLAGS  = -Wall -DENABLE_MEM -std=c++11 
LDFLAGS = virt
SRC     = ./*.cpp

all: Memory_coordinator

Memory_coordinator: $(SRC)
	$(CC) $(CFLAGS) -l$(LDFLAGS) $(SRC) -o $@

.PHONY: clean cleanest

clean:
	rm Memory_coordinator
	
