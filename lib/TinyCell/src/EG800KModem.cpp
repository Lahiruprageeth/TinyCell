#include "EG800KModem.h"

#include <cstdarg>
#include <stdio.h>
#include <string.h>

// constructor using existing QuectelAT instance
EG800KModem::EG800KModem(QuectelAT &at) : _at(&at), _ownsAt(false) {}

// convenience constructor that allocates its own QuectelAT
EG800KModem::EG800KModem(Stream &serial)
    : _at(new QuectelAT(serial)), _ownsAt(true) {}

EG800KModem::~EG800KModem() {
  if (_ownsAt && _at) {
    delete _at;
  }
}

bool EG800KModem::attachNetwork() {
  _at->sendCommand("AT");
  _at->waitResponse(1000);

  _at->sendCommand("ATE0");
  _at->waitResponse(1000);

  // Wait for SIM READY
  for (int i = 0; i < 5; i++) {
    _at->sendCommand("AT+CPIN?");
    if (_at->waitResponse(2000, "+CPIN: READY\r\n", "+CPIN: READY") > 0) {
      _at->waitResponse(1000); // clear trailing OK
      break;
    }
    delay(1000);
  }

  // Wait for Network
  bool attached = false;
  for (int i = 0; i < 30; i++) {
    _at->sendCommand("AT+CEREG?");
    int status = _at->waitResponse(2000, "+CEREG: 0,1", "+CEREG: 0,5",
                                   "+CEREG: 1", "+CEREG: 5");
    if (status > 0) {
      _at->waitResponse(1000); // clear trailing OK
      attached = true;
      break;
    }
    delay(1000);
  }
  return attached;
}

bool EG800KModem::activatePDP(const char *apn) {
  _at->sendCommand("AT+QICSGP=1,1,\"" + String(apn) + "\",\"\",\"\",0");
  _at->waitResponse(5000);
  _at->sendCommand("AT+QIACT=1");
  return _at->waitResponse(150000) == 1; // 150s per Quectel manual
}

bool EG800KModem::openSocket(const char *host, uint16_t port) {
  _socketConnected = false;
  _rxBufferPos = 0;
  _rxBufferLen = 0;

  _at->sendCommand("AT+QICLOSE=0");
  _at->waitResponse(2000);

  String cmd = "AT+QIOPEN=1,0,\"TCP\",\"";
  cmd += host;
  cmd += "\",";
  cmd += port;
  cmd += ",0,0";

  _at->sendCommand(cmd);
  if (_at->waitResponse(5000) != 1)
    return false;

  String resp;
  // Wait for the +QIOPEN URC with increased timeout
  int res = _at->waitResponse(150000, resp, "+QIOPEN: 0,0", "+QIOPEN:");
  if (res == 1) {
    _socketConnected = true;
    return true;
  } else if (res == 2) {
    // If we only matched prefix, consume the rest of the line
    String line;
    _at->waitResponse(1000, line, "\r\n");
    if (line.startsWith("0,0") || line.indexOf(",0") != -1) {
      _socketConnected = true;
      return true;
    }
  }
  return false;
}

// SSL helpers
void EG800KModem::setRootCA(const char *ca) { _rootCA = ca; }

void EG800KModem::setClientCert(const char *cert, const char *key) {
  _clientCert = cert;
  _clientKey = key;
}

bool EG800KModem::openSSLSocket(const char *host, uint16_t port) {
  if (!_at)
    return false;
  _socketConnected = false;
  _rxBufferPos = 0;
  _rxBufferLen = 0;

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
  int resp = _at->waitResponse(150000, "CONNECT", "ERROR", "CLOSED");
  _socketConnected = (resp == 1);
  return _socketConnected;
}

void EG800KModem::restart() {
  _at->sendCommand("AT+CFUN=1,1");
  _at->waitResponse(150000);
}

int EG800KModem::getSignalQuality() {
  _at->sendCommand("AT+CSQ");
  String resp;
  int quality = 0;
  if (_at->waitResponse(2000, resp, "+CSQ: ") == 1) {
    String line;
    _at->waitResponse(1000, line, "\r\n");
    quality = line.substring(0, line.indexOf(',')).toInt();
  }
  _at->waitResponse(1000, "OK\r\n");
  return quality;
}

String EG800KModem::getIMEI() {
  _at->sendCommand("AT+CGSN");
  String res;
  if (_at->waitResponse(2000, res) == 1) {
    res.trim();
    int idx = res.indexOf("\r\n");
    if (idx != -1)
      res = res.substring(0, idx);
    return res;
  }
  return "";
}

String EG800KModem::getIMSI() {
  _at->sendCommand("AT+CIMI");
  String res;
  if (_at->waitResponse(2000, res) == 1) {
    res.trim();
    int idx = res.indexOf("\r\n");
    if (idx != -1)
      res = res.substring(0, idx);
    return res;
  }
  return "";
}

String EG800KModem::getCCID() {
  _at->sendCommand("AT+QCCID");
  String res;
  if (_at->waitResponse(2000, res, "+QCCID: ") == 1) {
    String line;
    _at->waitResponse(1000, line, "\r\n");
    line.trim();
    return line;
  }
  return "";
}

int EG800KModem::send(const uint8_t *data, size_t len) {
  if (!_socketConnected)
    return 0;

  _at->sendCommand("AT+QISEND=0," + String(len));
  if (_at->waitResponse(5000, ">") != 1)
    return 0;

  _at->streamWrite(data, len);

  if (_at->waitResponse(10000, "SEND OK\r\n") > 0) {
    return (int)len;
  }
  return 0;
}

int EG800KModem::available() {
  if (!_socketConnected)
    return 0;
  if (_rxBufferLen > _rxBufferPos)
    return _rxBufferLen - _rxBufferPos;

  static uint32_t lastPoll = 0;
  // Poll immediately if URC was seen, otherwise throttle to 100ms
  if (!_at->isRecvUrcSeen() && (millis() - lastPoll < 100)) {
    return 0;
  }
  lastPoll = millis();
  _at->clearRecvUrc();

  _at->setSilent(true);
  _at->sendCommand("AT+QIRD=0,0");
  String resp;
  int res = _at->waitResponse(500, resp, "+QIRD: ");

  if (res == 1) {
    String line;
    _at->waitResponse(200, line, "\r\n");
    _at->waitResponse(200, "OK\r\n"); // Consuming final OK while silent
    _at->setSilent(false);
    int firstComma = line.indexOf(',');
    if (firstComma != -1) {
      int secondComma = line.indexOf(',', firstComma + 1);
      if (secondComma != -1) {
        return line.substring(secondComma + 1).toInt();
      }
      return line.substring(firstComma + 1).toInt();
    }
    return line.toInt();
  }
  _at->setSilent(false);
  return 0;
}

int EG800KModem::receive(uint8_t *buffer, size_t len) {
  if (!_socketConnected || len == 0)
    return 0;

  if (_rxBufferLen > _rxBufferPos) {
    size_t toRead = _rxBufferLen - _rxBufferPos;
    if (toRead > len)
      toRead = len;
    memcpy(buffer, _rxBuffer + _rxBufferPos, toRead);
    _rxBufferPos += toRead;
    return (int)toRead;
  }

  _rxBufferPos = 0;
  _rxBufferLen = 0;

  size_t fetchLen = (len > RX_BUFFER_SIZE) ? len : RX_BUFFER_SIZE;
  if (fetchLen > 1500)
    fetchLen = 1500;

  _at->sendCommand("AT+QIRD=0," + String(fetchLen));
  String resp;
  if (_at->waitResponse(1000, resp, "+QIRD: ") != 1)
    return 0;

  String line;
  _at->waitResponse(500, line, "\r\n");
  int modemLen = line.toInt();
  if (modemLen <= 0) {
    _at->waitResponse(500, "OK\r\n", "OK");
    return 0;
  }

  int bytesRead = 0;
  uint32_t start = millis();
  while (bytesRead < modemLen && bytesRead < RX_BUFFER_SIZE &&
         (millis() - start < 5000)) {
    if (_at->stream().available()) {
      _rxBuffer[bytesRead++] = (uint8_t)_at->streamRead();
    }
  }

  _rxBufferLen = (uint16_t)bytesRead;
  _at->waitResponse(1000, "OK\r\n", "OK");

  size_t toServe = _rxBufferLen;
  if (toServe > len)
    toServe = len;
  memcpy(buffer, _rxBuffer, toServe);
  _rxBufferPos = (uint16_t)toServe;

  return (int)toServe;
}

void EG800KModem::closeSocket() {
  _at->sendCommand("AT+QICLOSE=0");
  _at->waitResponse(10000);
  _socketConnected = false;
  _rxBufferPos = 0;
  _rxBufferLen = 0;
}

void EG800KModem::processURC() { _at->processURC(); }

uint8_t EG800KModem::connected() { return _socketConnected ? 1 : 0; }

void EG800KModem::setDebug(Stream *dbg) {
  if (_at) {
    _at->setDebug(dbg);
  }
}

// ---- AT helper wrappers ----
bool EG800KModem::sendAT(const char *fmt, ...) {
  if (!_at || !fmt)
    return false;
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  _at->sendCommand(buf);
  return true;
}

int EG800KModem::waitResponse(const char *r1, const char *r2, const char *r3,
                              const char *r4, const char *r5) {
  if (!_at)
    return 0;
  return _at->waitResponse(1000, r1, r2, r3, r4, r5);
}

bool EG800KModem::sendSMS(const char *to, const char *text) {
  if (!to || !text || !_at)
    return false;
  _at->sendCommand("AT+CMGS=\"" + String(to) + "\"");
  if (_at->waitResponse(5000, ">") != 1)
    return false;

  _at->streamWrite((const uint8_t *)text, strlen(text));
  const uint8_t ctrlZ = 26;
  _at->streamWrite(&ctrlZ, 1);

  int r = _at->waitResponse(30000, "+CMGS:", "OK", "ERROR");
  return (r == 1 || r == 2);
}

bool EG800KModem::dial(const char *number) {
  if (!number || !_at)
    return false;
  _at->sendCommand("ATD" + String(number) + ";");
  int r = _at->waitResponse(10000, "OK", "NO CARRIER", "NO DIALTONE");
  return (r == 1);
}

bool EG800KModem::answer() {
  if (!_at)
    return false;
  _at->sendCommand("ATA");
  int r = _at->waitResponse(10000, "OK", "NO CARRIER");
  return (r == 1);
}

void EG800KModem::hangup() {
  if (!_at)
    return;
  _at->sendCommand("ATH");
  _at->waitResponse(5000);
}