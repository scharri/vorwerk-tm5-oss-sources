/*
 * Chooser Prep common file
 *
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define TM41_DEBUG

#ifdef TM41_DEBUG
  void putc(char ch);
  void printhex(int data);
  void printf(char *fmt, ...);
#else
  #define printhex(data)
  #define printf(fmt, ...)
#endif

#endif
