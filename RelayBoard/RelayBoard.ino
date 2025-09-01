#include "WS_Dout.h"
#include <esp_now.h>
#include <WiFi.h>


bool switches[9]={0,0,0,0,0,0,0,0,0};

typedef struct struct_message {
  byte a;
  byte b;
} struct_message;

struct_message myData;


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  
  switches[myData.b]=!switches[myData.b];
  Set_EXIO(myData.b, switches[myData.b]);
}

void setup()
{
   //GPIO_Init();  // RGB . Buzzer GPIO
     // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

 I2C_Init();
 TCA9554PWR_Init(0x00, 0xFF);
 for(int i=1;i<9;i++)
 Set_EXIO(i, false);

}

void loop()
{


}