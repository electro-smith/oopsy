#include "daisy_petal_125b_sm.h"
#include <vector>

namespace daisy
{
    void Petal125BSM::Init()
    {
        /** Pot Pins */
        constexpr Pin knobpins[KNOB_LAST] = {
            Pin(PORTC, 4), /**< VR1 - sch: ADC_POT_6 */
            Pin(PORTA, 7), /**< VR2 - sch: ADC_POT_5 */
            Pin(PORTB, 1), /**< VR3 - sch: ADC_POT_3 */
            Pin(PORTA, 6), /**< VR4 - sch: ADC_POT_2 */
            Pin(PORTA, 2), /**< VR5 - sch: ADC_POT_4 */
            Pin(PORTA, 3), /**< VR6 - sch: ADC_POT_1 */
        };

        /** 3-way Toggle Switch Pins
         *  These are routed to the opposite sides
         *  of what the Switch3 class expects.
         *
         *  So below, we initialize them in what
         *  may appear to be the opposite direction.
         */
        constexpr Pin togglepins[TOGGLE_LAST][2] = {
            {Pin(PORTB, 4), Pin(PORTC, 12)},  /**< sch: GPIO_TSW (1A, 1B) */
            {Pin(PORTA, 8), Pin(PORTC, 7)},   /**< sch: GPIO_TSW (3A, 3B) */
            {Pin(PORTB, 15), Pin(PORTB, 14)}, /**< sch: GPIO_TSW (2A, 2B) */
        };

        /** Momentary Footswitch Pins */
        constexpr Pin footswpins[FOOTSWITCH_LAST] = {
            Pin(PORTD, 13), /**< sch: GPIO_MSW_2 */
            Pin(PORTC, 6),  /**< sch: GPIO_MSW_1 */
        };

        /** LED Pins */
        constexpr Pin ledpins[LED_LAST] = {
            Pin(PORTA, 5),  /**< LED1R - sch: GPIO_LED_1_R, */
            Pin(PORTB, 6),  /**< LED1G - sch: GPIO_LED_1_G, */
            Pin(PORTD, 7),  /**< LED1B - sch: GPIO_LED_1_B, */
            Pin(PORTG, 14), /**< LED2 - sch: GPIO_LED_2, */
            Pin(PORTG, 10), /**< LED3R - sch: GPIO_LED_3_R, */
            Pin(PORTC, 10), /**< LED3G - sch: GPIO_LED_3_G, */
            Pin(PORTC, 11), /**< LED3B - sch: GPIO_LED_3_B, */
            Pin(PORTB, 5),  /**< LED4 - sch: GPIO_LED_4, */
            Pin(PORTB, 7),  /**< LED5 - sch: GPIO_LED_5, */
        };

        /** Expression Pin */
        constexpr Pin exprpin = Pin(PORTC, 0);

        /** Relay Control Pin */
        constexpr Pin relay_pin = Pin(PORTG, 13);

        /** System Init (sys, ram, flash, etc.) */
        System::Config syscfg;
        syscfg.Defaults();
        sys.Init(syscfg);

        /** SDRAM */
        sdram.Init();

        /** EXT. FLASH */
        auto qspi_config = qspi.GetConfig();
        qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
        qspi_config.mode = QSPIHandle::Config::Mode::MEMORY_MAPPED;
        qspi_config.pin_config.io0 = dsy_pin(DSY_GPIOF, 8);
        qspi_config.pin_config.io1 = dsy_pin(DSY_GPIOF, 9);
        qspi_config.pin_config.io2 = dsy_pin(DSY_GPIOF, 7);
        qspi_config.pin_config.io3 = dsy_pin(DSY_GPIOF, 6);
        qspi_config.pin_config.clk = dsy_pin(DSY_GPIOF, 10);
        qspi_config.pin_config.ncs = dsy_pin(DSY_GPIOG, 6);
        qspi.Init(qspi_config);

        // Audio Init
        SaiHandle::Config sai_config;
        sai_config.periph = SaiHandle::Config::Peripheral::SAI_1;
        sai_config.sr = SaiHandle::Config::SampleRate::SAI_48KHZ;
        sai_config.bit_depth = SaiHandle::Config::BitDepth::SAI_24BIT;
        sai_config.a_sync = SaiHandle::Config::Sync::MASTER;
        sai_config.b_sync = SaiHandle::Config::Sync::SLAVE;
        sai_config.a_dir = SaiHandle::Config::Direction::RECEIVE;
        sai_config.b_dir = SaiHandle::Config::Direction::TRANSMIT;
        sai_config.pin_config.fs = {DSY_GPIOE, 4};
        sai_config.pin_config.mclk = {DSY_GPIOE, 2};
        sai_config.pin_config.sck = {DSY_GPIOE, 5};
        sai_config.pin_config.sa = {DSY_GPIOE, 6};
        sai_config.pin_config.sb = {DSY_GPIOE, 3};
        SaiHandle sai_1_handle;
        sai_1_handle.Init(sai_config);

        // TODO: Add Codec Init
        I2CHandle::Config i2c_cfg;
        i2c_cfg.periph = I2CHandle::Config::Peripheral::I2C_1;
        i2c_cfg.speed = I2CHandle::Config::Speed::I2C_400KHZ;
        i2c_cfg.mode = I2CHandle::Config::Mode::I2C_MASTER;
        i2c_cfg.pin_config.scl = {DSY_GPIOB, 8};
        i2c_cfg.pin_config.sda = {DSY_GPIOB, 9};

        I2CHandle i2c1;
        i2c1.Init(i2c_cfg);
        codec.Init(i2c1);

        AudioHandle::Config audio_config;
        audio_config.blocksize = 4;
        audio_config.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
        audio_config.postgain = 1.f;
        audio.Init(audio_config, sai_1_handle);
        callback_rate_ = AudioSampleRate() / AudioBlockSize();

        // Adc Init
        AdcChannelConfig adc_config[KNOB_LAST + 1];
        for (size_t i = 0; i < KNOB_LAST; i++)
        {
            adc_config[i].InitSingle(knobpins[i]);
        }
        // last ADC channel will be expression input.
        adc_config[KNOB_LAST].InitSingle(exprpin);
        adc.Init(adc_config, KNOB_LAST + 1);

        for (size_t i = 0; i < KNOB_LAST; i++)
        {
            knob[i].Init(adc.GetPtr(i), AudioCallbackRate());
        }
        expression.Init(adc.GetPtr(KNOB_LAST), AudioCallbackRate());
        // Other Inits (switches, analogcontrols, etc.)
        for (size_t i = 0; i < TOGGLE_LAST; i++)
        {
            toggle[i].Init(togglepins[i][1], togglepins[i][0]);
        }
        footswitch1.Init(footswpins[0], AudioCallbackRate());
        footswitch2.Init(footswpins[1], AudioCallbackRate());
        for (size_t i = 0; i < LED_LAST; i++)
        {
            // Invert RGB LEDs because they're common anode
            bool inv = (i == LED_1_R) || (i == LED_1_G) || (i == LED_1_B) ||
                               (i == LED_3_R) | (i == LED_3_G) | (i == LED_3_B)
                           ? true
                           : false;
            led[i].Init(ledpins[i], inv, AudioCallbackRate());
        }

        relay_control.Init(relay_pin, GPIO::Mode::OUTPUT);
        bypass_state_ = false;
        SetBypassState(bypass_state_);
        adc.Start();
        SetLed1(0.f, 0.f, 0.f);
        SetLed2(0.f);
        SetLed3(0.f, 0.f, 0.f);
        SetLed4(0.f);
        SetLed5(0.f);
        UpdateLeds();
    }

    void Petal125BSM::StartAudio(AudioHandle::InterleavingAudioCallback cb)
    {
        audio.Start(cb);
    }

    void Petal125BSM::StartAudio(AudioHandle::AudioCallback cb)
    {
        audio.Start(cb);
    }

    void Petal125BSM::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb)
    {
        audio.ChangeCallback(cb);
    }

    void Petal125BSM::ChangeAudioCallback(AudioHandle::AudioCallback cb)
    {
        audio.ChangeCallback(cb);
    }

    void Petal125BSM::StopAudio()
    {
        audio.Stop();
    }

    void Petal125BSM::SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate)
    {
        audio.SetSampleRate(samplerate);
        callback_rate_ = AudioSampleRate() / AudioBlockSize();
        for (size_t i = 0; i < LED_LAST; i++)
            led[i].SetSampleRate(callback_rate_);
        for (size_t i = 0; i < KNOB_LAST; i++)
            knob[i].SetSampleRate(callback_rate_);
    }

    float Petal125BSM::AudioSampleRate()
    {
        return audio.GetSampleRate();
    }

    void Petal125BSM::SetAudioBlockSize(size_t blocksize)
    {
        audio.SetBlockSize(blocksize);
        callback_rate_ = AudioSampleRate() / AudioBlockSize();
        for (size_t i = 0; i < LED_LAST; i++)
            led[i].SetSampleRate(callback_rate_);
        for (size_t i = 0; i < KNOB_LAST; i++)
            knob[i].SetSampleRate(callback_rate_);
    }

    size_t Petal125BSM::AudioBlockSize()
    {
        return audio.GetConfig().blocksize;
    }

    float Petal125BSM::AudioCallbackRate()
    {
        return callback_rate_;
    }

    void Petal125BSM::ProcessAnalogControls()
    {
        for (size_t i = 0; i < KNOB_LAST; i++)
        {
            knob[i].Process();
        }
        expression.Process();
    }

    void Petal125BSM::ProcessDigitalControls()
    {
        footswitch1.Debounce();
        footswitch2.Debounce();
    }

    void Petal125BSM::SetBypassState(bool state)
    {
        bypass_state_ = state;
        relay_control.Write(!bypass_state_);
    }

    void Petal125BSM::ToggleBypassState()
    {
        bypass_state_ = !bypass_state_;
        relay_control.Write(!bypass_state_);
    }

    void Petal125BSM::UpdateLeds()
    {
        for (size_t i = 0; i < LED_LAST; i++)
        {
            led[i].Update();
        }
    }

    void Petal125BSM::SetLed1(float r, float g, float b)
    {
        led[LED_1_R].Set(r);
        led[LED_1_G].Set(g);
        led[LED_1_B].Set(b);
    }

    void Petal125BSM::SetLed3(float r, float g, float b)
    {
        led[LED_3_R].Set(r);
        led[LED_3_G].Set(g);
        led[LED_3_B].Set(b);
    }

    bool Petal125BSM::ValidateSDRAM()
    {
        uint32_t *sdramptr = (uint32_t *)0xc0000000;
        uint32_t size_in_words = 16777216;
        uint32_t testval = 0xdeadbeef;
        uint32_t num_failed = 0;
        /** Write test val */
        for (uint32_t i = 0; i < size_in_words; i++)
        {
            uint32_t *word = sdramptr + i;
            *word = testval;
        }
        /** Compare written */
        for (uint32_t i = 0; i < size_in_words; i++)
        {
            uint32_t *word = sdramptr + i;
            if (*word != testval)
                num_failed++;
        }
        /** Write Zeroes */
        for (uint32_t i = 0; i < size_in_words; i++)
        {
            uint32_t *word = sdramptr + i;
            *word = 0x00000000;
        }
        /** Compare Cleared */
        for (uint32_t i = 0; i < size_in_words; i++)
        {
            uint32_t *word = sdramptr + i;
            if (*word != 0)
                num_failed++;
        }
        return num_failed == 0;
    }

    bool Petal125BSM::ValidateQSPI(bool quick)
    {
        uint32_t start;
        uint32_t size;
        if (quick)
        {
            start = 0x400000;
            size = 0x4000;
        }
        else
        {
            start = 0;
            size = 0x800000;
        }
        // Erase the section to be tested
        qspi.Erase(start, start + size);
        // Create some test data
        std::vector<uint8_t> test;
        test.resize(size);
        uint8_t *testmem = test.data();
        for (size_t i = 0; i < size; i++)
            testmem[i] = (uint8_t)(i & 0xff);
        // Write the test data to the device
        qspi.Write(start, size, testmem);
        // Read it all back and count any/all errors
        // I supppose any byte where ((data & 0xff) == data)
        // would be able to false-pass..
        size_t fail_cnt = 0;
        for (size_t i = 0; i < size; i++)
            if (testmem[i] != (uint8_t)(i & 0xff))
                fail_cnt++;
        return fail_cnt == 0;
    }

}; // namespace daisy
