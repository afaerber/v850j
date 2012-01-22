all: v850j-test rl78-test

.PHONY: test

CFLAGS = -std=gnu99 -Wall -Werror
DGFLAGS = -MMD -MP -MT $@

-include config.mak

-include v850j-test.d

v850j-test: main.c 78k0_usb_uart.c v850jx3l_flash.c
	$(CC) -o $@ $(CPPFLAGS) $(DGFLAGS) $(CFLAGS) main.c 78k0_usb_uart.c v850jx3l_flash.c $(LDFLAGS) -pthread -lusb-1.0

-include rl78-test.d

rl78-test: main_rl78.c 78k0_usb_uart.c
	$(CC) -o $@ $(CPPFLAGS) $(DGFLAGS) $(CFLAGS) main_rl78.c 78k0_usb_uart.c $(LDFLAGS) -pthread -lusb-1.0

test: v850j-test
	./v850j-test

test-rl78: rl78-test
	./rl78-test

clean:
	-rm v850j-test *.d
