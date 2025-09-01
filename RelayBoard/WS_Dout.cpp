#include "WS_Dout.h"

bool Failure_Flag = 0;
/*************************************************************  Dout I/O  *************************************************************/
bool Dout_Open(uint8_t CHx)
{
  if(!Set_EXIO(CHx, true)){
    printf("Failed to Open CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_Closs(uint8_t CHx)
{
  if(!Set_EXIO(CHx, false)){
    printf("Failed to LOW CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_CHx_Toggle(uint8_t CHx)
{
   if(!Set_Toggle(CHx)){
    printf("Failed to Toggle CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_CHx(uint8_t CHx, bool State)
{
  bool result = 0;
  if(State)
    result = Dout_Open(CHx);
  else
    result = Dout_Closs(CHx);
  if(!result)
    Failure_Flag = 1;
  return result;
}
bool Dout_CHxs_PinState(uint8_t PinState)
{
  if(!Set_EXIOS(PinState)){
    printf("Failed to set the dout status!!!\r\n");
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}

void DoutFailTask(void *parameter) {
  while(1){
    if(Failure_Flag)
    {
      Failure_Flag = 0;
      printf("Error: Dout control failed!!!\r\n");
      RGB_Open_Time(60,0,0,5000,500);
      Buzzer_Open_Time(5000, 500);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  vTaskDelete(NULL);
}
void Dout_Init(void)
{
  TCA9554PWR_Init(0x00, 0xFF);
  xTaskCreatePinnedToCore(
    DoutFailTask,    
    "DoutFailTask",   
    4096,                
    NULL,                 
    3,                   
    NULL,                 
    0                   
  );
}

/********************************************************  Data Analysis  ********************************************************/
bool Dout_Flag[8] = {1, 1, 1, 1, 1, 1, 1, 1};       // Dout current status flag
void Dout_Analysis(uint8_t *buf,uint8_t Mode_Flag)
{
  uint8_t ret = 0;
  if(Mode_Flag == Bluetooth_Mode_Trigger)
    printf("Bluetooth Data :\r\n");
  else if(Mode_Flag == WIFI_Mode_Trigger)
    printf("WIFI Data :\r\n");
  else if(Mode_Flag == RS485_Mode_Trigger)
    printf("RS485 Data :\r\n");  
  switch(buf[0])
  {
    case CH1: 
      ret = Dout_CHx_Toggle(GPIO_PIN_CH1);                                              //Toggle the level status of the GPIO_PIN_CH1 pin
      if(ret){
        Dout_Flag[0] =! Dout_Flag[0];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[0])
          printf("|***  Dout CH1 High  ***|\r\n");
        else
          printf("|***  Dout CH1 Low ***|\r\n");
      }
      break;
    case CH2: 
      ret = Dout_CHx_Toggle(GPIO_PIN_CH2);                                             //Toggle the level status of the GPIO_PIN_CH2 pin
      if(ret){
        Dout_Flag[1] =! Dout_Flag[1];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[1])
          printf("|***  Dout CH2 High  ***|\r\n");
        else
          printf("|***  Dout CH2 Low ***|\r\n");
      }
      break;
    case CH3:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH3);                                             //Toggle the level status of the GPIO_PIN_CH3 pin
      if(ret){
        Dout_Flag[2] =! Dout_Flag[2];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[2])
          printf("|***  Dout CH3 High  ***|\r\n");
        else
          printf("|***  Dout CH3 Low ***|\r\n");
      }
      break;
    case CH4:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH4);                                             //Toggle the level status of the GPIO_PIN_CH4 pin
      if(ret){
        Dout_Flag[3] =! Dout_Flag[3];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[3])
          printf("|***  Dout CH4 High  ***|\r\n");
        else
          printf("|***  Dout CH4 Low ***|\r\n");
      }
      break;
    case CH5:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH5);                                             //Toggle the level status of the GPIO_PIN_CH5 pin
      if(ret){  
        Dout_Flag[4] =! Dout_Flag[4];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[4])
          printf("|***  Dout CH5 High  ***|\r\n");
        else
          printf("|***  Dout CH5 Low ***|\r\n");
      }
      break;
    case CH6:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH6);                                             //Toggle the level status of the GPIO_PIN_CH6 pin
      if(ret){
        Dout_Flag[5] =! Dout_Flag[5];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[5])
          printf("|***  Dout CH6 High  ***|\r\n");
        else
          printf("|***  Dout CH6 Low ***|\r\n");
      }
      break;
    case CH7:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH7);                                             //Toggle the level status of the GPIO_PIN_CH6 pin
      if(ret){
        Dout_Flag[6] =! Dout_Flag[6];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[6])
          printf("|***  Dout CH7 High  ***|\r\n");
        else
          printf("|***  Dout CH7 Low ***|\r\n");
      }
      break;
    case CH8:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH8);                                             //Toggle the level status of the GPIO_PIN_CH6 pin
      if(ret){
        Dout_Flag[7] =! Dout_Flag[7];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[7])
          printf("|***  Dout CH8 High  ***|\r\n");
        else
          printf("|***  Dout CH8 Low ***|\r\n");
      }
      break;
    case ALL_ON:
      ret = Dout_CHxs_PinState(0xFF);                                              // Turn High all dout
      if(ret){
        memset(Dout_Flag,1, sizeof(Dout_Flag));
        printf("|***  Dout ALL High  ***|\r\n");
        Buzzer_Open_Time(500, 0);
      }

      break;
    case ALL_OFF:
      ret = Dout_CHxs_PinState(0x00);                                                // Turn Low all dout
      if(ret){
        memset(Dout_Flag,0, sizeof(Dout_Flag));
        printf("|***  Dout ALL Low ***|\r\n");
        Buzzer_Open_Time(500, 150); 
      }
      break;
    default:
      printf("Note : Non-instruction data was received !\r\n");
  }
}

void Dout_Immediate(uint8_t CHx, bool State, uint8_t Mode_Flag)
{
  if(!CHx || CHx > 8){
    printf("Dout_Immediate(function): Incoming parameter error!!!!\r\n");
    Failure_Flag = 1;
  }
  else{
    uint8_t ret = 0;
    if(Mode_Flag == DIN_Mode_Trigger)
      printf("DIN Data :\r\n"); 
    else if(Mode_Flag == RTC_Mode_Trigger)
      printf("RTC Data :\r\n");
    ret = Dout_CHx(CHx,State);                                               
    if(ret){
      Dout_Flag[CHx-1] = State;
      if(Dout_Flag[0])
        printf("|***  Dout CH%d High  ***|\r\n",CHx);
      else
        printf("|***  Dout CH%d Low ***|\r\n",CHx);
    }
  }
}
void Dout_Immediate_CHxn(Status_adjustment * Dout_n, uint8_t Mode_Flag)
{
  uint8_t ret = 0;
  if(Mode_Flag == DIN_Mode_Trigger)
    printf("DIN Data :\r\n"); 
  else if(Mode_Flag == RTC_Mode_Trigger)
    printf("RTC Data :\r\n");            
  for (int i = 0; i < 8; i++) {
    if(Dout_n[i] == STATE_Open || Dout_n[i] == STATE_Closs){
      Dout_Flag[i] = (bool)Dout_n[i];
      ret = Dout_CHx(i+1,Dout_n[i]);  
      if(Dout_n[i] == STATE_Open)
        printf("|***  Dout CH%d High  ***|\r\n",i+1);
      else if(Dout_n[i] == STATE_Closs)
        printf("|***  Dout CH%d Low ***|\r\n",i+1);
    }
  }
}

void Dout_Immediate_CHxs(uint8_t PinState, uint8_t Mode_Flag)
{
  uint8_t ret = 0;
  if(Mode_Flag == DIN_Mode_Trigger)
    printf("DIN Data :\r\n"); 
  else if(Mode_Flag == RTC_Mode_Trigger)
    printf("RTC Data :\r\n");
  for (int i = 0; i < 8; i++) {
    Dout_Flag[i] = (PinState >> i) & 0x01; // 提取每一位并赋值
  }
  ret = Dout_CHxs_PinState(PinState);                                               
  if(ret){
    for (int j = 0; j < 8; j++) {
      if(Dout_Flag[j])
        printf("|***  Dout CH%d High  ***|\r\n",j+1);
      else
        printf("|***  Dout CH%d Low ***|\r\n",j+1);
    }
    Buzzer_Open_Time(200, 0);
  }
  else
  {
    printf("Dout_Immediate_CHxs(function): Dout control failure!!!!\r\n");
    Failure_Flag = 1;
  }
}