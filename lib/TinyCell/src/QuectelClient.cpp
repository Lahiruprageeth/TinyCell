#include "QuectelClient.h"

QuectelClient::QuectelClient(QuectelModemBase &modem) : _modem(&modem) {}

int QuectelClient::connect(IPAddress ip, uint16_t port) {
  String host = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) +
                "." + String(ip[3]);
  return connect(host.c_str(), port);
}

int QuectelClient::connect(const char *host, uint16_t port) {
  _peekByte = -1;
  return _modem->openSocket(host, port) ? 1 : 0;
}

size_t QuectelClient::write(uint8_t b) { return _modem->send(&b, 1); }

size_t QuectelClient::write(const uint8_t *buf, size_t size) {
  return _modem->send(buf, size);
}

int QuectelClient::available() {
  int avail = _modem->available();
  if (_peekByte != -1) {
    avail++;
  }
  return avail;
}

int QuectelClient::read() {
  if (_peekByte != -1) {
    uint8_t b = _peekByte;
    _peekByte = -1;
    return b;
  }
  uint8_t b;
  if (_modem->receive(&b, 1) == 1) {
    return b;
  }
  return -1;
}

int QuectelClient::read(uint8_t *buf, size_t size) {
  if (size == 0)
    return 0;

  size_t bytesRead = 0;
  if (_peekByte != -1) {
    buf[0] = _peekByte;
    _peekByte = -1;
    bytesRead = 1;
    buf++;
    size--;
  }

  if (size > 0) {
    bytesRead += _modem->receive(buf, size);
  }

  return bytesRead;
}

int QuectelClient::peek() {
  if (_peekByte != -1) {
    return _peekByte;
  }
  uint8_t b;
  if (_modem->receive(&b, 1) == 1) {
    _peekByte = b;
    return _peekByte;
  }
  return -1;
}

void QuectelClient::flush() {
  // Usually no-op for generic AT clients unless implementing buffer clearing
}

void QuectelClient::stop() {
  _modem->closeSocket();
  _peekByte = -1;
}

uint8_t QuectelClient::connected() {
  uint8_t c = _modem->connected();
  if (!c && _peekByte != -1) {
    // Technically still connected if we have unread data in peek buffer
    return 1;
  }
  return c;
}