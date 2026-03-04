#include "QuectelClientSecure.h"
#include <stdio.h>

QuectelClientSecure::QuectelClientSecure(QuectelModemBase &modem)
    : _modem(&modem) {}

void QuectelClientSecure::setCACert(const char *ca) {
  _rootCA = ca;
  if (_modem)
    _modem->setRootCA(ca);
}

void QuectelClientSecure::setCertificate(const char *cert, const char *key) {
  _clientCert = cert;
  _clientKey = key;
  if (_modem)
    _modem->setClientCert(cert, key);
}

int QuectelClientSecure::connect(IPAddress ip, uint16_t port) {
  char buf[20];
  sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return connect(buf, port);
}

int QuectelClientSecure::connect(const char *host, uint16_t port) {
  if (!_modem)
    return 0;
  _peekByte = -1;
  _modem->processURC();
  _secureConnected = _modem->openSSLSocket(host, port);
  return _secureConnected ? 1 : 0;
}

size_t QuectelClientSecure::write(uint8_t b) {
  if (!_modem)
    return 0;
  _modem->processURC();
  return _modem->send(&b, 1);
}

size_t QuectelClientSecure::write(const uint8_t *buf, size_t size) {
  if (!_modem)
    return 0;
  _modem->processURC();
  return _modem->send(buf, size);
}

int QuectelClientSecure::available() {
  if (!_modem)
    return 0;
  _modem->processURC();
  int avail = _modem->available();
  if (_peekByte != -1)
    avail++;
  return avail;
}

int QuectelClientSecure::read() {
  if (_peekByte != -1) {
    uint8_t b = _peekByte;
    _peekByte = -1;
    return b;
  }
  uint8_t b;
  if (_modem && _modem->receive(&b, 1) == 1)
    return b;
  return -1;
}

int QuectelClientSecure::read(uint8_t *buf, size_t size) {
  if (!_modem || size == 0)
    return 0;
  size_t readCount = 0;
  if (_peekByte != -1) {
    buf[0] = _peekByte;
    _peekByte = -1;
    readCount = 1;
    buf++;
    size--;
  }
  if (size > 0) {
    readCount += _modem->receive(buf, size);
  }
  return (int)readCount;
}

int QuectelClientSecure::peek() {
  if (_peekByte != -1)
    return _peekByte;
  uint8_t b;
  if (_modem && _modem->receive(&b, 1) == 1) {
    _peekByte = b;
    return b;
  }
  return -1;
}

void QuectelClientSecure::flush() {
  if (_modem)
    _modem->flush();
}

void QuectelClientSecure::stop() {
  if (_modem)
    _modem->closeSocket();
  _secureConnected = false;
  _peekByte = -1;
}

uint8_t QuectelClientSecure::connected() {
  if (!_modem)
    return 0;
  uint8_t c = _modem->connected();
  if (!c && _peekByte != -1)
    return 1;
  return c;
}

bool QuectelClientSecure::startTLS(const char *host, uint16_t port) {
  // Directly handled by connect() calling openSSLSocket()
  return connect(host, port);
}
