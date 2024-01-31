#ifndef GENLIB_DAISY_H
#define GENLIB_DAISY_H

/*
Oopsy was authored in 2020-2021 by Graham Wakefield.  Copyright 2021 Electrosmith, Corp. and Graham Wakefield.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "daisy.h"
#include "genlib.h"
#include "genlib_ops.h"
#include "genlib_exportfunctions.h"
#include "daisy_seed.h"

#include <math.h>
#include <string>
#include <cstring> // memset
#include <stdarg.h> // vprintf

// #if defined(OOPSY_TARGET_SEED)
// 	typedef struct {
// 		daisy::DaisySeed seed;

// 		void Init() {
// 			seed.Configure();
// 			seed.Init();
// 		}
// 	} Daisy;
// #endif

#ifdef OOPSY_USE_USB_SERIAL_INPUT
static char   sumbuff[1024];
static uint32_t  rx_size = 0;
static bool      update = false;
#endif

// A temproary measure to preserve Field compatibility
#ifdef OOPSY_TARGET_FIELD
#include "daisy_field.h"
#endif

#ifdef OOPSY_TARGET_PETAL
#include "petal_led_hardcode.h"
#endif

////////////////////////// DAISY EXPORT INTERFACING //////////////////////////

#define OOPSY_MIDI_BUFFER_SIZE (1024)
#define OOPSY_LONG_PRESS_MS (333)
#define OOPSY_SUPER_LONG_PRESS_MS (20000)
#define OOPSY_DISPLAY_PERIOD_MS 10
#define OOPSY_SCOPE_MAX_ZOOM (8)
static const uint32_t OOPSY_SRAM_SIZE = 512 * 1024;
static const uint32_t OOPSY_SDRAM_SIZE = 64 * 1024 * 1024;

// Added dedicated global SDFile to replace old global from libDaisy
FIL SDFile;

namespace oopsy {

	uint32_t sram_used = 0, sram_usable = 0;
	uint32_t sdram_used = 0, sdram_usable = 0;
	char * sram_pool = nullptr;
	char DSY_SDRAM_BSS sdram_pool[OOPSY_SDRAM_SIZE];

	void init() {
		if (!sram_pool) sram_pool = (char *)malloc(OOPSY_SRAM_SIZE);
		// There's no guarantee the allocation will actually be
		// of size "OOPSY_SRAM_SIZE," so this just clamps the
		// usable space to what it really is.
		sram_usable = (0x24080000 - 1024) - ((size_t) sram_pool);
		sram_used = 0;
		sdram_usable = OOPSY_SDRAM_SIZE;
		sdram_used = 0;
	}

	void * allocate(uint32_t size) {
		if (size < sram_usable) {
			void * p = sram_pool + sram_used;
			sram_used += size;
			sram_usable -= size;
			return p;
		} else if (size < sdram_usable) {
			void * p = sdram_pool + sdram_used;
			sdram_used += size;
			sdram_usable -= size;
			return p;
		}
		return nullptr;
	}	

	void memset(void *p, int c, long size) {
		char *p2 = (char *)p;
		int i;
		for (i = 0; i < size; i++, p2++) *p2 = char(c);
	}

	// void genlib_memcpy(void *dst, const void *src, long size) {
	// 	char *s2 = (char *)src;
	// 	char *d2 = (char *)dst;
	// 	int i;
	// 	for (i = 0; i < size; i++, s2++, d2++)
	// 		*d2 = *s2;
	// }

	// void test() {
	// 	// memory test:
	// 	size_t allocated = 0;
	// 	size_t sz = 256;
	// 	int i;
	// 	while (sz < 515) {
	// 		sz++;
	// 		void * m = malloc(sz * 1024);
	// 		if (!m) break;
	// 		free(m);
	// 		log("%d: malloced %dk", i, sz);
	// 		i++;
	// 	}
	// 	log("all OK");
	// }

	struct Timer {
		int32_t period = OOPSY_DISPLAY_PERIOD_MS, 
				t = OOPSY_DISPLAY_PERIOD_MS;

		bool ready(int32_t dt) {
			t += dt;
			if (t > period) {
				t = 0;
				return true;
			}
			return false;
		}
	};

	struct AppDef {
		const char * name;
		void (*load)();
	};
	typedef enum {
		#ifdef OOPSY_TARGET_HAS_OLED
			MODE_SCOPE,
			#ifdef OOPSY_HAS_PARAM_VIEW
				MODE_PARAMS,
			#endif
			MODE_CONSOLE,
		#endif
		#ifdef OOPSY_MULTI_APP
			MODE_MENU,
		#endif
		MODE_COUNT
	} Mode;

	

	struct GenDaisy {

		Daisy hardware;
		#ifdef OOPSY_SOM_PETAL_SM
		daisy::Petal125BSM *som = &hardware.som;
		#else
			#ifdef OOPSY_SOM_PATCH_SM
			daisy::patch_sm::DaisyPatchSM *som = &hardware.som;
			#else
				#ifdef OOPSY_OLD_JSON
				daisy::DaisySeed *som = &hardware.seed;
				#else
				daisy::DaisySeed *som = &hardware.som;
				#endif
			#endif
		#endif
		AppDef * appdefs = nullptr;

		int mode, screensave=0;
		int app_count = 1, app_selected = 0, app_selecting = 0, app_load_scheduled = 0;
		int /*menu_button_held = 0, */menu_button_released = 0, menu_button_held_ms = 0, menu_button_incr = 0;
		int is_mode_selecting = 0;
		int param_count = 0;
		#ifdef OOPSY_HAS_PARAM_VIEW
		int param_selected = 0, param_is_tweaking = 0, param_scroll = 0;
		#endif

		uint32_t t = 0, dt = 10, blockcount = 0;
		Timer uitimer;

		// percent (0-100) of available processing time used
		float audioCpuUsage = 0; 

		void (*mainloopCallback)(uint32_t t, uint32_t dt);
		void (*displayCallback)(uint32_t t, uint32_t dt);
		#ifdef OOPSY_HAS_PARAM_VIEW
		void (*paramCallback)(int idx, char * label, int len, bool tweak);
		#endif
		void * app = nullptr;
		void * gen = nullptr;
		bool nullAudioCallbackRunning = false;
		
		#ifdef OOPSY_TARGET_HAS_OLED

		enum {
			SCOPESTYLE_OVERLAY = 0,
			SCOPESTYLE_TOPBOTTOM,
			SCOPESTYLE_LEFTRIGHT,
			SCOPESTYLE_LISSAJOUS,
			SCOPESTYLE_COUNT
		} ScopeStyles;

		enum {
			SCOPEOPTION_STYLE = 0,
			SCOPEOPTION_SOURCE,
			SCOPEOPTION_ZOOM,
			SCOPEOPTION_COUNT
		} ScopeOptions;
		
		FontDef& font = Font_6x8;
		uint_fast8_t scope_zoom = 7; 
		uint_fast8_t scope_step = 0; 
		uint_fast8_t scope_option = 0, scope_style = SCOPESTYLE_TOPBOTTOM, scope_source = OOPSY_IO_COUNT/2;
		uint16_t console_cols, console_rows, console_line;
		char * console_stats;
		char * console_memory;
		char ** console_lines;
		float scope_data[OOPSY_OLED_DISPLAY_WIDTH*2][2]; // 128 pixels
		char scope_label[11];
		#endif // OOPSY_TARGET_HAS_OLED

		#ifdef OOPSY_TARGET_USES_MIDI_UART

		struct MidiNote {
			uint8_t chan, pitch, vel, press;

			void init() {
				chan = 0;
				pitch = 36;
				vel = press = 0;
			}

			// call at block rate
			// (so long as at least velocity out is defined)
			void update(GenDaisy& daisy, uint8_t v, uint8_t p=36, uint8_t c=0) {
				// a change of pitch or chan must stop an ongoing note
				if (vel && (p != pitch || c != chan)) {
					// send note off to stop old note
					vel = 0;
					daisy.midi_message3(144 + chan, pitch, vel);
				}
				pitch = p;
				chan = c;
				// a change of velocity between zero and nonzero should trigger a note on/off
				if ((!v) != (!vel)) {
					daisy.midi_message3(144 + chan, pitch, v);
				}
				vel = v;
			}

			// call in the throttled section (if a pressure output was defined)
			void update_pressure(GenDaisy& daisy, uint8_t pressure) {
				if (vel && pressure != press) {
					// send pressure
					daisy.midi_message3(160 + chan, pitch, pressure);
				}
				press = pressure;
			}
		};

		struct {
			uint8_t status=0;
			uint8_t lastbyte=0;
			uint8_t byte[2];
		} midi;

		daisy::UartHandler uart;
		uint32_t midi_out_writeidx = 0;
		uint32_t midi_out_readidx = 0;

		uint8_t midi_in_written = 0;//, midi_out_written = 0;
		uint8_t midi_in_active = 0, midi_out_active = 0;
		uint8_t midi_out_data[OOPSY_MIDI_BUFFER_SIZE];
		float midi_in_data[OOPSY_BLOCK_SIZE];
		int midi_data_idx = 0;
		int midi_parse_state = 0;
		#endif //OOPSY_TARGET_USES_MIDI_UART

		#ifdef OOPSY_TARGET_USES_SDMMC
		struct WavFormatChunk {
			uint32_t size; 			// 16
			uint16_t format;   		// 1=PCM
			uint16_t chans;   		// e.g. 2
			uint32_t samplerate;    // e.g. 44100
			uint32_t bytespersecond;      // bytes per second, = sr * bitspersample * chans/8
			uint16_t bytesperframe;    // bytes per frame, = bitspersample * chans/8
			uint16_t bitspersample;  // e.g. 16 for 16-bit
		};

		// at minimum this should fit one frame of 4 bytes-per-sample x numchans
		#define OOPSY_WAV_WORKSPACE_BYTES (256)

		daisy::SdmmcHandler handler;
		daisy::FatFSInterface fsi;

		uint8_t workspace[OOPSY_WAV_WORKSPACE_BYTES];
		
		void sdcard_init() {
			daisy::SdmmcHandler::Config sdconfig;
			sdconfig.Defaults(); // 4-bit, 50MHz
			// sdconfig.clock_powersave = false;
			// sdconfig.speed           = daisy::SdmmcHandler::Speed::FAST;
			sdconfig.width           = daisy::SdmmcHandler::BusWidth::BITS_1;
			handler.Init(sdconfig);
			fsi.Init(daisy::FatFSInterface::Config::MEDIA_SD);
			f_mount(&fsi.GetSDFileSystem(), fsi.GetSDPath(), 1);
		}

		// TODO: resizing without wasting memory
		int sdcard_load_wav(const char * filename, Data& gendata) {
			float * buffer = gendata.mData;
			size_t buffer_frames = gendata.dim;
			size_t buffer_channels = gendata.channels;
			size_t bytesread = 0;
			WavFormatChunk format;
			uint32_t header[3];
			uint32_t marker, frames, chunksize, frames_per_read, frames_to_read, frames_read, total_frames_to_read;
			uint32_t buffer_index = 0;
			size_t bytespersample;
			if(f_open(&SDFile, filename, (FA_OPEN_EXISTING | FA_READ)) != FR_OK) {
				log("no %s", filename);
				return -1;
			}
			if (f_eof(&SDFile) 
				|| f_read(&SDFile, (void *)&header, sizeof(header), &bytesread) != FR_OK
				|| header[0] != daisy::kWavFileChunkId 
				|| header[2] != daisy::kWavFileWaveId) goto badwav;
			// find the format chunk:
			do {
				if (f_eof(&SDFile) || f_read(&SDFile, (void *)&marker, sizeof(marker), &bytesread) != FR_OK) break;
			} while (marker != daisy::kWavFileSubChunk1Id);
			if (f_eof(&SDFile) 
				|| f_read(&SDFile, (void *)&format, sizeof(format), &bytesread) != FR_OK
				|| format.chans == 0 
				|| format.samplerate == 0 
				|| format.bitspersample == 0) goto badwav;
			// find the data chunk:
			do {
				if (f_eof(&SDFile) || f_read(&SDFile, (void *)&marker, sizeof(marker), &bytesread) != FR_OK) break;
			} while (marker != daisy::kWavFileSubChunk2Id);
			bytespersample = format.bytesperframe / format.chans;
			if (f_eof(&SDFile) 
				|| f_read(&SDFile, (void *)&chunksize, sizeof(chunksize), &bytesread) != FR_OK
				|| format.format != 1 
				|| bytespersample < 2 
				|| bytespersample > 4) goto badwav; // only 16/24/32-bit PCM, sorry
			// make sure we read in (multiples of) whole frames
			frames = chunksize / format.bytesperframe;
			frames_per_read = OOPSY_WAV_WORKSPACE_BYTES / format.bytesperframe;
			frames_to_read = frames_per_read;
			total_frames_to_read = buffer_frames;
			// log("b=%u c=%u t=%u", buffer_frames, buffer_channels, total_frames_to_read);
			// log("f=%u c=%u p=%u", frames, format.chans, frames_per_read);
			// log("bp=%u c=%u p=%u", frames_to_read * format.bytesperframe);
			do {
				if (frames_to_read > total_frames_to_read) frames_to_read = total_frames_to_read;
				f_read(&SDFile, workspace, frames_to_read * format.bytesperframe, &bytesread);
				frames_read = bytesread / format.bytesperframe;
				//log("_r=%u t=%u", frames_read, frames_to_read);
				switch (bytespersample) {
					case 2:  { // 16 bit
						for (size_t f=0; f<frames_read; f++) {
							for (size_t c=0; c<buffer_channels; c++) {
								uint8_t * frame = workspace + f*format.bytesperframe + (c % format.chans)*bytespersample;
								buffer[(buffer_index+f)*buffer_channels + c] = ((int16_t *)frame)[0] * 0.000030517578125f;
							}
						}
					} break;
					case 3: { // 24 bit
						for (size_t f=0; f<frames_read; f++) {
							for (size_t c=0; c<buffer_channels; c++) {
								uint8_t * frame = workspace + f*format.bytesperframe + (c % format.chans)*bytespersample;
								int32_t b = (int32_t)(
									((uint32_t)(frame[0]) <<  8) | 
									((uint32_t)(frame[1]) << 16) | 
									((uint32_t)(frame[2]) << 24)
								) >> 8;
								buffer[(buffer_index+f)*buffer_channels + c] = (float)(((double)b) * 0.00000011920928955078125);
							}
						}
					} break;
					case 4: { // 32 bit
						for (size_t f=0; f<frames_read; f++) {
							for (size_t c=0; c<buffer_channels; c++) {
								uint8_t * frame = workspace + f*format.bytesperframe + (c % format.chans)*bytespersample;
								buffer[(buffer_index+f)*buffer_channels + c] = ((int32_t *)frame)[0] / 2147483648.f;
							}
						}
					} break;
				}
				total_frames_to_read -= frames_read;
				buffer_index += frames_read;
			} while (!f_eof(&SDFile) && bytesread > 0 && total_frames_to_read > 0);
			f_close(&SDFile);
			log("read %s", filename);
			return buffer_index;
		badwav:
			f_close(&SDFile);
			log("bad %s", filename);
			return -1;
		}
		#endif

		template<typename A>
		void reset(A& newapp) {
			// first, remove callbacks:
			mainloopCallback = nullMainloopCallback;
			displayCallback = nullMainloopCallback;
			nullAudioCallbackRunning = false;
			som->ChangeAudioCallback(nullAudioCallback);
			while (!nullAudioCallbackRunning) daisy::System::Delay(1);
			// reset memory
			oopsy::init();
			// install new app:
			app = &newapp;
			newapp.init(*this);
			// install new callbacks:
			mainloopCallback = newapp.staticMainloopCallback;
			displayCallback = newapp.staticDisplayCallback;
			#if defined(OOPSY_TARGET_HAS_OLED) && defined(OOPSY_HAS_PARAM_VIEW)
			paramCallback = newapp.staticParamCallback;
			#endif

			som->ChangeAudioCallback(newapp.staticAudioCallback);
			log("gen~ %s", appdefs[app_selected].name);
			log("SR %dkHz / %dHz", (int)(som->AudioSampleRate()/1000), (int)som->AudioCallbackRate());
			{
				log("%d%s/%dKB+%d%s/%dMB", 
					oopsy::sram_used > 1024 ? oopsy::sram_used/1024 : oopsy::sram_used, 
					(oopsy::sram_used > 1024 || oopsy::sram_used == 0) ? "" : "B", 
					OOPSY_SRAM_SIZE/1024, 
					oopsy::sdram_used > 1048576 ? oopsy::sdram_used/1048576 : oopsy::sdram_used/1024, 
					(oopsy::sdram_used > 1048576 || oopsy::sdram_used == 0) ? "" : "KB", 
					OOPSY_SDRAM_SIZE/1048576);
				// console_display();
				// hardware.display.Update();
			}

			// reset some state:
			menu_button_incr = 0;
			#if defined(OOPSY_TARGET_SEED)
			hardware.menu_rotate = 0;
			#endif
			#ifdef OOPSY_TARGET_USES_MIDI_UART
			midi_out_writeidx = 0;
			midi_out_readidx = 0;
			midi_data_idx = 0;
			midi_in_written = 0;//, midi_out_written = 0;
			midi_in_active = 0, midi_out_active = 0;
			// reset:
			midi_message1(255);
			midi_message3(176, 123, 0);
			#endif
			blockcount = 0;
		}

		#ifdef OOPSY_USE_USB_SERIAL_INPUT
		static void UsbCallback(uint8_t* buf, uint32_t* len) {
			memcpy(sumbuff, buf, *len);
			rx_size = *len;
			update  = true;
		}
		#endif

		int run(AppDef * appdefs, int count) {
			this->appdefs = appdefs;
			app_count = count;
			mode = 0;

			#ifdef OOPSY_USE_USB_SERIAL_INPUT
				som->usb.Init(daisy::UsbHandle::FS_INTERNAL);
				daisy::System::Delay(500);
				som->usb.SetReceiveCallback(UsbCallback, daisy::UsbHandle::FS_INTERNAL);
			#endif

			#ifdef OOPSY_USE_LOGGING
			daisy::Logger<daisy::LOGGER_INTERNAL>::StartLog(false);

			//usbhandle SetReceiveCallback(ReceiveCallback cb, UsbPeriph dev);

			// TODO REMOVE THIS HACK WHEN STARTING SERIAL OVER USB DOESN'T FREAK OUT WITH AUDIO CALLBACK
			daisy::System::Delay(275);
			#endif
			
			#ifdef OOPSY_TARGET_HAS_OLED
			console_cols = OOPSY_OLED_DISPLAY_WIDTH / font.FontWidth + 1; // +1 to accommodate null terminators.
			console_rows = OOPSY_OLED_DISPLAY_HEIGHT / font.FontHeight; 
			console_memory = (char *)calloc(console_cols, console_rows);
			console_stats = (char *)calloc(console_cols, 1);
			for (int i=0; i<console_rows; i++) {
				console_lines[i] = &console_memory[i*console_cols];
			}
			console_line = console_rows-1;
			#endif

			som->StartAudio(nullAudioCallback);
			mainloopCallback = nullMainloopCallback;
			displayCallback = nullMainloopCallback;

			#ifdef OOPSY_TARGET_USES_SDMMC
			sdcard_init();
			#endif

			#ifdef OOPSY_TARGET_USES_MIDI_UART
			midi_out_writeidx = 0;
			midi_out_readidx = 0;
			midi_data_idx = 0;
			midi_in_written = 0;//, midi_out_written = 0;
			midi_in_active = 0, midi_out_active = 0;
			daisy::UartHandler::Config config;
			config.baudrate      = 31250;
			config.periph        = daisy::UartHandler::Config::Peripheral::USART_1;
			config.stopbits      = daisy::UartHandler::Config::StopBits::BITS_1;
			config.parity        = daisy::UartHandler::Config::Parity::NONE;
			config.mode          = daisy::UartHandler::Config::Mode::TX_RX;
			config.wordlength    = daisy::UartHandler::Config::WordLength::BITS_8;
			config.pin_config.rx = {DSY_GPIOB, 7};
			config.pin_config.tx = {DSY_GPIOB, 6};
			uart.Init(config);
			uart.StartRx();
			#endif

			app_selected = 0;
			appdefs[app_selected].load();

			#ifdef OOPSY_TARGET_HAS_OLED
			console_display();
			#endif 

			while(1) {
				uint32_t t1 = daisy::System::GetNow();
				dt = t1-t;
				t = t1;

				// pulse seed LED for status according to CPU usage:
				#ifndef OOPSY_SOM_PETAL_SM
				som->SetLed((t % 1000)/10 <= uint32_t(audioCpuUsage));
				#endif

				if (app_load_scheduled) {
					app_load_scheduled = 0;
					appdefs[app_selected].load();
					continue;
				}
				
				// handle app-level code (e.g. for CV/gate outs)
				mainloopCallback(t, dt);
				#ifdef OOPSY_TARGET_USES_MIDI_UART
				// send data if there's something to read:
				if (midi_out_readidx != midi_out_writeidx) {
					uint32_t size = (OOPSY_MIDI_BUFFER_SIZE + midi_out_writeidx - midi_out_readidx) % OOPSY_MIDI_BUFFER_SIZE;
					size = ((midi_out_readidx + size) <= OOPSY_MIDI_BUFFER_SIZE) ? size : OOPSY_MIDI_BUFFER_SIZE - midi_out_readidx;
					//for (uint32_t i=0; i<size; i++) log("midi %d", midi_out_data[midi_out_readidx+i]);
					daisy::UartHandler::Result res = uart.PollTx(midi_out_data + midi_out_readidx, size);
					if (res == daisy::UartHandler::Result::OK) {
						midi_out_active = 1;
						midi_out_readidx = (midi_out_readidx + size) % OOPSY_MIDI_BUFFER_SIZE;
					} else {
						log("MIDI transmit err %d", res);
					}
				}
				#endif
				
				if (uitimer.ready(dt)) {
					#ifdef OOPSY_USE_LOGGING
						som->PrintLine("the time is"FLT_FMT3"", FLT_VAR3(t/1000.f));
					#endif
					#ifdef OOPSY_USE_USB_SERIAL_INPUT
					if(update && rx_size > 0) {
						// TODO check bytes for a reset message and jump to bootloader
						update = false;
						log(sumbuff);
					}
					#endif

					// CLEAR DISPLAY
					#ifdef OOPSY_TARGET_HAS_OLED
					hardware.display.Fill(false);
					#endif
					#ifdef OOPSY_TARGET_PETAL 
					hardware.led_driver.SetAllTo((uint8_t) 0);
					#endif

					if (menu_button_held_ms > OOPSY_LONG_PRESS_MS) {
						is_mode_selecting = 1;
					} 
					#ifdef OOPSY_TARGET_PETAL
					// has no mode selection
					is_mode_selecting = 0;
					#if defined(OOPSY_MULTI_APP)
					// multi-app is always in menu mode:
					mode = MODE_MENU;
					#endif
					for(int i = 0; i < 8; i++) {
						float white = (i == app_selecting || menu_button_released);

						SetRingLed(&hardware.led_driver, (daisy::DaisyPetal::RingLed)i, 
							(i == app_selected || white) * 1.f,
							white * 1.f,
							(i < app_count) * 0.3f + white * 1.f
						);
					}
					#endif //OOPSY_TARGET_PETAL

					// #ifdef OOPSY_TARGET_VERSIO
					// // has no mode selection
					// is_mode_selecting = 0;
					// #if defined(OOPSY_MULTI_APP)
					// // multi-app is always in menu mode:
					// mode = MODE_MENU;
					// #endif
					// for(int i = 0; i < 4; i++) {
					// 	float white = (i == app_selecting || menu_button_released);
					// 	hardware.SetLed(i, 
					// 		(i == app_selected || white) * 1.f,
					// 		white * 1.f,
					// 		(i < app_count) * 0.3f + white * 1.f
					// 	);
					// }
					// #endif //OOPSY_TARGET_VERSIO

					// Handle encoder increment actions:
					if (is_mode_selecting) {
						mode += menu_button_incr;
						#ifdef OOPSY_TARGET_FIELD
						// mode menu rotates infinitely
						if (mode >= MODE_COUNT) mode = 0;
						if (mode < 0) mode = MODE_COUNT-1;
						#else
						// mode menu clamps at either end
						if (mode >= MODE_COUNT) mode = MODE_COUNT-1; 
						if (mode < 0) mode = 0;
						#endif	
					#ifdef OOPSY_MULTI_APP
					} else if (mode == MODE_MENU) {
						#ifdef OOPSY_TARGET_VERSIO
						app_selecting = menu_button_incr;
						#else
						app_selecting += menu_button_incr;
						#endif
						if (app_selecting >= app_count) app_selecting -= app_count;
						if (app_selecting < 0) app_selecting += app_count;
					#endif // OOPSY_MULTI_APP
					#ifdef OOPSY_TARGET_HAS_OLED
					} else if (mode == MODE_SCOPE) {
						switch (scope_option) {
							case SCOPEOPTION_STYLE: {
								scope_style = (scope_style + menu_button_incr) % SCOPESTYLE_COUNT;
							} break;
							case SCOPEOPTION_SOURCE: {
								scope_source = (scope_source + menu_button_incr) % (OOPSY_IO_COUNT*2);
							} break;
							case SCOPEOPTION_ZOOM: {
								scope_zoom = (scope_zoom + menu_button_incr) % OOPSY_SCOPE_MAX_ZOOM;
							} break;
						}
					#ifdef OOPSY_HAS_PARAM_VIEW
					} else if (mode == MODE_PARAMS) {
						if (!param_is_tweaking) {
							param_selected += menu_button_incr;
							if (param_selected >= param_count) param_selected = 0;
							if (param_selected < 0) param_selected = param_count-1;
						} 
					#endif //OOPSY_HAS_PARAM_VIEW
					#endif //OOPSY_TARGET_HAS_OLED
					}
				
					// SHORT PRESS	
					if (menu_button_released) {
						menu_button_released = 0;
						if (is_mode_selecting) {
							is_mode_selecting = 0;
						#ifdef OOPSY_MULTI_APP
						} else if (mode == MODE_MENU) {
							if (app_selected != app_selecting) {
								app_selected = app_selecting;
								#ifndef OOPSY_TARGET_HAS_OLED
								mode = 0;
								#endif
								schedule_app_load(app_selected); //appdefs[app_selected].load();
								//continue;
							}
						#endif
						#ifdef OOPSY_TARGET_HAS_OLED
						} else if (mode == MODE_SCOPE) {
							scope_option = (scope_option + 1) % SCOPEOPTION_COUNT;
						#if defined (OOPSY_HAS_PARAM_VIEW) && defined(OOPSY_CAN_PARAM_TWEAK)
						} else if (mode == MODE_PARAMS) {
							param_is_tweaking = !param_is_tweaking;
						#endif //OOPSY_HAS_PARAM_VIEW && OOPSY_CAN_PARAM_TWEAK
						#endif //OOPSY_TARGET_HAS_OLED
						}
					} 

					// OLED DISPLAY:
					#ifdef OOPSY_TARGET_HAS_OLED
					int showstats = 0;
					switch(mode) {
						#ifdef OOPSY_MULTI_APP
						case MODE_MENU: {
							showstats = 1;
							for (int i=0; i<console_rows; i++) {
								if (i == app_selecting) {
									hardware.display.SetCursor(0, font.FontHeight * i);
									hardware.display.WriteString((char *)">", font, true);
								}
								if (i < app_count) {
									hardware.display.SetCursor(font.FontWidth, font.FontHeight * i);
									hardware.display.WriteString((char *)appdefs[i].name, font, i != app_selected);
								}
							}
						} break;
						#endif //OOPSY_MULTI_APP
						#ifdef OOPSY_HAS_PARAM_VIEW
						case MODE_PARAMS: {
							char label[console_cols+1];
							// ensure selected parameter is on-screen:
							if (param_scroll > param_selected) param_scroll = param_selected;
							if (param_scroll < (param_selected - console_rows + 1)) param_scroll = (param_selected - console_rows + 1);
							int idx = param_scroll; // offset this for screen-scroll
							for (int line=0; line<console_rows && idx < param_count; line++, idx++) {
								paramCallback(idx, label, console_cols, param_is_tweaking && idx == param_selected);
								hardware.display.SetCursor(0, font.FontHeight * line);
								hardware.display.WriteString(label, font, (param_selected != idx));	
							}
						} break;
						#endif // OOPSY_HAS_PARAM_VIEW
						case MODE_SCOPE: {
							showstats = 1;
							uint8_t h = OOPSY_OLED_DISPLAY_HEIGHT;
							uint8_t w2 = OOPSY_OLED_DISPLAY_WIDTH/2, w4 = OOPSY_OLED_DISPLAY_WIDTH/4;
							uint8_t h2 = h/2, h4 = h/4;
							size_t zoomlevel = scope_samples();
							hardware.display.Fill(false);

							// stereo views:
							switch (scope_style) {
							case SCOPESTYLE_OVERLAY: {
								// stereo overlay:
								for (uint_fast8_t i=0; i<OOPSY_OLED_DISPLAY_WIDTH; i++) {
									int j = i*2;
									hardware.display.DrawLine(i, (1.f-scope_data[j][0])*h2, i, (1.f-scope_data[j+1][0])*h2, 1);
									hardware.display.DrawLine(i, (1.f-scope_data[j][1])*h2, i, (1.f-scope_data[j+1][1])*h2, 1);
								}
							} break;
							case SCOPESTYLE_TOPBOTTOM:
							{
								// stereo top-bottom
								for (uint_fast8_t i=0; i<OOPSY_OLED_DISPLAY_WIDTH; i++) {
									int j = i*2;
									hardware.display.DrawLine(i, (1.f-scope_data[j][0])*h4, i, (1.f-scope_data[j+1][0])*h4, 1);
									hardware.display.DrawLine(i, (1.f-scope_data[j][1])*h4+h2, i, (1.f-scope_data[j+1][1])*h4+h2, 1);
								}
							} break;
							case SCOPESTYLE_LEFTRIGHT:
							{
								// stereo L/R:
								for (uint_fast8_t i=0; i<w2; i++) {
									int j = i*4;
									hardware.display.DrawLine(i, (1.f-scope_data[j][0])*h2, i, (1.f-scope_data[j+1][0])*h2, 1);
									hardware.display.DrawLine(i + w2, (1.f-scope_data[j][1])*h2, i + w2, (1.f-scope_data[j+1][1])*h2, 1);
								}
							} break;
							default:
							{
								for (uint_fast8_t i=0; i<OOPSY_OLED_DISPLAY_WIDTH; i++) {
									int j = i*2;
									hardware.display.DrawPixel(
										w2 + h2*scope_data[j][0],
										h2 + h2*scope_data[j][1],
										1
									);
								}

								// for (uint_fast8_t i=0; i<OOPSY_OLED_DISPLAY_WIDTH; i++) {
								// 	int j = i*2;
								// 	hardware.display.DrawLine(
								// 		w2 + h2*scope_data[j][0],
								// 		h2 + h2*scope_data[j][1],
								// 		w2 + h2*scope_data[j+1][0],
								// 		h2 + h2*scope_data[j+1][1],
								// 		1
								// 	);
								// }
							} break;
							} // switch

							// labelling:
							switch (scope_option) {
								case SCOPEOPTION_SOURCE: {
									hardware.display.SetCursor(0, h - font.FontHeight);
									switch(scope_source) {
									#if (OOPSY_IO_COUNT == 4)
										case 0: hardware.display.WriteString("in1  in2", font, true); break;
										case 1: hardware.display.WriteString("in3  in4", font, true); break;
										case 2: hardware.display.WriteString("out1 out2", font, true); break;
										case 3: hardware.display.WriteString("out3 out4", font, true); break;
										case 4: hardware.display.WriteString("in1  out1", font, true); break;
										case 5: hardware.display.WriteString("in2  out2", font, true); break;
										case 6: hardware.display.WriteString("in3  out3", font, true); break;
										case 7: hardware.display.WriteString("in4  out4", font, true); break;
									#else
										case 0: hardware.display.WriteString("in1  in2", font, true); break;
										case 1: hardware.display.WriteString("out1 out2", font, true); break;
										case 2: hardware.display.WriteString("in1  out1", font, true); break;
										case 3: hardware.display.WriteString("in2  out2", font, true); break;
									#endif
									}
								} break;
								case SCOPEOPTION_ZOOM: {
									// each pixel is zoom samples; zoom/samplerate seconds
									float scope_duration = OOPSY_OLED_DISPLAY_WIDTH*(1000.f*zoomlevel/som->AudioSampleRate());
									int offset = snprintf(scope_label, console_cols, "%dx %dms", zoomlevel, (int)ceilf(scope_duration));
									hardware.display.SetCursor(0, h - font.FontHeight);
									hardware.display.WriteString(scope_label, font, true);
								} break;
								// for view style, just leave it blank :-)
							}
						} break;
						case MODE_CONSOLE: 
						{
							showstats = 1;
							console_display(); 
							break;
						}
						default: {
						}
					}
					if (is_mode_selecting) {
						hardware.display.DrawRect(0, 0, OOPSY_OLED_DISPLAY_WIDTH-1, OOPSY_OLED_DISPLAY_HEIGHT-1, 1);
					} 
					if (showstats) {
						int offset = 0;
						#ifdef OOPSY_TARGET_USES_MIDI_UART
						offset += snprintf(console_stats+offset, console_cols-offset, "%c%c", midi_in_active ? '<' : ' ', midi_out_active ? '>' : ' ');
						midi_in_active = midi_out_active = 0;
						#endif
						offset += snprintf(console_stats+offset, console_cols-offset, "%02d%%", int(audioCpuUsage));
						// stats:
						hardware.display.SetCursor(OOPSY_OLED_DISPLAY_WIDTH - (offset) * font.FontWidth, font.FontHeight * 0);
						hardware.display.WriteString(console_stats, font, true);
					}
					#endif //OOPSY_TARGET_HAS_OLED
					menu_button_incr = 0;
					
					// handle app-level code (e.g. for LED etc.)
					displayCallback(t, dt);

					#ifdef OOPSY_TARGET_HAS_OLED
					hardware.display.Update();
					#endif //OOPSY_TARGET_HAS_OLED

					#if (OOPSY_TARGET_PETAL)
					hardware.led_driver.SwapBuffersAndTransmit();
					#endif //(OOPSY_TARGET_PETAL)
				} // uitimer.ready
				
			}
			return 0;
		}

		void schedule_app_load(int which) {
			app_selected = app_selecting = which % app_count;
			app_load_scheduled = 1;
		}

		void audio_preperform(size_t size) {
			#ifdef OOPSY_TARGET_USES_MIDI_UART
			// fill remainder of midi buffer with non-data:
			for (size_t i=midi_in_written; i<size; i++) midi_in_data[i] = -0.1f;
			// done with midi input:
			midi_in_written = 0;
			#endif

			hardware.ProcessAllControls();

			#if defined(OOPSY_TARGET_SEED)
			menu_button_incr += hardware.menu_rotate;
			menu_button_held_ms = hardware.menu_hold;
			if (hardware.menu_click) menu_button_released = hardware.menu_click;
			#elif defined(OOPSY_TARGET_FIELD)
			//menu_button_held = hardware.GetSwitch(0)->Pressed();
			menu_button_incr += hardware.sw2.FallingEdge();
			menu_button_held_ms = hardware.sw1.TimeHeldMs();
			if (hardware.sw1.FallingEdge()) menu_button_released = 1;
			#elif defined(OOPSY_TARGET_VERSIO)
            // menu_button_held = hardware.tap.Pressed();
			// menu_button_incr += hardware.GetKnobValue(6) * app_count;
			// menu_button_held_ms = hardware.tap.TimeHeldMs();
			// if (hardware.tap_.FallingEdge()) menu_button_released = 1;
			#elif defined(OOPSY_TARGET_POD) || defined(OOPSY_TARGET_PETAL) || defined(OOPSY_TARGET_PATCH)
            //menu_button_held = hardware.encoder.Pressed();
			menu_button_incr += hardware.encoder.Increment();
			menu_button_held_ms = hardware.encoder.TimeHeldMs();
			if (hardware.encoder.FallingEdge()) menu_button_released = 1;
			#endif
		}

		void audio_postperform(float **buffers, size_t size) {
			#ifdef OOPSY_TARGET_HAS_OLED
			if (mode == MODE_SCOPE) {
				// selector for scope storage source:
				// e.g. for OOPSY_IO_COUNT=4, inputs:outputs as 0123:4567 makes:
				// 01, 23, 45, 67  2n:2n+1  i1i2 i3i4 o1o2 o3o4
				// 04, 15, 26, 37  n:n+ch   i1o1 i2o2 i3o3 i4o4
				int n = scope_source % OOPSY_IO_COUNT;
				float * buf0 = (scope_source < OOPSY_IO_COUNT) ? buffers[2*n  ] : buffers[n   ]; 
				float * buf1 = (scope_source < OOPSY_IO_COUNT) ? buffers[2*n+1] : buffers[n+OOPSY_IO_COUNT];

				// float * buf0 = scope_source ? buffers[0] : buffers[2];
				// float * buf1 = scope_source ? buffers[1] : buffers[3];
				size_t samples = scope_samples();
				if (samples > size) samples=size;

				for (size_t i=0; i<size/samples; i++) {
					float min0 = 10.f, max0 = -10.f;
					float min1 = 10.f, max1 = -10.f;
					for (size_t j=0; j<samples; j++) {
						float pt0 = *buf0++;
						float pt1 = *buf1++;
						min0 = min0 > pt0 ? pt0 : min0;
						max0 = max0 < pt0 ? pt0 : max0;
						min1 = min1 > pt1 ? pt1 : min1;
						max1 = max1 < pt1 ? pt1 : max1;
					}
					scope_data[scope_step][0] = (min0);
					scope_data[scope_step][1] = (min1);
					scope_step++;
					scope_data[scope_step][0] = (max0); 
					scope_data[scope_step][1] = (max1);
					scope_step++;
					if (scope_step >= OOPSY_OLED_DISPLAY_WIDTH*2) scope_step = 0;
				}
			}
			#endif
			blockcount++;
		}

		#ifdef OOPSY_TARGET_HAS_OLED
		inline int scope_samples() {
			// valid zoom sizes: 1, 2, 3, 4, 6, 8, 12, 16, 24
			switch(scope_zoom) {
				case 1: case 2: case 3: case 4: return scope_zoom; break;
				case 5: return 6; break;
				case 6: return 12; break;
				case 7: return 16; break;
				default: return 24; break;
			}
		}

		GenDaisy& console_display() {
			for (int i=0; i<console_rows; i++) {
				hardware.display.SetCursor(0, font.FontHeight * i);
				hardware.display.WriteString(console_lines[(i+console_line) % console_rows], font, true);
			}
			return *this;
		}
		#endif // OOPSY_TARGET_HAS_OLED

		GenDaisy& log(const char * fmt, ...) {
			#ifdef OOPSY_TARGET_HAS_OLED
			va_list argptr;
			va_start(argptr, fmt);
			vsnprintf(console_lines[console_line], console_cols, fmt, argptr);
			va_end(argptr);
			console_line = (console_line + 1) % console_rows;
			#endif
			return *this;
		}

		#if OOPSY_TARGET_USES_MIDI_UART
		void midi_postperform(float * buf, size_t size) {
			size_t i = 0;
			int8_t byte = buf[i] * 256.0f;
			uint32_t w1 = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
			while (byte >= 0 && i < size && w1 != midi_out_readidx) {
				// scale (0.0, 1.0) back to (0, 255) for MIDI bytes
				midi_out_data[midi_out_writeidx] = byte;
				midi_out_writeidx = w1;
				w1 = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
				i++;
				byte = buf[i] * 256.0f;
			}
			// for (size_t i=0; i<size && midi_out_writeidx+1 < OOPSY_MIDI_BUFFER_SIZE; i++) {
			// 	if (buf[i] >= 0.f) {
			// 		// scale (0.0, 1.0) back to (0, 255) for MIDI bytes
			// 		midi_out_data[midi_out_written] = buf[i] * 256.0f;
			// 		midi_out_written++;
			// 	}
			// }
		}

		void midi_message1(uint8_t byte) {
			uint32_t r = (midi_out_readidx + OOPSY_MIDI_BUFFER_SIZE - midi_out_writeidx) % OOPSY_MIDI_BUFFER_SIZE;
			if (r > 0 && r <= 1) { log("midi buffer full"); return; }
			uint32_t i0 = midi_out_writeidx;
			uint32_t i1 = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
			midi_out_data[i0] = byte;
			midi_out_writeidx = i1;
		}

		void midi_message2(uint8_t status, uint8_t b1) {
			uint32_t r = (midi_out_readidx + OOPSY_MIDI_BUFFER_SIZE - midi_out_writeidx) % OOPSY_MIDI_BUFFER_SIZE;
			if (r > 0 && r <= 2) { log("midi buffer full"); return; }
			uint32_t i0 = midi_out_writeidx;
			uint32_t i1 = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
			uint32_t i2 = (midi_out_writeidx+2) % OOPSY_MIDI_BUFFER_SIZE;
			midi_out_data[i0] = status;
			midi_out_data[i1] = b1;
			midi_out_writeidx = i2;
		}

		void midi_message3(uint8_t status, uint8_t b1, uint8_t b2) {
			uint32_t r = (midi_out_readidx + OOPSY_MIDI_BUFFER_SIZE - midi_out_writeidx) % OOPSY_MIDI_BUFFER_SIZE;
			if (r > 0 && r <= 3) { log("midi buffer full"); return; }
			uint32_t i0 = midi_out_writeidx;
			uint32_t i1 = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
			uint32_t i2 = (midi_out_writeidx+2) % OOPSY_MIDI_BUFFER_SIZE;
			uint32_t i3 = (midi_out_writeidx+3) % OOPSY_MIDI_BUFFER_SIZE;
			midi_out_data[i0] = status;
			midi_out_data[i1] = b1;
			midi_out_data[i2] = b2;
			midi_out_writeidx = i3;
		}

		void midi_nullData(Data& data) {
			for (int i=0; i<data.dim; i++) {
				data.write(-1, i, 0);
			}
		}

		void midi_fromData(Data& data) {
			double b = data.read(midi_data_idx, 0);
			while (b >= 0. && midi_out_writeidx != midi_out_readidx) {
				// erase it from [data midi]
				data.write(-1, midi_data_idx, 0);
				// write it to our active outbuffer:
				midi_out_data[midi_out_writeidx] = b;
				midi_out_writeidx = (midi_out_writeidx+1) % OOPSY_MIDI_BUFFER_SIZE;
				// and advance one index in the [data midi]
				midi_data_idx++; if (midi_data_idx >= data.dim) midi_data_idx = 0;
				b = data.read(midi_data_idx, 0);
			}
		}
		#endif //OOPSY_TARGET_USES_MIDI_UART

		// TODO -- need better way to handle this to avoid hardcoding
		#if (OOPSY_TARGET_FIELD)
		void setFieldLedsFromData(Data& data) {
			for(long i = 0; i < daisy::DaisyField::LED_LAST && i < data.dim; i++) {
				// LED indices run A1..8, B8..1, Knob1..8
				// switch here to re-order the B8-1 to B1-8
				long idx=i;
				if (idx > 7 && idx < 16)
					idx = 23 - i;
				hardware.led_driver.SetLed(idx, data.read(i, 0));
			}
			// hardware.led_driver.SwapBuffersAndTransmit(); // now handled by hardware class
		};
		#endif

		static void nullAudioCallback(daisy::AudioHandle::InputBuffer ins, daisy::AudioHandle::OutputBuffer outs, size_t size);
		
		static void nullMainloopCallback(uint32_t t, uint32_t dt) {}
	} daisy;

	void GenDaisy::nullAudioCallback(daisy::AudioHandle::InputBuffer ins, daisy::AudioHandle::OutputBuffer outs, size_t size) {
		daisy.nullAudioCallbackRunning = true;
		// zero audio outs:
		for (int i=0; i<OOPSY_IO_COUNT; i++) {
			memset(outs[i], 0, sizeof(float)*size);
		}
	}


	// Curiously-recurring template to make App definitions simpler:
	template<typename T>
	struct App {
		
		static void staticMainloopCallback(uint32_t t, uint32_t dt) {
			T& self = *(T *)daisy.app;
			self.mainloopCallback(daisy, t, dt);
		}

		static void staticDisplayCallback(uint32_t t, uint32_t dt) {
			T& self = *(T *)daisy.app;
			self.displayCallback(daisy, t, dt);
		}

		static void staticAudioCallback(daisy::AudioHandle::InputBuffer hardware_ins, daisy::AudioHandle::OutputBuffer hardware_outs, size_t size) {
			uint32_t start = daisy::System::GetUs(); 
			daisy.audio_preperform(size);
			((T *)daisy.app)->audioCallback(daisy, hardware_ins, hardware_outs, size);
			#if (OOPSY_IO_COUNT == 4)
			float * buffers[] = {
				(float *)hardware_ins[0], (float *)hardware_ins[1], (float *)hardware_ins[2], (float *)hardware_ins[3], 
				hardware_outs[0], hardware_outs[1], hardware_outs[2], hardware_outs[3]};
			#else
			float * buffers[] = {(float *)hardware_ins[0], (float *)hardware_ins[1], hardware_outs[0], hardware_outs[1]};
			#endif
			daisy.audio_postperform(buffers, size);
			// convert elapsed time (us) to CPU percentage (0-100) of available processing time
			// 100 (%) * (0.000001 * used_us) * callbackrateHz
			float percent = (daisy::System::GetUs() - start)*0.0001f*daisy.som->AudioCallbackRate();
			percent = percent > 100.f ? 100.f : percent;
			// with a falling-only slew to capture spikes, since we care most about worst-case performance
			daisy.audioCpuUsage = (percent > daisy.audioCpuUsage) ? percent 
				: daisy.audioCpuUsage + 0.02f*(percent - daisy.audioCpuUsage);
		}

		#if defined(OOPSY_TARGET_HAS_OLED) && defined(OOPSY_HAS_PARAM_VIEW)
		static void staticParamCallback(int idx, char * label, int len, bool tweak) {
			T& self = *(T *)daisy.app;
			self.paramCallback(daisy, idx, label, len, tweak);
		}
		#endif //defined(OOPSY_TARGET_HAS_OLED) && defined(OOPSY_HAS_PARAM_VIEW)
	};

}; // oopsy::

void genlib_report_error(const char *s) { oopsy::daisy.log(s); }
void genlib_report_message(const char *s) { oopsy::daisy.log(s); }

unsigned long genlib_ticks() { 
	return 0; //daisy::System::GetTick(); 
}

t_ptr genlib_sysmem_newptr(t_ptr_size size) {
	return (t_ptr)oopsy::allocate(size);
}

t_ptr genlib_sysmem_newptrclear(t_ptr_size size) {
	t_ptr p = genlib_sysmem_newptr(size);
	if (p) oopsy::memset(p, 0, size);
	return p;
}


#endif //GENLIB_DAISY_H
