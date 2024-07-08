#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

#include "timer.h"
#include "relay.h"
#include "secrets.h"
#include "blynkHeader.h"

#define PZEM_RX_PIN 12
#define PZEM_TX_PIN 13

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Relay powerSwitch(14);
SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);
WiFiClient client;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

Timer displayTimer;
Timer wifiTimer;
Timer blynkUpdateTimer;

Timer thingSpeakUpdateTimer;

const char *displayLabels[6] = {"Voltage", "Current", "Power", "Energy", "Frequency", "PF"};
const char *displayUnits[6] = {"V", "A", "W", "KWh", "Hz", ""};

bool blynkState;

inline float *getCurrentValues(PZEM004Tv30 *pzem){
    return (float *)&pzem->_currentValues;
}

void BlynkOnConnected()
{
    Blynk.virtualWrite(V0, powerSwitch.getStatus() ? 1 : 0);
}

BLYNK_WRITE(V0)
{
    int pinValue = param.asInt();
    
    switch(pinValue)
    {
    case 0:
        powerSwitch.disable();
        break;
    case 1:
        powerSwitch.enable();
        break;
    case 2: // Reset energy
        pzem.resetEnergy();
        Serial.println("Resetting!");
        break;
    case 3:
        Blynk.virtualWrite(V0, powerSwitch.getStatus() ? 1 : 0);
        break;

    default: 
        break; //ignore
    }
}

void updateBlynk()
{
    // if (!pzem.updateValues()) No real need to do this here, values are already updated inside refreshDisplay and it messes up the error handling anyways
    // {
    //     Serial.println("[updateBlynk] Error reading values!");
    //     return;
    // }

    if(!Blynk.connected())
    {
        Serial.println("Blynk disconnected... skipping");
        return;
    }

    Blynk.virtualWrite(V1, (double)pzem.voltage()); // Voltage
    Blynk.virtualWrite(V2, (double)pzem.current()); // Current
    Blynk.virtualWrite(V3, (double)pzem.power()); // Power

    char writeBuff[0x80] = {0};
    snprintf(writeBuff, sizeof(writeBuff), "%.3f KWh", pzem.energy());

    Blynk.virtualWrite(V4, writeBuff);
}

void refreshDisplay()
{
    display.clearDisplay();
    display.setCursor(0,0);

#ifdef _DEBUG
    for (int i = 0; i < 6; i++)
        Serial.printf("%s: %.3f\n", displayLabels[i], getCurrentValues(&pzem)[i]);
#endif

    switch(WiFi.status())
    {
    default:
    case WL_DISCONNECTED:
        display.println("WiFi: Connecting");
        break;

    case WL_CONNECTED:
        display.printf("WiFi: %s\n", WiFi.SSID().c_str());
        break;

    case WL_CONNECT_FAILED:
        display.println("Wifi: Connection failed");
        break;

    case WL_NO_SSID_AVAIL:
        display.println("Wifi: Not found");
        break;

    case WL_WRONG_PASSWORD:
        display.println("Wifi: Incorrect password");
    }

    if (!pzem.updateValues())
    {
        
        Serial.println("[refreshDisplay] Error reading values!");

        display.println("\nFailed to read meter\n\nEnsure the device is powered on");
        display.display();

        return;
    }

    for(int i = 0; i < 6; i++)
    {
        char nameBuffer[] = {"         "};
        memcpy(nameBuffer, displayLabels[i], strnlen(displayLabels[i], 12));

        display.printf("%s %.3f %s\n", nameBuffer, getCurrentValues(&pzem)[i], displayUnits[i]);
    }

    display.display();
}

void thingSpeakUpdate()
{
    if (!pzem.updateValues())
    {
        Serial.println("[thingSpeakUpdate] Error reading values!");
        return;
    }

    for (int i = 0; i < 6; i++)
        ThingSpeak.setField(i + 1, i == 2 ? getCurrentValues(&pzem)[i] / 1000 : getCurrentValues(&pzem)[i]);

    int httpCode = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    if (httpCode != 200) // HTTP OK
    {
        Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
        return;
    }
}

void handleWifi()
{
    if(WiFi.status() == WL_IDLE_STATUS)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);

        WiFi.begin(WIFI_SSID, PASS);
        
    }
    
    if(WiFi.status() == WL_CONNECTED && !Blynk.connected() && GetBlynkState() != BlynkWifi::CONNECTING)
    {
        Serial.println("\nConnected.");
        SetBlynkState(BlynkWifi::CONNECTING);
        Blynk.config(BLYNK_AUTH_TOKEN);
    }
}

void setup()
{
    Serial.begin(115200);

    powerSwitch.enable();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("SSD1306 allocation failed");
    }

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0 ,0);

    displayTimer.setup(1000, refreshDisplay);
    thingSpeakUpdateTimer.setup(15000, thingSpeakUpdate);
    wifiTimer.setup(500, handleWifi);
    blynkUpdateTimer.setup(1000, updateBlynk);

    WiFi.mode(WIFI_STA);

    ThingSpeak.begin(client);

    SetBlynkState(BlynkWifi::DISCONNECTED);
}

void loop()
{
    wifiTimer.loop();
    displayTimer.loop();
    blynkUpdateTimer.loop();

    if(WiFi.status() == WL_CONNECTED)
    {
        thingSpeakUpdateTimer.loop();
        Blynk.run();
    }
}
