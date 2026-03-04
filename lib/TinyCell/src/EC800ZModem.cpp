#include "EC800ZModem.h"
#include "QuectelAT.h"
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

EC800ZModem::EC800ZModem(QuectelAT &at) : _at(&at), _ownsAt(false) {}

EC800ZModem::EC800ZModem(Stream &serial)
    : _at(new QuectelAT(serial)), _ownsAt(true) {}

EC800ZModem::~EC800ZModem() {
  if (_ownsAt)
    delete _at;
}

bool EC800ZModem::attachNetwork() {
  _at->sendCommand("AT+CGATT=1");
  if (_at->waitResponse(30000) != 1)
    return false;
  _at->sendCommand("AT+CGREG?");
  if (_at->waitResponse(5000, "+CGREG: 0,1") == 1 ||
      _at->waitResponse(500, "+CGREG: 0,5") == 1)
    return true;
  return false;
}

bool EC800ZModem::activatePDP(const char *apn) {
  _at->sendCommand("AT+QICSGP=1,1,\"" + String(apn) + "\",\"\",\"\",0");
  _at->waitResponse(5000);
  _at->sendCommand("AT+QIACT=1");
  return _at->waitResponse(30000) == 1;
}

bool EC800ZModem::openSocket(const char *host, uint16_t port) {
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

  // 1. Wait for the command to be accepted
  if (_at->waitResponse(5000) != 1)
    return false;

  // 2. Wait for the asynchronous "+QIOPEN: " response
  String resp;
  if (_at->waitResponse(25000, resp, "+QIOPEN: ") == 1) {
    // Read the rest of the line (e.g., "0,0")
    String line;
    _at->waitResponse(1000, line, "\r\n");

    int comma = line.indexOf(',');
    if (comma != -1) {
      int err = line.substring(comma + 1).toInt();
      // 0 = Success, 562 = Already connected
      if (err == 0 || err == 562) {
        _socketConnected = true;
        return true;
      }
    }
  }
  return false;
}

bool EC800ZModem::openSSLSocket(const char *host, uint16_t port) {
  // SSL not implemented in this demo; reuse plain TCP socket
  return openSocket(host, port);
}

void EC800ZModem::restart() {
  closeSocket();
  attachNetwork();
}

int EC800ZModem::getSignalQuality() {
  _at->sendCommand("AT+CSQ");
  String resp;
  if (_at->waitResponse(2000, resp) == 1) {
    int start = resp.indexOf("+CSQ: ");
    if (start != -1) {
      return resp.substring(start + 6, resp.indexOf(',')).toInt();
    }
  }
  return 0;
}

String EC800ZModem::getIMEI() {
  _at->sendCommand("AT+GSN");
  String resp;
  if (_at->waitResponse(2000, resp) == 1) {
    resp.trim();
    int idx = resp.indexOf("\r\n");
    if (idx != -1)
      resp = resp.substring(0, idx);
    return resp;
  }
  return "";
}

String EC800ZModem::getIMSI() {
  _at->sendCommand("AT+CIMI");
  String resp;
  if (_at->waitResponse(2000, resp) == 1) {
    resp.trim();
    int idx = resp.indexOf("\r\n");
    if (idx != -1)
      resp = resp.substring(0, idx);
    return resp;
  }
  return "";
}

String EC800ZModem::getCCID() {
  _at->sendCommand("AT+QCCID");
  String resp;
  if (_at->waitResponse(2000, resp, "+QCCID: ") == 1) {
    String line;
    _at->waitResponse(1000, line, "\r\n");
    line.trim();
    return line;
  }
  return "";
}

void EC800ZModem::setRootCA(const char *ca) { _rootCA = ca; }
void EC800ZModem::setClientCert(const char *cert, const char *key) {
  _clientCert = cert;
  _clientKey = key;
}

int EC800ZModem::send(const uint8_t *data, size_t len) {
  if (!_socketConnected)
    return 0;
  String cmd = "AT+QISEND=0," + String(len);
  _at->sendCommand(cmd);
  if (_at->waitResponse(5000, ">") != 1)
    return 0;
  _at->streamWrite(data, len);
  if (_at->waitResponse(10000, "SEND OK\r\n", "SEND OK") > 0)
    return (int)len;
  return 0;
}

int EC800ZModem::available() {
  if (!_socketConnected)
    return 0;
  if (_rxBufferLen > _rxBufferPos)
    return _rxBufferLen - _rxBufferPos;
  _at->processURC();
  if (_at->isRecvUrcSeen())
    return 1;
  if (_at->stream().available() > 0)
    return 1;
  return 0;
}

int EC800ZModem::receive(uint8_t *buffer, size_t len) {
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
  _at->clearRecvUrc();
  _at->sendCommand("AT+QIRD=0,256");
  String header;
  if (_at->waitResponse(1000, header, "+QIRD: ") != 1)
    return 0;
  String line;
  _at->waitResponse(500, line, "\r\n");
  int readLen = line.toInt();
  if (readLen <= 0) {
    _at->waitResponse(500, "OK\r\n", "OK");
    return 0;
  }
  int bytesRead = 0;
  uint32_t start = millis();
  while (bytesRead < readLen && millis() - start < 2000) {
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

void EC800ZModem::closeSocket() {
  _at->sendCommand("AT+QICLOSE=0");
  _at->waitResponse(2000);
  _socketConnected = false;
}

uint8_t EC800ZModem::connected() { return _socketConnected ? 1 : 0; }

void EC800ZModem::setDebug(Stream &dbg) {
  if (_at)
    _at->setDebug(dbg);
}

bool EC800ZModem::sendAT(const char *fmt, ...) {
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

int EC800ZModem::waitResponse(const char *r1, const char *r2, const char *r3,
                              const char *r4, const char *r5) {
  if (!_at)
    return 0;
  return _at->waitResponse(1000, r1, r2, r3, r4, r5);
}

bool EC800ZModem::sendSMS(const char *to, const char *text) {
  if (!to || !text || !_at)
    return false;
  _at->sendCommand("AT+CMGS=\"" + String(to) + "\"");
  if (_at->waitResponse(5000, ">") != 1)
    return false;
  _at->streamWrite((const uint8_t *)text, strlen(text));
  const uint8_t ctrlZ = 26;
  _at->streamWrite(&ctrlZ, 1);
  return _at->waitResponse(30000, "+CMGS:") == 1;
}

bool EC800ZModem::dial(const char *number) {
  // Not implemented in this demo
  return false;
}

bool EC800ZModem::answer() {
  // Not implemented in this demo
  return false;
}

void EC800ZModem::hangup() {
  // Not implemented in this demo
}
