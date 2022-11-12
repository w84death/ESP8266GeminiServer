// Fork of https://github.com/Astrrra/ESP8266GeminiServer
// Modified by Krzysztof Krystian Jankowski
// https://krzysztofjankowski.com
// gemini://gemini.p1x.in

#include<ESP8266WiFi.h>
#include<time.h>
#include<LittleFS.h>

#include "request.h"
#include "splitstr.h"

#include "config.h"

#define	CHUNK_SIZE 256

// response headers
static const char *HEADER_GEM_OK            = "20 text/gemini\r\n";                 // .gmi
static const char *HEADER_MARKDOWN_OK       = "20 text/markdown\r\n";               // .md
static const char *HEADER_PLAIN_OK          = "20 text/plain\r\n";                  // .txt
static const char *HEADER_JPEG_OK           = "20 image/jpeg\r\n";                  // .jpg
static const char *HEADER_BIN_OK            = "20 application/octet-stream\r\n";    // other stuff
static const char *HEADER_NOT_FOUND         = "51 File Not Found\r\n";
static const char *HEADER_INTERNAL_FAIL     = "50 Internal Server Error\r\n";

//WiFiServer server(PORT);

// tls server
BearSSL::WiFiServerSecure server(PORT);

void setup()
{
    Serial.begin(9600);
    delay(1000);
    Serial.print("-------------------------\n");
    Serial.print("#> Starting GEMINI Server\n");

    // LED blinking
    pinMode(LED, OUTPUT);

    // setup LittleFS
    LittleFSConfig cfg;
    cfg.setAutoFormat(false);

    if(!LittleFS.begin()){
      Serial.println("!> An Error has occurred while mounting LittleFS");
      return;
    }

    Serial.print("*> Connecting to WiFi...");
    // connect to wifi as defined in config.h
    WiFi.mode(WIFI_STA);
    WiFi.hostname(HOSTNAME);
    WiFi.begin(SSID, PASSWD);

    // wait for connection
    while(WiFi.status() != WL_CONNECTED){delay(1000);}
    Serial.print(" connected IP: [");
    Serial.print(WiFi.localIP());
    Serial.print(" ]\n");

    // add private cert and key
    BearSSL::X509List *serverCertList = new BearSSL::X509List(server_cert);
    BearSSL::PrivateKey *serverPrivateKey = new BearSSL::PrivateKey(server_private_key);
    server.setRSACert(serverCertList, serverPrivateKey);

    Serial.print("*> Starting gemini server...");

    // begin accepting connections
    server.begin();
    Serial.print("started!\n");
    Serial.print("#> Waiting for incoming connections...\n");
}

void loop()
{
    BearSSL::WiFiClientSecure incomingConnection = server.available();

    if (!incomingConnection){return;} // if no connections then do nothing

    while (incomingConnection.connected()){
        Serial.print("...");
        if (incomingConnection.available()){
            Serial.print("\n*> Client connected...\n");
            digitalWrite(LED, LOW);
            String requestString = incomingConnection.readStringUntil('\r\n'); // get request string
            Request request = Request(requestString);
            Serial.print(request.getFile_path());
            if (!LittleFS.exists(request.getFile_path().c_str())) // if file does not exist
            {
                // Send file not found header
                incomingConnection.read();
                incomingConnection.write((uint8_t*)HEADER_NOT_FOUND, strlen(HEADER_NOT_FOUND));
                incomingConnection.flush();
                incomingConnection.stop();
                Serial.print(" ...not found\n");
            }
            else // file exists
            {
                File file = LittleFS.open(request.getFile_path().c_str(), "r"); // open file
                Serial.print(" ...sending!\n");
                if (!file.isFile()) // error opening file
                {
                    file.close();

                    incomingConnection.read();
                    incomingConnection.write((uint8_t*)HEADER_INTERNAL_FAIL, strlen(HEADER_INTERNAL_FAIL));
                    incomingConnection.flush();
                    incomingConnection.stop();
                }
                else // file is OK
                {
                    incomingConnection.read();  // read the rest of the incomig data

                    String extension = request.getFile_ext(); // get the file extension and send the required header
                    if (extension.equals("gmi"))        {incomingConnection.write((uint8_t*)HEADER_GEM_OK, strlen(HEADER_GEM_OK));}
                    else if (extension.equals("txt"))   {incomingConnection.write((uint8_t*)HEADER_PLAIN_OK, strlen(HEADER_PLAIN_OK));}
                    else if (extension.equals("md"))    {incomingConnection.write((uint8_t*)HEADER_MARKDOWN_OK, strlen(HEADER_MARKDOWN_OK));}
                    else if (extension.equals("jpg"))   {incomingConnection.write((uint8_t*)HEADER_JPEG_OK, strlen(HEADER_JPEG_OK));}
                    else                                {incomingConnection.write((uint8_t*)HEADER_BIN_OK, strlen(HEADER_BIN_OK));}

                    // time to read the file itself

                    // calculate count of chunks and size of the stuff that didn't fit
                    int fsize = file.size();
                    int chunk_count = (fsize / CHUNK_SIZE);
                    int rest_size = (fsize % CHUNK_SIZE);

                    // read chunks from the file and send them
                    char chunk[CHUNK_SIZE];
                    for (int i = 0; i < chunk_count; i++){
                        file.read((uint8_t*)chunk, CHUNK_SIZE);
                        incomingConnection.write((uint8_t*)chunk, CHUNK_SIZE);
                    }

                    // read and send what's left
                    char rest[rest_size];
                    file.read((uint8_t*)rest, rest_size);
                    incomingConnection.write((uint8_t*)rest, rest_size);

                    file.close(); // guess what this one does ^_^

                    // those ones too
                    incomingConnection.flush();
                    incomingConnection.stop();
                }
            }
            digitalWrite(LED, HIGH);
        }
    }
}
