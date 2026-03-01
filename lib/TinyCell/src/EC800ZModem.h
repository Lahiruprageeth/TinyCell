#pragma once
#include "QuectelAT.h"
#include "QuectelModemBase.h"

class EC800ZModem : public QuectelModemBase {
 public:
  // construct with existing AT helper
  EC800ZModem(QuectelAT& at);
  // convenience constructor that takes a Stream and builds its own QuectelAT
  EC800ZModem(Stream& serial);
  ~EC800ZModem();

  bool attachNetwork() override;
  bool activatePDP(const char* apn) override;

  bool openSocket(const char* host, uint16_t port) override;
  bool openSSLSocket(const char* host, uint16_t port) override;

  // Optional SSL certificate setters
  void setRootCA(const char* ca);
  void setClientCert(const char* cert, const char* key);

  int send(const uint8_t* data, size_t len) override;
  int available() override;
  int receive(uint8_t* buffer, size_t len) override;
  void closeSocket() override;
  uint8_t connected() override;
  void setDebug(Stream& dbg) override;

  // AT helper wrappers
  bool sendAT(const char* fmt, ...) override;
  int waitResponse(const char* r1 = "OK\r\n", const char* r2 = "ERROR\r\n",
                   const char* r3 = "", const char* r4 = "",
                   const char* r5 = "") override;

  // SMS / voice helpers
  bool sendSMS(const char* to, const char* text);
  bool dial(const char* number);
  bool answer();
  void hangup();

 private:
  QuectelAT* _at;
  bool _ownsAt = false;
  bool _socketConnected = false;

  // pointers to certificates for SSL sockets
  const char* _rootCA = nullptr;
  const char* _clientCert = nullptr;
  const char* _clientKey = nullptr;

  // Simple buffer to avoid losing bytes between available() and receive() calls
  uint16_t _cachedAvailable = 0;
};
