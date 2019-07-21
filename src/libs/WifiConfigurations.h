#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#include <Ticker.h>
Ticker pairingModeTicker;

int httpCode = 0;
String server ,port, freqMinutes, sensorName = "";
bool shouldSaveConfig = false;
WiFiManager wifiManager;

void tick()
{
    digitalWrite(D2, !digitalRead(D2));
}
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    pairingModeTicker.attach(0.2, tick);
}
void getSavedConfig()
{
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
        //file exists, reading and loading
        Serial.println("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile)
        {
            Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(buf.get());
            json.prettyPrintTo(Serial);
            if (json.success())
            {
                Serial.println("\nparsed json");
                server = json.get<String>("server");
                port = json.get<String>("port");
                freqMinutes = json.get<String>("freqMinutes");
                sensorName = json.get<String>("sensorName");
            }
            else
            {
                Serial.println("failed to load json config");
            }
            configFile.close();
        }
    }
}
void setupWifiManager()
{
    pinMode(D2, OUTPUT);
    SPIFFS.begin();
    pairingModeTicker.attach(0.6, tick);
    WiFiManagerParameter customIp("ip", "ip Hub", server.c_str(), 40);
    WiFiManagerParameter customPort("port", "port", port.c_str(), 6);
    WiFiManagerParameter customFreq("Freq", "Frecventa citire", freqMinutes.c_str(), 2);
    WiFiManagerParameter customSensorName("sensorName", "Nume senzor", sensorName.c_str(), 50);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&customIp);
    wifiManager.addParameter(&customPort);
    wifiManager.addParameter(&customFreq);
    wifiManager.addParameter(&customSensorName);
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect("TempSensor"))
    {
        Serial.println("Failed to connect and hit timeout. Sensor will reset in 3 seconds!");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        wifiManager.resetSettings();
        Serial.println("Sensor succesfuly reset. Module will reboot in 5 seconds.");
        delay(5000);
        ESP.reset();
    }
    server = customIp.getValue();
    port = customPort.getValue();
    freqMinutes = customFreq.getValue();
    sensorName = customSensorName.getValue();

    Serial.println("Dupa Configurare " + sensorName);

    if (shouldSaveConfig)
    {
        Serial.println("Saving configuration file");
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.createObject();
        json["server"] = server;
        json["port"] = port;
        json["freqMinutes"] = freqMinutes;
        json["sensorName"] = sensorName;

        File configFile = SPIFFS.open("/config.json", "w");

        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        //end save
    }
    pairingModeTicker.detach();
    digitalWrite(D2, HIGH);
    Serial.println("I have connection to wifi.");
    getSavedConfig();
}