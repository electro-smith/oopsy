#ifndef GENLIB_DAISY_H
#define GENLIB_DAISY_H

#include "daisy.h"
#include "genlib.h"
#include "genlib_ops.h"
#include "genlib_exportfunctions.h"
#include <math.h>
#include <string>
#include <cstring> // memset
#include <stdarg.h> // vprintf

#if defined(OOPSY_TARGET_PATCH)
	#include "daisy_patch.h"
	#define OOPSY_IO_COUNT (4)
	typedef daisy::DaisyPatch Daisy;

#elif defined(OOPSY_TARGET_FIELD)
	#include "daisy_field.h"
	#define OOPSY_IO_COUNT (2)
	typedef daisy::DaisyField Daisy;

#elif defined(OOPSY_TARGET_PETAL)
	#include "daisy_petal.h"
	#define OOPSY_IO_COUNT (2)
	typedef daisy::DaisyPetal Daisy;

#elif defined(OOPSY_TARGET_POD)
	#include "daisy_pod.h"
	#define OOPSY_IO_COUNT (2)
	typedef daisy::DaisyPod Daisy;

#else 
	#include "daisy_seed.h"
	#define OOPSY_IO_COUNT (2)
	typedef daisy::DaisySeed Daisy;

#endif

////////////////////////// DAISY EXPORT INTERFACING //////////////////////////

#define OOPSY_BUFFER_SIZE 48
#define OOPSY_MIDI_BUFFER_SIZE 64
#define OOPSY_LONG_PRESS_MS 250
#define OOPSY_DISPLAY_PERIOD_MS 20
#define OOPSY_SCOPE_MAX_ZOOM (9)
static const uint32_t OOPSY_SRAM_SIZE = 512 * 1024; 
static const uint32_t OOPSY_SDRAM_SIZE = 64 * 1024 * 1024;

namespace oopsy {

	uint32_t sram_used = 0, sram_usable = 0;
	uint32_t sdram_used = 0, sdram_usable = 0;
	char * sram_pool = nullptr;
	char DSY_SDRAM_BSS sdram_pool[OOPSY_SDRAM_SIZE];

	void init() {
		if (!sram_pool) sram_pool = (char *)malloc(OOPSY_SRAM_SIZE);
		sram_usable = OOPSY_SRAM_SIZE;
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
		MODE_NONE = 0,
		#ifdef OOPSY_MULTI_APP
		MODE_MENU,
		#endif

		#ifdef OOPSY_TARGET_HAS_OLED
		MODE_CONSOLE,
		MODE_SCOPE,
		#endif

		MODE_COUNT
	} Mode;

	#ifdef OOPSY_TARGET_USES_MIDI_UART
	struct {
		daisy::UartHandler uart;
		uint8_t in_written = 0, out_written = 0;
		uint8_t in_active = 0, out_active = 0;
		uint8_t out_data[OOPSY_MIDI_BUFFER_SIZE];
		float in_data[OOPSY_BUFFER_SIZE];

		void init() {
			uart.Init(); 
			uart.StartRx();
		}

		void preperform(size_t size) {
			// fill remainder of midi buffer with non-data:
			for (size_t i=in_written; i<size; i++) in_data[i] = -0.1f;
			// done with midi input:
			in_written = 0;
		}

		void postperform(float * buf, size_t size) {
			for (size_t i=0; i<size && out_written < OOPSY_MIDI_BUFFER_SIZE; i++) {
				if (buf[i] >= 0.f) {
					// scale (0.0, 1.0) back to (0, 255) for MIDI bytes
					out_data[out_written++] = buf[i] * 256.0f;
					out_active = 1;
				}
			}
		}

		void mainloop() {
			// input:
			while(uart.Readable()) {
				uint8_t byte = uart.PopRx();
				if (in_written < OOPSY_BUFFER_SIZE) {
					// scale (0, 255) to (0.0, 1.0)
					// to protect hardware from accidental patching
					in_data[in_written] = byte / 256.0f;
					in_written++;
				}
				in_active = 1;
			}
			// output:
			if (out_written) {
				if (0 == uart.PollTx(out_data, out_written)) out_written = 0;
			}
		}
	} midi;
	#endif

	struct GenDaisy {

		Daisy hardware;
		AppDef * appdefs = nullptr;

		int mode, mode_default;
		int app_count = 1, app_selected = 0, app_selecting = 0;
		int encoder_held = 0, encoder_held_ms = 0, encoder_released = 0, encoder_incr = 0;
		int is_mode_selecting = 0;

		uint32_t t = 0, dt = 10;
		Timer uitimer;

		// microseconds spent in audio callback
		uint32_t audioCpuUs = 0; 

		float samplerate; // default 48014
		size_t blocksize; // default 48

		void (*mainloopCallback)(uint32_t t, uint32_t dt);
		void * app = nullptr;
		void * gen = nullptr;
		bool nullAudioCallbackRunning = false;
		
		#ifdef OOPSY_TARGET_HAS_OLED
		FontDef& font = Font_6x8;
		uint_fast8_t scope_zoom = 5; 
		uint_fast8_t scope_step = 0;
		uint16_t console_cols, console_rows, console_line;
		char * console_stats;
		char * console_memory;
		char ** console_lines;
		float scope_data[SSD1309_WIDTH*2]; // 128 pixels
		char scope_label[11];
		#endif // OOPSY_TARGET_HAS_OLED

		template<typename A>
		void reset(A& newapp) {
			// first, remove callbacks:
			mainloopCallback = nullMainloopCallback;
			nullAudioCallbackRunning = false;
			hardware.ChangeAudioCallback(nullAudioCallback);
			while (!nullAudioCallbackRunning) dsy_system_delay(10);
			// reset memory
			oopsy::init();
			// install new app:
			app = &newapp;
			newapp.init(*this);
			// install new callbacks:
			mainloopCallback = newapp.staticMainloopCallback;
			hardware.ChangeAudioCallback(newapp.staticAudioCallback);
			log("loaded %s", appdefs[app_selected].name);
			log("%d/%dK+%d/%dM", oopsy::sram_used/1024, OOPSY_SRAM_SIZE/1024, oopsy::sdram_used/1048576, OOPSY_SDRAM_SIZE/1048576);
		}

		int run(AppDef * appdefs, int count) {
			this->appdefs = appdefs;
			app_count = count;
			
			mode_default = (Mode)(MODE_COUNT-1);
			mode = mode_default;

			hardware.Init(); 
			samplerate = hardware.AudioSampleRate(); // default 48014
			blocksize = hardware.AudioBlockSize();  // default 48
			
			#ifdef OOPSY_TARGET_HAS_OLED
			console_cols = SSD1309_WIDTH / font.FontWidth + 1; // +1 to accommodate null terminators.
			console_rows = SSD1309_HEIGHT / font.FontHeight - 1; // leave one row free for stats
			console_memory = (char *)calloc(console_cols, console_rows);
			console_stats = (char *)calloc(console_cols, 1);
			for (int i=0; i<console_rows; i++) {
				console_lines[i] = &console_memory[i*console_cols];
			}
			console_line = console_rows-1;
			#endif

			hardware.StartAdc();
			hardware.StartAudio(nullAudioCallback);
			mainloopCallback = nullMainloopCallback;

			#ifdef OOPSY_TARGET_USES_MIDI_UART
			midi.init(); 
			#endif

			app_selected = 0;
			appdefs[app_selected].load();

			#ifdef OOPSY_TARGET_HAS_OLED
			console_display();
			#endif 

			static bool blink;
			while(1) {
				uint32_t t1 = dsy_system_getnow();
				dt = t1-t;
				t = t1;
				
				if (uitimer.ready(dt)) {

					#ifdef OOPSY_TARGET_USES_MIDI_UART
					midi.mainloop();
					#endif

					// CLEAR DISPLAY
					#ifdef OOPSY_TARGET_HAS_OLED
					hardware.display.Fill(false);
					#endif
					#ifdef OOPSY_TARGET_PETAL 
					hardware.ClearLeds();
					#endif
					
					// handle app-level code (e.g. for LED/CV/gate outs)
					mainloopCallback(t, dt);

					#ifdef OOPSY_TARGET_PETAL 
					// petal has no mode selection
					is_mode_selecting = 0;
					#if defined(OOPSY_MULTI_APP)
					// multi-app petal is always in menu mode:
					mode = MODE_MENU;
					#endif
					// blink = !blink;
					// hardware.SetFootswitchLed((daisy::DaisyPetal::FootswitchLed)0, blink);

					for(int i = 0; i < 8; i++) {
						float white = (i == app_selecting || encoder_released);
						hardware.SetRingLed((daisy::DaisyPetal::RingLed)i, 
							(i == app_selected || white) * 1.f,
							white * 1.f,
							(i < app_count) * 0.3f + white * 1.f
						);
					}
					#endif //OOPSY_TARGET_PETAL

					if (encoder_held_ms > OOPSY_LONG_PRESS_MS) {
						// LONG PRESS
						#ifndef OOPSY_TARGET_PETAL
						is_mode_selecting = 1;
						#endif
					}
			
					// Handle encoder increment actions:
					if (is_mode_selecting) {
						mode += encoder_incr;
						if (mode >= MODE_COUNT) mode = 1;
						if (mode < 1) mode = MODE_COUNT-1;	
					#ifdef OOPSY_MULTI_APP
					} else if (mode == MODE_MENU) {
						app_selecting += encoder_incr;
						if (app_selecting >= app_count) app_selecting -= app_count;
						if (app_selecting < 0) app_selecting += app_count;
					#endif // OOPSY_MULTI_APP
					#ifdef OOPSY_TARGET_HAS_OLED
					} else if (mode == MODE_SCOPE) {
						if (encoder_incr > 0) {
							scope_zoom = scope_zoom + 1;
							if (scope_zoom > OOPSY_SCOPE_MAX_ZOOM) scope_zoom = 1;
						} else if (encoder_incr < 0) {
							scope_zoom = scope_zoom - 1;
							if (scope_zoom < 1) scope_zoom = OOPSY_SCOPE_MAX_ZOOM;
						}
					#endif //OOPSY_TARGET_HAS_OLED
					}
					encoder_incr = 0;

					// SHORT PRESS	
					if (encoder_released) {
						if (is_mode_selecting) {
							is_mode_selecting = 0;
						#ifdef OOPSY_MULTI_APP
						} else if (mode == MODE_MENU) {
							if (app_selected != app_selecting) {
								app_selected = app_selecting;
								appdefs[app_selected].load();
								mode = mode_default;
							}
						#endif
						}
					} 
					encoder_released = 0;

					switch(mode) {
						#ifdef OOPSY_TARGET_HAS_OLED
						#ifdef OOPSY_MULTI_APP
						case MODE_MENU: {
							for (int i=0; i<8; i++) {
								if (i == app_selecting) {
									hardware.display.SetCursor(0, font.FontHeight * i);
									hardware.display.WriteString((char *)">", font, true);
								}
								if (i < app_count) {
									hardware.display.SetCursor(font.FontWidth, font.FontHeight * i);
									hardware.display.WriteString((char *)appdefs[i].name, font, true);
								}
							}
						} break;
						#endif //OOPSY_MULTI_APP
						case MODE_SCOPE: {
							uint8_t h = SSD1309_HEIGHT - font.FontHeight;
							hardware.display.Fill(false);
							for (uint_fast8_t i=0; i<SSD1309_WIDTH; i++) {
								int j = i*2;
								hardware.display.DrawLine(i, (1.f-scope_data[j])*(h/2), i, (1.f-scope_data[j+1])*(h/2), 1);
							}
							size_t samples = scope_samples();
							// each pixel is zoom samples; zoom/samplerate seconds
							float scope_duration = SSD1309_WIDTH*(1000.f*samples/samplerate);
							snprintf(scope_label, 10, "%dx=%dms", samples, (int)ceilf(scope_duration));
							hardware.display.SetCursor(SSD1309_WIDTH - font.FontWidth*strlen(scope_label), h);
							hardware.display.WriteString(scope_label, font, true);

						} break;
						case MODE_CONSOLE: 
						{
							console_display(); 
							break;
						}
						
						#else  // !OOPSY_TARGET_HAS_OLED
						#ifdef OOPSY_MULTI_APP
						case MODE_MENU: {
							// TODO show menu selection via LEDs
							
						} break;
						#endif
						#endif //OOPSY_TARGET_HAS_OLED
						default: {
						}
					}

					if (is_mode_selecting) {
						#ifdef OOPSY_TARGET_HAS_OLED
						hardware.display.DrawRect(0, 0, SSD1309_WIDTH-1, SSD1309_HEIGHT-1, 1);
						#endif
					} 
					#ifdef OOPSY_TARGET_HAS_OLED
					#ifdef OOPSY_MULTI_APP
					if (mode != MODE_MENU) 
					#endif //OOPSY_MULTI_APP
					{
						// status bar
						int offset = 0;
						offset += snprintf(console_stats+offset, console_cols-offset, "%02d%%", int(0.0001f*audioCpuUs*(samplerate)/blocksize));
						#ifdef OOPSY_TARGET_USES_MIDI_UART
						offset += snprintf(console_stats+offset, console_cols-offset, " %cM%c", midi.in_active ? '<' : ' ', midi.out_active ? '>' : ' ');
						midi.in_active = midi.out_active = 0;
						#endif
						// stats:
						hardware.display.SetCursor(0, font.FontHeight * console_rows);
						hardware.display.WriteString(console_stats, font, true);
					}
					hardware.display.Update();
					#endif //OOPSY_TARGET_HAS_OLED

					#ifdef OOPSY_TARGET_PETAL
					hardware.UpdateLeds();
					#endif //OOPSY_TARGET_PETAL

					
				} // uitimer.ready
			}
			return 0;
		}

		void audio_preperform(size_t size) {
			#ifdef OOPSY_TARGET_USES_MIDI_UART
			midi.preperform(size);
			#endif

			#if (OOPSY_TARGET_FIELD)
			hardware.ProcessAnalogControls();
			hardware.UpdateDigitalControls();
			#else
			hardware.DebounceControls();
			hardware.UpdateAnalogControls();
			#endif

			#ifdef OOPSY_TARGET_FIELD
			encoder_held = hardware.GetSwitch(0)->Pressed();
			encoder_incr += hardware.GetSwitch(1)->FallingEdge();
			encoder_held_ms = hardware.GetSwitch(0)->TimeHeldMs();
			if (hardware.GetSwitch(0)->FallingEdge()) encoder_released = 1;
			#else
			encoder_held = hardware.encoder.Pressed();
			encoder_incr += hardware.encoder.Increment();
			encoder_held_ms = hardware.encoder.TimeHeldMs();
			if (hardware.encoder.FallingEdge()) encoder_released = 1;
			#endif
		}

		void audio_postperform(float **hardware_ins, float **hardware_outs, size_t size) {
			#ifdef OOPSY_TARGET_HAS_OLED
			if (mode == MODE_SCOPE) {
				// TODO: add selector for scope storage source:
				float * buf = hardware_outs[0];
				size_t samples = scope_samples();
				for (size_t i=0; i<size/samples; i++) {
					float min0 = 1.f, max0 = -1.f;
					float min1 = 1.f, max1 = -1.f;
					for (size_t j=0; j<samples; j++) {
						float pt0 = *buf++;
						min0 = min0 > pt0 ? pt0 : min0;
						max0 = max0 < pt0 ? pt0 : max0;
					}
					scope_data[scope_step] = (min0);
					scope_step++;
					scope_data[scope_step] = (max0); 
					scope_step++;
					if (scope_step >= SSD1309_WIDTH*2) scope_step = 0;
				}
			}
			#endif
		}

		#ifdef OOPSY_TARGET_HAS_OLED
		int scope_samples() {
			// valid zoom sizes: 1, 2, 3, 4, 6, 8, 12, 16, 24
			switch(scope_zoom) {
				case 1: case 2: case 3: case 4: return scope_zoom; break;
				case 5: return 6; break;
				case 6: return 8; break;
				case 7: return 12; break;
				case 8: return 16; break;
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

		#if (OOPSY_TARGET_FIELD)
		void setFieldLedsFromData(Data& data) {
			for(long i = 0; i < daisy::DaisyField::LED_LAST && i < data.dim; i++) {
				hardware.led_driver_.SetLed(i, data.mData[i]);
			}
			hardware.led_driver_.SwapBuffersAndTransmit();
		};
		#endif

		static void nullAudioCallback(float **hardware_ins, float **hardware_outs, size_t size);
		
		static void nullMainloopCallback(uint32_t t, uint32_t dt) {}
	} daisy;

	void GenDaisy::nullAudioCallback(float **hardware_ins, float **hardware_outs, size_t size) {
		daisy.nullAudioCallbackRunning = true;
	}


	// Curiously-recurring template to make App definitions simpler:
	template<typename T>
	struct App {
		
		static void staticMainloopCallback(uint32_t t, uint32_t dt) {
			T& self = *(T *)daisy.app;
			self.mainloopCallback(daisy, t, dt);
		}

		static void staticAudioCallback(float **hardware_ins, float **hardware_outs, size_t size) {
			uint32_t start = dsy_tim_get_tick(); // 200MHz
			daisy.audio_preperform(size);
			((T *)daisy.app)->audioCallback(daisy, hardware_ins, hardware_outs, size);
			daisy.audio_postperform(hardware_ins, hardware_outs, size);
			// convert elapsed time (200Mhz ticks) to CPU percentage:
			daisy.audioCpuUs = (dsy_tim_get_tick() - start) / 200;
		}
	};

}; // oopsy::

void genlib_report_error(const char *s) { oopsy::daisy.log(s); }
void genlib_report_message(const char *s) { oopsy::daisy.log(s); }

unsigned long genlib_ticks() { return dsy_system_getnow(); }

t_ptr genlib_sysmem_newptr(t_ptr_size size) {
	return (t_ptr)oopsy::allocate(size);
}

t_ptr genlib_sysmem_newptrclear(t_ptr_size size) {
	t_ptr p = genlib_sysmem_newptr(size);
	if (p) oopsy::memset(p, 0, size);
	return p;
}


#endif //GENLIB_DAISY_H
