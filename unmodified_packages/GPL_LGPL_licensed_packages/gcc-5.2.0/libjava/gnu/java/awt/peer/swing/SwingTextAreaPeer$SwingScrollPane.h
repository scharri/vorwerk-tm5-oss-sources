
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_awt_peer_swing_SwingTextAreaPeer$SwingScrollPane__
#define __gnu_java_awt_peer_swing_SwingTextAreaPeer$SwingScrollPane__

#pragma interface

#include <javax/swing/JScrollPane.h>
extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace awt
      {
        namespace peer
        {
          namespace swing
          {
              class SwingTextAreaPeer;
              class SwingTextAreaPeer$SwingScrollPane;
              class SwingTextAreaPeer$SwingTextArea;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace awt
    {
        class Container;
        class Graphics;
        class Image;
        class Point;
      namespace event
      {
          class FocusEvent;
          class KeyEvent;
          class MouseEvent;
      }
    }
  }
  namespace javax
  {
    namespace swing
    {
        class JComponent;
    }
  }
}

class gnu::java::awt::peer::swing::SwingTextAreaPeer$SwingScrollPane : public ::javax::swing::JScrollPane
{

public: // actually package-private
  SwingTextAreaPeer$SwingScrollPane(::gnu::java::awt::peer::swing::SwingTextAreaPeer *, ::gnu::java::awt::peer::swing::SwingTextAreaPeer$SwingTextArea *);
public:
  virtual ::javax::swing::JComponent * getJComponent();
  virtual void handleMouseEvent(::java::awt::event::MouseEvent *);
  virtual jboolean isLightweight();
  virtual void handleMouseMotionEvent(::java::awt::event::MouseEvent *);
  virtual void handleKeyEvent(::java::awt::event::KeyEvent *);
  virtual void handleFocusEvent(::java::awt::event::FocusEvent *);
  virtual ::java::awt::Point * getLocationOnScreen();
  virtual jboolean isShowing();
  virtual ::java::awt::Image * createImage(jint, jint);
  virtual ::java::awt::Graphics * getGraphics();
  virtual ::java::awt::Container * getParent();
  virtual void requestFocus();
  virtual jboolean requestFocus(jboolean);
public: // actually package-private
  ::gnu::java::awt::peer::swing::SwingTextAreaPeer$SwingTextArea * __attribute__((aligned(__alignof__( ::javax::swing::JScrollPane)))) textArea;
  ::gnu::java::awt::peer::swing::SwingTextAreaPeer * this$0;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_awt_peer_swing_SwingTextAreaPeer$SwingScrollPane__
