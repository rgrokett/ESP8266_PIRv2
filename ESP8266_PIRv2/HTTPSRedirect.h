/*
 * HTTPS client with bounded redirect handling and chunked response support.
 * Based on HTTPSRedirect 2.1 by Sujay Phadke.
 */

#pragma once

#include <WiFiClientSecure.h>

class HTTPSRedirect : public WiFiClientSecure {
 private:
  struct HeaderFields {
    String transferEncoding;
    size_t contentLength;
    bool hasContentLength;
  };

  struct Response {
    int statusCode;
    String reasonPhrase;
    bool redirected;
    String body;
  };

  const uint16_t _httpsPort;
  bool _keepAlive;
  String _redirectUrl;
  String _redirectHost;
  unsigned int _maxRedirects;
  const char* _contentTypeHeader;
  HeaderFields _headers;
  String _request;
  Response _response;
  bool _printResponseBody;

  void initialize();
  void initializeResponse();
  bool sendRequest();
  bool readHeaders(String* location);
  bool parseLocation(const String& location);
  bool readBody();
  bool readFixedBody(size_t length);
  bool readChunkedBody();
  bool readUntilClose();
  int readResponseStatus();
  void appendBody(const char* data, size_t length);
  void createGetRequest(const String& url, const char* host);
  void createPostRequest(const String& url, const char* host,
                         const String& payload);

 public:
  HTTPSRedirect();
  explicit HTTPSRedirect(uint16_t port);
  ~HTTPSRedirect();

  bool GET(const String& url, const char* host);
  bool GET(const String& url, const char* host, const bool& displayBody);
  bool POST(const String& url, const char* host, const String& payload);
  bool POST(const String& url, const char* host, const String& payload,
            const bool& displayBody);

  int getStatusCode() const;
  String getReasonPhrase() const;
  String getResponseBody() const;

  void setPrintResponseBody(bool display);
  void setMaxRedirects(unsigned int count);
  void setContentTypeHeader(const char* type);
  bool reConnectFinalEndpoint();
};
