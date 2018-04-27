LIBDIR ?= /usr/lib
INCDIR ?= /usr/include/

SRCS := m2mxmlin.c m2mxmlou.c m2mmem.c m2mbsm.c expat/explight.c m2mitoa.c
OBJS := $(SRCS:.c=.o)
TARGET := libm2m.so.1

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^
	ln -s $(TARGET) libm2m.so

.PHONY: clean install

clean:
	rm -rf *.o expat/*.o *.so

install: all
	install -d $(DESTDIR)/$(LIBDIR)
	install -d $(DESTDIR)/$(INCDIR)
	install -d $(DESTDIR)/$(INCDIR)expat
	install -m 0755 $(TARGET) $(DESTDIR)/$(LIBDIR)
	ln -s $(TARGET) $(DESTDIR)/$(LIBDIR)/libm2m.so
	install -m 0644 *.h $(DESTDIR)/$(INCDIR)
	install -m 0644 expat/*.h $(DESTDIR)/$(INCDIR)expat

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -fPIC  $< -o $@
