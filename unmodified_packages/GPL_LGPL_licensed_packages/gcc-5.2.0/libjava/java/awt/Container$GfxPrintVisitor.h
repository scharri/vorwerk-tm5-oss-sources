
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_Container$GfxPrintVisitor__
#define __java_awt_Container$GfxPrintVisitor__

#pragma interface

#include <java/awt/Container$GfxVisitor.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Component;
        class Container$GfxPrintVisitor;
        class Container$GfxVisitor;
        class Graphics;
    }
  }
}

class java::awt::Container$GfxPrintVisitor : public ::java::awt::Container$GfxVisitor
{

public: // actually package-private
  Container$GfxPrintVisitor();
public:
  virtual void visit(::java::awt::Component *, ::java::awt::Graphics *);
  static ::java::awt::Container$GfxVisitor * INSTANCE;
  static ::java::lang::Class class$;
};

#endif // __java_awt_Container$GfxPrintVisitor__
