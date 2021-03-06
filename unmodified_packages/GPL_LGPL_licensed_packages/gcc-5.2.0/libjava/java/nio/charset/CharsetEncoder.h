
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_nio_charset_CharsetEncoder__
#define __java_nio_charset_CharsetEncoder__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace java
  {
    namespace nio
    {
        class ByteBuffer;
        class CharBuffer;
      namespace charset
      {
          class Charset;
          class CharsetEncoder;
          class CoderResult;
          class CodingErrorAction;
      }
    }
  }
}

class java::nio::charset::CharsetEncoder : public ::java::lang::Object
{

public: // actually protected
  CharsetEncoder(::java::nio::charset::Charset *, jfloat, jfloat);
  CharsetEncoder(::java::nio::charset::Charset *, jfloat, jfloat, JArray< jbyte > *);
public:
  virtual jfloat averageBytesPerChar();
  virtual jboolean canEncode(jchar);
  virtual jboolean canEncode(::java::lang::CharSequence *);
private:
  jboolean canEncode(::java::nio::CharBuffer *);
public:
  virtual ::java::nio::charset::Charset * charset();
  virtual ::java::nio::ByteBuffer * encode(::java::nio::CharBuffer *);
  virtual ::java::nio::charset::CoderResult * encode(::java::nio::CharBuffer *, ::java::nio::ByteBuffer *, jboolean);
public: // actually protected
  virtual ::java::nio::charset::CoderResult * encodeLoop(::java::nio::CharBuffer *, ::java::nio::ByteBuffer *) = 0;
public:
  virtual ::java::nio::charset::CoderResult * flush(::java::nio::ByteBuffer *);
public: // actually protected
  virtual ::java::nio::charset::CoderResult * implFlush(::java::nio::ByteBuffer *);
  virtual void implOnMalformedInput(::java::nio::charset::CodingErrorAction *);
  virtual void implOnUnmappableCharacter(::java::nio::charset::CodingErrorAction *);
  virtual void implReplaceWith(JArray< jbyte > *);
  virtual void implReset();
public:
  virtual jboolean isLegalReplacement(JArray< jbyte > *);
  virtual ::java::nio::charset::CodingErrorAction * malformedInputAction();
  virtual jfloat maxBytesPerChar();
  virtual ::java::nio::charset::CharsetEncoder * onMalformedInput(::java::nio::charset::CodingErrorAction *);
  virtual ::java::nio::charset::CodingErrorAction * unmappableCharacterAction();
  virtual ::java::nio::charset::CharsetEncoder * onUnmappableCharacter(::java::nio::charset::CodingErrorAction *);
  virtual JArray< jbyte > * replacement();
  virtual ::java::nio::charset::CharsetEncoder * replaceWith(JArray< jbyte > *);
  virtual ::java::nio::charset::CharsetEncoder * reset();
private:
  static const jint STATE_RESET = 0;
  static const jint STATE_CODING = 1;
  static const jint STATE_END = 2;
  static const jint STATE_FLUSHED = 3;
  static JArray< jbyte > * DEFAULT_REPLACEMENT;
  ::java::nio::charset::Charset * __attribute__((aligned(__alignof__( ::java::lang::Object)))) charset__;
  jfloat averageBytesPerChar__;
  jfloat maxBytesPerChar__;
  JArray< jbyte > * replacement__;
  jint state;
  ::java::nio::charset::CodingErrorAction * malformedInputAction__;
  ::java::nio::charset::CodingErrorAction * unmappableCharacterAction__;
public:
  static ::java::lang::Class class$;
};

#endif // __java_nio_charset_CharsetEncoder__
