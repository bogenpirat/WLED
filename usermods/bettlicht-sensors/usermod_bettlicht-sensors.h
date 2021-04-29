#pragma once

#include "wled.h"

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 * 
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

//class name. Use something descriptive and leave the ": public Usermod" part :)
class BettlichtSensors : public Usermod {
  private:
    //Private class members. You can declare variables and functions only accessible to your usermod here
    unsigned long lastTime = 0;

    // make this ui-configurable
    std::vector<unsigned short> ldrPins;
    std::vector<unsigned short> pirPins;
    unsigned int ldrThreshold;
    unsigned long stayOnTime; // millis

    // values retrieved from the sensors
    std::vector<unsigned int> ldrValues;
    std::vector<char> pirValues;

    // state management
    boolean lastOnManual = false;
    unsigned long lastPirTriggeredTime = 0;
  public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
      // TODO: move this out to config
      this->ldrPins.push_back(32);
      this->pirPins.push_back(13);
      this->ldrThreshold = 500; // ADC res is 12bit, so values of 2^12 = 0-4095
      this->stayOnTime = 60*1000; // ms

      // set up digital pins
      for(int i = 0; i < this->pirPins.size(); i++) {
        pinMode(this->pirPins[i], INPUT);
      }
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      //Serial.println("Connected to WiFi!");
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
      if (millis() - lastTime > 300) {
        //uint16_t start = millis(); // TODO: benchmarking
        
        updateSensors();
        
        // uint16_t end = this->lastTime = millis(); // TODO: benchmarking
        //Serial.print("loop timing: "); Serial.print(end - start); Serial.println("ms"); // TODO:debugging
      }
    }


    /*
     * read data from sensors - not a WLED callback
     */
    void updateSensors() {
      this->pirValues.clear();
      for(int i = 0; i < this->pirPins.size(); i++) {
        this->pirValues.push_back((char) (digitalRead(this->pirPins[i]) == 1));
      }
      this->ldrValues.clear();
      for(int i = 0; i < this->ldrPins.size(); i++) {
        this->ldrValues.push_back(analogRead(this->ldrPins[i]));
      }
      
      // check if PIR was triggered and if so, note the time
      if(isPirTriggered()) {
        this->lastPirTriggeredTime = millis();
      }

      boolean isOn = bri != 0;

      /* 
       * turn on the LEDs iff: 
       * - they are currently off
       * - the PIR detects motion
       * - the LDR shows low light conditions
       */
      if(!isOn && isPirTriggered() && isLdrTriggered()) {
        lastOnManual = false;
        bri = briLast;
        colorUpdated(NOTIFIER_CALL_MODE_NO_NOTIFY);
      }

      /* 
       * turn off LEDs iff:
       * - they are currently on
       * - they were last turned on by the automatic logic and not manually
       * - the PIR hasn't shown motion for a pre-set time
       */
      if(isOn && !this->lastOnManual && millis() - this->lastPirTriggeredTime > this->stayOnTime) {
        briLast = bri;
        bri = 0;
        colorUpdated(NOTIFIER_CALL_MODE_NO_NOTIFY);
      }
    }

    /*
     * checks if the light-dependent resistor is triggered/activated
     */
    boolean isLdrTriggered() {
      boolean ret = false;

      for(int i = 0; i < this->ldrValues.size(); i++) {
        if(this->ldrValues[i] < this->ldrThreshold) {
          ret = true;
          break;
        }
      }

      return ret;
    }

    /*
     * checks if the passive infrared sensor is triggered/activated
     */
    boolean isPirTriggered() {
      boolean ret = false;

      for(int i = 0; i < this->pirValues.size(); i++) {
        if((bool) this->pirValues[i]) {
          ret = true;
          break;
        }
      }

      return ret;
    }


    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    /*
    void addToJsonInfo(JsonObject& root)
    {
      int reading = 20;
      //this code adds "u":{"Light":[20," lux"]} to the info object
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");

      JsonArray lightArr = user.createNestedArray("Light"); //name
      lightArr.add(reading); //value
      lightArr.add(" lux"); //unit
    }
    */


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      JsonObject sensors = root.createNestedObject("sensors");
      
      // add pir values
      JsonArray pirs = sensors.createNestedArray("pir");
      for(int i = 0; i < this->pirValues.size(); i++) {
        pirs.add((bool) this->pirValues[i]);
      }

      // add ldr values
      JsonArray ldrs = sensors.createNestedArray("ldr");
      for(int i = 0; i < this->ldrValues.size(); i++) {
        ldrs.add(this->ldrValues[i]);
      }

      // other state/config info
      sensors["lastPirTriggeredTime"] = this->lastPirTriggeredTime;
      sensors["lastOnManual"] = this->lastOnManual;
      sensors["ldrThreshold"] = this->ldrThreshold;
      sensors["stayOnTime"] = this->stayOnTime;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      if(root["on"].as<boolean>() == true) {
        this->lastOnManual = true;
      }
    }


    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will also not yet add your setting to one of the settings pages automatically.
     * To make that work you still have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("exampleUsermod");
      top["great"] = userVar0; //save this var persistently whenever settings are saved
    }


    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens once immediately after boot)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     */
    void readFromConfig(JsonObject& root)
    {
      JsonObject top = root["top"];
      userVar0 = top["great"] | 42; //The value right of the pipe "|" is the default value in case your setting was not present in cfg.json (e.g. first boot)
    }

   
    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_EXAMPLE;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};