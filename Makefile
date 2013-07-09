
all: sl-apdu.elf test.run

clean:
	rm -f *.o *~ *.elf *.run

MODULES := apdu.c

OBJS := $(MODULES:.c=.o)

CFLAGS := -I../SaleaeDeviceSdk-1.1.14/include -g
LDFLAGS := -L ../SaleaeDeviceSdk-1.1.14/lib/ -lSaleaeDevice64 -Xlinker -rpath -Xlinker ../SaleaeDeviceSdk-1.1.14/lib/

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@
sl-apdu.elf: main.cpp $(OBJS)
	g++ $(CFLAGS) $< -o $@ $(OBJS) $(LDFLAGS) 

test.elf: test.c $(OBJS)
	gcc $(CFLAGS) $< -o $@ apdu.o
test.run: test.elf
	bzcat sim-4MHz.bin.bz2 | ./test.elf


