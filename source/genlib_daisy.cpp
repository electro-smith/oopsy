/*******************************************************************************************************************
Cycling '74 License for Max-Generated Code for Export
Copyright (c) 2016 Cycling '74
The code that Max generates automatically and that end users are capable of exporting and using, and any
  associated documentation files (the “Software”) is a work of authorship for which Cycling '74 is the author
  and owner for copyright purposes.  A license is hereby granted, free of charge, to any person obtaining a
  copy of the Software (“Licensee”) to use, copy, modify, merge, publish, and distribute copies of the Software,
  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The Software is licensed to Licensee only for non-commercial use. Users who wish to make commercial use of the
  Software must contact the copyright owner to determine if a license for commercial use is available, and the
  terms and conditions for same, which may include fees or royalties. For commercial use, please send inquiries
  to licensing@cycling74.com.  The determination of whether a use is commercial use or non-commercial use is based
  upon the use, not the user. The Software may be used by individuals, institutions, governments, corporations, or
  other business whether for-profit or non-profit so long as the use itself is not a commercialization of the
  materials or a use that generates or is intended to generate income, revenue, sales or profit.
The above copyright notice and this license shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
  THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*******************************************************************************************************************/

#include "genlib.h"
#include <stdlib.h> // not cstdlib (causes problems with ARM embedded compiler)
#include <cstdio>
#include <cstring>

#ifndef MSP_ON_CLANG
#	include <cmath>
#endif

#include <malloc.h>
#define malloc_size malloc_usable_size

// DATA_MAXIMUM_ELEMENTS * 8 bytes = 256 mb limit
#define DATA_MAXIMUM_ELEMENTS	(33554432)

t_ptr genlib_sysmem_resizeptr(void *ptr, t_ptr_size newsize) {
	// this function only seems to get called by SineData if reset() happens a second time
	// this shouldn't ever happen, so we default to new allocation:
	return genlib_sysmem_newptr(newsize);
}

void genlib_sysmem_freeptr(void *ptr) {
	// nothing to do, we are not running memory management here
}

void genlib_set_zero64(t_sample *memory, long size) {
	long i;
	for (i = 0; i < size; i++, memory++) *memory = 0.;
}

// NEED THIS FOR WINDOWS:
void *operator new(size_t size) { return genlib_sysmem_newptr(size); }
void *operator new[](size_t size) { return genlib_sysmem_newptr(size); }
void operator delete(void *p) throw() { genlib_sysmem_freeptr(p); }
void operator delete[](void *p) throw() { genlib_sysmem_freeptr(p); }

void *genlib_obtain_reference_from_string(const char *name) {
	return 0; // to be implemented
}

// the rest is stuff to isolate gensym, attrs, atoms, buffers etc.
t_genlib_buffer *genlib_obtain_buffer_from_reference(void *ref) {
	return 0; // to be implemented
}

t_genlib_err genlib_buffer_edit_begin(t_genlib_buffer *b) {
	return 0; // to be implemented
}

t_genlib_err genlib_buffer_edit_end(t_genlib_buffer *b, long valid) {
	return 0; // to be implemented
}

t_genlib_err genlib_buffer_getinfo(t_genlib_buffer *b, t_genlib_buffer_info *info) {
	return 0; // to be implemented
}

char *genlib_reference_getname(void *ref) {
	return 0; // to be implemented
}

void genlib_buffer_dirty(t_genlib_buffer *b) {
	 // to be implemented
}

t_genlib_err genlib_buffer_perform_begin(t_genlib_buffer *b) {
	return 0; // to be implemented
}
void genlib_buffer_perform_end(t_genlib_buffer *b) {
	// to be implemented
}

t_sample gen_msp_pow(t_sample value, t_sample power) {
	return pow(value, power);
}

void genlib_data_setbuffer(t_genlib_data *b, void *ref) {
	//genlib_report_error("not supported for export targets\n");
}

typedef struct {
	t_genlib_data_info	info;
	t_sample			cursor;	// used by Delay
	//t_symbol *		name;
} t_dsp_gen_data;

t_genlib_data *genlib_obtain_data_from_reference(void *ref) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)genlib_sysmem_newptr(sizeof(t_dsp_gen_data));
	self->info.dim = 0;
	self->info.channels = 0;
	self->info.data = 0;
	self->cursor = 0;
	return (t_genlib_data *)self;
}

t_genlib_err genlib_data_getinfo(t_genlib_data *b, t_genlib_data_info *info) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)b;
	info->dim = self->info.dim;
	info->channels = self->info.channels;
	info->data = self->info.data;
	return GENLIB_ERR_NONE;
}

void genlib_data_release(t_genlib_data *b) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)b;
	if (self->info.data) {
		genlib_sysmem_freeptr(self->info.data);
		self->info.data = 0;
	}
}

long genlib_data_getcursor(t_genlib_data *b) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)b;
	return long(self->cursor);
}

void genlib_data_setcursor(t_genlib_data *b, long cursor) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)b;
	self->cursor = t_sample(cursor);
}

void genlib_data_resize(t_genlib_data *b, long s, long c) {
	t_dsp_gen_data *self = (t_dsp_gen_data *)b;

	size_t sz, oldsz, copysz;
	t_sample *old = 0;
	t_sample *replaced = 0;
	int i, j, copydim, copychannels, olddim, oldchannels;

	//printf("data resize %d %d\n", s, c);

	// cache old for copying:
	old = self->info.data;
	olddim = self->info.dim;
	oldchannels = self->info.channels;

	// limit [data] size:
	if (s * c > DATA_MAXIMUM_ELEMENTS) {
		s = DATA_MAXIMUM_ELEMENTS/c;
		genlib_report_message("warning: constraining [data] to < 256MB");
	}
	// bytes required:
	sz = sizeof(t_sample) * s * c;
	oldsz = sizeof(t_sample) * olddim * oldchannels;

	if (old && sz == oldsz) {
		// no need to re-allocate, just resize
		// careful, audio thread may still be using it:
		if (s > olddim) {
			self->info.channels = c;
			self->info.dim = s;
		} else {
			self->info.dim = s;
			self->info.channels = c;
		}

		genlib_set_zero64(self->info.data, s * c);
		return;

	} else {

		// allocate new:
		replaced = (t_sample *)genlib_sysmem_newptr(sz);

		// check allocation:
		if (replaced == 0) {
			genlib_report_error("data: out of memory");
			// try to reallocate with a default/minimal size instead:
			if (s > 512 || c > 1) {
				genlib_data_resize((t_genlib_data *)self, 512, 1);
			} else {
				// if this fails, then Max is kaput anyway...
				genlib_data_resize((t_genlib_data *)self, 4, 1);
			}
			return;
		}

		// fill with zeroes:
		genlib_set_zero64(replaced, s * c);

		// copy in old data:
		if (old) {
			// frames to copy:
			// clamped:
			copydim = olddim > s ? s : olddim;
			// use memcpy if channels haven't changed:
			if (c == oldchannels) {
				copysz = sizeof(t_sample) * copydim * c;
				//post("reset resize (same channels) %p %p, %d", self->info.data, old, copysz);
				memcpy(replaced, old, copysz);
			} else {
				// memcpy won't work if channels have changed,
				// because data is interleaved.
				// clamp channels copied:
				copychannels = oldchannels > c ? c : oldchannels;
				//post("reset resize (different channels) %p %p, %d %d", self->info.data, old, copydim, copychannels);
				for (i = 0; i < copydim; i++) {
					for (j = 0; j < copychannels; j++) {
						replaced[j + i * c] = old[j + i * oldchannels];
					}
				}
			}
		}

		// now update info:
		if (old == 0) {
			self->info.data = replaced;
			self->info.dim = s;
			self->info.channels = c;
		} else {
			// need to be careful; the audio thread may still be using it
			// since dsp_gen_data is preserved through edits
			// the order of resizing has to be carefully done
			// to prevent indexing out of bounds
			// (or maybe I'm being too paranoid here...)
			if (oldsz > sz) {
				// shrink size first
				if (s > olddim) {
					self->info.channels = c;
					self->info.dim = s;
				} else {
					self->info.dim = s;
					self->info.channels = c;
				}
				self->info.data = replaced;
			} else {
				// shrink size after
				self->info.data = replaced;
				if (s > olddim) {
					self->info.channels = c;
					self->info.dim = s;
				} else {
					self->info.dim = s;
					self->info.channels = c;
				}
			}

			// done with old:
			genlib_sysmem_freeptr(old);

		}
	}
}

void genlib_reset_complete(void *data) {}

size_t genlib_getstatesize(CommonState *cself, getparameter_method getmethod) {
	return 0;
}

short genlib_getstate(CommonState *cself, char *state, getparameter_method getmethod) {
	return 0;
}

short genlib_setstate(CommonState *cself, const char *state, setparameter_method setmethod) {
	return 0;
}

