
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_crypto_sasl_srp_SRPServer__
#define __gnu_javax_crypto_sasl_srp_SRPServer__

#pragma interface

#include <gnu/javax/crypto/sasl/ServerMechanism.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace security
      {
        namespace util
        {
            class PRNG;
        }
      }
    }
    namespace javax
    {
      namespace crypto
      {
        namespace key
        {
            class IKeyAgreementParty;
        }
        namespace sasl
        {
          namespace srp
          {
              class CALG;
              class IALG;
              class SRP;
              class SRPServer;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace math
    {
        class BigInteger;
    }
  }
}

class gnu::javax::crypto::sasl::srp::SRPServer : public ::gnu::javax::crypto::sasl::ServerMechanism
{

public:
  SRPServer();
public: // actually protected
  virtual void initMechanism();
  virtual void resetMechanism();
public:
  virtual JArray< jbyte > * evaluateResponse(JArray< jbyte > *);
public: // actually protected
  virtual JArray< jbyte > * engineUnwrap(JArray< jbyte > *, jint, jint);
  virtual JArray< jbyte > * engineWrap(JArray< jbyte > *, jint, jint);
  virtual ::java::lang::String * getNegotiatedQOP();
  virtual ::java::lang::String * getNegotiatedStrength();
  virtual ::java::lang::String * getNegotiatedRawSendSize();
  virtual ::java::lang::String * getReuse();
private:
  JArray< jbyte > * sendProtocolElements(JArray< jbyte > *);
  JArray< jbyte > * sendEvidence(JArray< jbyte > *);
  ::java::lang::String * createL();
  void parseO(::java::lang::String *);
  void setupSecurityServices(jboolean);
  ::gnu::java::security::util::PRNG * getDefaultPRNG();
  static ::java::util::logging::Logger * log;
  ::java::lang::String * __attribute__((aligned(__alignof__( ::gnu::javax::crypto::sasl::ServerMechanism)))) U;
  ::java::math::BigInteger * N;
  ::java::math::BigInteger * g;
  ::java::math::BigInteger * A;
  ::java::math::BigInteger * B;
  JArray< jbyte > * s;
  JArray< jbyte > * cIV;
  JArray< jbyte > * sIV;
  JArray< jbyte > * cn;
  JArray< jbyte > * sn;
  ::gnu::javax::crypto::sasl::srp::SRP * srp;
  JArray< jbyte > * sid;
  jint ttl;
  JArray< jbyte > * cCB;
  ::java::lang::String * mandatory;
  ::java::lang::String * L;
  ::java::lang::String * o;
  ::java::lang::String * chosenIntegrityAlgorithm;
  ::java::lang::String * chosenConfidentialityAlgorithm;
  jint rawSendSize;
  JArray< jbyte > * K;
  jboolean replayDetection;
  jint inCounter;
  jint outCounter;
  ::gnu::javax::crypto::sasl::srp::IALG * inMac;
  ::gnu::javax::crypto::sasl::srp::IALG * outMac;
  ::gnu::javax::crypto::sasl::srp::CALG * inCipher;
  ::gnu::javax::crypto::sasl::srp::CALG * outCipher;
  ::gnu::javax::crypto::key::IKeyAgreementParty * serverHandler;
  ::gnu::java::security::util::PRNG * prng;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_crypto_sasl_srp_SRPServer__