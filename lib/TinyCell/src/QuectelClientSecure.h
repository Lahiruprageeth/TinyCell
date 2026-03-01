#pragma once
#include <Client.h>
#include "QuectelModemBase.h"

class QuectelClientSecure : public Client {
 public:
  QuectelClientSecure(QuectelModemBase& modem);

  // Connect using IP or host + port
  int connect(IPAddress ip, uint16_t port) override;
  int connect(const char* host, uint16_t port) override;

  // Set optional root CA certificate (PEM format)
  void setCACert(const char* ca);

  // Set optional client certificate and private key
  void setCertificate(const char* cert, const char* key);

  // Client overrides
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buf, size_t size) override;
  int available() override;
  int read() override;
  int read(uint8_t* buf, size_t size) override;
  int peek() override;
  void flush() override;
  void stop() override;
  uint8_t connected() override;
  operator bool() { return connected() != 0; }

 private:
  QuectelModemBase* _modem;
  int _peekByte = -1;

  const char* _rootCA = nullptr;
  const char* _clientCert = nullptr;
  const char* _clientKey = nullptr;

  bool _secureConnected = false;

  // Internal helper to start TLS connection
  bool startTLS(const char* host, uint16_t port);
};