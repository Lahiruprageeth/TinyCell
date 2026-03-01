#include "QuectelAT.h"

QuectelAT::QuectelAT(Stream& serial) : _serial(&serial) {}

void QuectelAT::setDebug(Stream& dbg) {
  _debug = &dbg;
}

bool QuectelAT::sendCommand(const String& cmd) {
  if (_debug) {
    _debug->print("AT> ");
    _debug->println(cmd);
  }
  streamClear();
  streamWrite(cmd.c_str());
  streamWrite("\r\n");
  return true;
}

bool QuectelAT::sendCommand(const char* cmd) {
  if (_debug) {
    _debug->print("AT> ");
    _debug->println(cmd);
  }
  streamClear();
  streamWrite(cmd);
  streamWrite("\r\n");
  return true;
}

void QuectelAT::streamWrite(const char* s) { _serial->print(s); }

void QuectelAT::streamWrite(const uint8_t* buf, size_t size) {
  _serial->write(buf, size);
}

int QuectelAT::streamRead() { return _serial->read(); }

void QuectelAT::streamClear() {
  while (_serial->available()) {
    _serial->read();
  }
}

int QuectelAT::waitResponse(uint32_t timeout_ms, const char* r1, const char* r2,
                            const char* r3, const char* r4, const char* r5) {
  String data;
  String s1 = r1 ? String(r1) : String();
  String s2 = r2 ? String(r2) : String();
  String s3 = r3 ? String(r3) : String();
  String s4 = r4 ? String(r4) : String();
  String s5 = r5 ? String(r5) : String();
  return waitResponse(timeout_ms, data, s1, s2, s3, s4, s5);
}

int QuectelAT::waitResponse(uint32_t timeout_ms, String& data, String r1,
                            String r2, String r3, String r4, String r5) {
  data.reserve(64);
  uint32_t startMillis = millis();
  int index = 0;

  while (millis() - startMillis < timeout_ms) {
    while (_serial->available() > 0) {
      char c = _serial->read();
      if (_debug) {
        _debug->print(c);
      }
      data += c;

            if (r1.length() && data.endsWith(r1)) {
        index = 1;
        goto finish;
      }
      if (r2.length() && data.endsWith(r2)) {
        index = 2;
        goto finish;
      }
      if (r3.length() && data.endsWith(r3)) {
        index = 3;
        goto finish;
      }
      if (r4.length() && data.endsWith(r4)) {
        index = 4;
        goto finish;
      }
      if (r5.length() && data.endsWith(r5)) {
        index = 5;
        goto finish;
      }
    }
    delay(1);  // yield to prevent watchdog timeouts
  }

finish:
  return index;
}

void QuectelAT::processURC() {
  // We will expand this if the modem asynchronously spews '+QIURC: "recv",...'
  // Usually these are handled when we read from the stream while waiting for
  // responses.
}