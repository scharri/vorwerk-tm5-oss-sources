
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_PopupMenu$AccessibleAWTPopupMenu__
#define __java_awt_PopupMenu$AccessibleAWTPopupMenu__

#pragma interface

#include <java/awt/Menu$AccessibleAWTMenu.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class PopupMenu;
        class PopupMenu$AccessibleAWTPopupMenu;
    }
  }
  namespace javax
  {
    namespace accessibility
    {
        class AccessibleRole;
    }
  }
}

class java::awt::PopupMenu$AccessibleAWTPopupMenu : public ::java::awt::Menu$AccessibleAWTMenu
{

public: // actually protected
  PopupMenu$AccessibleAWTPopupMenu(::java::awt::PopupMenu *);
public:
  virtual ::javax::accessibility::AccessibleRole * getAccessibleRole();
private:
  static const jlong serialVersionUID = -4282044795947239955LL;
public: // actually package-private
  ::java::awt::PopupMenu * __attribute__((aligned(__alignof__( ::java::awt::Menu$AccessibleAWTMenu)))) this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_PopupMenu$AccessibleAWTPopupMenu__
