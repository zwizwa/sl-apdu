
## dependencies: SaleaeDeviceSdk-1.1.14 in the parent directory:
# cd ..
# wget http://downloads.saleae.com/SDK/SaleaeDeviceSdk-1.1.14.zip
# unzip SaleaeDeviceSdk-1.1.14.zip

all: sl-apdu.elf test.run

clean:
	rm -f *.o *~ *.elf *.run

MODULES := apdu.c apdu_split.c

OBJS := $(MODULES:.c=.o)

CFLAGS := -I../SaleaeDeviceSdk-1.1.14/include -g -O0
LDFLAGS := -L ../SaleaeDeviceSdk-1.1.14/lib/ -lSaleaeDevice64 -Xlinker -rpath -Xlinker ../SaleaeDeviceSdk-1.1.14/lib/

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@
sl-apdu.elf: main.cpp $(OBJS)
	g++ $(CFLAGS) $< -o $@ $(OBJS) $(LDFLAGS) 

test.elf: test.c $(OBJS)
	gcc $(CFLAGS) $< -o $@ $(OBJS)
test.run: test.elf
	bzcat sim-4MHz.bin.bz2 | ./test.elf



test1: iso7816_uart.o

test2: apdu_split.o
