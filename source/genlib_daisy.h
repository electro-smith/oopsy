#ifndef GENLIB_DAISY_H
#define GENLIB_DAISY_H

#include "daisysp.h"
#include "genlib.h"
#include "genlib_exportfunctions.h"
#include <math.h>
#include <string>
#include <cstring> // memset
#include <stdarg.h> // vprintf

#if defined(GEN_DAISY_TARGET_PATCH)
#include "daisy_patch.h"
#define GEN_DAISY_TARGET_HAS_OLED 1
#define GEN_DAISY_TARGET_USES_MIDI_UART 1
#define GEN_DAISY_IO_COUNT (4)
typedef daisy::DaisyPatch Daisy;

#elif defined(GEN_DAISY_TARGET_FIELD)
#include "daisy_field.h"
#define GEN_DAISY_TARGET_HAS_OLED 1
#define GEN_DAISY_TARGET_USES_MIDI_UART 1
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
struct Scope {
	float data0[SSD1309_WIDTH*2]; // 128 pixels
	float data1[SSD1309_WIDTH*2]; // 128 pixels
	uint_fast8_t zoom = 8; 
	uint_fast8_t step = 0;

	void increment(int32_t incr) {
		if (incr > 0) {
			zoom = zoom * 2;
			if (zoom >= 16) zoom = 1;
		} else if (incr < 0) {
			if (zoom < 2) {
				zoom = 32;
			} else {
				zoom /= 2;
			}
		}
	} 

	void store(float ** inputs, size_t size) {
		float * buf0 = inputs[0];
		float * buf1 = inputs[1];
		for (uint_fast8_t i=0; i<zoom; i++) {
			float min0 = 1.f, max0 = -1.f;
			float min1 = 1.f, max1 = -1.f;
			for (size_t j=0; j<size/zoom; j++) {
				float pt0 = *buf0++;
				float pt1 = *buf1++;
				// if (pt > 0.f && last < 0.f && step >= SSD1309_WIDTH) {
				// 	sync = step/2;
				// 	step = 0;
				// }
				min0 = min0 > pt0 ? pt0 : min0;
				max0 = max0 < pt0 ? pt0 : max0;
				min1 = min1 > pt1 ? pt1 : min1;
				max1 = max1 < pt1 ? pt1 : max1;
				//last = pt;
			}
			data0[step] = (min0);
			data1[step] = (min1);
			step++;
			data0[step] = (max0); 
			data1[step] = (max1);
			step++;
			if (step >= SSD1309_WIDTH*2) step = 0;
		}
	}

	void display(daisy::OledDisplay& oled) {
		oled.Fill(false);
		for (uint_fast8_t i=0; i<SSD1309_WIDTH; i++) {
			//int j = ((sync + i) % SSD1309_WIDTH)*2;
			int j = i*2;
			
			oled.DrawLine(i, (1.f-data0[j])*(SSD1309_HEIGHT/4), i, (1.f-data0[j+1])*(SSD1309_HEIGHT/4), 1);
			oled.DrawLine(i, (3.f-data1[j])*(SSD1309_HEIGHT/4), i, (3.f-data1[j+1])*(SSD1309_HEIGHT/4), 1);

			//oled.DrawLine(data0[j], data1[j], data0[j+1], data1[j+1], 1);
			
			//oled.DrawPixel(i, (data[i]+1.f)*(SSD1309_HEIGHT/2), 1);
			//oled.DrawPixel(i, i, 1);
		}
		oled.Update();
	}
};

struct Console {
	FontDef& font = Font_6x8;
	uint16_t console_cols, console_rows, console_line;
	char * console_memory;
	char ** console_lines;

	Console& init() {
		console_cols = SSD1309_WIDTH / font.FontWidth + 1; // +1 to accommodate null terminators.
		console_rows = SSD1309_HEIGHT / font.FontHeight;
		console_memory = (char *)calloc(console_cols, console_rows);
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
		oled.Fill(false);
		for (int i=0; i<console_rows; i++) {
			oled.SetCursor(0, font.FontHeight * i);
 			oled.WriteString(console_lines[(i+console_line) % console_rows], font, true);
		}
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
		MODE_MENU,

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

	Mode mode, mode_default;

	int app_count = 1;
	int app_selected = 0, app_selecting = 0;

	uint32_t t = 0, dt = 10;
	Timer displaytimer;

	// microseconds spent in audio callback
	uint32_t audioCpuUs = 0; 

	float samplerate; // default 48014
	size_t blocksize; // default 48

	#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
	MidiUart midi;
	#endif
	
	Console console;
	#ifdef GEN_DAISY_TARGET_HAS_OLED
	Scope scope;
	#endif

	void (*mainloopCallback)(uint32_t t, uint32_t dt);
	void * app = nullptr;
	void * gen = nullptr;
	bool nullAudioCallbackRunning = false;


	int run(AppDef * appdefs, int count) {

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
		console.init().log("gen~ with %d apps", app_count);

		hardware.StartAdc();
		hardware.StartAudio(nullAudioCallback);
		console.log("started audio");
		mainloopCallback = nullMainloopCallback;

		#ifdef GEN_DAISY_TARGET_USES_MIDI_UART
		midi.init(); 
		console.log("uart midi started");
		#endif

		appdefs[0].load();
		console.log("loaded %s", appdefs[0].name);

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
			
			#if GEN_DAISY_TARGET_HAS_OLED
			if (displaytimer.ready(dt)) {

				// displaying app menu?
				#ifdef GEN_DAISY_TARGET_FIELD
				if (hardware.GetSwitch(0)->TimeHeldMs() > GEN_DAISY_LONG_PRESS_MS) {
				#else
				if (hardware.encoder.TimeHeldMs() > GEN_DAISY_LONG_PRESS_MS) {
				#endif
					mode = MODE_MENU;
				} else if (mode == MODE_MENU) {
					// just released:
					mode = mode_default;
					if (app_selected != app_selecting) {
						app_selected = app_selecting;
						console.log("load %s", appdefs[app_selected].name);
						appdefs[app_selected].load();
					}
				} 

				switch(mode) {
					#ifdef GEN_DAISY_TARGET_HAS_OLED
					case MODE_MENU: {
						FontDef& font = console.font;
						hardware.display.Fill(false);
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
						hardware.display.Update();
					} break;
					case MODE_SCOPE: scope.display(hardware.display); break;
					case MODE_CONSOLE: 
					{
						console.display(hardware.display); 
						break;
					}
					#else 
					case MODE_MENU: {
						// TODO show menu selection via LEDs
					} break;
					#endif
					default: break;
				}
			}
			#endif
		}
		return 0;
	}

	template<typename A>
	void reset(A& newapp) {
		// first, remove callbacks:
		mainloopCallback = nullMainloopCallback;
		nullAudioCallbackRunning = false;
		hardware.ChangeAudioCallback(nullAudioCallback);
		while (!nullAudioCallbackRunning) dsy_system_delay(10);
		// reset memory
		genlib_init();
		// install new app:
		app = &newapp;
		newapp.init(*this);
		// install new callbacks:
		mainloopCallback = newapp.staticMainloopCallback;
		hardware.ChangeAudioCallback(newapp.staticAudioCallback);
		//console.log("app loaded");
		genlib_info();
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
		int incr = hardware.GetSwitch(1)->FallingEdge();
		#else
		int incr =  hardware.encoder.Increment();
		#endif

		switch (mode) {
			case MODE_MENU: {
				app_selecting += incr;
				if (app_selecting >= app_count) app_selecting -= app_count;
				if (app_selecting < 0) app_selecting += app_count;

			} break;
			#ifdef GEN_DAISY_TARGET_HAS_OLED
			case MODE_SCOPE: scope.increment(incr); break;
			#endif
			default: break;
		}
	}

	void audio_postperform(float **hardware_ins, float **hardware_outs, size_t size) {
		#ifdef GEN_DAISY_TARGET_HAS_OLED
		if (mode == MODE_SCOPE) {
			//scope.store(hardware_ins, size);
			scope.store(hardware_outs, size);
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

namespace gendaisylib {

	static const uint32_t SRAM_SIZE = 512 * 1024; 
	static const uint32_t SDRAM_SIZE = 64 * 1024 * 1024;

	char * sram_pool = nullptr;
	char DSY_SDRAM_BSS sdram_pool[SDRAM_SIZE];

	uint32_t sram_used = 0, sram_usable = 0;
	uint32_t sdram_used = 0, sdram_usable = 0;

	void info() {
		gendaisy.console.log("sr %d/%d of %d", sram_used/1024, sram_usable/1024, SRAM_SIZE/1024);
		gendaisy.console.log("sd %d/%d of %d", sdram_used/1024, sdram_usable/1024, SDRAM_SIZE/1024);
	}

	void init() {
		if (!sram_pool) sram_pool = (char *)malloc(SRAM_SIZE);
		sram_usable = SRAM_SIZE;
		sram_used = 0;
		sdram_usable = SDRAM_SIZE;
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
		gendaisy.console.log("memerror %d", size);
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

void genlib_report_error(const char *s) { gendaisy.console.log(s); }
void genlib_report_message(const char *s) { gendaisy.console.log(s); }
unsigned long genlib_ticks() { return dsy_system_getnow(); }

void genlib_init() {
	gendaisylib::init();
}

void genlib_info() {
	gendaisylib::info();
}

t_ptr genlib_sysmem_newptr(t_ptr_size size) {
	return (t_ptr)gendaisylib::allocate(size);
}

t_ptr genlib_sysmem_newptrclear(t_ptr_size size) {
	t_ptr p = genlib_sysmem_newptr(size);
	if (p) gendaisylib::memset(p, 0, size);
	return p;
}

#endif //GENLIB_DAISY_H
