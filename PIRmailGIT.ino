/*********
Compiled by dehalo@gmx.net
 Based on  
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-pir-motion-detector-photo-capture/
 Created by K. Suwatchai (Mobizt)
 * 
 * Email: k_suwatchai@hotmail.com
 * 
 * Github: https://github.com/mobizt
 * 
  
*********/
 
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "dl_lib.h"
#include "driver/rtc_io.h"
#include "ESP32_MailClient.h"
#include <EEPROM.h>            // read and write from flash memory
// define the number of bytes you want to access
#define EEPROM_SIZE 1
 
RTC_DATA_ATTR int bootCount = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
 
int pictureNumber = 0;
String path;

#define uS_TO_S_FACTOR 1000000
#define WIFI_SSID "XXXXXXXXX"
#define WIFI_PASSWORD "XXXXXXXXXXXXX"
ESP32TimeHelper timeHelper; //modAM
//The Email Sending data object contains config and data to send
SMTPData smtpData;

char *month[] ={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"}; //modAM ex https://forum.arduino.cc/index.php?topic=58119.0

//Callback function to get the Email sending status
void sendCallback(SendStatus info);  
void setup() {
  int j=0;
  //pinMode (GPIO_NUM_13, INPUT);
  //if (digitalRead(GPIO_NUM_13) == LOW);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);
 
  Serial.setDebugOutput(true);
 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  pinMode(4, INPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_dis(GPIO_NUM_4);
 
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 8;  //modAM, was 10
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
 
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
 
  Serial.println("Starting SD Card");
 
  delay(500);
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    //return;
  }
 
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
   
  AMphoto();
  
  //delay(1000); //START eMail Section
  Serial.print("Connecting to AP");
  j=0;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while ((WiFi.status() != WL_CONNECTED) && (j < 21))
  {
    Serial.print(".");
    delay(500);
    j=j+1;
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Mounting SD Card...");
  //if (SD_MMC.begin()) // MailClient.sdBegin(14,2,15,13) for TTGO T8 v1.7 or 1.8
  //{
  //Serial.println("Preparing attach file...");
  Serial.println("Aquiring time from NTP server...");
  Serial.println();
  if (!timeHelper.setClock(1,0)); //time zone, summer time 
  {
    Serial.println("Can't set clock...");
     //   return;
  }
  Serial.println();
  Serial.println(timeHelper.getYear()); //modAM
  Serial.println(String(timeHelper.getDay()));
  Serial.println(month[timeHelper.getMonth()-1]);

  AMmail();
  //delay(3000);
  //AMphoto(); das geht, damit doch GPIO13 related
  j=0;
  //pinMode (GPIO_NUM_13, INPUT);
  //AMphoto(); hier gehts nicht mehr
  while (j<60) //60 s alert period after picture
  {
    if (digitalRead(GPIO_NUM_13) == LOW) {
      //if(!SD_MMC.begin()){ hilft nicht
      //  Serial.println("SD Card Mount Failed");
      //  //return;
      //}
      Serial.println();
      //delay(3000); hilft nicht
      AMphoto();
      AMmail();
      j=0;
    }
    else delay(1000);
    Serial.print(j);
    Serial.print (" ");
    j=j+1;
  }
  Serial.println();
  
  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
 
  Serial.println("Going to sleep now");
  delay(1000);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
} 
 
void loop() {
}

//Callback function to get the Email sending status
void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
    Serial.println("----------------");
  }
} 


void AMphoto()
{
camera_fb_t * fb = NULL;
 
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
 
  // Path where new picture will be saved in SD Card
  path = "/picture" + String(pictureNumber) +".jpg"; //modAM public String weg, pre-setup->global
 
  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());
 
  File file = fs.open(path.c_str(), FILE_WRITE); //hier spuckt er beim 2. Durchgang, braucht er Deklaration? wohl schon, file ist local
  if(!file){
    Serial.println("Failed to open file in writing mode");
  }
  else {
    Serial.println(fb->len);
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);  
}

void AMmail()
{
 Serial.println("Sending email...");

  //Set the Email host, port, account and password
   smtpData.setLogin("smtp.hosting-ch.ch", 587, "name@mails.ch", "Password");
  //For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
  //enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
  //smtpData.setSTARTTLS(false); //modAM half nicht

  //Set the sender name and Email
  smtpData.setSender("ESP32", "cam@mails.ch");

  //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  //Set the subject
  smtpData.setSubject("ESP32 PIR Image");

  //Set the message - normal text or html format
  smtpData.setMessage("<div style=\"color:#ff0000;font-size:20px;\">Hello World! - From ESP32</div>", true);

  //Add recipients, can add more than one recipient
  smtpData.addRecipient("dehalo@gmx.net");
  smtpData.addAttachFile(path.c_str());

  //Add some custom header to message
  //See https://tools.ietf.org/html/rfc822
  //These header fields can be read from raw or source of message when it received)
  smtpData.addCustomMessageHeader("Date: Sat, " +String(timeHelper.getDay()) + " " + month[timeHelper.getMonth()-1] + " " + String(timeHelper.getYear()) + " " + String(timeHelper.getHour()) + ":" + String(timeHelper.getMin()) + ":" + String(timeHelper.getSec()) + " +0100 (MET)");
  //Be careful when set Message-ID, it should be unique, otherwise message will not store
  smtpData.addCustomMessageHeader("Message-ID: <abcde." + String(timeHelper.getYear()) +  String(timeHelper.getDay()) + String(timeHelper.getMin()) + String(timeHelper.getSec())+ "@gmx.com>");  //modAM 

  //Set the storage types to read the attach files (SD is default)
  //smtpData.setFileStorageType(MailClientStorageType::SPIFFS);
  smtpData.setFileStorageType(MailClientStorageType::SD);

  smtpData.setSendCallback(sendCallback);

  //Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
  smtpData.empty(); 
}
