#include <Wire.h>
#include <Arduino.h>
#include <EEPROM.h>
#include "pin_config.h"
#include <lvgl.h>
#include "SensorPCF85063.hpp"
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "XPowersLib.h"
#include "lv_conf.h"
#include "ui.h"
#include <demos/lv_demos.h>
#include <esp_now.h>
#include <WiFi.h>


esp_now_peer_info_t peerInfo;
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
typedef struct struct_message {
  byte a;
  byte b;
} struct_message;

struct_message myData;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
}


#include "HWCDC.h"
HWCDC USBSerial;
SensorPCF85063 rtc;

XPowersPMU power;
bool adc_switch = false;

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_display_t *disp;
lv_color_t *disp_draw_buf;

unsigned long timebuff = 0;
int offTime = 4;
uint32_t lastTouched = 0;
uint32_t lastMillis;
bool timeFormat = 1;  // 0=24 1=12 (am,pm)
int bright = 50;
bool firstRun = 0;

int year = 2025;
int month = 7;
int day = 21;
int hour = 11;
int minute = 9;
int second = 0;

String dateS = "";
String yearS = "";
String batS = "";
String monthsS[] = {
  "", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
  "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

#define DIRECT_RENDER_MODE

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
  LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);

Arduino_GFX *gfx = new Arduino_CO5300(bus, LCD_RESET /* RST */,
                                      0 /* rotation */, LCD_WIDTH, LCD_HEIGHT,
                                      22 /* col_offset1 */,
                                      0 /* row_offset1 */,
                                      0 /* col_offset2 */,
                                      0 /* row_offset2 */);


std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> FT3168(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS,
                                                       DRIVEBUS_DEFAULT_VALUE, TP_INT, Arduino_IIC_Touch_Interrupt));

void Arduino_IIC_Touch_Interrupt(void) {
  FT3168->IIC_Interrupt_Flag = true;
}


#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  USBSerial.println(buf);
  USBSerial.flush();
}
#endif

uint32_t millis_cb(void) {
  return millis();
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
#ifndef DIRECT_RENDER_MODE
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
#endif  // #ifndef DIRECT_RENDER_MODE

  /*Call it to tell LVGL you are ready*/
  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  int32_t touchX = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
  int32_t touchY = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

  if (FT3168->IIC_Interrupt_Flag == true) {
    FT3168->IIC_Interrupt_Flag = false;
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;

    lastTouched = millis();

    USBSerial.print("Data x ");
    USBSerial.print(touchX);

    USBSerial.print("Data y ");
    USBSerial.println(touchY);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void rounder_event_cb(lv_event_t *e) {
  lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
  uint16_t x1 = area->x1;
  uint16_t x2 = area->x2;

  uint16_t y1 = area->y1;
  uint16_t y2 = area->y2;

  // round the start of coordinate down to the nearest 2M number
  area->x1 = (x1 >> 1) << 1;
  area->y1 = (y1 >> 1) << 1;
  // round the end of coordinate up to the nearest 2N+1 number
  area->x2 = ((x2 >> 1) << 1) + 1;
  area->y2 = ((y2 >> 1) << 1) + 1;
}

void adcOn() {
  power.enableTemperatureMeasure();
  // Enable internal ADC detection
  power.enableBattDetection();
  power.enableVbusVoltageMeasure();
  power.enableBattVoltageMeasure();
  power.enableSystemVoltageMeasure();
}

void adcOff() {
  power.disableTemperatureMeasure();
  // Enable internal ADC detection
  power.disableBattDetection();
  power.disableVbusVoltageMeasure();
  power.disableBattVoltageMeasure();
  power.disableSystemVoltageMeasure();
}


void saveData(lv_event_t *e) {
  EEPROM.write(1, bright);
  EEPROM.write(2, offTime);
  EEPROM.commit();
  lv_screen_load(ui_Screen1);
}

void choseFormat(lv_event_t *e) {
  if (lv_obj_has_state(ui_formatSWC, LV_STATE_CHECKED))
    timeFormat = 1;
  else
    timeFormat = 0;

  EEPROM.write(0, timeFormat);
  EEPROM.commit();
}

void setupSet(lv_event_t *e) {
  RTC_DateTime datetime2 = rtc.getDateTime();
  minute = datetime2.getMinute();
  hour = datetime2.getHour();
}

void setupDate(lv_event_t *e) {
  RTC_DateTime datetime2 = rtc.getDateTime();
  day = datetime2.getDay();
  month = datetime2.getMonth();
  year = datetime2.getYear();
}

void sendSwitch(lv_event_t *e) {

  lv_obj_t *btn = lv_event_get_target_obj(e);
  if (btn == NULL) return;

  if (btn == ui_SW1 || btn == ui_mainSWbtn1) myData.b = 1;
  if (btn == ui_SW2 || btn == ui_mainSWbtn2) myData.b = 2;
  if (btn == ui_SW3 || btn == ui_mainSWbtn3) myData.b = 3;
  if (btn == ui_SW4) myData.b = 4;
  if (btn == ui_SW5) myData.b = 5;
  if (btn == ui_SW6) myData.b = 6;
  if (btn == ui_SW7) myData.b = 7;
  if (btn == ui_SW8) myData.b = 8;




  myData.a = 4;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
}

void setDate(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target_obj(e);  // gumb koji je kliknut
  if (btn == NULL) return;

  if (btn == ui_setDateBTN1) {
    year++;
    if (year > 2100) year = 25;
  }

  if (btn == ui_setDateBTN2) {
    year--;
    if (year < 2000) year = 2025;
  }

  if (btn == ui_setDateBTN3) {
    month++;
    if (month > 12) month = 1;
  }
  if (btn == ui_setDateBTN4) {
    month--;
    if (month < 1) month = 12;
  }

  if (btn == ui_setDateBTN5) {
    day++;
    if (day > 31) day = 1;
  }
  if (btn == ui_setDateBTN6) {
    day--;
    if (day < 1) day = 31;
  }

  if (btn == ui_setDateBTN7) {
    RTC_DateTime datetime2 = rtc.getDateTime();
    minute = datetime2.getMinute();
    hour = datetime2.getHour();
    second = datetime2.getSecond();
    rtc.setDateTime(year, month, day, hour, minute, second);
    lv_screen_load(ui_Screen1);
  }
}

void changeOff(lv_event_t *e) {
  offTime = lv_slider_get_value(ui_Slider1);
}



void changeBRIGHT(lv_event_t *e) {
  bright = lv_arc_get_value(ui_Arc1);
  ((Arduino_CO5300 *)gfx)->setBrightness(bright);
}
void setTime(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target_obj(e);  // gumb koji je kliknut
  if (btn == NULL) return;

  if (btn == ui_setTimeBTN1) {
    hour++;
    if (hour > 23) hour = 0;
  }

  if (btn == ui_setTimeBTN2) {
    hour--;
    if (hour < 0) hour = 23;
  }

  if (btn == ui_setTimeBTN3) {
    minute++;
    if (minute > 59) minute = 0;
  }

  if (btn == ui_setTimeBTN4) {
    minute--;
    if (minute < 0) minute = 59;
  }

  if (btn == ui_setTimeBTN5) {
    RTC_DateTime datetime2 = rtc.getDateTime();
    day = datetime2.getDay();
    month = datetime2.getMonth();
    year = datetime2.getYear();
    rtc.setDateTime(year, month, day, hour, minute, second);
    lv_screen_load(ui_Screen1);
  }
}



void setup() {

  pinMode(0, INPUT_PULLUP);
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  USBSerial.begin(115200);
  // USBSerial.setDebugOutput(true);
  // while(!USBSerial);
  USBSerial.println("Arduino_GFX LVGL_Arduino_v9 example ");
  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  USBSerial.println(LVGL_Arduino);

  EEPROM.begin(5);

  timeFormat = EEPROM.read(0);
  if (timeFormat > 1) timeFormat = 0;

  bright = EEPROM.read(1);
  if (bright < 50) bright = 100;

  offTime = EEPROM.read(2);
  if (offTime > 15) offTime = 10;

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }




  // Init Display
  if (!gfx->begin()) {
    USBSerial.println("gfx->begin() failed!");
  }



  Wire.begin(IIC_SDA, IIC_SCL);

  while (FT3168->begin() == false) {
    USBSerial.println("FT3168 initialization fail");
    delay(2000);
  }
  USBSerial.println("FT3168 initialization successfully");

  FT3168->IIC_Write_Device_State(FT3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
                                 FT3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);


  if (!rtc.begin(Wire, IIC_SDA, IIC_SCL)) {
    USBSerial.println("Failed to find PCF8563 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  ///rtc.setDateTime(year, month, day, hour, minute, second);

  power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  power.setChargeTargetVoltage(3);
  power.clearIrqStatus();
  power.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);

  adcOn();

  lv_init();

  /*Set a tick source so that LVGL will know how much time elapsed. */
  lv_tick_set_cb(millis_cb);

  /* register print function for debugging */
#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_RENDER_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 50;
#endif

#ifdef ESP32
#if defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
  disp_draw_buf = (lv_color_t *)gfx->getFramebuffer();
#else   // !(defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
#endif  // !(defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))
#else   // !ESP32
  USBSerial.println("LVGL disp_draw_buf heap_caps_malloc failed! malloc again...");
  disp_draw_buf = (lv_color_t *)malloc(bufSize * 2);
#endif  // !ESP32
  if (!disp_draw_buf) {
    USBSerial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
#ifdef DIRECT_RENDER_MODE
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);



    ui_init();
  }



  if (offTime == 0)
    lv_obj_add_state(ui_formatSWC, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(ui_formatSWC, LV_STATE_CHECKED);

  ((Arduino_CO5300 *)gfx)->setBrightness(bright);
}
// setup done

void loop() {
  lv_task_handler(); /* let the GUI do its work */

  if (firstRun == 0) {
    firstRun = true;
    lastTouched = millis();
  }

  if (millis() > lastTouched + (offTime * 1000))
    power.shutdown();


  if (digitalRead(0) == 0)
    lv_scr_load(ui_menuSCR);

#ifdef DIRECT_RENDER_MODE
#if defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)
  gfx->flush();
#else   // !(defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif  // !(defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
#else   // !DIRECT_RENDER_MODE
#ifdef CANVAS
  gfx->flush();
#endif
#endif  // !DIRECT_RENDER_MODE

  delay(5);

  if (lv_screen_active() == ui_Screen1) {

    timebuff = (lastTouched + (offTime * 1000)) - millis();
    lv_bar_set_value(ui_offBAR, map(timebuff, 0, offTime * 1000, 0, 100), LV_ANIM_OFF);

    if (millis() - lastMillis > 1000) {
      lastMillis = millis();
      RTC_DateTime datetime = rtc.getDateTime();

      int hour2 = datetime.getHour();

      if (timeFormat) {
        if (hour2 > 12) {
          lv_label_set_text(ui_mainFormatLBL, " PM ");
          hour2 = hour2 - 12;
        } else if (hour2 == 0) {
          lv_label_set_text(ui_mainFormatLBL, " AM ");
          hour2 = 12;
        } else if (hour2 == 12) {
          lv_label_set_text(ui_mainFormatLBL, " PM ");
          hour2 = 12;
        } else {
          lv_label_set_text(ui_mainFormatLBL, " AM ");
        }
      } else {
        lv_label_set_text(ui_mainFormatLBL, "");
      }

      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", hour2, datetime.getMinute());

      char buf2[3];
      snprintf(buf2, sizeof(buf2), "%02d", datetime.getSecond());



      // Update label with current time
      lv_label_set_text(ui_timeLBL, buf);
      lv_label_set_text(ui_secLBL, buf2);

      lv_bar_set_value(ui_batBAR, power.getBatteryPercent(), LV_ANIM_OFF);
      lv_label_set_text(ui_brightLBL2, String(bright).c_str());
      lv_label_set_text(ui_offTimeLBL1, String(offTime).c_str());

      dateS = monthsS[datetime.getMonth()] + ", " + String(datetime.getDay());
      lv_label_set_text(ui_date_LBL, dateS.c_str());

      batS = String(power.getBatteryPercent()) + "%";
      lv_label_set_text(ui_batLBL, batS.c_str());

      yearS = " " + String(datetime.getYear()) + " ";
      lv_label_set_text(ui_yearLBL1, yearS.c_str());
    }
  }

  // lv_screen_load(scr1);
  if (lv_screen_active() == ui_setSCR) {

    if (timeFormat) {
      if (hour > 12) {
        lv_label_set_text(ui_mainFormatLBL2, "PM");
        lv_label_set_text(ui_deHrLBL, String(hour - 12).c_str());
      } else if (hour == 0) {
        lv_label_set_text(ui_mainFormatLBL2, "AM");
        lv_label_set_text(ui_deHrLBL, "12");
      } else if (hour == 12) {
        lv_label_set_text(ui_mainFormatLBL2, "PM");
        lv_label_set_text(ui_deHrLBL, "12");
      } else {
        lv_label_set_text(ui_deHrLBL, String(hour).c_str());
        lv_label_set_text(ui_mainFormatLBL2, "AM");
      }
    } else {
      lv_label_set_text(ui_deHrLBL, String(hour).c_str());
      lv_label_set_text(ui_mainFormatLBL2, "");
    }

    lv_label_set_text(ui_deMinLBL, String(minute).c_str());
  }

  if (lv_screen_active() == ui_setDateSCR) {
    lv_label_set_text(ui_deYearLBL, String(year - 2000).c_str());
    lv_label_set_text(ui_deMonthLBL, String(month).c_str());
    lv_label_set_text(ui_deDayLBL, String(day).c_str());
  }
}
