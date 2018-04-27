/*
 * Chooser Prep common file
 *
 */

#include <stdarg.h>
#include "debug.h"

#ifdef TM41_DEBUG
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

int abs (int val)
{
    return ((val < 0) ? -val : val);
}

char * itoa (int val)
{
    static char buf[32] = {0};
    int i = 30;
    int base = 10;
    int orig = val;

    do {
        buf [i] = "0123456789abcdef" [abs(val % base)];
        val /= base;
        i--;
    } while (val && i > 0);

    if (orig < 0) {
        buf [i] = '-';
        return &buf [i];
    }
    else {
        return &buf [i+1];
    }
}

void printf(char *fmt, ...)
{
	va_list args;
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

// printf version for romlib.a needs
void my_printf(char *fmt, ...)
{
#if 0
	va_list args;
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
#endif
}
#else
void my_printf(char *fmt, ...)
{
}
#endif
