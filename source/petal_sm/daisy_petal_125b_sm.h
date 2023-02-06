#pragma once
#ifndef DSY_BUD_H
#define DSY_BUD_H

#include "daisy.h"

namespace daisy
{
    /**
     * @brief Configurable Guitar Pedal SOM based on Daisy.
     * @ingroup boards
     */
    class Petal125BSM
    {
    public:
        enum
        {
            KNOB_1, /**< Top Left */
            KNOB_2, /**< Top Midddle */
            KNOB_3, /**< Top Right */
            KNOB_4, /**< Bottom Left */
            KNOB_5, /**< Bottom Middle */
            KNOB_6, /**< Bottom Right */
            KNOB_LAST,
        };

        enum
        {
            LED_1_R,
            LED_1_G,
            LED_1_B,
            LED_2,
            LED_3_R,
            LED_3_G,
            LED_3_B,
            LED_4,
            LED_5,
            LED_LAST,
        };

        enum
        {
            TOGGLE_1, /**< Left */
            TOGGLE_2, /**< Middle */
            TOGGLE_3, /**< Right */
            TOGGLE_LAST
        };

        enum
        {
            FOOTSWITCH_1, /** Top */
            FOOTSWITCH_2, /** Bottom */
            FOOTSWITCH_LAST
        };

        Petal125BSM() {}
        ~Petal125BSM() {}

        /** @brief Initializes the submodule hardware
         *  This includes SDRAM, External Flash,
         *  Knobs, toggles, etc. etc.
         */
        void Init();

        /** @brief Begins the audio.
         *  the specified Interleaving callback will get called whenever
         *  new data is ready to be prepared.
         */
        void StartAudio(AudioHandle::InterleavingAudioCallback cb);

        /** @brief Begins the audio.
         *  the specified Non-Interleaving callback will get called whenever
         *  new data is ready to be prepared.
         */
        void StartAudio(AudioHandle::AudioCallback cb);

        /** @brief Changes to a new interleaved callback */
        void ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb);

        /** @brief Changes to a new non-interleaving callback */
        void ChangeAudioCallback(AudioHandle::AudioCallback cb);

        /** @brief Stops audio stream if its running, disabling the SAI DMA */
        void StopAudio();

        /** @brief Updates the Audio Sample Rate, and reinitializes.
         *  @note Audio must be stopped for this to work.
         */
        void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate);

        /** @brief Returns the audio sample rate in Hz as a floating point number. */
        float AudioSampleRate();

        /** @brief Sets the number of samples processed per channel by the audio callback. */
        void SetAudioBlockSize(size_t blocksize);

        /** @brief Returns the number of samples per channel in a block of audio. */
        size_t AudioBlockSize();

        /** @brief Returns the rate in Hz that the Audio callback is called */
        float AudioCallbackRate();

        /** @brief Processes Analog Controls (all 6 knobs, and expression)  */
        void ProcessAnalogControls();

        /** @brief Processes Digital buttons, debouncing as necessary. */
        void ProcessDigitalControls();

        /** @brief Processes all controls */
        inline void ProcessAllControls()
        {
            ProcessAnalogControls();
            ProcessDigitalControls();
        }

        /** @brief Returns the value of a specific knob.
         *  @param index number representing which knob to use 0-5
         *  @return 0-1 value corresponding to knob position
         */
        inline float GetKnobValue(int index) const
        {
            return knob[index < KNOB_LAST ? index : 0].Value();
        }

        /** @brief Returns the value of Knob 1
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob1() const { return knob[0].Value(); }

        /** @brief Returns the value of Knob 2
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob2() const { return knob[1].Value(); }

        /** @brief Returns the value of Knob 3
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob3() const { return knob[2].Value(); }

        /** @brief Returns the value of Knob 4
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob4() const { return knob[3].Value(); }

        /** @brief Returns the value of Knob 5
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob5() const { return knob[4].Value(); }

        /** @brief Returns the value of Knob 6
         *  @return 0-1 value corresponding to the knob position.
         */
        inline float GetKnob6() const { return knob[5].Value(); }

        /** @brief Returns the current expression value
         *  @return 0-1 value corresponding to the expression input
         */
        inline float GetExpressionValue() const { return expression.Value(); }

        /** @brief Enables/Disables True Bypass */
        void SetBypassState(bool state);

        /** @brief Toggles the state of the True Bypass feature */
        void ToggleBypassState();

        /** @brief Returns whether True Bypass is enabled or not. */
        inline bool BypassState() const { return bypass_state_; }

        /** @brief Updates the PWM value for any direct GPIO LEDs
         *
         *  @note This should be called at AudioCallbackRate
         *
         *  For setting individual LED brightnesses you can access the
         *  leds[] array, and use the `Set` function.
         *
         *  For example:
         *  hw.leds[LED_2].Set(0.5);
         *
         *  The RGB LEDs each have convenience functions below.
         */
        void UpdateLeds();

        /** @brief Convenience function for setting the RGB values for LED 1
         *  @param r 0-1 value for red LED
         *  @param g 0-1 value for green LED
         *  @param b 0-1 value for blue LED
         */
        void SetLed1(float r, float g, float b);

        /** @brief Convenience function for Setting LED2
         *  @param val 0-1 value for brightness of LED.
         */
        inline void SetLed2(float val) { led[LED_2].Set(val); }

        /** @brief Convenience function for setting the RGB values for LED 3
         *  @param r 0-1 value for red LED
         *  @param g 0-1 value for green LED
         *  @param b 0-1 value for blue LED
         */
        void SetLed3(float r, float g, float b);

        /** @brief Convenience function for Setting LED2
         *  @param val 0-1 value for brightness of LED.
         */
        inline void SetLed4(float val) { led[LED_4].Set(val); }

        /** @brief Convenience function for Setting LED2
         *  @param val 0-1 value for brightness of LED.
         */
        inline void SetLed5(float val) { led[LED_5].Set(val); }

        /** @brief Print formatted debug log message */
        template <typename... VA>
        static void Print(const char *format, VA... va)
        {
            Log::Print(format, va...);
        }

        /** @brief Print formatted debug log message with automatic line termination */
        template <typename... VA>
        static void PrintLine(const char *format, VA... va)
        {
            Log::PrintLine(format, va...);
        }

        /** @brief Start the logging session.
         *  @param wait_for_pc Optionally wait for terminal connection before proceeding.
         *                      defaults to not waiting
         */
        static void StartLog(bool wait_for_pc = false)
        {
            Log::StartLog(wait_for_pc);
        }

        /** @brief Tests entirety of SDRAM for validity
         *         This will wipe contents of SDRAM when testing.
         *
         *  @note   If using the SDRAM for the default bss, or heap,
         *          and using constructors as initializers do not
         *          call this function. Otherwise, it could
         *          overwrite changes performed by constructors.
         *
         *  \retval returns true if SDRAM is okay, otherwise false
         */
        bool ValidateSDRAM();

        /** @brief Tests the QSPI for validity
         *         This will wipe contents of QSPI when testing.
         *
         *  @note  If called with quick = false, this will erase all memory
         *         the "quick" test starts 0x400000 bytes into the memory and
         *         test 16kB of data
         *
         *  \param quick if this is true the test will only test a small piece of the QSPI
         *               checking the entire 8MB can take roughly over a minute.
         *
         *  \retval returns true if SDRAM is okay, otherwise false
         */
        bool ValidateQSPI(bool quick = true);

        AnalogControl knob[KNOB_LAST];   /**< Knob Objects */
        AnalogControl expression;        /**< Expresion Object */
        Switch3 toggle[TOGGLE_LAST];     /**< Toggle Objects */
        Switch footswitch1, footswitch2; /**< Footswitch Objects */

        /** LED Objects -- RGBs are handled as separate LEDs
         *  @see SetLed1
         *  @see SetLed3
         */
        Led led[LED_LAST];

        QSPIHandle qspi;   /**< External Flash Access */
        AudioHandle audio; /**< Audio Engine Access */
        AdcHandle adc;     /**< Analog to Digital Converter Access */
        Pcm3060 codec;     /**< PCM3060 codec control */

    private:
        using Log = Logger<LOGGER_INTERNAL>;
        float callback_rate_;
        GPIO relay_control;
        bool bypass_state_;
        System sys;
        SdramHandle sdram;
    };

} // namespace daisy

#endif
