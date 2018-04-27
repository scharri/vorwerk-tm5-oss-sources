/*
 * Chooser Prep common file
 *
 */

#include <stdarg.h>
#include "debug.h"
#include "regsuartdbg.h"
#include "regspinctrl.h"

#ifdef TM41_DEBUG
/* Debug uart have been init by boot rom. */
void putc(char ch)
{
	int loop = 0;
	while (HW_UARTDBGFR_RD()&BM_UARTDBGFR_TXFF) {
		loop++;
		if (loop > 10000)
			break;
	};
	HW_UARTDBGDR_WR(ch);
}

void printhex(int data)
{
	int i = 0;
	char c;
	for (i = sizeof(int)*2-1; i >= 0; i--) {
		c = data>>(i*4);
		c &= 0xf;
		if (c > 9)
			putc(c-10+'A');
		else
			putc(c+'0');
	}
}

void printhex2(int data)
{
	int i;
	char c;
	for (i = 1; i >= 0; i--) {
		c = data>>(i*4);
		c &= 0xf;
		if (c > 9)
			putc(c-10+'A');
		else
			putc(c+'0');
	}
}

char * itoa (int val)
{
    static char buf[32] = {0};
    int i = 30;
    int base = 10;

    do {
        buf [i] = "0123456789abcdef" [val % base];
        val /= base;
        i--;
    } while (val && i > 0);

    return &buf [i+1];
}


void printf(char *fmt, ...)
{
	va_list args;
	int one;
	char *st;
	va_start(args, fmt);
	while (*fmt) {

		if (*fmt == '%') {
			fmt++;
			switch (*fmt) {

			case 'x':
			case 'X':
				printhex(va_arg(args, int));
				break;
			case 'h':
				printhex2(va_arg(args, int));
				break;
			case '%':
				putc('%');
				break;
			case 'd':
				st = itoa(va_arg(args, int));
				while (*st) {
				    putc (*st);
				    st++;
				}
				break;
			default:
				putc('%');
				putc(*fmt);
				break;
			}
		} else if (*fmt == '\n') {
			putc('\r');
			putc('\n');
		} else {
			putc(*fmt);
		}
		fmt++;
	}
	va_end(args);
}

#endif
