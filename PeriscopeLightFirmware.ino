/*
 * --------------------------------------------------------------------------
 * PeriscopeLightFirmware (https://github.com/reeltwo/PeriscopeLightFirmware)
 * --------------------------------------------------------------------------
 *
 * PeriscopeLightFirmware is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * PeriscopeLightFirmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PeriscopeLightFirmware; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

///////////////////////////////////

#if __has_include("build_version.h")
#include "build_version.h"
#else
#define BUILD_VERSION "custom"
#endif

#if __has_include("reeltwo_build_version.h")
#include "reeltwo_build_version.h"
#endif

///////////////////////////////////

#define SERIAL_BAUD_RATE 9600
// Enable if you want to use ABC control pins
#define USE_ABC_PINS
// Enable if you want Serial control.
// NOTE you cannot enable both USE_SERIAL and USE_ABC_PINS for Printed-Droid periscope.
//#define USE_SERIAL
#ifdef USE_SERIAL
// Disable if you don't want debug messages
#define USE_DEBUG
#endif

// RGB ONLY Periscope Lights
#define MAX_RGB_BRIGHTNESS 80   // 0-255, higher number is brighter.
#define COLOR_ORDER    GRB

///////////////////////////////////

#include "pin-map.h"

///////////////////////////////////

#include "ReelTwo.h"
#include "core/SetupEvent.h"
#include "core/AnimatedEvent.h"
#include "core/CommandEvent.h"
#include "core/StringUtils.h"
#include "core/LedControlMAX7221.h"

///////////////////////////////////

enum PeriscopeSequence
{
    kPeriscope_FIRST = 0,
    kPeriscope_Random = 0,
    kPeriscope_Off,
    kPeriscope_ObiWan,
    kPeriscope_Yoda,
    kPeriscope_Sith,
    kPeriscope_SearchLight,
    kPeriscope_Dagobah,
    kPeriscope_Strobe,
    kPeriscope_LAST
};

///////////////////////////////////

enum PeriscopeLeds
{
    kTopRed = 0,
    kRear,
    kBottom,
    kLeft,
    kRight,
    kCenter,
    kTopGreen,
    kTopBlue
};

enum PeriscopeColor
{
    kRed = 0,
    kGreen,
    kBlue,
    kMagenta,
    kCyan,
    kYellow,
    kWhite
};

// Flash patterns
enum PeriscopePatterns
{
    kAll          = 0b11111111,
    kFrontRed     = 0b00100000,
    kFrontGreen   = 0b01000000,
    kFrontBlue    = 0b10000000,
    kFrontYellow  = 0b01100000,
    kFrontCyan    = 0b11000000,
    kFrontMagenta = 0b10100000,
    kFrontWhite   = 0b11100000,
    kBackRed      = 0b00000100,
    kBackGreen    = 0b00001000,
    kBackBlue     = 0b00010000,
    kBackYellow   = 0b00001100,
    kBackCyan     = 0b00011000,
    kBackMagenta  = 0b00010100,
    kBackWhite    = 0b00011100,
    kTopWhite     = 0b00000001,
    kBottomWhite  = 0b00000010
};

/**
  * \ingroup Periscope
  *
  * \class PeriscopeLightsBase
  *
  * \brief Base class for Periscope Light Kit
  *
  * The PeriscopeLightsBase implements the functionality of the periscope light and depends on the LedControl
  * instance to drive the actual LEDs.
  *
  */
class PeriscopeLightsBase :
    public AnimatedEvent, SetupEvent, CommandEvent
{
public:
    /**
      * \brief Default Constructor
      */
    PeriscopeLightsBase(LedControl& ledControl) :
        fLC(ledControl),
        fDisplayEffect(kNormal),
        fPreviousEffect(~fDisplayEffect),
        fStatusDelay(10),
        fStatusMillis(0),
        fMainLED(*this),
        fTopLED(*this),
        fFastTopLED(*this),
        fDagobah(*this),
        fSith(*this),
        fObiWan(*this),
        fYoda(*this),
        fSearchLight(*this),
        fSparkle(*this),
        fLeftSideLED(*this),
        fRightSideLED(*this),
        fRearSingleLED(*this),
        fRearTopLED(*this),
        fRearBottomLED(*this),
        fBounceTopLED(*this),
        fSplitBounceTopLED(*this),
        fRadiateTopLED(*this),
        fFlashTopLED(*this),
        fBigLED(*this),
        fMainRingFlash(*this),
        fMainRingSpin(*this),
        fBottomLEDChase(*this),
        fBottomLEDFlash(*this)
    {
    #ifdef LIGHTKIT_CENTER_LED_PIN
        pinMode(LIGHTKIT_CENTER_LED_PIN, OUTPUT);
    #endif
    #ifdef LIGHTKIT_PIN_A
        pinMode(LIGHTKIT_PIN_A, INPUT_PULLUP);
    #endif
    #ifdef LIGHTKIT_PIN_B
        pinMode(LIGHTKIT_PIN_B, INPUT_PULLUP);
    #endif
    #ifdef LIGHTKIT_PIN_C
        pinMode(LIGHTKIT_PIN_C, INPUT_PULLUP);
    #endif
    }

    enum EffectValue
    {
        kNormalVal = 0
    };

    enum Sequence
    {
        kNormal = 0,
    };

    /**
      * Perform any initialzation not possible in the constructor
      */
    virtual void setup() override
    {
        fStatusMillis = millis();
    }

    /**
      * PL00000 - Normal
      */
    virtual void handleCommand(const char* cmd) override
    {
        if (*cmd++ == 'P' && *cmd++ == 'L')
        {
            long int cmdvalue = 0;
            const char* c = cmd;
            while (*c >= '0' && *c <= '9')
            {
                cmdvalue = cmdvalue * 10 + (*c++ - '0');
            }
            // selectEffect(cmdvalue);
        }
    }

    void selectSequence(unsigned sequence)
    {
        if (sequence < kPeriscope_LAST)
            selectSequence(PeriscopeSequence(sequence));
    }

    void selectSequence(PeriscopeSequence sequence)
    {
        if (sequence != fCurrentSequence)
        {
            init();
            fCurrentSequence = sequence;
        }
    }

    void nextSequence()
    {
        selectSequence((fCurrentSequence < kPeriscope_LAST) ? unsigned(fCurrentSequence) + 1 : 0);
    }

    /**
      * Perform a single frame of LED animation based on the selected sequence.
      */
    virtual void animate() override
    {
        unsigned long currentMillis = millis();
        if (currentMillis - fStatusMillis >= fStatusDelay)
        {
            fStatusMillis = currentMillis;
            unsigned inputSelect = 0;
        #ifdef USE_ABC_PINS
            inputSelect = readInputHeaders();
        #endif
            if (fFirstTime || fInputState != inputSelect)
            {
                fInputState = inputSelect;
                selectSequence(PeriscopeSequence(inputSelect));
                fFirstTime = false;
            }
            switch (fCurrentSequence)
            {
                case kPeriscope_Off:
                    break;

                case kPeriscope_Random:
                    fMainLED.animate();
                    fTopLED.animate();
                    break;

                case kPeriscope_ObiWan:
                    fObiWan.animate(20);
                    break;

                case kPeriscope_Yoda:
                    fYoda.animate(100);
                    break;

                case kPeriscope_Sith:
                    fSith.animate();
                    break;

                case kPeriscope_SearchLight:
                    fSearchLight.animate();
                    break;

                case kPeriscope_Dagobah:
                    fDagobah.animate();
                    break;

                case kPeriscope_Strobe:
                    fSparkle.animate(0);
                    fFastTopLED.animate();
                    break;

                default:
                    break;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    virtual void searchLightState(bool state) = 0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    void setLed(PeriscopeLeds led, unsigned col, bool state)
    {
        fLC.setLed(0, unsigned(led), col, state);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    void setRow(PeriscopeLeds led, unsigned bits)
    {
        fLC.setRow(0, unsigned(led), bits);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    void setTopLed(PeriscopeColor color, unsigned col, bool state)
    {
        switch (color)
        {
            case kRed:
                setLed(kTopRed, col, state);
                setLed(kTopGreen, col, false);
                setLed(kTopBlue, col, false);
                break;
            case kGreen:
                setLed(kTopRed, col, false);
                setLed(kTopGreen, col, state);
                setLed(kTopBlue, col, false);
                break;
            case kBlue:
                setLed(kTopRed, col, false);
                setLed(kTopGreen, col, false);
                setLed(kTopBlue, col, state);
                break;
            case kYellow:
                setLed(kTopRed, col, state);
                setLed(kTopGreen, col, state);
                setLed(kTopBlue, col, false);
                break;
            case kMagenta:
                setLed(kTopRed, col, state);
                setLed(kTopGreen, col, false);
                setLed(kTopBlue, col, state);
                break;
            case kCyan:
                setLed(kTopRed, col, false);
                setLed(kTopGreen, col, state);
                setLed(kTopBlue, col, state);
                break;
            case kWhite:
                setLed(kTopRed, col, state);
                setLed(kTopGreen, col, state);
                setLed(kTopBlue, col, state);
                break;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    void setTopRow(PeriscopeColor color, unsigned bits)
    {
        switch (color)
        {
            case kRed:
                setRow(kTopRed, bits);
                setRow(kTopGreen, 0);
                setRow(kTopBlue, 0);
                break;
            case kGreen:
                setRow(kTopRed, 0);
                setRow(kTopGreen, bits);
                setRow(kTopBlue, 0);
                break;
            case kBlue:
                setRow(kTopRed, 0);
                setRow(kTopGreen, 0);
                setRow(kTopBlue, bits);
                break;
            case kYellow:
                setRow(kTopRed, bits);
                setRow(kTopGreen, bits);
                setRow(kTopBlue, 0);
                break;
            case kMagenta:
                setRow(kTopRed, bits);
                setRow(kTopGreen, 0);
                setRow(kTopBlue, bits);
                break;
            case kCyan:
                setRow(kTopRed, 0);
                setRow(kTopGreen, bits);
                setRow(kTopBlue, bits);
                break;
            case kWhite:
                setRow(kTopRed, bits);
                setRow(kTopGreen, bits);
                setRow(kTopBlue, bits);
                break;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class LEDState
    {
    public:
        LEDState(PeriscopeLightsBase& lightKit) :
            fLK(lightKit),
            fNext(nullptr)
        {
            if (fLK.fHead == nullptr)
                fLK.fHead = this;
            if (fLK.fTail != nullptr)
                fLK.fTail->fNext = this;
            fLK.fTail = this;
        }

        virtual void init()
        {
        }

        void searchLightState(bool state)
        {
            fLK.searchLightState(state);
        }

        void setLed(PeriscopeLeds led, unsigned col, bool state)
        {
            fLK.setLed(led, col, state);
        }

        void setRow(PeriscopeLeds led, unsigned bits)
        {
            fLK.setRow(led, bits);
        }

        void setTopLed(PeriscopeColor color, unsigned col, bool state)
        {
            fLK.setTopLed(color, col, state);
        }

        void setTopRow(PeriscopeColor color, unsigned bits)
        {
            fLK.setTopRow(color, bits);
        }

        inline void setIntensity(unsigned intensity)
        {
            fLK.fLC.setIntensity(0, intensity);
        }

        inline void MainRingSpin(unsigned state, unsigned rate, bool dir)
        {
            fLK.fMainRingSpin.animate(state, rate, dir);
        }

        inline void BigLED(unsigned onTime, unsigned offTime)
        {
            fLK.fBigLED.animate(onTime, offTime);
        }

        inline void MainRingFlash(unsigned pattern, unsigned onTime, unsigned offTime, bool rand)
        {
            fLK.fMainRingFlash.animate(pattern, onTime, offTime, rand);
        }

        inline void FlashTopLED(unsigned pattern, PeriscopeColor color, unsigned onTime, unsigned offTime, bool rand)
        {
            fLK.fFlashTopLED.animate(pattern, color, onTime, offTime, rand);
        }

        inline void BounceTopLED(PeriscopeColor color, unsigned rate)
        {
            fLK.fBounceTopLED.animate(color, rate);
        }

        inline unsigned SplitBounceTopLED(PeriscopeColor color, unsigned rate)
        {
            return fLK.fSplitBounceTopLED.animate(color, rate);
        }

        inline void RadiateTopLED(PeriscopeColor color, unsigned rate)
        {
            fLK.fRadiateTopLED.animate(color, rate);
        }

        inline void LeftSideLED(unsigned pattern, unsigned pattern2, unsigned onTime, unsigned offTime)
        {
            fLK.fLeftSideLED.animate(pattern, pattern2, onTime, offTime);
        }

        inline void RightSideLED(unsigned pattern, unsigned pattern2, unsigned onTime, unsigned offTime)
        {
            fLK.fRightSideLED.animate(pattern, pattern2, onTime, offTime);
        }

        inline void RearSingleLED(unsigned onTime, unsigned offTime)
        {
            fLK.fRearSingleLED.animate(onTime, offTime);
        }

        inline void RearBottomLED(unsigned onTime, unsigned offTime)
        {
            fLK.fRearBottomLED.animate(onTime, offTime);
        }

        inline void RearTopLED(unsigned onTime, unsigned offTime)
        {
            fLK.fRearTopLED.animate(onTime, offTime);
        }

        inline void BottomLEDChase(unsigned time = 0)
        {
            fLK.fBottomLEDChase.animate(time);
        }

        inline void BottomLEDFlash(unsigned pattern, unsigned onTime, unsigned offTime, bool rand = false)
        {
            fLK.fBottomLEDFlash.animate(pattern, onTime, offTime, rand);
        }

    private:
        friend class PeriscopeLightsBase;
        PeriscopeLightsBase& fLK;
        LEDState* fNext;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class MainLED : public LEDState
    {
    public:
        MainLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {
        }

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
            fPattern = 0;
            fSpinDirection = 0;
            fSpinNumber = 0;
            fFlashPattern = 0;
            fPatternDuration = 0;
            fRate = 0;
            fTimeUntilOff = 0;
        }

        void animate()
        {
            if (fState == 0)
            {
                fPattern = random(0, 2);
                fSpinDirection = random(0, 2);
                fSpinNumber = random(1, 6);

                fFlashPattern = random(0, 256);
                fRate = random(0, 100);
                fTimeUntilOff = random(0, 100);

                if (random(0,4) > 1)
                    fTimeUntilOff = 0;
                fPatternDuration = random(3000, 12000);

                fState++;
            }
            switch (fPattern)
            {
                case 0:
                    MainRingSpin(fSpinNumber, fRate, fSpinDirection);
                    break;
                case 1:
                    MainRingFlash(fFlashPattern, fRate, fTimeUntilOff, 0);
                    break;
            }
            if (fTime ++ > fPatternDuration)
            {
                fTime = 0;
                fState = 0;
            }
        }
    private:
        unsigned fState;
        unsigned fTime;
        unsigned fPattern;
        unsigned fSpinDirection;
        unsigned fSpinNumber;
        unsigned fFlashPattern;
        unsigned fPatternDuration;
        unsigned fRate;
        unsigned fTimeUntilOff;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class TopLED : public LEDState
    {
    public:
        TopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {
        }

        virtual void init() override
        {
            fState = 0;
            fPattern = 0;
            fColor = kRed;
            fRate = 10;
            fRateOff = 10;
            fDuration = 1000;
            fTime = 0;
        }

        void animate()
        {
            if (fState == 0)
            {
                if (fPattern == 1)
                {
                    setTopRow(kWhite, 0);
                }

                fPattern = random(0, 4);
                if (fPattern == 1)
                {
                    setTopRow(kWhite, 0);
                }
                fColor = PeriscopeColor(random(7));
                fRate = random(5, 100);
                fRateOff = random(5, 100);
                fDuration = random(3000, 10000);
                fState++;
            }
            switch (fPattern)
            {
                case 0:
                    FlashTopLED(0b11111111, fColor, fRate, fRateOff, 0);
                    break;
                case 1:
                    BounceTopLED(fColor, fRate);
                    break;
                case 2:
                    SplitBounceTopLED(fColor, fRate);
                    break;
                case 3:
                    RadiateTopLED(fColor, fRate);
                    break;
            }
            switch (fColor)
            {
                case kRed:
                    RightSideLED(kFrontRed, kBackRed, 1, 0);
                    LeftSideLED(kFrontRed, kBackRed, 1, 0);
                    break;
                case kGreen:
                    RightSideLED(kFrontGreen, kBackGreen, 1, 0);
                    LeftSideLED(kFrontGreen, kBackGreen, 1, 0);
                    break;
                case kBlue:
                    RightSideLED(kFrontBlue, kBackBlue, 1, 0);
                    LeftSideLED(kFrontBlue, kBackBlue, 1, 0);
                    break;
                case kMagenta:
                    RightSideLED(kFrontMagenta, kBackMagenta, 1, 0);
                    LeftSideLED(kFrontMagenta, kBackMagenta, 1, 0);
                    break;
                case kCyan:
                    RightSideLED(kFrontCyan, kBackCyan, 1, 0);
                    LeftSideLED(kFrontCyan, kBackCyan, 1, 0);
                    break;
                case kYellow:
                    RightSideLED(kFrontYellow, kBackYellow, 1, 0);
                    LeftSideLED(kFrontYellow, kBackYellow, 1, 0);
                    break;
                case kWhite:
                    RightSideLED(kFrontWhite, kBackWhite, 1, 0);
                    LeftSideLED(kFrontWhite, kBackWhite, 1, 0);
                    break;
            }
            BottomLEDChase(20);
            RearSingleLED(100, 200);
            RearBottomLED(200, 500);
            RearTopLED(300, 1000);

            if (fTime++ > fDuration)
            {
                fTime = 0;
                fState = 0;
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
        unsigned fPattern;
        PeriscopeColor fColor;
        unsigned fRate;
        unsigned fRateOff;
        unsigned fDuration;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class FastTopLED : public LEDState
    {
    public:
        FastTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
            fColor = kRed;
            fPattern = 0;
            fSelectTime = 100;
            fRate = 10;
            fRateOff = 10;
        }

        void animate()
        {
            if (fState == 0)
            {
                if (fPattern == 1)
                    setTopRow(kWhite, 0b00000000);
                fPattern = random(0, 4);
                if (fPattern == 1)
                    setTopRow(kWhite, 0b00000000);

                fColor = PeriscopeColor(random(7));
                fRate = random(2, 10);
                fRateOff = random(5, 20);
                fSelectTime = random(70, 200);
                fState++;
            }
            switch (fPattern)
            {
                case 0:
                    FlashTopLED(0b11111111, fColor, fRate, fRateOff, 0);
                    break;
                case 1:
                    BounceTopLED(fColor, fRate);
                    break;
                case 2:
                    SplitBounceTopLED(fColor, fRate);
                    break;
                case 3:
                    RadiateTopLED(fColor, fRate);
                    break;
            }
            switch (fColor)
            {
                case kRed:
                    LeftSideLED(kFrontRed, kBackRed, fColor, fRate);
                    RightSideLED(kFrontRed, kBackRed, fColor, fRate);
                    break;
                case kGreen:
                    LeftSideLED(kFrontGreen, kBackGreen, fColor, fRate);
                    RightSideLED(kFrontGreen, kBackGreen, fColor, fRate);
                    break;
                case kBlue:
                    LeftSideLED(kFrontBlue, kBackBlue, fColor, fRate);
                    RightSideLED(kFrontBlue, kBackBlue, fColor, fRate);
                    break;
                case kMagenta:
                    LeftSideLED(kFrontMagenta, kBackMagenta, fColor, fRate);
                    RightSideLED(kFrontMagenta, kBackMagenta, fColor, fRate);
                    break;
                case kCyan:
                    LeftSideLED(kFrontCyan, kBackCyan, fColor, fRate);
                    RightSideLED(kFrontCyan, kBackCyan, fColor, fRate);
                    break;
                case kYellow:
                    LeftSideLED(kFrontYellow, kBackYellow, fColor, fRate);
                    RightSideLED(kFrontYellow, kBackYellow, fColor, fRate);
                    break;
                case kWhite:
                    LeftSideLED(kFrontWhite, kBackWhite, fColor, fRate);
                    RightSideLED(kFrontWhite, kBackWhite, fColor, fRate);
                    break;
            }
            BottomLEDChase(25);
            RearSingleLED(50, 100);
            RearBottomLED(50, 100);
            RearTopLED(100, 100);

            if (fTime++ > fSelectTime)
            {
                fTime = 0;
                fState = 0;
            }
        }

    private:
        unsigned fState;
        unsigned fPattern;
        PeriscopeColor fColor;
        unsigned fRate;
        unsigned fRateOff;
        unsigned fTime;
        unsigned fSelectTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class Dagobah : public LEDState
    {
    public:
        Dagobah(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        void animate()
        {
            FlashTopLED(0b11111111, kWhite, 0, 1, 0);

            MainRingFlash(kAll, 10, 0, 0);
            LeftSideLED(kTopWhite, kBottomWhite, 10, 0);
            RightSideLED(kTopWhite, kBottomWhite, 10, 0);

            RearSingleLED(100, 200);
            RearBottomLED(200, 500);
            RearTopLED(300, 1000);

            BottomLEDFlash(kAll, 10, 0);
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class Sith : public LEDState
    {
    public:
        Sith(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fNextTime = 0;
        }

        void animate()
        {
            if (fNextTime < millis())
            {
                LeftSideLED(kFrontRed, kBackRed, 0, 1);
                RightSideLED(kFrontRed, kBackRed, 0, 1);
                MainRingFlash(kAll, 0, 1, 0);

                BottomLEDChase(20);
                RearSingleLED(100, 200);
                RearBottomLED(200, 500);
                RearTopLED(300, 1000);

                if (SplitBounceTopLED(kRed, 15) == 1)
                {
                    fLK.fLeftSideLED.setState(0, 1);
                    LeftSideLED(kFrontRed, kBackRed, 1, 0);

                    fLK.fRightSideLED.setState(0, 1);
                    RightSideLED(kFrontRed, kBackRed, 1, 0);
                    
                    fLK.fBottomLEDFlash.setTime(1);
                    BottomLEDFlash(kAll, 1, 0);

                    fNextTime = millis() + 400;
                }
            }
        }

    private:
        uint32_t fNextTime = 0;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class ObiWan : public LEDState
    {
    public:
        ObiWan(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
        }

        void animate(unsigned rate)
        {
            switch (fState)
            {
                case 0:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 1, 0);
                    LeftSideLED(kFrontBlue, kBackBlue, 1, 0);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > random(80, 160) * rate)
                    {
                        fTime = 0;
                        fState++;
                    }
                    break;

                case 1:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 0, 1);
                    LeftSideLED(kFrontBlue, kBackBlue, 0, 1);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        fState++;
                    }
                    break;

                case 2:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 1, 0);
                    LeftSideLED(kFrontBlue, kBackBlue, 1, 0);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        fState++;
                    }
                    break;

                case 3:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 0, 1);
                    LeftSideLED(kFrontBlue, kBackBlue, 0, 1);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        fState++;
                    }
                    break;

                case 4:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 1, 0);
                    LeftSideLED(kFrontBlue, kBackBlue, 1, 0);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate * 2)
                    {
                        fTime = 0;
                        fState++;
                    }
                    break;

                case 5:
                    MainRingFlash(0b00000000, 80, 0, 1);

                    RightSideLED(kFrontBlue, kBackBlue, 0, 1);
                    LeftSideLED(kFrontBlue, kBackBlue, 0, 1);

                    RadiateTopLED(kBlue, 10);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate * 2)
                    {
                        fTime = 0;
                        fState = 0;
                    }
                    break;
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class Yoda : public LEDState
    {
    public:
        Yoda(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
            fBrightness = LedControl::kMaxBrightness;
        }

        void animate(unsigned rate)
        {
            switch (fState)
            {
                case 0:
                    MainRingFlash(kAll, 0, 1, 0);
                    RightSideLED(kFrontGreen, kBackGreen, 1, 0);
                    LeftSideLED(kFrontGreen, kBackGreen, 1, 0);
                    FlashTopLED(0b11111111, kGreen, 1, 0, 0);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate / 25)
                    {
                        fBrightness--;
                        setIntensity(fBrightness);
                        fTime = 0;
                        if (fBrightness < 1)
                            fState++;
                    }
                    break;

                case 1:
                    MainRingFlash(kAll, 0, 1, 0);
                    RightSideLED(kFrontGreen, kBackGreen, 0, 1);
                    LeftSideLED(kFrontGreen, kBackGreen, 0, 1);
                    FlashTopLED(0b11111111, kGreen, 0, 1, 0);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate)
                    {
                        setIntensity(0);
                        fTime = 0;
                        fState++;
                    }
                    else if (fTime > 5)
                    {
                        setIntensity(LedControl::kMaxBrightness);
                    }
                    break;

                case 2:
                    MainRingFlash(kAll, 0, 1, 0);
                    RightSideLED(kFrontGreen, kBackGreen, 1, 0);
                    LeftSideLED(kFrontGreen, kBackGreen, 1, 0);
                    FlashTopLED(0b11111111, kGreen, 1, 0, 0);
                    BottomLEDChase(20);
                    RearSingleLED(100, 200);
                    RearBottomLED(200, 500);
                    RearTopLED(300, 1000);
                    if (fTime++ > rate / 25)
                    {
                        fBrightness++;
                        setIntensity(fBrightness);
                        fTime = 0;
                        if (fBrightness > 14)
                            fState = 0;
                    }
                    break;
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
        unsigned fBrightness;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class SearchLight : public LEDState
    {
    public:
        SearchLight(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        void animate()
        {
            RightSideLED(kFrontWhite, kBackWhite, 0, 1);
            LeftSideLED(kFrontWhite, kBackWhite, 0, 1);
            FlashTopLED(0b11111111, kWhite, 0, 1, 0);

            BigLED(1, 0);
            MainRingFlash(kAll, 1, 0, 0);

            BottomLEDChase(20);
            RearSingleLED(100, 200);
            RearBottomLED(200, 500);
            RearTopLED(300, 1000);
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class Sparkle : public LEDState
    {
    public:
        Sparkle(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        void animate(bool mode)
        {
            MainRingFlash(kAll, 1, 5, 1);
            BigLED(1, random(25,250));
            BottomLEDChase(20);
            RearSingleLED(100, 200);
            RearBottomLED(200, 500);
            RearTopLED(300, 1000);

            if (mode)
            {
                LeftSideLED(kFrontWhite, kBackWhite, 3, 16);
                RightSideLED(kFrontWhite, kBackWhite, 2, 17);
                FlashTopLED(0b11111111, kWhite, 2, 13, 1);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class RightSideLED : public LEDState
    {
    public:
        RightSideLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void setState(unsigned state, unsigned time)
        {
            fState = state;
            fTime = time;
        }

        void animate(unsigned pattern, unsigned pattern2, unsigned onTime, unsigned offTime)
        {
            pattern |= pattern2;
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                        setRow(kRight, 0b00000000);
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                        setRow(kRight, pattern);
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class LeftSideLED : public LEDState
    {
    public:
        LeftSideLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void setState(unsigned state, unsigned time)
        {
            fState = state;
            fTime = time;
        }

        void animate(unsigned pattern, unsigned pattern2, unsigned onTime, unsigned offTime)
        {
            pattern |= pattern2;
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                        setRow(kLeft, 0b00000000);
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                        setRow(kLeft, pattern);
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class RearSingleLED : public LEDState
    {
    public:
        RearSingleLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned onTime, unsigned offTime)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                    {
                        setLed(kRear, 0, false);
                    }
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        setLed(kRear, 0, true);
                    }
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class RearTopLED : public LEDState
    {
    public:
        RearTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned onTime, unsigned offTime)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                    {
                        setLed(kRear, 2, false);
                    }
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        setLed(kRear, 2, true);
                    }
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class RearBottomLED : public LEDState
    {
    public:
        RearBottomLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned onTime, unsigned offTime)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                    {
                        setLed(kRear, 4, false);
                    }
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        setLed(kRear, 4, true);
                    }
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class BounceTopLED : public LEDState
    {
    public:
        BounceTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fPosition = 6;
            fTime = 0;
        }

        void animate(PeriscopeColor color, unsigned rate)
        {
            switch (fState)
            {
                case 0:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, fPosition, false);
                        fPosition = 6;
                        setTopLed(color, fPosition, true);
                        fState++;
                    }
                    break;
                case 1:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, fPosition, false);
                        fPosition--;
                        setTopLed(color, fPosition, true);
                        if (fPosition < 1)
                            fState++;
                    }
                    break;
                case 2:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, fPosition, false);
                        fPosition++;
                        setTopLed(color, fPosition, true);
                        if (fPosition > 5)
                            fState--;
                    }
                    break;
            }
        }

    private:
        unsigned fState;
        unsigned fPosition;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class SplitBounceTopLED : public LEDState
    {
    public:
        SplitBounceTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
        }

        unsigned animate(PeriscopeColor color, unsigned rate)
        {
            switch (fState)
            {
                case 0:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 2, false);
                        setTopLed(color, 4, false);
                        setTopLed(color, 3, true);
                        fState++;
                    }
                    break;
                case 1:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 3, false);
                        setTopLed(color, 2, true);
                        setTopLed(color, 4, true);
                        fState++;
                    }
                    break;
                case 2:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 2, false);
                        setTopLed(color, 4, false);
                        setTopLed(color, 1, true);
                        setTopLed(color, 5, true);
                        fState++;
                    }
                    break;
                case 3:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 1, false);
                        setTopLed(color, 5, false);
                        setTopLed(color, 0, true);
                        setTopLed(color, 6, true);
                        fState++;
                        return 1;
                    }
                    break;
                case 4:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 0, false);
                        setTopLed(color, 6, false);
                        setTopLed(color, 1, true);
                        setTopLed(color, 5, true);
                        fState++;
                    }
                    break;
                case 5:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 1, false);
                        setTopLed(color, 5, false);
                        setTopLed(color, 2, true);
                        setTopLed(color, 4, true);
                        fState = 0;
                    }
                    break;
            }
            return 0;
        }

    private:
        unsigned fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class RadiateTopLED : public LEDState
    {
    public:
        RadiateTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
        }

        void animate(PeriscopeColor color, unsigned rate)
        {
            switch (fState)
            {
                case 0:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, 2, false);
                        setTopLed(kWhite, 4, false);
                        setTopLed(color, 3, true);
                        fState++;
                    }
                    break;
                case 1:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 2, true);
                        setTopLed(color, 4, true);
                        fState++;
                    }
                    break;
                case 2:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 1, true);
                        setTopLed(color, 5, true);
                        fState++;
                    }
                    break;
                case 3:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(color, 0, true);
                        setTopLed(color, 6, true);
                        fState++;
                    }
                    break;
                case 4:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, 0, false);
                        setTopLed(kWhite, 6, false);
                        setTopLed(color, 1, true);
                        setTopLed(color, 5, true);
                        fState++;
                    }
                    break;
                case 5:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, 1, false);
                        setTopLed(kWhite, 5, false);
                        setTopLed(color, 2, true);
                        setTopLed(color, 4, true);
                        fState++;
                    }
                    break;
                case 6:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, 2, false);
                        setTopLed(kWhite, 4, false);
                        setTopLed(color, 3, true);
                        fState++;
                    }
                    break;
                case 7:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setTopLed(kWhite, 3, false);
                        fState = 0;
                    }
                    break;
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class FlashTopLED : public LEDState
    {
    public:
        FlashTopLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned pattern, PeriscopeColor color, unsigned onTime, unsigned offTime, bool rand)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                    {
                        setTopRow(kWhite, 0b00000000);
                    }
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        if (rand)
                        {
                            pattern = random(1, 255);
                        }
                        setTopRow(color, pattern);
                    }
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class BigLED : public LEDState
    {
    public:
        BigLED(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned onTime, unsigned offTime)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                        searchLightState(false);
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                        searchLightState(true);
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class MainRingFlash : public LEDState
    {
    public:
        MainRingFlash(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = false;
            fTime = 0;
        }

        void animate(unsigned pattern, unsigned onTime, unsigned offTime, bool rand)
        {
            if (fState)
            {
                if (fTime++ >= onTime)
                {
                    fTime = 0;
                    if (offTime)
                    {
                        setRow(kCenter, 0);
                    }
                    fState = false;
                }
            }
            else
            {
                if (fTime++ >= offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        if (rand)
                            pattern = random(1, 255);
                        setRow(kCenter, pattern);
                    }
                    fState = true;
                }
            }
        }

    private:
        bool fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class MainRingSpin : public LEDState
    {
    public:
        MainRingSpin(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fTime = 0;
            fPosition = 0;
        }

        void animate(unsigned state, unsigned rate, bool dir)
        {
            switch (state)
            {
                case 1:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setLed(kCenter, fPosition, false);
                        updatePosition(dir);
                        setLed(kCenter, fPosition, true);
                    }
                    break;
                case 2:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setLed(kCenter, fPosition, false);
                        setLed(kCenter, ((fPosition + 4) % 8), false);
                        updatePosition(dir);
                        setLed(kCenter, fPosition, true);
                        setLed(kCenter, ((fPosition + 4) % 8), true);
                    }
                    break;
                case 3:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setLed(kCenter, fPosition, false);
                        setLed(kCenter, ((fPosition + 5) % 8), false);
                        setLed(kCenter, ((fPosition + 3) % 8), false);
                        updatePosition(dir);
                        setLed(kCenter, fPosition,true);
                        setLed(kCenter, ((fPosition + 5) % 8), true);
                        setLed(kCenter, ((fPosition + 3) % 8), true);
                    }
                    break;
                case 4:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setLed(kCenter, fPosition, false);
                        setLed(kCenter, ((fPosition + 2) % 8), false);
                        setLed(kCenter, ((fPosition + 4) % 8), false);
                        setLed(kCenter, ((fPosition + 6) % 8), false);
                        updatePosition(dir);
                        setLed(kCenter, fPosition, true);
                        setLed(kCenter, ((fPosition + 2) % 8), true);
                        setLed(kCenter, ((fPosition + 4) % 8), true);
                        setLed(kCenter, ((fPosition + 6) % 8), true);
                    }
                    break;
                case 5:
                    if (fTime++ > rate)
                    {
                        fTime = 0;
                        setLed(kCenter, fPosition,false);
                        setLed(kCenter, ((fPosition + 1) % 8), false);
                        setLed(kCenter, ((fPosition + 2) % 8), false);
                        setLed(kCenter, ((fPosition + 3) % 8), false);
                        setLed(kCenter, ((fPosition + 4) % 8), false);
                        updatePosition(dir);
                        setLed(kCenter, fPosition, true);
                        setLed(kCenter, ((fPosition + 1) % 8), true);
                        setLed(kCenter, ((fPosition + 2) % 8), true);
                        setLed(kCenter, ((fPosition + 3) % 8), true);
                        setLed(kCenter, ((fPosition + 4) % 8), true);
                    }
                    break;
            }
        }

    private:
        unsigned fTime;
        int fPosition;

        inline void updatePosition(bool dir)
        {
            if (dir)
            {
                if (++fPosition > 7)
                    fPosition = 0;
            }
            else
            {
                if (--fPosition < 0)
                    fPosition = 7;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class BottomLEDChase : public LEDState
    {
    public:
        BottomLEDChase(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
            fPosition = 0;
        }

        void animate(unsigned time = 0)
        {
            switch (fState)
            {
                case 0:
                    if (fTime++ > time)
                    {
                        fTime = 0;
                        fPosition = 0;
                        setLed(kBottom, fPosition, true);
                        fState++;
                    }
                    break;
                case 1:
                    if (fTime++ > time)
                    {
                        fTime = 0;
                        setLed(kBottom, fPosition, false);
                        fPosition++;
                        setLed(kBottom, fPosition, true);
                        if (fPosition >= 5)
                            fState++;
                    }
                    break;
                case 2:
                    if (fTime++ > time)
                    {
                        fTime = 0;
                        setLed(kBottom, fPosition, false);
                        fPosition--;
                        setLed(kBottom, fPosition, true);
                        if (fPosition < 1)
                            fState = 1;
                    }
                    break;
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
        unsigned fPosition;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class BottomLEDFlash : public LEDState
    {
    public:
        BottomLEDFlash(PeriscopeLightsBase& lightKit) :
            LEDState(lightKit)
        {}

        virtual void init() override
        {
            fState = 0;
            fTime = 0;
        }

        void setTime(unsigned time)
        {
            fTime = time;
        }

        void animate(unsigned pattern, unsigned onTime, unsigned offTime, bool rand = false)
        {
            if (fState)
            {
                if (fTime++ > onTime)
                {
                    fTime = 0;
                    if (offTime)
                        setRow(kBottom, 0);
                    fState = false;
                }
            }
            else
            {
                if (fTime++ > offTime)
                {
                    fTime = 0;
                    if (onTime)
                    {
                        if (rand)
                            pattern = random(1, 255);
                        setRow(kBottom, pattern);
                    }
                    fState = true;
                }
            }
        }

    private:
        unsigned fState;
        unsigned fTime;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    friend class LEDState;
    LedControl& fLC;
    unsigned long fDisplayEffect;
    unsigned long fPreviousEffect;
    unsigned long fStatusDelay;
    unsigned long fStatusMillis;
    bool fFirstTime = true;
    unsigned fInputState = 0;
    PeriscopeSequence fCurrentSequence = kPeriscope_Random;
    LEDState* fHead = nullptr;
    LEDState* fTail = nullptr;
    MainLED fMainLED;
    TopLED fTopLED;
    FastTopLED fFastTopLED;
    Dagobah fDagobah;
    Sith fSith;
    ObiWan fObiWan;
    Yoda fYoda;
    SearchLight fSearchLight;
    Sparkle fSparkle;
    LeftSideLED fLeftSideLED;
    RightSideLED fRightSideLED;
    RearSingleLED fRearSingleLED;
    RearTopLED fRearTopLED;
    RearBottomLED fRearBottomLED;
    BounceTopLED fBounceTopLED;
    SplitBounceTopLED fSplitBounceTopLED;
    RadiateTopLED fRadiateTopLED;
    FlashTopLED fFlashTopLED;
    BigLED fBigLED;
    MainRingFlash fMainRingFlash;
    MainRingSpin fMainRingSpin;
    BottomLEDChase fBottomLEDChase;
    BottomLEDFlash fBottomLEDFlash;

#ifdef USE_ABC_PINS
    static int readInputHeaders()
    {
        bool inputA = digitalRead(LIGHTKIT_PIN_A);
        bool inputB = digitalRead(LIGHTKIT_PIN_B);
        bool inputC = digitalRead(LIGHTKIT_PIN_C);

        return (uint8_t(!inputA)<<2) |
               (uint8_t(!inputB)<<1) |
               (uint8_t(!inputC)<<0);
    }
#endif

    void init()
    {
        fLC.setAllPower(true);
        fLC.clearDisplay(0);
        fLC.setIntensity(0, LedControl::kMaxBrightness);
        fLC.setScanLimit(0, 7);
        searchLightState(false);
        for (LEDState* led = fHead; led != NULL; led = led->fNext)
            led->init();
    }
};

#ifdef IA_PARTS
/**
  * \ingroup Dome
  *
  * \class PeriscopeLights
  *
  * \brief PeriscopeLights by ia-parts.com
  *
  * The PeriscopeLights is implemented as a single device MAX7221. 
  *
  * Default pinout:
  *   pin MAX7221_DATA_PIN is connected to the DataIn 
  *   pin MAX7221_CLK_PIN is connected to the CLK 
  *   pin MAX7221_CS_PIN is connected to LOAD 
  */
class PeriscopeLights :
    protected LedControlMAX7221<1>,
    public PeriscopeLightsBase
{
public:
    /**
      * \brief Default Constructor
      */
    PeriscopeLights(byte dataPin = MAX7221_DATA_PIN, byte clkPin = MAX7221_CLK_PIN, byte csPin = MAX7221_CS_PIN) :
        LedControlMAX7221<1>(dataPin, clkPin, csPin),
        PeriscopeLightsBase((LedControl&)*this)
    {
    }

    virtual void searchLightState(bool state) override
    {
    #ifdef LIGHTKIT_CENTER_LED_PIN
        digitalWrite(LIGHTKIT_CENTER_LED_PIN, state ? HIGH : LOW);
    #endif
    }
};
#elif defined(PRINTED_DROID)
#include "PrintedDroid.h"
/**
  * \ingroup Dome
  *
  * \class PeriscopeLights
  *
  * \brief PeriscopeLights by ia-parts.com
  *
  * The PeriscopeLights is implemented as a single device MAX7221. 
  *
  * Default pinout:
  *   pin MAX7221_DATA_PIN is connected to the DataIn 
  *   pin MAX7221_CLK_PIN is connected to the CLK 
  *   pin MAX7221_CS_PIN is connected to LOAD 
  */
class PeriscopeLights :
    public LedControlPrintedDroid,
    public PeriscopeLightsBase
{
public:
    /**
      * \brief Default Constructor
      */
    PeriscopeLights() :
        PeriscopeLightsBase((LedControl&)*this)
    {
    }

    virtual void searchLightState(bool state) override
    {
        setSearchLight(state);
    }
};
#endif

PeriscopeLights lights;

void setup()
{
    REELTWO_READY();

#ifdef USE_SERIAL
#ifndef USE_DEBUG
    Serial.begin(SERIAL_BAUD_RATE);
#endif
    PrintReelTwoInfo(Serial, "Periscope Lights");
#endif

    SetupEvent::ready();

    randomSeed(analogRead(0));
}

#ifdef USE_SERIAL
bool processSerialCommand(char* cmd)
{
    switch (cmd[0])
    {
        case ':':
            if (cmd[1] == 'P' && cmd[2] == 'L')
            {
                uint32_t seq = strtolu(cmd+3, &cmd);
                if (*cmd == '\0')
                {
                    // Play new sequence
                    lights.selectSequence(seq);
                    return true;
                }
            }
            break;
        case 'O':
            if (strcmp(cmd, "ON") == 0)
            {
                lights.selectSequence(kPeriscope_Random);
                return true;
            }
            else if (strcmp(cmd, "OFF") == 0)
            {
                lights.selectSequence(kPeriscope_Off);
                return true;
            }
            break;
        case '#':
            if (cmd[1] == 'P' && cmd[2] == 'L')
            {
                /* Currently no configure commands */
                return true;
            }
            break;
    }
    return false;
}
#endif

void loop()
{
    AnimatedEvent::process();
#ifdef USE_SERIAL
    if (Serial.available())
    {
        static uint8_t sPos;
        static char sBuffer[32];
        char ch = Serial.read();
        if (ch == 0x0A || ch == 0x0D)
        {
            if (*sBuffer != 0 && !processSerialCommand(sBuffer))
            {
                Serial.print(F("Invalid: \""));
                Serial.print(sBuffer);
                Serial.println(F("\""));
            }
            sPos = 0;
            sBuffer[sPos] = '\0';
        }
        else if (sPos < SizeOfArray(sBuffer)-1)
        {
            sBuffer[sPos++] = ch;
            sBuffer[sPos] = '\0';
        }
    }
#endif
#ifdef FASTLED_VERSION
    FastLED.show();
#endif
}
