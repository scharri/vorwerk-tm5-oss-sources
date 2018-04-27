#ifndef __dbg_plugin_h__
#define __dbg_plugin_h__




#if (defined(TGT_DILLO) || defined(TGT_3700) || defined(TGT_MX28))

#ifdef DBG_PRINTF
#undef DBG_PRINTF
//extern void dillo_printf(const char *fmt, ...);
//#define DBG_PRINTF(...) dillo_printf(__VA_ARGS__)
extern void my_printf(char *fmt, ...);
#define DBG_PRINTF(...) my_printf(__VA_ARGS__)
#endif

#ifdef DBG_EXIT
#undef DBG_EXIT
extern void dillo_exit( uint32_t rc );
#define DBG_EXIT(rc) dillo_exit(rc)
#endif
#endif


#endif 
