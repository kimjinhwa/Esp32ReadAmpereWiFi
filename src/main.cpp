#include <Arduino.h>
#include "WiFi.h"
#include "myESPTelnet.h"
#include "esp_adc_cal.h"
#include "DateTime.h"

#define WIFI_TIMEOUT_MS 20000
TaskHandle_t h_WiFiTask;

myESPTelnet telnet;
TaskHandle_t h_TelnetServerTask;

volatile bool interruptCounter = false;
hw_timer_t *timer = NULL;

// #define WIFI_PASSWORD "iftech0273"
//#define WIFI_NETWORK "IFTECH"
// IPAddress ipaddress(192, 168, 1, 230);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnetmask(255, 255, 255, 0);

#define WIFI_NETWORK "JHKIMS"
#define WIFI_PASSWORD "87654321"
IPAddress ipaddress(192, 168, 45, 230);
IPAddress gateway(192, 168, 45, 218);
IPAddress subnetmask(255, 255, 255, 0);

// #define WIFI_NETWORK "myTablet"
// #define WIFI_PASSWORD "iftech0273"
// #define WIFI_TIMEOUT_MS 20000
// IPAddress ipaddress(192, 168, 137, 230);
// IPAddress gateway(192, 168, 137, 1);
// IPAddress subnetmask(255, 255, 255, 0);
void setupDateTime();
// const IPAddress ipaddress(192, 168, 1, 230);
// const IPAddress gateway(192, 168, 1, 1);
// const IPAddress subnetmask(255, 255, 255, 0);
// const IPAddress ipaddress(192, 168, 137, 230);
// const IPAddress gateway(192, 168, 137, 1);
// const IPAddress subnetmask(255, 255, 255, 0);

void keepWiFiAlive(void *parameter)
{
  {
    // WiFi.config(ipaddress, subnetmask, gateway);
    WiFi.config(ipaddress, gateway, subnetmask, IPAddress(8, 8, 8, 8), IPAddress(164, 124, 101, 2));
  }
  printf("Connection to Wifi");
  // WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
               { 
                ipaddress = WiFi.localIP(); 
                gateway = WiFi.gatewayIP();
                subnetmask = WiFi.subnetMask();
 
                printf("\nIp address :%s\n", ipaddress.toString().c_str());
                printf("%s\n",WiFi.localIP().toString().c_str()); 
                printf("\nsubnetmask :%s\n", subnetmask.toString().c_str());

                printf("\ngateway :%s\n", gateway.toString().c_str());
                printf( "LOG LOCAL VALUE is %d\n", LOG_LOCAL_LEVEL);
                printf( "Wifi connected to %s\n",WiFi.localIP().toString().c_str() ); },
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
               { ESP_LOGW(TAG, "Wifi Disconnected from  %s\n", WiFi.localIP().toString().c_str()); },
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
  {
    printf(".");
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
    {
      printf(".");
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      printf("Failed!");
    }
    else
    {
      printf(WiFi.localIP().toString().c_str());
    }
  }
}
float readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  uint32_t val = esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars);
  return (val * 0.001) / (4.0 / 150.0);
  // float val_1 = 3.3 * ADC_Raw / 4095.0;
  // return (val_1) / (4.0 / 150.0);
}
void onTelnetConnect(String ip)
{
  // Read ADC  port 36
  printf("- Telnet : ");
  printf(ip.c_str());
  printf(" Connected");
  printf("\nWelcome %s ", telnet.getIP().c_str());
  printf("\n(Use ^] + q to disconnect");
  printf("\n/>");
}
void onTelnetDisconnect(String ip)
{
  printf("- Telnet : ");
  printf(ip.c_str());
  printf(" Disconnected");
}
void onTelnetReconnect(String ip)
{
  printf("- Telnet : ");
  printf(ip.c_str());
  printf("\n Reconnected");
}
void onTelnetConnectionAttemp(String ip)
{
  printf("- Telnet : ");
  printf(ip.c_str());
  printf("\n Reconnected");
}

void setupTelnet()
{
  telnet.onConnect(onTelnetConnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onReconnect(onTelnetConnectionAttemp);

  telnet.onInputReceived([](String str)
                         {
    if (str.length() == 1)
    {
      if (str.c_str() == "N" || str.c_str() == "n" || str.c_str() == "y" || str.c_str() == "Y")
      {
        telnet.receivedLength = str.length();
        telnet.getchar = *(str.c_str() + 0);
      }
    } });

  // telnet.onInputReceived([](String str) {}); // telnetInput = str; //ESP_LOGW(TAG,"%s",telnetInput.c_str());
  if (telnet.begin(23))
  {
    Serial.println("Running");
  }
  else
  {
    Serial.println("\nerrro....system reboot from telnet");
    vTaskDelay(2000);
    ESP.restart();
  }
}

// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 0;
// const int   daylightOffset_sec = 3600;
// String getLocalTime()
// {
//     String nowTime = "00:00:00";
//     struct tm timeinfo;
//     if (!getLocalTime(&timeinfo))
//     {
//       printf("\nFailed to obtain time");
//       return "";
//     }
//     nowTime = timeinfo.tm_hour + ":";
//     nowTime = timeinfo.tm_min;
//     nowTime += ":";
//     nowTime += timeinfo.tm_sec;
//     return nowTime;

const int batVoltagePin_0 = 36;
const int batVoltagePin_3 = 39;
void TelnetServerTask(void *parameters)
{
  uint32_t adcValue_Voltage_0 = 0;
  uint32_t adcValue_Voltage_3 = 0;
  ;

  while (WiFi.status() != WL_CONNECTED)
  {
    printf("\nTelnet is waiting to connect network...");
    vTaskDelay(500);
  };
  printf("\nTelnet is initing..");
  setupDateTime();
  // printf("Now Time is %s", DateTime.now());
  setupTelnet();

// configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//  getLocalTime();
//  printf("\nNow Time is %s",getLocalTime().c_str());
#define BUFFER 500
  u16_t buf_1[BUFFER];
  u16_t buf_2[BUFFER];
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
      telnet.loop();
    vTaskDelay(1);
    if (interruptCounter)
    {
      interruptCounter = false;
      for (u16_t i = 0; i < BUFFER; i++)
      {
        buf_1[i] = analogRead(batVoltagePin_0);
        buf_2[i] = analogRead(batVoltagePin_3);
        vTaskDelay(2);
      }
      for (u16_t i = 0; i < BUFFER; i++)
        adcValue_Voltage_0 += buf_1[i];
      for (u16_t i = 0; i < BUFFER; i++)
        adcValue_Voltage_3 += buf_2[i];
      adcValue_Voltage_0 /= BUFFER;
      adcValue_Voltage_3 /= BUFFER;
      // telnet.printf("\r\n%d\t%d", adcValue_Voltage_0, adcValue_Voltage_3);
      // printf("\r\n%d\t%d", adcValue_Voltage_0, adcValue_Voltage_3);
      telnet.printf("\r\n%s\t%3.3f\t%3.3f", DateTime.format(DateFormatter::TIME_ONLY).c_str(), readADC_Cal(adcValue_Voltage_0), readADC_Cal(adcValue_Voltage_3));
      printf("\r\n%s\t%3.3f\t%3.3f", DateTime.format(DateFormatter::TIME_ONLY).c_str(), readADC_Cal(adcValue_Voltage_0), readADC_Cal(adcValue_Voltage_3));
    }
  }
}
void setupDateTime()
{
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  // DateTime.setTimeZone(-4);
  //   DateTime.setServer("asia.pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.setServer("time.pool.aliyun.com");
  // DateTime.setServer("182.92.12.11");
  DateTime.setTimeZone("CST-8");
  DateTime.begin();
  if (!DateTime.isTimeValid())
  {
    Serial.println("Failed to get time from server.");
  }
  else
  {
    Serial.printf("Date Now is %s\n", DateTime.toISOString().c_str());
    Serial.printf("Timestamp is %ld\n", DateTime.now());
  }
}

void IRAM_ATTR onTimer()
{
  interruptCounter = true;
}
void setup()
{
  Serial.begin(9600);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 * 3, true);

  xTaskCreatePinnedToCore(keepWiFiAlive, "WiFi Connection,", 5000, NULL, 1, &h_WiFiTask, CONFIG_ARDUINO_RUNNING_CORE);
  xTaskCreate(TelnetServerTask, "TelnetServerTask", 5000, NULL, 1, &h_TelnetServerTask);
  timerAlarmEnable(timer);
}

void loop()
{
  // put your main code here, to run repeatedly:
}