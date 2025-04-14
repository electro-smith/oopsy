#include <cstdint>
#include "daisy.h"
#include "daisy_petal.h"

enum LedOrder
{
    LED_RING_1_R,
    LED_RING_1_G,
    LED_RING_1_B,
    LED_RING_5_R,
    LED_RING_5_G,
    LED_RING_5_B,
    LED_RING_2_R,
    LED_RING_2_G,
    LED_RING_2_B,
    LED_RING_6_R,
    LED_RING_6_G,
    LED_RING_6_B,
    LED_RING_3_R,
    LED_RING_3_G,
    LED_RING_3_B,
    LED_FS_1,
    LED_RING_4_R,
    LED_RING_4_G,
    LED_RING_4_B,
    LED_RING_7_R,
    LED_RING_7_G,
    LED_RING_7_B,
    LED_RING_8_R,
    LED_RING_8_G,
    LED_RING_8_B,
    LED_FS_2,
    LED_FS_3,
    LED_FS_4,
    LED_FAKE1,
    LED_FAKE2,
    LED_FAKE3,
    LED_FAKE4,
    LED_LAST,
};

template<int numDrivers, bool persistentBufferContents>
void SetRingLed(daisy::LedDriverPca9685<numDrivers, persistentBufferContents>* led_driver, daisy::DaisyPetal::RingLed idx, float r, float g, float b)
{
    uint8_t r_addr[daisy::DaisyPetal::RingLed::RING_LED_LAST] = {LED_RING_1_R,
                                     LED_RING_2_R,
                                     LED_RING_3_R,
                                     LED_RING_4_R,
                                     LED_RING_5_R,
                                     LED_RING_6_R,
                                     LED_RING_7_R,
                                     LED_RING_8_R};
    uint8_t g_addr[daisy::DaisyPetal::RingLed::RING_LED_LAST] = {LED_RING_1_G,
                                     LED_RING_2_G,
                                     LED_RING_3_G,
                                     LED_RING_4_G,
                                     LED_RING_5_G,
                                     LED_RING_6_G,
                                     LED_RING_7_G,
                                     LED_RING_8_G};
    uint8_t b_addr[daisy::DaisyPetal::RingLed::RING_LED_LAST] = {LED_RING_1_B,
                                     LED_RING_2_B,
                                     LED_RING_3_B,
                                     LED_RING_4_B,
                                     LED_RING_5_B,
                                     LED_RING_6_B,
                                     LED_RING_7_B,
                                     LED_RING_8_B};


    led_driver->SetLed(r_addr[idx], r);
    led_driver->SetLed(g_addr[idx], g);
    led_driver->SetLed(b_addr[idx], b);
}