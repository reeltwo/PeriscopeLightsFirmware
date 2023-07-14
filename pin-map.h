#ifdef ESP32
 #define PRINTED_DROID
#else
 #define IA_PARTS
#endif
#ifdef IA_PARTS

#define LIGHTKIT_CENTER_LED_PIN 5

#ifdef USE_ABC_PINS
#define LIGHTKIT_PIN_A 17
#define LIGHTKIT_PIN_B 18
#define LIGHTKIT_PIN_C 19
#endif

#define MAX7221_DATA_PIN 12
#define MAX7221_CLK_PIN 11
#define MAX7221_CS_PIN 10

#elif defined(PRINTED_DROID)

#ifdef ESP32
#define SDA_PIN 21
#define SCL_PIN 22
#define RX_PIN 3

#define TOP_PIN 33
#define LEFT_PIN 16
#define CENTER_PIN 27
#define RIGHT_PIN 25
#define BOTTOM_PIN 17
#define REAR_PIN 26

#define TOGGLE_BUTTON 0
#elif defined(__AVR__)
#define SDA_PIN SDA
#define SCL_PIN SCL
#define RX_PIN 0

#define TOP_PIN 6
#define LEFT_PIN 3
#define CENTER_PIN 5
#define RIGHT_PIN 10
#define REAR_PIN 9
#define BOTTOM_PIN 2
#endif

#ifdef USE_ABC_PINS
#define LIGHTKIT_PIN_A RX_PIN
#define LIGHTKIT_PIN_B SDA_PIN
#define LIGHTKIT_PIN_C SCL_PIN
#endif

#define __ERROR(a)    _Pragma(#a)
#define _ERROR( a) __ERROR(GCC error #a)

#if defined(LIGHTKIT_PIN_A) && LIGHTKIT_PIN_A == RX_PIN
#define Serial _ERROR(Cannot use Serial in Uppity Spinner Mode. RX pins used as pin A)
#endif

#endif
