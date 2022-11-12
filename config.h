#ifndef _CONFIG_H_
#define _CONFIG_H_

#define SSID   "WIFI-NAME"
#define PASSWD  "WIFI-PASSWORD"

#define HOSTNAME "gemini"
#define PORT 1965
#define LED 2

const char server_private_key[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
PASTE HERE CONTENT OF KEY FILE
-----END PRIVATE KEY-----


)EOF";

// The server's public certificate which must be shared
const char server_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
PASTE HERE CONTENT OF CERT FILE
-----END CERTIFICATE-----


)EOF";

#endif // _CONFIG_H_
