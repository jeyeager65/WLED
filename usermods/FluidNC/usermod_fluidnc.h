#pragma once

#include "wled.h"

/*
 *  FluidNC Usermod - Connects to FluidNC via TCP/Telnet and monitors the status to display X position and preset.
 */
class FluidNcUsermod : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = true;
    bool initDone = false;
    unsigned long lastTime = 0;

    String fluidNcUrl = "";
    int fluidNcTelnetPort = 23;
    unsigned int widthX = 1000;
    unsigned int positionLedCount = 1;
    int positionOffsetX = 0;
    int ledStripLength = 1000;
    bool reversePositionX = false;
    
    WiFiClient client;
    bool fluidNcConnected = false;
    int ledCount = 0;

    String status = "UNKNOWN";
    String statusPrev = "UNKNOWN";
    int mPosX = 0;
    int mPosXPrev = 0;

    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];

  public:
    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() override {
      ledCount = strip.getMainSegment().length();
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() override {
      while(!fluidNcConnected) {
        Serial.println("Connecting to FluidNC...");
        if(client.connect(fluidNcUrl.c_str(), fluidNcTelnetPort)) {
          client.write("$Report/Interval=200\n");
          fluidNcConnected = true;
          Serial.println("Connected to FluidNC!");
        }
        else {
          Serial.println("Not Connected");
          delay(500);
        }
      }

      initDone = true;
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     */
    void loop() override {
      // if usermod is disabled or called during strip updating just exit
      // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
      if (!enabled /*|| strip.isUpdating() */) return;

      // do your magic here
      if (millis() - lastTime > 200) {
        //Serial.println("I'm alive!");
        lastTime = millis();

        String tempLine = "", statusLine = "";

        while (client.available()) {
          tempLine = client.readStringUntil('\n');
          Serial.println(tempLine);

          if(tempLine.startsWith("<")) {
            statusLine = tempLine;
          }
        }

        // If there are multiple lines, only the last one matters
        if(statusLine.length() > 0) {
          // Parse status
          unsigned int pos = statusLine.indexOf("|");

          status = statusLine.substring(1, pos);
          
          unsigned int pos2 = statusLine.indexOf("|", pos+1);
          String mpos = statusLine.substring(pos, pos2);

          // Only care about integer portion
          String mPosXStr = mpos.substring(6, mpos.indexOf("."));
          mPosX = mPosXStr.toInt();
          
          if(status != statusPrev) {

            Serial.printf("Status: %s", status.c_str());

            if(status == "Idle") {
              applyPreset(1);
            } else if(status == "Home") {
              applyPreset(2);
            } else if(status == "Alarm") {
              applyPreset(3);
            } else if(status.startsWith("Hold")) {
              applyPreset(4);
            } else if(status == "Run") {
              applyPreset(5);
            } else if(status == "Jog") {
              applyPreset(6);
            } else {
              applyPreset(7);
            }
          }

          statusPrev = status;
        }
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
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     * 
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.  
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root) override
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;

      //save these vars persistently whenever settings are saved
      top["FluidNC URL"] = fluidNcUrl;
      top["FluidNC Port"] = fluidNcTelnetPort;
      top["CNC X Width (mm)"] = widthX;                     // Total usable X width
      top["CNC X Position LED Width"] = positionLedCount;   // Number of LEDs used to display the position
      top["CNC X Position Offset (mm)"] = positionOffsetX;  // Offset between the first LED and the endmill
      top["CNC X Position Reverse"] = reversePositionX;     // Reverse X position - set to true if start of strip is at X max end
      top["LED Strip Length (mm)"] = ledStripLength;        // Distance from center of first LED to center of last LED
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root) override
    {
      // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
      // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

      JsonObject top = root[FPSTR(_name)];

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      configComplete &= getJsonValue(top["FluidNC URL"], fluidNcUrl);
      configComplete &= getJsonValue(top["FluidNC Port"], fluidNcTelnetPort);
      configComplete &= getJsonValue(top["CNC X Width (mm)"], widthX);
      configComplete &= getJsonValue(top["CNC X Position LED Width"], positionLedCount);
      configComplete &= getJsonValue(top["CNC X Position Offset (mm)"], positionOffsetX);
      configComplete &= getJsonValue(top["CNC X Position Reverse"], reversePositionX);
      configComplete &= getJsonValue(top["LED Strip Length (mm)"], ledStripLength);

      return configComplete;
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw() override
    {
      if(status == "Jog" || status == "Run") {
        Serial.println("Processing Overlay");

        // Get number of pixels used by machine bounds
        int machinePixels = ((float)widthX/(float)ledStripLength) * ledCount;

        // Get pixel position within machine X bounds
        int tmp = constrain(mPosX, 0, widthX);
        tmp = map(mPosX, 0, widthX, 0, machinePixels-1);

        // Translate with offset - translate offset to pixels
        int offsetPixels = map(positionOffsetX, 0, ledStripLength, 0, ledCount);
        tmp += offsetPixels;

        /*
        // Mirror Option 1 - Flip part after position
        for(int i = tmp+1, j = ledCount-1; i <= j; i++, j--) {
          const uint32_t tmpColor = strip.getPixelColor(i);
          strip.setPixelColor(i, strip.getPixelColor(j));
          strip.setPixelColor(j, tmpColor);
        } 
        
        // Mirror Option 2 - Mirror the longer side  
        if(tmp > ledCount/2) {
          // First side is longer, mirror to second side
          for(int i = tmp+1, j=tmp-1; i < ledCount; i++, j--) {
            strip.setPixelColor(i, strip.getPixelColor(j));
          }
        } else {
          // Second side is longer, mirror to first side
          for(int i = tmp-1, j=tmp+1; i >=0; i--, j++) {
            strip.setPixelColor(i, strip.getPixelColor(j));
          }

          // Then flip again to keep consistent pattern direction
          for(int i = 0, j = ledCount-1; i < ledCount/2; i++, j--) {
            const uint32_t tmpColor = strip.getPixelColor(i);
            strip.setPixelColor(i, strip.getPixelColor(j));
            strip.setPixelColor(j, tmpColor);
          }
        }
        */

        if(positionLedCount > 0) {
          int positionIndex = tmp;

          if(reversePositionX) {
            positionIndex = ledCount - tmp;
          }

          // Add an extra LED if even
          if(positionLedCount%2 == 0) {
            positionLedCount++;
          }

          strip.setPixelColor(positionIndex, RGBW32(0xFF,0xFF,0xFF,255));

          for(int i = 1; i <= positionLedCount/2; i++) {
            if(positionIndex-i >= 0) {
              strip.setPixelColor(positionIndex-i, RGBW32(0xFF,0xFF,0xFF,255));
            }
            if(positionIndex+i <= widthX) {
              strip.setPixelColor(positionIndex+i, RGBW32(0xFF,0xFF,0xFF,255));
            }
          }
        }
      }
    }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId() override
    {
      return USERMOD_ID_FLUIDNC;
    }
};

// add more strings here to reduce flash memory usage
const char FluidNcUsermod::_name[]    PROGMEM = "FluidNC";
const char FluidNcUsermod::_enabled[] PROGMEM = "enabled";