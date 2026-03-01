#pragma once
#include "QuectelModemBase.h"
#include <Client.h>


class QuectelClient : public Client {
public:
  QuectelClient(QuectelModemBase &modem);

  int connect(IPAddress ip, uint16_t port) override;
  int connect(const char *host, uint16_t port) override;

  size_t write(uint8_t b) override;
  size_t write(const uint8_t *buf, size_t size) override;
  int available() override;
  int read() override;
  int read(uint8_t *buf, size_t size) override;
  int peek() override;
  void flush() override;
  void stop() override;
  uint8_t connected() override;
  operator bool() { return connected() != 0; }

private:
  QuectelModemBase *_modem;
  int _peekByte = -1;
};