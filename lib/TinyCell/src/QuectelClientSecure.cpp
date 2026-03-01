#include "QuectelClientSecure.h"

QuectelClientSecure::QuectelClientSecure(QuectelModemBase& modem)
    : _modem(&modem) {}

// Minimal secure client stub: TLS not implemented here. This file exists so
// sketches can construct `QuectelClientSecure` and the class can be
// extended later with real TLS using modem AT commands.

// certificate setters simply record pointers; no actual TLS support yet
void QuectelClientSecure::setCACert(const char* ca) {
  _rootCA = ca;
}

void QuectelClientSecure::setCertificate(const char* cert, const char* key) {
  _clientCert = cert;
  _clientKey = key;
}

int QuectelClientSecure::connect(IPAddress ip, uint16_t port) {
  // no TLS, just forward to modem
  return _modem ? _modem->sendAT("AT+QIOPEN=1,0,\"TCP\",\"%u.%u.%u.%u\",%u,0,0",
                                 ip[0], ip[1], ip[2], ip[3], port),
         1      : 0;
}

int QuectelClientSecure::connect(const char* host, uint16_t port) {
  // try open socket; TLS handshake would go here in a real implementation
  if (!_modem) return 0;
  _secureConnected = startTLS(host, port);
  return _secureConnected ? 1 : 0;
}

size_t QuectelClientSecure::write(uint8_t b) { return _modem ? _modem->write(&b, 1) : 0; }
size_t QuectelClientSecure::write(const uint8_t* buf, size_t size) { return _modem ? _modem->write(buf, size) : 0; }
int QuectelClientSecure::available() { return _modem ? _modem->available() : 0; }
int QuectelClientSecure::read() {
  uint8_t b;
  return (_modem && _modem->read(&b, 1) == 1) ? b : -1;
}
int QuectelClientSecure::read(uint8_t* buf, size_t size) { return _modem ? _modem->read(buf, size) : 0; }
int QuectelClientSecure::peek() { return _peekByte; }
void QuectelClientSecure::flush() {
  if (_modem) _modem->flush();
}
void QuectelClientSecure::stop() {
  if (_modem) _modem->closeSocket();
  _secureConnected = false;
}
uint8_t QuectelClientSecure::connected() { return _secureConnected ? 1 : 0; }

bool QuectelClientSecure::startTLS(const char* /*host*/, uint16_t /*port*/) {
  // stub: pretend we opened a secure socket but don't actually do TLS
  if (!_modem) return false;
  return true;
}
