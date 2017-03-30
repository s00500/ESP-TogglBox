#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>  // Use WiFiClientSecure class to create TLS connection
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIN D7

#include "global.h"
#include "eeprom.h"
#include "webpage.h"
#include "timekeeping.h"
#include "base64.h"
#include "toggle_API.h"

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, NEOPIN, NEO_GRB + NEO_KHZ800);
long oldTime = 0;

bool breathing = true;

void setup() {
  pixel.begin(); // This initializes the NeoPixel library.


  pixel.setPixelColor(0, pixel.Color(0, 255, 255));
  pixel.show();

  Serial.begin(115200);
  Serial.println();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);  // delete any old wifi configuration
  delay(200);
  switch_changed = false;

  WiFi.mode(WIFI_STA);

  EEPROM.begin(128);
  if (!ReadConfig()) {
    writeDefaultConfig();
  }

  server.on("/", sendPage);
  server.onNotFound(handleNotFound);  // handle page not found (send 404 response)
  server.begin();                     // start webserver

  // if string read from eeprom is used directly, it won't connect after AP mode. A bug?

  String ssidstr = config.ssid;
  String pwstr = config.password;
  WiFi.begin(ssidstr.c_str(), pwstr.c_str());

  Serial.print(F("connecting to "));
  Serial.println(ssidstr);

  uint8_t trialcounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    trialcounter++;

    if (trialcounter > 70 || (startpressed && stoppressed))  // timeout or both buttons pressed during connect
    {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP_togglr", "12345678");  // start accesspoint with config page
      Serial.println(F("Starting Accesspoint:"));
      Serial.println(F("SSID: ESP_togglr"));
      Serial.println(F("PW: 12345678"));

      while (true) {
        server.handleClient();
        yield();
      }
    }
  }
  Serial.println("");
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  Serial.println(F("WiFi connected"));
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Toggl Server"));

  // create authentication string for toggl account from API key
  authentication = config.APIkey + ":api_token";
  size_t encodedlength;
  authentication = String(base64_encode((const unsigned char*)authentication.c_str(), authentication.length(), &encodedlength));
  authentication.remove(encodedlength);  // trim the returned array to actual length

  WiFiClientSecure client;
  if (toggl_connect(&client)) {
    Serial.println(F("Connected"));
    Serial.print(F("."));
  }

  if (readWorkspaces(&client)) {
    Serial.println(F("Server Response OK"));
  }

  if (readProjects(&client)) {
    Serial.println(F("Server Response OK"));
  }

  // check if we got at least one workspace and one project:
  if (projectID[0].length() > 1 && workspaceID[0].length() > 1) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F(" FAIL!"));
    Serial.println(F("No Projects"));
    delay(5000);
    ESP.restart();  // reboot
  }

  Serial.println("\n\n");

  for (int i = 0; i < 12; i++) {
    Serial.print(i);
    Serial.print(":\t");
    Serial.print(projectID[i]);
    Serial.print("\t");
    Serial.println(projectName[i]);
  }
  if (readCurrentTimerID(&client))  // if timer is currently running
  {
    Serial.println(F("Timer is running!"));
    isrunning = true;
  }
}
void breath() {
  float val = ((exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0) + 1;
  pixel.setPixelColor(0, pixel.Color(val, 0, 0));
  pixel.show();
}

void loop() {
  //if (breathing) breath();
  
  if (isrunning) {
    breathing = true;
    pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    pixel.show();
  } else {
    breathing = false;
    pixel.setPixelColor(0, pixel.Color(0, 0, 0));
    //pixel.setPixelColor(0, pixel.Color(0, 0, 255));
    pixel.show();
  }
  bool switchproject = false;  // switch project
  /*
    if (WiFi.status() == WL_CONNECTED) {

    if (projectID[switchposition] == "") //empty project position
    {
      startpressed = false; //ignore start, cannot start an empty project
    }

    if (startpressed) {
      if (!isrunning) {
        WiFiClientSecure client;

        Serial.println(F("START!"));
        toggl_connect(&client);

        if (client.connected())  // todo: need to make this more safe, maybe try to connect a few times... or record time locally and update remote later?
        {
          projectstarttime = millis(); //start the local timer
          // send the POST to start the time
          String url = "/api/v8/time_entries/start";
          String messagebody = "{\"time_entry\":{\"description\":\"ButtonBox OK\",\"pid\":" + projectID[switchposition] + ",\"created_with\": \"ButtonBox\"}}\r\n";

          String header = "POST " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Authorization: Basic " + authentication + "\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " +
                          String(messagebody.length()) + "\r\n\r\n";

          client.print(header);
          client.print(messagebody);

          // Serial.println(header);
          // Serial.println(messagebody);

          // print the response
          uint8_t timeout = 0;
          bool responseok = false;
          while (client.available() == 0) {
            delay(10);
            if (timeout++ > 200)
              break;  // wait 2s max
          }

          // Read all the lines of the reply and find the "id" entry
          while (client.available()) {
            String line = client.readStringUntil('\r');
            //  Serial.print(line);
            if (line.indexOf("200 OK", 0) != -1) {
              responseok = true;
            }

            int idx_start = line.indexOf("\"id\"") + 5;
            int idx_end = line.indexOf(",", idx_start);

            if (idx_start > 5 && idx_end > 5) {
              runningTimerID = line.substring(idx_start, idx_end);
              Serial.print(F("Timer ID = "));
              Serial.println(runningTimerID);
              startpressed = false;
              isrunning = true;

            }
          }

          if (responseok)
            Serial.println(F("Server Response OK"));
          // client.flush();
        }
        //  client.stop(); //disconnect
      }
      // if already running and switch changed before start button is pressed
      else if (switch_changed) {
        switchproject = true;
        stoppressed = true;  // stop current project timer before starting new timer in next loop
      }
      else
      {
        startpressed = false; //if start pressed, running and project did not change, discard button action
      }
    }

    if (stoppressed) {
      if (isrunning)
      {
        if (!switchproject) {    // not switching projects
          startpressed = false;  // clear any pending start button detections
        }
        WiFiClientSecure client;
        //display.setFont(&FreeSans18pt7b);
        //display.clearDisplay();
        //display.setCursor(11, 45);
        //display.println(F("STOP!"));
        //display.display();

        toggl_connect(&client);

        if (client.connected())  // todo: need to make this more safe, maybe try to connect a few times... or record time locally and update remote later?
        {
          // send PUT to stop the time

          String url = "/api/v8/time_entries/" + runningTimerID + "/stop";

          String header = "PUT " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Authorization: Basic " + authentication + "\r\n" + "Content-Type: application/json\r\n\r\n";

          client.print(header);
          // Serial.println(header);

          // print the response

          uint8_t timeout = 0;
          while (client.available() == 0) {
            delay(10);
            if (timeout++ > 200)
              break;  // wait 2s max
          }

          while (client.available()) {
            String line = client.readStringUntil('\r');
            //  Serial.print(line);
            if (line.indexOf("200 OK", 0) != -1) {
              isrunning = false;
              stoppressed = false;
              Serial.println(F("Timer Stopped"));
            }
          }
        }
        // client.stop(); //disconnect
      }
      else {
        stoppressed = false; //if not running, discard stop pressed
      }
    }
    updateState();
    } else {
    Serial.println(F("WIFI DISCONNECTED"));
    }
  */
  if (WiFi.status() == WL_CONNECTED) {
    if ((millis() - oldTime) > 5000) {
      WiFiClientSecure client;
      toggl_connect(&client);
      if (client.connected()) {
        if (readCurrentTimerID(&client))  // if timer is currently running
        {
          Serial.println(F("Timer is running!"));
          isrunning = true;
        } else {
          Serial.println(F("Timer is NOT running!"));
          isrunning = false;
        }
      }
      oldTime = millis();
    }
  }

  delay(50);
}
