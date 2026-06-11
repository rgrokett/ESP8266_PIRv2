/*
 * HTTPS client with bounded redirect handling and chunked response support.
 * Based on HTTPSRedirect 2.1 by Sujay Phadke.
 */

#include "HTTPSRedirect.h"

#include "DebugMacros.h"

namespace {

constexpr size_t READ_BUFFER_SIZE = 128;

String trimCopy(String value) {
  value.trim();
  return value;
}

}  // namespace

HTTPSRedirect::HTTPSRedirect() : _httpsPort(443) {
  initialize();
}

HTTPSRedirect::HTTPSRedirect(uint16_t port) : _httpsPort(port) {
  initialize();
}

HTTPSRedirect::~HTTPSRedirect() {
}

void HTTPSRedirect::initialize() {
  _keepAlive = false;
  _printResponseBody = false;
  _maxRedirects = 3;
  _contentTypeHeader = "application/x-www-form-urlencoded";
  setTimeout(15000);
  initializeResponse();
}

void HTTPSRedirect::initializeResponse() {
  _headers.transferEncoding = "";
  _headers.contentLength = 0;
  _headers.hasContentLength = false;
  _response.statusCode = 0;
  _response.reasonPhrase = "";
  _response.redirected = false;
  _response.body = "";
}

bool HTTPSRedirect::sendRequest() {
  unsigned int redirectCount = 0;

  while (true) {
    if (!connected()) {
      DPRINTLN(F("Not connected to host"));
      return false;
    }

    while (available()) {
      read();
    }

    DPRINTLN(_request);
    if (print(_request) != _request.length()) {
      DPRINTLN(F("Failed to write complete HTTP request"));
      return false;
    }

    const int statusCode = readResponseStatus();
    if (statusCode == 0) {
      return false;
    }

    String location;
    if (!readHeaders(&location)) {
      return false;
    }

    if (statusCode >= 200 && statusCode < 300) {
      return readBody();
    }

    const bool isRedirect =
        statusCode == 301 || statusCode == 302 || statusCode == 303 ||
        statusCode == 307 || statusCode == 308;
    if (!isRedirect) {
      DPRINT(F("Unexpected HTTP status: "));
      DPRINTLN(statusCode);
      return false;
    }

    if (redirectCount >= _maxRedirects) {
      DPRINTLN(F("Maximum redirect count reached"));
      return false;
    }
    ++redirectCount;

    if (!parseLocation(location)) {
      DPRINTLN(F("Invalid or missing redirect Location header"));
      return false;
    }

    _response.redirected = true;
    createGetRequest(_redirectUrl, _redirectHost.c_str());
    stop();
    if (!connect(_redirectHost.c_str(), _httpsPort)) {
      DPRINTLN(F("Connection to redirected host failed"));
      return false;
    }
  }
}

void HTTPSRedirect::createGetRequest(const String& url, const char* host) {
  _request = String(F("GET ")) + url + F(" HTTP/1.1\r\nHost: ") + host +
             F("\r\nUser-Agent: ESP8266-PIR/3\r\n") +
             (_keepAlive ? "" : "Connection: close\r\n") + F("\r\n");
}

void HTTPSRedirect::createPostRequest(const String& url, const char* host,
                                      const String& payload) {
  _request = String(F("POST ")) + url + F(" HTTP/1.1\r\nHost: ") + host +
             F("\r\nUser-Agent: ESP8266-PIR/3\r\n") +
             (_keepAlive ? "" : "Connection: close\r\n") +
             F("Content-Type: ") + _contentTypeHeader +
             F("\r\nContent-Length: ") + payload.length() + F("\r\n\r\n") +
             payload;
}

int HTTPSRedirect::readResponseStatus() {
  String line;

  do {
    line = readStringUntil('\n');
    if (line.length() == 0) {
      DPRINTLN(F("Timed out waiting for HTTP status"));
      return 0;
    }
    line.trim();
  } while (line.length() == 0);

  if (!line.startsWith(F("HTTP/1."))) {
    DPRINTLN(F("Invalid HTTP status line"));
    return 0;
  }

  const int firstSpace = line.indexOf(' ');
  const int secondSpace = line.indexOf(' ', firstSpace + 1);
  if (firstSpace < 0) {
    return 0;
  }

  _response.statusCode =
      line.substring(firstSpace + 1,
                     secondSpace < 0 ? line.length() : secondSpace)
          .toInt();
  _response.reasonPhrase =
      secondSpace < 0 ? "" : line.substring(secondSpace + 1);
  return _response.statusCode;
}

bool HTTPSRedirect::readHeaders(String* location) {
  _headers.transferEncoding = "";
  _headers.contentLength = 0;
  _headers.hasContentLength = false;
  *location = "";

  while (true) {
    String line = readStringUntil('\n');
    if (line.length() == 0) {
      DPRINTLN(F("Timed out while reading HTTP headers"));
      return false;
    }
    if (line == "\r") {
      return true;
    }

    line.trim();
    const int separator = line.indexOf(':');
    if (separator < 0) {
      continue;
    }

    String name = line.substring(0, separator);
    name.toLowerCase();
    const String value = trimCopy(line.substring(separator + 1));

    if (name == F("transfer-encoding")) {
      _headers.transferEncoding = value;
      _headers.transferEncoding.toLowerCase();
    } else if (name == F("content-length")) {
      _headers.contentLength = static_cast<size_t>(value.toInt());
      _headers.hasContentLength = true;
    } else if (name == F("location")) {
      *location = value;
    }
  }
}

bool HTTPSRedirect::parseLocation(const String& location) {
  if (location.length() == 0) {
    return false;
  }

  if (location.startsWith(F("https://"))) {
    const int pathStart = location.indexOf('/', 8);
    if (pathStart < 0) {
      _redirectHost = location.substring(8);
      _redirectUrl = "/";
    } else {
      _redirectHost = location.substring(8, pathStart);
      _redirectUrl = location.substring(pathStart);
    }
    return _redirectHost.length() > 0;
  }

  if (location[0] == '/') {
    _redirectUrl = location;
    return _redirectHost.length() > 0;
  }

  return false;
}

bool HTTPSRedirect::readBody() {
  if (_headers.transferEncoding.indexOf(F("chunked")) >= 0) {
    return readChunkedBody();
  }
  if (_headers.hasContentLength) {
    return readFixedBody(_headers.contentLength);
  }
  return readUntilClose();
}

void HTTPSRedirect::appendBody(const char* data, size_t length) {
  _response.body.concat(data, length);
  if (_printResponseBody) {
    Serial.write(reinterpret_cast<const uint8_t*>(data), length);
  }
}

bool HTTPSRedirect::readFixedBody(size_t length) {
  char buffer[READ_BUFFER_SIZE];

  while (length > 0) {
    const size_t requested =
        length < sizeof(buffer) ? length : sizeof(buffer);
    const size_t received = readBytes(buffer, requested);
    if (received == 0) {
      DPRINTLN(F("Timed out while reading HTTP body"));
      return false;
    }
    appendBody(buffer, received);
    length -= received;
  }
  return true;
}

bool HTTPSRedirect::readChunkedBody() {
  while (true) {
    String sizeLine = readStringUntil('\n');
    if (sizeLine.length() == 0) {
      return false;
    }
    sizeLine.trim();

    const int extension = sizeLine.indexOf(';');
    if (extension >= 0) {
      sizeLine.remove(extension);
    }

    const size_t chunkSize =
        static_cast<size_t>(strtoul(sizeLine.c_str(), nullptr, 16));
    if (chunkSize == 0) {
      while (true) {
        const String trailer = readStringUntil('\n');
        if (trailer.length() == 0 || trailer == "\r") {
          break;
        }
      }
      return true;
    }

    if (!readFixedBody(chunkSize)) {
      return false;
    }

    char terminator[2];
    if (readBytes(terminator, sizeof(terminator)) != sizeof(terminator) ||
        terminator[0] != '\r' || terminator[1] != '\n') {
      DPRINTLN(F("Invalid chunk terminator"));
      return false;
    }
  }
}

bool HTTPSRedirect::readUntilClose() {
  char buffer[READ_BUFFER_SIZE];

  while (connected() || available()) {
    const size_t received = readBytes(buffer, sizeof(buffer));
    if (received > 0) {
      appendBody(buffer, received);
    } else if (!connected()) {
      break;
    } else {
      DPRINTLN(F("Timed out while waiting for connection close"));
      return false;
    }
  }
  return true;
}

bool HTTPSRedirect::GET(const String& url, const char* host) {
  return GET(url, host, _printResponseBody);
}

bool HTTPSRedirect::GET(const String& url, const char* host,
                        const bool& displayBody) {
  const bool previousDisplay = _printResponseBody;
  _printResponseBody = displayBody;
  _redirectHost = host;
  _redirectUrl = url;
  initializeResponse();
  createGetRequest(url, host);
  const bool result = sendRequest();
  _printResponseBody = previousDisplay;
  return result;
}

bool HTTPSRedirect::POST(const String& url, const char* host,
                         const String& payload) {
  return POST(url, host, payload, _printResponseBody);
}

bool HTTPSRedirect::POST(const String& url, const char* host,
                         const String& payload, const bool& displayBody) {
  const bool previousDisplay = _printResponseBody;
  _printResponseBody = displayBody;
  _redirectHost = host;
  _redirectUrl = url;
  initializeResponse();
  createPostRequest(url, host, payload);
  const bool result = sendRequest();
  _printResponseBody = previousDisplay;
  return result;
}

int HTTPSRedirect::getStatusCode() const {
  return _response.statusCode;
}

String HTTPSRedirect::getReasonPhrase() const {
  return _response.reasonPhrase;
}

String HTTPSRedirect::getResponseBody() const {
  return _response.body;
}

void HTTPSRedirect::setPrintResponseBody(bool display) {
  _printResponseBody = display;
}

void HTTPSRedirect::setMaxRedirects(unsigned int count) {
  _maxRedirects = count;
}

void HTTPSRedirect::setContentTypeHeader(const char* type) {
  _contentTypeHeader = type;
}

bool HTTPSRedirect::reConnectFinalEndpoint() {
  stop();
  if (!connect(_redirectHost.c_str(), _httpsPort)) {
    return false;
  }
  return sendRequest();
}
