#include "midi_output.h"

namespace midi_output {

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
  to licensing (at) cycling74.com.  The determination of whether a use is commercial use or non-commercial use is based
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

// global noise generator
Noise noise;
static const int GENLIB_LOOPCOUNT_BAIL = 100000;


// The State struct contains all the state and procedures for the gendsp kernel
typedef struct State {
	CommonState __commonstate;
	Change __m_change_5;
	Data m_midioutbuf_3;
	Train __m_train_4;
	int vectorsize;
	int __exception;
	t_sample m_read_2;
	t_sample samplerate;
	t_sample m_written_1;
	t_sample __m_latch_6;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) {
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_written_1 = ((int)0);
		m_read_2 = ((int)0);
		m_midioutbuf_3.reset("midioutbuf", ((int)2048), ((int)1));
		__m_train_4.reset(0);
		__m_change_5.reset(0);
		__m_latch_6 = 0;
		genlib_reset_complete(this);
		
	};
	// the signal processing routine;
	inline int perform(t_sample ** __ins, t_sample ** __outs, int __n) {
		vectorsize = __n;
		const t_sample * __in1 = __ins[0];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		t_sample * __out3 = __outs[2];
		t_sample * __out4 = __outs[3];
		t_sample * __out5 = __outs[4];
		if (__exception) {
			return __exception;
			
		} else if (( (__in1 == 0) || (__out1 == 0) || (__out2 == 0) || (__out3 == 0) || (__out4 == 0) || (__out5 == 0) )) {
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		t_sample mstosamps_444 = (((int)300) * (samplerate * 0.001));
		int midioutbuf_dim = m_midioutbuf_3.dim;
		int midioutbuf_channels = m_midioutbuf_3.channels;
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			t_sample train_452 = __m_train_4(mstosamps_444, ((t_sample)0.5), ((int)0));
			int change_451 = __m_change_5(train_452);
			int gt_450 = (change_451 > ((int)0));
			t_sample noise_445 = noise();
			t_sample abs_446 = fabs(noise_445);
			t_sample mul_447 = (abs_446 * ((int)36));
			t_sample add_448 = (mul_447 + ((int)36));
			__m_latch_6 = ((gt_450 != 0) ? add_448 : __m_latch_6);
			t_sample latch_449 = __m_latch_6;
			int notetrig = change_451;
			if (notetrig) {
				int midioutbuf_dim = m_midioutbuf_3.dim;
				int midioutbuf_channels = m_midioutbuf_3.channels;
				int index_trunc_7 = fixnan(floor((m_written_1 + ((int)1))));
				int index_wrap_8 = ((index_trunc_7 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_7 + 1) % midioutbuf_dim)) : (index_trunc_7 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.5625), index_wrap_8, 0);
				int index_trunc_9 = fixnan(floor((m_written_1 + ((int)2))));
				int index_wrap_10 = ((index_trunc_9 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_9 + 1) % midioutbuf_dim)) : (index_trunc_9 % midioutbuf_dim));
				m_midioutbuf_3.write((latch_449 * ((t_sample)0.00390625)), index_wrap_10, 0);
				int index_trunc_11 = fixnan(floor((m_written_1 + ((int)3))));
				int index_wrap_12 = ((index_trunc_11 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_11 + 1) % midioutbuf_dim)) : (index_trunc_11 % midioutbuf_dim));
				m_midioutbuf_3.write(((train_452 * ((int)127)) * ((t_sample)0.00390625)), index_wrap_12, 0);
				m_written_1 = (m_written_1 + ((int)3));
				
			};
			if ((((int)0) && ((int)0))) {
				int midioutbuf_dim = m_midioutbuf_3.dim;
				int midioutbuf_channels = m_midioutbuf_3.channels;
				int index_trunc_13 = fixnan(floor((m_written_1 + ((int)1))));
				int index_wrap_14 = ((index_trunc_13 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_13 + 1) % midioutbuf_dim)) : (index_trunc_13 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.625), index_wrap_14, 0);
				int index_trunc_15 = fixnan(floor((m_written_1 + ((int)2))));
				int index_wrap_16 = ((index_trunc_15 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_15 + 1) % midioutbuf_dim)) : (index_trunc_15 % midioutbuf_dim));
				m_midioutbuf_3.write(((int)0), index_wrap_16, 0);
				int index_trunc_17 = fixnan(floor((m_written_1 + ((int)3))));
				int index_wrap_18 = ((index_trunc_17 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_17 + 1) % midioutbuf_dim)) : (index_trunc_17 % midioutbuf_dim));
				m_midioutbuf_3.write(((int)0), index_wrap_18, 0);
				m_written_1 = (m_written_1 + ((int)3));
				
			} else {
				if (((int)0)) {
					int midioutbuf_dim = m_midioutbuf_3.dim;
					int midioutbuf_channels = m_midioutbuf_3.channels;
					int index_trunc_19 = fixnan(floor((m_written_1 + ((int)1))));
					int index_wrap_20 = ((index_trunc_19 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_19 + 1) % midioutbuf_dim)) : (index_trunc_19 % midioutbuf_dim));
					m_midioutbuf_3.write(((t_sample)0.8125), index_wrap_20, 0);
					int index_trunc_21 = fixnan(floor((m_written_1 + ((int)2))));
					int index_wrap_22 = ((index_trunc_21 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_21 + 1) % midioutbuf_dim)) : (index_trunc_21 % midioutbuf_dim));
					m_midioutbuf_3.write(((int)0), index_wrap_22, 0);
					m_written_1 = (m_written_1 + ((int)2));
					
				};
				
			};
			if (((int)0)) {
				int midioutbuf_dim = m_midioutbuf_3.dim;
				int midioutbuf_channels = m_midioutbuf_3.channels;
				int index_trunc_23 = fixnan(floor((m_written_1 + ((int)1))));
				int index_wrap_24 = ((index_trunc_23 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_23 + 1) % midioutbuf_dim)) : (index_trunc_23 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.6875), index_wrap_24, 0);
				int index_trunc_25 = fixnan(floor((m_written_1 + ((int)2))));
				int index_wrap_26 = ((index_trunc_25 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_25 + 1) % midioutbuf_dim)) : (index_trunc_25 % midioutbuf_dim));
				m_midioutbuf_3.write(((int)0), index_wrap_26, 0);
				int index_trunc_27 = fixnan(floor((m_written_1 + ((int)3))));
				int index_wrap_28 = ((index_trunc_27 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_27 + 1) % midioutbuf_dim)) : (index_trunc_27 % midioutbuf_dim));
				m_midioutbuf_3.write(((int)0), index_wrap_28, 0);
				m_written_1 = (m_written_1 + ((int)3));
				
			};
			if (((int)0)) {
				int midioutbuf_dim = m_midioutbuf_3.dim;
				int midioutbuf_channels = m_midioutbuf_3.channels;
				int index_trunc_29 = fixnan(floor((m_written_1 + ((int)1))));
				int index_wrap_30 = ((index_trunc_29 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_29 + 1) % midioutbuf_dim)) : (index_trunc_29 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.75), index_wrap_30, 0);
				int index_trunc_31 = fixnan(floor((m_written_1 + ((int)2))));
				int index_wrap_32 = ((index_trunc_31 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_31 + 1) % midioutbuf_dim)) : (index_trunc_31 % midioutbuf_dim));
				m_midioutbuf_3.write(((int)0), index_wrap_32, 0);
				m_written_1 = (m_written_1 + ((int)2));
				
			};
			if (((int)0)) {
				int midioutbuf_dim = m_midioutbuf_3.dim;
				int midioutbuf_channels = m_midioutbuf_3.channels;
				int index_trunc_33 = fixnan(floor((m_written_1 + ((int)1))));
				int index_wrap_34 = ((index_trunc_33 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_33 + 1) % midioutbuf_dim)) : (index_trunc_33 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.875), index_wrap_34, 0);
				int index_trunc_35 = fixnan(floor((m_written_1 + ((int)2))));
				int index_wrap_36 = ((index_trunc_35 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_35 + 1) % midioutbuf_dim)) : (index_trunc_35 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)-0.25), index_wrap_36, 0);
				int index_trunc_37 = fixnan(floor((m_written_1 + ((int)3))));
				int index_wrap_38 = ((index_trunc_37 < 0) ? ((midioutbuf_dim - 1) + ((index_trunc_37 + 1) % midioutbuf_dim)) : (index_trunc_37 % midioutbuf_dim));
				m_midioutbuf_3.write(((t_sample)0.5), index_wrap_38, 0);
				m_written_1 = (m_written_1 + ((int)3));
				
			};
			m_written_1 = wrap(m_written_1, ((int)0), midioutbuf_dim);
			int newdata = (m_read_2 != m_written_1);
			m_read_2 = wrap((m_read_2 + newdata), ((int)0), midioutbuf_dim);
			int index_trunc_39 = fixnan(floor(m_read_2));
			bool index_ignore_40 = ((index_trunc_39 >= midioutbuf_dim) || (index_trunc_39 < 0));
			// samples midioutbuf channel 1;
			t_sample peek_469 = (index_ignore_40 ? 0 : m_midioutbuf_3.read(index_trunc_39, 0));
			t_sample peek_470 = m_read_2;
			t_sample expr_481 = (newdata ? peek_469 : ((int)-1));
			t_sample out5 = expr_481;
			// assign results to output buffer;
			(*(__out1++)) = 0;
			(*(__out2++)) = 0;
			(*(__out3++)) = 0;
			(*(__out4++)) = 0;
			(*(__out5++)) = out5;
			
		};
		return __exception;
		
	};
	inline void set_midioutbuf(void * _value) {
		m_midioutbuf_3.setbuffer(_value);
	};
	
} State;


///
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs

int gen_kernel_numins = 1;
int gen_kernel_numouts = 5;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 1; }

/// Assistive lables for the signal inputs and outputs

const char *gen_kernel_innames[] = { "in1" };
const char *gen_kernel_outnames[] = { "out1", "out2", "out3", "out4", "midi" };

/// Invoke the signal process of a State object

int perform(CommonState *cself, t_sample **ins, long numins, t_sample **outs, long numouts, long n) {
	State* self = (State *)cself;
	return self->perform(ins, outs, n);
}

/// Reset all parameters and stateful operators of a State object

void reset(CommonState *cself) {
	State* self = (State *)cself;
	self->reset(cself->sr, cself->vs);
}

/// Set a parameter of a State object

void setparameter(CommonState *cself, long index, t_param value, void *ref) {
	State *self = (State *)cself;
	switch (index) {
		case 0: self->set_midioutbuf(ref); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		
		
		default: break;
	}
}

/// Get the name of a parameter of a State object

const char *getparametername(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].name;
	}
	return 0;
}

/// Get the minimum value of a parameter of a State object

t_param getparametermin(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmin;
	}
	return 0;
}

/// Get the maximum value of a parameter of a State object

t_param getparametermax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmax;
	}
	return 0;
}

/// Get parameter of a State object has a minimum and maximum value

char getparameterhasminmax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].hasminmax;
	}
	return 0;
}

/// Get the units of a parameter of a State object

const char *getparameterunits(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].units;
	}
	return 0;
}

/// Get the size of the state of all parameters of a State object

size_t getstatesize(CommonState *cself) {
	return genlib_getstatesize(cself, &getparameter);
}

/// Get the state of all parameters of a State object

short getstate(CommonState *cself, char *state) {
	return genlib_getstate(cself, state, &getparameter);
}

/// set the state of all parameters of a State object

short setstate(CommonState *cself, const char *state) {
	return genlib_setstate(cself, state, &setparameter);
}

/// Allocate and configure a new State object and it's internal CommonState:

void *create(t_param sr, long vs) {
	State *self = new State;
	self->reset(sr, vs);
	ParamInfo *pi;
	self->__commonstate.inputnames = gen_kernel_innames;
	self->__commonstate.outputnames = gen_kernel_outnames;
	self->__commonstate.numins = gen_kernel_numins;
	self->__commonstate.numouts = gen_kernel_numouts;
	self->__commonstate.sr = sr;
	self->__commonstate.vs = vs;
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(1 * sizeof(ParamInfo));
	self->__commonstate.numparams = 1;
	// initialize parameter 0 ("m_midioutbuf_3")
	pi = self->__commonstate.params + 0;
	pi->name = "midioutbuf";
	pi->paramtype = GENLIB_PARAMTYPE_SYM;
	pi->defaultvalue = 0.;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = false;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	
	return self;
}

/// Release all resources and memory used by a State object:

void destroy(CommonState *cself) {
	State *self = (State *)cself;
	genlib_sysmem_freeptr(cself->params);
		
	delete self;
}


} // midi_output::
