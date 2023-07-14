
#include "core/SetupEvent.h"
#include <FastLED.h>

#ifndef MAX_RGB_BRIGHTNESS
#define MAX_RGB_BRIGHTNESS 80   // 0-255, higher number is brighter.
#endif
#ifndef COLOR_ORDER
#define COLOR_ORDER GRB
#endif

class LedControlPrintedDroid : public LedControl, SetupEvent
{
public:
    /**
      * \brief Constructor
      */
    LedControlPrintedDroid()
    {
        clearDisplay(0);
        setIntensity(0, kMaxBrightness);
        setPower(0, false);
    }

    virtual void setup() override
    {
        FastLED.addLeds<WS2812, TOP_PIN, COLOR_ORDER>(fTopLeds, SizeOfArray(fTopLeds));
        FastLED.addLeds<WS2812, LEFT_PIN, COLOR_ORDER>(fLeftLeds, SizeOfArray(fLeftLeds));
        FastLED.addLeds<WS2812, CENTER_PIN, COLOR_ORDER>(fCenterLeds, SizeOfArray(fCenterLeds));
        FastLED.addLeds<WS2812, RIGHT_PIN, COLOR_ORDER>(fRightLeds, SizeOfArray(fRightLeds));
        FastLED.addLeds<WS2812, BOTTOM_PIN, COLOR_ORDER>(fBottomLeds, SizeOfArray(fBottomLeds));
        FastLED.addLeds<WS2812, REAR_PIN, COLOR_ORDER>(fRearLeds, SizeOfArray(fRearLeds));
    }

    virtual byte addDevice(byte count = 1) override
    {
        return 0;
    }

    virtual int getDeviceCount() override
    {
        return 1;
    }

    virtual bool isPowered(byte device) override
    {
        return false;
    }

    virtual void setPower(byte device, bool onState, byte count = 1) override
    {
    }

    virtual void setScanLimit(byte device, byte limit, byte count = 1) override
    {
    }

    virtual void setIntensity(byte device, byte intensity, byte count = 1) override
    {
        FastLED.setBrightness((intensity / float(kMaxBrightness)) * uint8_t(MAX_RGB_BRIGHTNESS&0xFF));
    }

    virtual void clearDisplay(byte device, byte count = 1) override
    {
        fill_solid(fTopLeds, SizeOfArray(fTopLeds), CRGB::Black);
        fill_solid(fLeftLeds, SizeOfArray(fLeftLeds), CRGB::Black);
        fill_solid(fCenterLeds, SizeOfArray(fCenterLeds), CRGB::Black);
        fill_solid(fRightLeds, SizeOfArray(fRightLeds), CRGB::Black);
        fill_solid(fBottomLeds, SizeOfArray(fBottomLeds), CRGB::Black);
        fill_solid(fRearLeds, SizeOfArray(fRearLeds), CRGB::Black);
    }

    virtual void setLed(byte device, int row, int column, boolean state) override
    {
        PeriscopeLeds led = PeriscopeLeds(row);
        switch (led)
        {
            case kTopRed:
                setTopLedRed(column, state ? 0xFF : 0x00);
                break;
            case kTopGreen:
                setTopLedGreen(column, state ? 0xFF : 0x00);
                break;
            case kTopBlue:
                setTopLedBlue(column, state ? 0xFF : 0x00);
                break;

            case kRear:
                setRearLed(column>>1, state ? CRGB::Red : CRGB::Black);
                break;
            case kBottom:
                setBottomLed(column, state ? CRGB::Red : CRGB::Black);
                break;
            case kLeft:
                /* Not used */
                break;
            case kRight:
                /* Not used */
                break;
            case kCenter:
                setCenterLed(column, state ? CRGB::White : CRGB::Black);
                break;
        }
    }

    virtual void setRow(byte device, int row, byte value) override
    {
        PeriscopeLeds led = PeriscopeLeds(row);
        switch (led)
        {
            case kTopRed:
                setTopPatternRed(value);
                break;
            case kTopGreen:
                setTopPatternGreen(value);
                break;
            case kTopBlue:
                setTopPatternBlue(value);
                break;

            case kLeft:
                setLeftPattern(value);
                break;
            case kCenter:
                setCenterPattern(value);
                break;
            case kRight:
                setRightPattern(value);
                break;
            case kBottom:
                setBottomPattern(value);
                break;
            default:
                /* Not used */
                break;
        }
    }

    virtual void setRowNoCache(byte device, int row, byte value) override
    {
        /* Not used */
    }
    
    virtual void setColumn(byte device, int column, byte value) override
    {
        /* Not used */
    }

    virtual byte getRow(byte device, byte row) override
    {
        /* Not used */
        return 0;
    }

    void setSearchLight(bool state)
    {
        fill_solid(fCenterLeds, SizeOfArray(fCenterLeds), state ? CRGB::White : CRGB::Black);
    }

private:
    CRGB fTopLeds[7];
    CRGB fLeftLeds[9];
    CRGB fCenterLeds[9];
    CRGB fRightLeds[9];
    CRGB fBottomLeds[8];
    CRGB fRearLeds[3];

    #define CHECK_BOUNDS(arr,idx) \
        if ((idx) >= SizeOfArray(arr)) { \
            DEBUG_PRINT(__PRETTY_FUNCTION__); DEBUG_PRINT(" out of bounds error index="); DEBUG_PRINTLN(idx); \
            return; \
        }

    void setTopLedRed(unsigned index, unsigned val)
    {
        CHECK_BOUNDS(fTopLeds, index);
        fTopLeds[index].r = val;
    }

    void setTopLedGreen(unsigned index, unsigned val)
    {
        CHECK_BOUNDS(fTopLeds, index);
        fTopLeds[index].g = val;
    }

    void setTopLedBlue(unsigned index, unsigned val)
    {
        CHECK_BOUNDS(fTopLeds, index);
        fTopLeds[index].b = val;
    }

    void setTopPatternRed(unsigned pattern)
    {
        for (unsigned col = 0; col < 7; col++)
        {
            setTopLedRed(col, ((pattern & (1<<col)) != 0) ? 0xFF : 0x00);
        }
    }

    void setTopPatternGreen(unsigned pattern)
    {
        for (unsigned col = 0; col < 7; col++)
        {
            setTopLedGreen(col, ((pattern & (1<<col)) != 0) ? 0xFF : 0x00);
        }
    }

    void setTopPatternBlue(unsigned pattern)
    {
        for (unsigned col = 0; col < 7; col++)
        {
            setTopLedBlue(col, ((pattern & (1<<col)) != 0) ? 0xFF : 0x00);
        }
    }

    void setSidePattern(CRGB* leds, unsigned num, unsigned pattern)
    {
        if ((pattern & kFrontRed) != 0)
        {
            fill_solid(leds, num, CRGB::Red);
        }
        else if ((pattern & kFrontGreen) != 0)
        {
            fill_solid(leds, num, CRGB::Green);
        }
        else if ((pattern & kFrontBlue) != 0)
        {
            fill_solid(leds, num, CRGB::Blue);
        }
        else if ((pattern & kFrontYellow) != 0)
        {
            fill_solid(leds, num, CRGB::Yellow);
        }
        else if ((pattern & kFrontCyan) != 0)
        {
            fill_solid(leds, num, CRGB::Cyan);
        }
        else if ((pattern & kFrontMagenta) != 0)
        {
            fill_solid(leds, num, CRGB::Magenta);
        }
        else if ((pattern & kFrontWhite) != 0)
        {
            fill_solid(leds, num, CRGB::White);
        }
        else if ((pattern & kTopWhite) != 0)
        {
            fill_solid(leds, num, CRGB::White);
        }
        else
        {
            fill_solid(leds, num, CRGB::Black);
        }
    }

    void setLeftPattern(unsigned pattern)
    {
        setSidePattern(fLeftLeds, SizeOfArray(fLeftLeds), pattern);
    }

    void setRightPattern(unsigned pattern)
    {
        setSidePattern(fRightLeds, SizeOfArray(fRightLeds), pattern);
    }

    void setBottomLed(unsigned index, CRGB color)
    {
        CHECK_BOUNDS(fBottomLeds, index);
        static uint8_t sPairs[][2] = {
            { 0, 1 },
            { 2, 3 },
            { 2, 4 },
            { 3, 5 },
            { 4, 5 },
            { 6, 7 }
        };
        fBottomLeds[sPairs[index][0]] = color;
        fBottomLeds[sPairs[index][1]] = color;
    }

    void setBottomPattern(unsigned pattern)
    {
        for (unsigned col = 0; col < 6; col++)
        {
            setBottomLed(col, ((pattern & (1<<col)) != 0) ? CRGB::Red : CRGB::Black);
        }
    }

    void setCenterLed(unsigned index, CRGB color)
    {
        CHECK_BOUNDS(fCenterLeds, index);
        fCenterLeds[index+1] = color;
    }

    void setCenterPattern(unsigned pattern)
    {
        for (unsigned col = 0; col < 8; col++)
        {
            setCenterLed(col, ((pattern & (1<<col)) != 0) ? CRGB::White : CRGB::Black);
        }
    }

    void setRearLed(unsigned index, CRGB color)
    {
        CHECK_BOUNDS(fRearLeds, index);
        fRearLeds[index] = color;
    }
};
