#ifndef GENLIB_DAISY_H
#define GENLIB_DAISY_H

#include "daisy.h"
#include "genlib.h"
#include "genlib_exportfunctions.h"
#include <math.h>
#include <string>
#include <cstring> // memset
#include <stdarg.h> // vprintf


#if defined(GEN_DAISY_TARGET_PATCH)
#include "daisy_patch.h"
#define GEN_DAISY_TARGET_HAS_OLED 1
#define GEN_DAISY_IO_COUNT (4)
typedef daisy::DaisyPatch Daisy;

#elif defined(GEN_DAISY_TARGET_FIELD)
#include "daisy_field.h"
#define GEN_DAISY_TARGET_HAS_OLED 1
#define GEN_DAISY_IO_COUNT (2)
typedef daisy::DaisyField Daisy;

#elif defined(GEN_DAISY_TARGET_PETAL)
#include "daisy_petal.h"
#define GEN_DAISY_IO_COUNT (2)
typedef daisy::DaisyPetal Daisy;

#elif defined(GEN_DAISY_TARGET_POD)
#include "daisy_pod.h"
#define GEN_DAISY_IO_COUNT (2)
typedef daisy::DaisyPod Daisy;

#else 
#include "daisy_seed.h"
#define GEN_DAISY_IO_COUNT (2)
typedef daisy::DaisySeed Daisy;

#endif


////////////////////////// DAISY EXPORT INTERFACING //////////////////////////

#define GEN_DAISY_BUFFER_SIZE 48
#define GEN_DAISY_MIDI_BUFFER_SIZE 64
#define GEN_DAISY_LONG_PRESS_MS 250
#define GEN_DAISY_DISPLAY_PERIOD_MS 20
static const uint32_t GEN_DAISY_SRAM_SIZE = 512 * 1024; 
static const uint32_t GEN_DAISY_SDRAM_SIZE = 64 * 1024 * 1024;


namespace gendaisylib {

	char * sram_pool = nullptr;
	char DSY_SDRAM_BSS sdram_pool[GEN_DAISY_SDRAM_SIZE];

	uint32_t sram_used = 0, sram_usable = 0;
	uint32_t sdram_used = 0, sdram_usable = 0;

	void init() {
		if (!sram_pool) sram_pool = (char *)malloc(GEN_DAISY_SRAM_SIZE);
		sram_usable = GEN_DAISY_SRAM_SIZE;
		sram_used = 0;
		sdram_usable = GEN_DAISY_SDRAM_SIZE;
		sdram_used = 0;
	}

	void * allocate(uint32_t size) {
		//console.log("alloc %d/%d", size, sram_usable);
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
		for (i = 0; i < size; i++, p2++)
			*p2 = char(c);
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
	// 		console.log("%d: malloced %dk", i, sz);
	// 		i++;
	// 	}
	// 	console.log("all OK");
	// }
};


struct Timer {
	int32_t period = GEN_DAISY_DISPLAY_PERIOD_MS, t=0;

	bool ready(int32_t dt) {
		t += dt;
		if (t > period) {
			t = 0;
			return true;
		}
		return false;
	}
};


struct MidiUart {
	daisy::UartHandler uart;
	uint8_t in_written = 0, out_written = 0;
	uint8_t in_active = 0, out_active = 0;
	uint8_t out_data[GEN_DAISY_MIDI_BUFFER_SIZE];
	float in_data[GEN_DAISY_BUFFER_SIZE];

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
		for (size_t i=0; i<size && out_written < GEN_DAISY_MIDI_BUFFER_SIZE; i++) {
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
			if (in_written < GEN_DAISY_BUFFER_SIZE) {
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
};


#if defined(GEN_DAISY_TARGET_HAS_OLED)
#define GEN_DAISY_SCOPE_MAX_ZOOM (9)
struct Console {
	FontDef& font = Font_6x8;
	uint_fast8_t scope_zoom = 5; 
	uint_fast8_t scope_step = 0;
	uint16_t console_cols, console_rows, console_line;
	char * console_stats;
	char * console_memory;
	char ** console_lines;
	float scope_data[SSD1309_WIDTH*2]; // 128 pixels
	char scope_label[11];

	Console& init() {
		console_cols = SSD1309_WIDTH / font.FontWidth + 1; // +1 to accommodate null terminators.
		console_rows = SSD1309_HEIGHT / font.FontHeight - 1; // leave one row free for stats
		console_memory = (char *)calloc(console_cols, console_rows);
		console_stats = (char *)calloc(console_cols, 1);
		for (int i=0; i<console_rows; i++) {
			console_lines[i] = &console_memory[i*console_cols];
		}
		console_line = console_rows-1;
		return *this;
	}

	Console& newline() {
		console_line = (console_line + 1) % console_rows;
		return *this;
	}

	Console& log(const char * fmt, ...) {
		va_list argptr;
		va_start(argptr, fmt);
		vsnprintf(console_lines[console_line], console_cols, fmt, argptr);
		va_end(argptr);
		newline();
		return *this;
	}

	Console& display(daisy::OledDisplay& oled) {
		for (int i=0; i<console_rows; i++) {
			oled.SetCursor(0, font.FontHeight * i);
 			oled.WriteString(console_lines[(i+console_line) % console_rows], font, true);
		}
		return *this;
	}

	Console& displayStats(daisy::OledDisplay& oled) {
		// stats:
		oled.SetCursor(0, font.FontHeight * console_rows);
		oled.WriteString(console_stats, font, true);
		return *this;
	}

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

	Console&  scope_increment(int incr) {
		if (incr > 0) {
			scope_zoom = scope_zoom + 1;
			if (scope_zoom > GEN_DAISY_SCOPE_MAX_ZOOM) scope_zoom = 1;
		} else if (incr < 0) {
			scope_zoom = scope_zoom - 1;
			if (scope_zoom < 1) scope_zoom = GEN_DAISY_SCOPE_MAX_ZOOM;
		}
		return *this;
	} 

	Console&  scope_store(float * buf, size_t size, float samplerate) {
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
			//console.log("m1 %d %d", int(min1 * 10), int(max1 * 10));
			if (scope_step >= SSD1309_WIDTH*2) scope_step = 0;
		}
		return *this;
	}

	Console&  scope_display(daisy::OledDisplay& oled, float samplerate) {
		uint8_t h = SSD1309_HEIGHT - font.FontHeight;
		oled.Fill(false);
		for (uint_fast8_t i=0; i<SSD1309_WIDTH; i++) {
		 	int j = i*2;
			oled.DrawLine(i, (1.f-scope_data[j])*(h/2), i, (1.f-scope_data[j+1])*(h/2), 1);
		}
		size_t samples = scope_samples();
		// each pixel is zoom samples; zoom/samplerate seconds
		float scope_duration = SSD1309_WIDTH*(1000.f*samples/samplerate);
		
		snprintf(scope_label, 10, "%dx=%dms", samples, (int)ceilf(scope_duration));
		oled.SetCursor(SSD1309_WIDTH - font.FontWidth*strlen(scope_label), h);
		oled.WriteString(scope_label, font, true);
		oled.Update();
		return *this;
	}
};

#else // GEN_DAISY_TARGET_HAS_OLED

struct Console {
	Console&  init() { return *this; }
	Console&  newline() {return *this; }
	Console&  log(const char * fmt, ...) {return *this; }
	Console&  display(daisy::OledDisplay& oled) {return *this; }
};

#endif // GEN_DAISY_TARGET_HAS_OLED

struct GenDaisy {

	typedef enum {
		MODE_NONE = 0,
		#ifdef GEN_DAISY_MULTI_APP
		MODE_MENU,
		#endif

		#ifdef GEN_DAISY_TARGET_HAS_OLED
		MODE_CONSOLE,
		MODE_SCOPE,
		#endif

		MODE_COUNT
	} Mode;

	struct AppDef {
		const char * name;
		void (*load)();
	};

	Daisy hardware;
	AppDef * appdefs = nullptr;

	int mode, mode_default;

	int app_count = 1;
	int app_selected = 0, app_selecting = 0;
	
	int encoder_held = 0, encoder_held_ms = 0, encoder_released = 0, encoder_incr = 0;

	int is_mode_selecting = 0;


	uint32_t t = 0, dt = 10;
	Timer uitimer;

	// microseconds spent in audio callback
	uint32_t audioCpuUs = 0; 

	float samplerate; // default 48014
	size_t blocksize; // default 48

	#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
	MidiUart midi;
	#endif
	
	Console console;

	void (*mainloopCallback)(uint32_t t, uint32_t dt);
	void * app = nullptr;
	void * gen = nullptr;
	bool nullAudioCallbackRunning = false;

	template<typename A>
	void reset(A& newapp) {
		// first, remove callbacks:
		mainloopCallback = nullMainloopCallback;
		nullAudioCallbackRunning = false;
		hardware.ChangeAudioCallback(nullAudioCallback);
		while (!nullAudioCallbackRunning) dsy_system_delay(10);
		// reset memory
		gendaisylib::init();
		// install new app:
		app = &newapp;
		newapp.init(*this);
		// install new callbacks:
		mainloopCallback = newapp.staticMainloopCallback;
		hardware.ChangeAudioCallback(newapp.staticAudioCallback);
		//console.log("app loaded");
		
		console.log("loaded %s", appdefs[app_selected].name);
		console.log("%d/%dK+%d/%dM", gendaisylib::sram_used/1024, GEN_DAISY_SRAM_SIZE/1024, gendaisylib::sdram_used/1048576, GEN_DAISY_SDRAM_SIZE/1048576);
	}

	int run(AppDef * appdefs, int count) {
		this->appdefs = appdefs;

		mode_default = (Mode)(MODE_COUNT-1);
		mode = mode_default;

		hardware.Init(); 
		#ifdef GEN_DAISY_TARGET_FIELD
		samplerate = hardware.SampleRate(); // default 48014
		blocksize = hardware.BlockSize();  // default 48
		#else
		samplerate = hardware.AudioSampleRate(); // default 48014
		blocksize = hardware.AudioBlockSize();  // default 48
		#endif
		app_count = count;
		console.init();

		hardware.StartAdc();
		hardware.StartAudio(nullAudioCallback);
		mainloopCallback = nullMainloopCallback;

		#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
		midi.init(); 
		#endif

		app_selected = 0;
		appdefs[app_selected].load();

		#ifdef GEN_DAISY_TARGET_HAS_OLED
		console.display(hardware.display);
		#endif 
		while(1) {
			uint32_t t1 = dsy_system_getnow();
			dt = t1-t;
			t = t1;

			#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
			midi.mainloop();
			#endif
			// handle app-level code (e.g. for LED/CV/gate outs)
			mainloopCallback(t, dt);
			
			if (uitimer.ready(dt)) {
				// Handle encoder press/longpress actions:
				if (encoder_held_ms > GEN_DAISY_LONG_PRESS_MS) {
					// LONG PRESS
					is_mode_selecting = 1;
				} 
		
				// Handle encoder increment actions:
				if (is_mode_selecting) {
					//if (encoder_incr) console.log("mode %d %d %d", encoder_incr, mode, MODE_COUNT);
					mode += encoder_incr;
					if (mode >= MODE_COUNT) mode = 1;
					if (mode < 1) mode = MODE_COUNT-1;	
				#ifdef GEN_DAISY_MULTI_APP
				} else if (mode == MODE_MENU) {
					app_selecting += encoder_incr;
					if (app_selecting >= app_count) app_selecting -= app_count;
					if (app_selecting < 0) app_selecting += app_count;
				#endif
				#ifdef GEN_DAISY_TARGET_HAS_OLED
				} else if (mode == MODE_SCOPE) {
					console.scope_increment(encoder_incr);
				#endif
				}
				encoder_incr = 0;

				// SHORT PRESS	
				if (encoder_released) {
					if (is_mode_selecting) {
						is_mode_selecting = 0;
					#ifdef GEN_DAISY_MULTI_APP
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
				
				// if (encoder_held_ms > GEN_DAISY_LONG_PRESS_MS) {
				// 	mode = MODE_MENU;
				// } else if (mode == MODE_MENU) {
				// 	// just released:
				// 	mode = mode_default;
				// 	if (app_selected != app_selecting) {
				// 		app_selected = app_selecting;
				// 		console.log("load %s", appdefs[app_selected].name);
				// 		appdefs[app_selected].load();
				// 	}
				// } 

				// switch (mode) {
				// 	case MODE_MENU: {
				// 		app_selecting += encoder_incr;
				// 		if (app_selecting >= app_count) app_selecting -= app_count;
				// 		if (app_selecting < 0) app_selecting += app_count;

				// 	} break;
				// 	#ifdef GEN_DAISY_TARGET_HAS_OLED
				// 	case MODE_SCOPE: scope.increment(encoder_incr); break;
				// 	#endif
				// 	default: break;
				// }


				#ifdef GEN_DAISY_TARGET_HAS_OLED
				hardware.display.Fill(false);
				#endif
				switch(mode) {
					#ifdef GEN_DAISY_TARGET_HAS_OLED
					#ifdef GEN_DAISY_MULTI_APP
					case MODE_MENU: {
						FontDef& font = console.font;
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
					#endif //GEN_DAISY_MULTI_APP
					case MODE_SCOPE: console.scope_display(hardware.display, samplerate); break;
					case MODE_CONSOLE: 
					{
						console.display(hardware.display); 
						break;
					}
					
					#else  //GEN_DAISY_TARGET_HAS_OLED
					case MODE_MENU: {
						// TODO show menu selection via LEDs
					} break;
					#endif //GEN_DAISY_TARGET_HAS_OLED
					default: {
					}
				}

				if (is_mode_selecting) {
					#ifdef GEN_DAISY_TARGET_HAS_OLED
					hardware.display.DrawRect(0, 0, SSD1309_WIDTH-1, SSD1309_HEIGHT-1, 1);
					hardware.display.Update();
					#endif
				} 
				#ifdef GEN_DAISY_TARGET_HAS_OLED
				#ifdef GEN_DAISY_MULTI_APP
				if (mode != MODE_MENU) 
				#endif //GEN_DAISY_MULTI_APP
				{
					// fraction of audio time used:
					float percent = 0.0001f*audioCpuUs*(samplerate)/blocksize;
					snprintf(console.console_stats, console.console_cols, "%02d%%", int(percent));
					//snprintf(console.console_stats, console.console_cols, "%f%%", (100.f*percent));
					console.displayStats(hardware.display);
				}
				hardware.display.Update();
				#endif //GEN_DAISY_TARGET_HAS_OLED
			} // uitimer.ready
		}
		return 0;
	}

	void audio_preperform(size_t size) {

		#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
		midi.preperform(size);
		#endif

		#if (GEN_DAISY_TARGET_FIELD)
		hardware.ProcessAnalogControls();
		hardware.UpdateDigitalControls();
		#else
		hardware.DebounceControls();
		hardware.UpdateAnalogControls();
		#endif

		#ifdef GEN_DAISY_TARGET_FIELD
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
		#ifdef GEN_DAISY_TARGET_HAS_OLED
		if (mode == MODE_SCOPE) {
			//scope.store(hardware_ins, size, samplerate);
			console.scope_store(hardware_outs[0], size, samplerate);
		}
		#endif
	}

	static void nullAudioCallback(float **hardware_ins, float **hardware_outs, size_t size);
	
	static void nullMainloopCallback(uint32_t t, uint32_t dt) {}
} gendaisy;

void GenDaisy::nullAudioCallback(float **hardware_ins, float **hardware_outs, size_t size) {
	gendaisy.nullAudioCallbackRunning = true;
}

// Curiously-recurring template to make App definitions simpler:
template<typename T>
struct StaticApp {
	
	static void staticMainloopCallback(uint32_t t, uint32_t dt) {
		T& self = *(T *)gendaisy.app;
		self.mainloopCallback(gendaisy, t, dt);
	}

	static void staticAudioCallback(float **hardware_ins, float **hardware_outs, size_t size) {
		uint32_t start = dsy_tim_get_tick(); // 200MHz
		gendaisy.audio_preperform(size);
		((T *)gendaisy.app)->audioCallback(gendaisy, hardware_ins, hardware_outs, size);
		gendaisy.audio_postperform(hardware_ins, hardware_outs, size);
		// convert elapsed time (200Mhz ticks) to CPU percentage:
		gendaisy.audioCpuUs = (dsy_tim_get_tick() - start) / 200;
	}
};

////////////////////////// BINDING DAISY TO GENLIB //////////////////////////

void genlib_report_error(const char *s) { gendaisy.console.log(s); }
void genlib_report_message(const char *s) { gendaisy.console.log(s); }
unsigned long genlib_ticks() { return dsy_system_getnow(); }

t_ptr genlib_sysmem_newptr(t_ptr_size size) {
	return (t_ptr)gendaisylib::allocate(size);
}

t_ptr genlib_sysmem_newptrclear(t_ptr_size size) {
	t_ptr p = genlib_sysmem_newptr(size);
	if (p) gendaisylib::memset(p, 0, size);
	return p;
}

#endif //GENLIB_DAISY_H
