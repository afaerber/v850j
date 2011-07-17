all: v850j-test

.PHONY: test

CFLAGS = -std=gnu99 -Wall -Werror
DGFLAGS = -MMD -MP -MT $@

-include config.mak

-include v850j-test.d

v850j-test: main.c 78k0_usb_uart.c v850jx3l_flash.c
	$(CC) -o $@ $(CPPFLAGS) $(DGFLAGS) $(CFLAGS) main.c 78k0_usb_uart.c v850jx3l_flash.c $(LDFLAGS) -lusb-1.0

test: v850j-test
	./v850j-test
