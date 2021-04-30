#include <Arduino.h>
#include <Ethernet3.h>
#include "DataStore.h"
#include "PowerView.h"
#include "EthernetManager.h"
#include "WisunManager.h"
#include "BP35C0_J1.h"
#include "HeadView.h"
#include "FunctionButton.h"
#include <Seeed_Arduino_FreeRTOS.h>

#define NTP_SERVER "time.nist.gov"
const long gmtOffset_sec = 9 * 3600; //9時間の時差を入れる

static TFT_eSPI tft;
HeadView headView(&tft);
FunctionButton btnA(&BtnA, &tft, POS_A_X);
FunctionButton btnB(&BtnB, &tft, POS_B_X);
FunctionButton btnC(&BtnC, &tft, POS_C_X);
ViewController viewController(&btnC, &tft);
SmartMeter sm(0x02, 0x88, 0x01);
PowerView pv(&sm, &tft);
DataStore dataStore(&sm);

EthernetUDP ntpUdp;
NTPClient ntc(ntpUdp, NTP_SERVER, gmtOffset_sec, 10 * 60 * 1000);

EthernetUDP multiUdp;
EthernetUDP uniUdp;
EthernetManager emi(&multiUdp, &uniUdp);

BP35C0_J1 wmi;

void loop();
static void loopTask(void *)
{
    for (;;)
    {
        loop();
        vTaskDelay(0);
    }
}

void setup()
{
    Serial.begin(115200);

    // debug_init();

    tft.begin();
    tft.fillScreen(TFT_BLACK);
    headView.init();

    Ethernet.setCsPin(BCM8);
    Ethernet.setRstPin(BCM24);
    int ret = Ethernet.begin();
    while (ret != 1)
    {
        tft.setCursor(0, 0);
        tft.print("connect Ethernet !");
        delay(1000);
        ret = Ethernet.begin();
    }
    headView.setNwType("Ethernet");

    // EthernetManager
    emi.begin();
    emi.setDataStore(&dataStore);

    // WisunManager
    wmi.begin();
    wmi.setDataStore(&dataStore);

    // DataStore
    dataStore.setEcho(emi.getEcho());

    // NTPClient
    ntc.begin();
    while (!ntc.update())
        Serial.println("NTP timeout.");

    // views
    IPAddress addr = Ethernet.localIP();
    headView.setIpAddress(addr);
    headView.setNtp(&ntc);
    viewController.setView((const char *)"POWER", &pv);

    // buttons
    btnA.enable((const char *)"NTP");
    btnB.enable((const char *)"CONNECT");
    btnC.enable((const char *)"DUMP");

    xTaskCreate(loopTask, (const char *)"loop", 2048, NULL, 0, NULL);
    vTaskStartScheduler();
}

boolean btnB_action = false;
boolean btnB_state = false;
unsigned long preEpoch = 0;
void loop()
{
    emi.update();
    wmi.update();

    uint32_t epoch = ntc.getEpochTime();
    if (preEpoch != epoch)
    {
        preEpoch = epoch;
        byte *heap = new byte[4];
        debug_printfln(true, "heap:%p", heap);
        delete[] heap;

        headView.update();
        viewController.update();
    }
    if (btnA.isEnable() && btnA.getButton()->wasPressed())
    {
        btnA.disable("NTP");
        ntc.update();
        btnA.enable("NTP");
    }
    if (btnB.isEnable() && btnB.getButton()->wasPressed())
    {
        btnB.disable(btnB.getLabel());
        // delay(100);
        if (wmi.getState() == CONNECT) // 接続状態
            wmi.disconnect();
        else if (wmi.getState() == DIS_CONNECT) // 切断状態
            wmi.connect();
        btnB_action = true;
    }
    if (btnC.isEnable() && btnC.getButton()->wasPressed())
    {
        btnC.disable("DUMP");
        // delay(100);
        emi.getEcho()->profile.printAll();
        emi.getEcho()->details.printAll();
        btnC.enable("DUMP");
    }
    if (btnB_action)
    {
        if (wmi.getState() == CONNECT)
        { // 接続状態
            btnB.enable("DISCONNECT");
            btnB_action = false;
        }
        else if (wmi.getState() == DIS_CONNECT)
        { // 切断状態
            btnB.enable("CONNECT");
            btnB_action = false;
        }
    }

    delay(1);
    FunctionButton::update();
}