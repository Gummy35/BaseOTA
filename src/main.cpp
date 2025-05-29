#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <Logger.h>
#include <ControllerWebServer.h>
#include <WebSerial.h>


#define DEBUG

#ifdef DEBUG
#define debug(MyCode) MyCode
#else
#endif

/// @brief Init
void InitDevices()
{
  byte devId = 0;

  // set default logger callback
  Logger.SetLogger([](const char *logString)
                   {
                   
                     WebSerial.print(logString); });

  // Wait for serial
  Serial.begin(115200);
  while (!Serial)
    delay(10);
}

void PrintFreeRam()
{
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); // internal RAM, memory capable to store data or to create new task
  WebSerial.printf("Total free : %d, minimum free : %d, largest block : %d\n",
                   info.total_free_bytes,    // total currently free in all non-continues blocks
                   info.minimum_free_bytes,  // minimum free ever
                   info.largest_free_block); // largest continues block to allocate big array
}

/// @brief setup Web serial commands handling
void SetupWebSerialCommands()
{
  WebSerial.onMessage([&](uint8_t *data, size_t len)
                      {
                        unsigned long ts = millis();
                        debug(Serial.printf("Received %u bytes from WebSerial: ", len));
                        debug(Serial.write(data, len));
                        //debug(Serial.println());
                        String d(data, len);
                        WebSerial.print(d);
                        if (d.equals("help"))
                        {
                          
                          WebSerial.print("freeram");
                          WebSerial.print("reboot");
                        }
                       
                        else if (d.equals("freeram"))
                        {
                          PrintFreeRam();
                        }
                        else if (d.equals("reboot"))
                        {
                          LittleFS.end();
                          ESP.restart();
                        }
                        
                        debug(WebSerial.printf("%d ms\n", millis() - ts));
                        });
}

void setup()
{
  // start filesystem
  LittleFS.begin();
  // web server (ota + serial)
  ControllerWebServer.begin();
  delay(500);
  // init devices
  InitDevices();
  // Logger.Log("v1.4");
  
  // configure web serial commands
  SetupWebSerialCommands();

  // create FreeRTOS tasks
  // xTaskCreatePinnedToCore(TaskKeypadCode, "TaskKeypad", 10000, NULL, 1, &TaskKeypad, 0);
  // xTaskCreatePinnedToCore(TaskDisplayCode, "TaskDisplay", 10000, NULL, 1, &TaskDisplayController, 0);
  // xTaskCreatePinnedToCore(TaskCommsCode, "TaskComms", 10000, NULL, 1, &TaskComms, 1);
  // xTaskCreatePinnedToCore(TaskUpdateLedsCode, "TaskLedcontroller", 10000, NULL, 1, &TaskLedController, 1);
  // wait for everyone to be ready
  delay(1000);
 
}

unsigned long lasttick = 0;

void loop()
{
  // handle web server (OTA, web serial...)
  if (millis() - lasttick > 50)
  {
    ControllerWebServer.loop();
    lasttick = millis();
  }

}
