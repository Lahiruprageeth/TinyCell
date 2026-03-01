#include "EC800ZModem.h"

// Implementation is identical to EG800K for now; commands may need tuning
#include <cstdarg>

EC800ZModem::EC800ZModem(QuectelAT& at) : _at(&at), _ownsAt(false) {}

EC800ZModem::EC800ZModem(Stream& serial)
    : _at(new QuectelAT(serial)), _ownsAt(true) {}

EC800ZModem::~EC800ZModem() {
  if (_ownsAt && _at) {
    delete _at;
  }
}

bool EC800ZModem::attachNetwork() {
  // Duplicate EG800K logic; modify if commands differ
  _at->sendCommand("AT");
  _at->waitResponse(1000);

  _at->sendCommand("ATE0");
  _at->waitResponse(1000);

  for (int i = 0; i < 5; i++) {
    _at->sendCommand("AT+CPIN?");
    if (_at->waitResponse(2000, "+CPIN: READY\r\n", "+CPIN: READY") > 0) {
      _at->waitResponse(1000);
      break;
    }
    delay(1000);
  }

  bool attached = false;
  for (int i = 0; i < 30; i++) {
    _at->sendCommand("AT+CEREG?");
    int status = _at->waitResponse(2000, "+CEREG: 0,1", "+CEREG: 0,5",
                                   "+CEREG: 1", "+CEREG: 5");
    if (status > 0) {
      _at->waitResponse(1000);
      attached = true;
      break;
    }
    delay(1000);
  }
  return attached;
}

bool EC800ZModem::activatePDP(const char* apn) {
  String cmd = "AT+QICSGP=1,1,\"";
  cmd += apn;
  cmd += "\"";
  _at->sendCommand(cmd);
  if (_at->waitResponse(5000) != 1)
    return false;

  _at->sendCommand("AT+QIACT=1");
  return _at->waitResponse(150000) == 1;
}

bool EC800ZModem::openSocket(const char* host, uint16_t port) {
  _socketConnected = false;
  _cachedAvailable = 0;

  String cmd = "AT+QIOPEN=1,0,\"TCP\",\"";
  cmd += host;
  cmd += "\",";
  cmd += port;
  cmd += ",0,0";

  _at->sendCommand(cmd);
  if (_at->waitResponse(5000) != 1)
    return false;

  String resp;
  int res = _at->waitResponse(60000, resp, "+QIOPEN: 0,0", "+QIOPEN:");
  if (res == 1) {
    _socketConnected = true;
    return true;
  }
  return false;
}

bool EC800ZModem::openSSLSocket(const char* host, uint16_t port) {
  if (!_at) return false;
  _socketConnected = false;
  _cachedAvailable = 0;

  if (_rootCA) {
    sendAT("AT+QSSLCFG=\"cacert\",0,\"%s\"", _rootCA);
    _at->waitResponse("OK", "ERROR");
  }
  if (_clientCert && _clientKey) {
    sendAT("AT+QSSLCFG=\"clientcert\",0,\"%s\"", _clientCert);
    _at->waitResponse("OK", "ERROR");
    sendAT("AT+QSSLCFG=\"clientkey\",0,\"%s\"", _clientKey);
    _at->waitResponse("OK", "ERROR");
  }

  sendAT("AT+QSSLOPEN=0,\"%s\",%u", host, port);
  int resp = _at->waitResponse("CONNECT", "ERROR", "CLOSED");
  _socketConnected = (resp == 1);
  return _socketConnected;
}

int EC800ZModem::send(const uint8_t* data, size_t len) {
  if (!_socketConnected)
    return 0;

  String cmd = "AT+QISEND=0,";
  cmd += len;
  _at->sendCommand(cmd);

  if (_at->waitResponse(5000, ">") != 1)
    return 0;

  _at->streamWrite(data, len);

  if (_at->waitResponse(10000, "SEND OK\r\n", "SEND OK") > 0) {
    return len;
  }
  return 0;
}

int EC800ZModem::available() {
  if (!_socketConnected)
    return 0;
  if (_cachedAvailable > 0)
    return _cachedAvailable;

  _at->sendCommand("AT+QIRD=0,0");
  String resp;
  if (_at->waitResponse(2000, resp, "+QIRD: ") == 1) {
    String line;
    _at->waitResponse(1000, line, "\r\n");
    int firstComma = line.indexOf(',');
    if (firstComma > 0) {
      int secondComma = line.indexOf(',', firstComma + 1);
      if (secondComma > 0) {
        _cachedAvailable = line.substring(secondComma + 1).toInt();
      } else {
        _cachedAvailable = line.substring(firstComma + 1).toInt();
      }
    } else {
      _cachedAvailable = line.toInt();
    }
    _at->waitResponse(1000, "OK\r\n");
  }
  return _cachedAvailable;
}

int EC800ZModem::receive(uint8_t* buffer, size_t len) {
  if (!_socketConnected || len == 0)
    return 0;

  String cmd = "AT+QIRD=0,";
  cmd += len;
  _at->sendCommand(cmd);

  String resp;
  if (_at->waitResponse(2000, resp, "+QIRD: ") != 1)
    return 0;

  String line;
  _at->waitResponse(1000, line, "\r\n");
  int readLen = line.toInt();
  if (readLen == 0) {
    _at->waitResponse(1000, "OK\r\n");
    return 0;
  }

  int bytesRead = 0;
  uint32_t start = millis();
  while (bytesRead < readLen && millis() - start < 5000) {
    if (_at->stream().available()) {
      buffer[bytesRead++] = _at->streamRead();
    }
  }

  _at->waitResponse(1000, "OK\r\n");

  if (_cachedAvailable >= bytesRead) {
    _cachedAvailable -= bytesRead;
  } else {
    _cachedAvailable = 0;
  }

  return bytesRead;
}

void EC800ZModem::closeSocket() {
  _at->sendCommand("AT+QICLOSE=0");
  _at->waitResponse(10000);
  _socketConnected = false;
  _cachedAvailable = 0;
}

uint8_t EC800ZModem::connected() { return _socketConnected ? 1 : 0; }

void EC800ZModem::setDebug(Stream& dbg) {
  if (_at) {
    _at->setDebug(dbg);
  }
}

// SSL helpers
void EC800ZModem::setRootCA(const char* ca) {
  _rootCA = ca;
}

void EC800ZModem::setClientCert(const char* cert, const char* key) {
  _clientCert = cert;
  _clientKey = key;
}

// ---- AT helper wrappers ----
bool EC800ZModem::sendAT(const char* fmt, ...) {
  if (!_at || !fmt) return false;
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  _at->sendCommand(buf);
  return true;
}

int EC800ZModem::waitResponse(const char* r1, const char* r2, const char* r3,
                              const char* r4, const char* r5) {
  if (!_at) return 0;
  // Use default 1000ms timeout and call QuectelAT's waitResponse
  // Note: QuectelAT overload with 4 patterns uses 1000ms by default
  if (r5 && r5[0]) {
    // If 5th pattern is provided, use the 5-parameter version via timeout variant
    return _at->waitResponse(1000, r1, r2, r3, r4);
  }
  return _at->waitResponse(r1, r2, r3, r4);
}
bool EC800ZModem::sendSMS(const char* to, const char* text) {
  if (!to || !text || !_at) return false;
  String cmd = "AT+CMGS=\"";
  cmd += to;
  cmd += "\"";
  _at->sendCommand(cmd);
  if (_at->waitResponse(5000, ">") != 1) return false;
  _at->streamWrite((const uint8_t*)text, strlen(text));
  _at->streamWrite((const uint8_t*)"\x1A", 1);
  int r = _at->waitResponse(20000, "+CMGS", "OK", "ERROR");
  return (r == 1 || r == 2);
}
bool EC800ZModem::dial(const char* number) {
  if (!number || !_at) return false;
  String cmd = "ATD";
  cmd += number;
  cmd += ";";
  _at->sendCommand(cmd);
  int r = _at->waitResponse(10000, "OK", "NO CARRIER", "NO DIALTONE");
  return (r == 1);
}
bool EC800ZModem::answer() {
  if (!_at) return false;
  _at->sendCommand("ATA");
  int r = _at->waitResponse(10000, "OK", "NO CARRIER");
  return (r == 1);
}
void EC800ZModem::hangup() {
  if (!_at) return;
  _at->sendCommand("ATH");
  _at->waitResponse(5000);
}
