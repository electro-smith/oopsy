#include "reverb.h"

namespace reverb {

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
	Delay m_delay_10;
	Delay m_delay_6;
	Delay m_delay_5;
	Delay m_delay_7;
	Delay m_delay_8;
	Delay m_delay_9;
	Delay m_delay_4;
	Delay m_delay_13;
	Delay m_delay_16;
	Delay m_delay_12;
	Delay m_delay_15;
	Delay m_delay_14;
	Delay m_delay_11;
	SineCycle __m_cycle_22;
	SineCycle __m_cycle_21;
	SineData __sinedata;
	int vectorsize;
	int __exception;
	t_sample m_wet_19;
	t_sample m_decay_20;
	t_sample m_damping_18;
	t_sample m_predelay_17;
	t_sample m_history_1;
	t_sample samplerate;
	t_sample m_history_2;
	t_sample m_history_3;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) {
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_history_1 = ((int)0);
		m_history_2 = ((int)0);
		m_history_3 = ((int)0);
		m_delay_4.reset("m_delay_4", ((int)924));
		m_delay_5.reset("m_delay_5", ((int)688));
		m_delay_6.reset("m_delay_6", ((int)4217));
		m_delay_7.reset("m_delay_7", ((int)3163));
		m_delay_8.reset("m_delay_8", ((int)2656));
		m_delay_9.reset("m_delay_9", ((int)3720));
		m_delay_10.reset("m_delay_10", ((int)1800));
		m_delay_11.reset("m_delay_11", ((int)4453));
		m_delay_12.reset("m_delay_12", ((int)142));
		m_delay_13.reset("m_delay_13", ((int)107));
		m_delay_14.reset("m_delay_14", ((int)379));
		m_delay_15.reset("m_delay_15", ((int)277));
		m_delay_16.reset("m_delay_16", (samplerate * 0.1));
		m_predelay_17 = ((int)10);
		m_damping_18 = ((t_sample)0.5);
		m_wet_19 = ((int)0);
		m_decay_20 = ((t_sample)0.5);
		__m_cycle_21.reset(samplerate, 0);
		__m_cycle_22.reset(samplerate, 0);
		genlib_reset_complete(this);
		
	};
	// the signal processing routine;
	inline int perform(t_sample ** __ins, t_sample ** __outs, int __n) {
		vectorsize = __n;
		const t_sample * __in1 = __ins[0];
		const t_sample * __in2 = __ins[1];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		if (__exception) {
			return __exception;
			
		} else if (( (__in1 == 0) || (__in2 == 0) || (__out1 == 0) || (__out2 == 0) )) {
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		t_sample mstosamps_615 = (m_predelay_17 * (samplerate * 0.001));
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			t_sample add_522 = (in1 + in2);
			t_sample tap_646 = m_delay_16.read_linear(mstosamps_615);
			t_sample mix_665 = (tap_646 + (m_damping_18 * (m_history_1 - tap_646)));
			t_sample mix_644 = mix_665;
			t_sample tap_621 = m_delay_15.read_step(((int)277));
			t_sample mul_619 = (tap_621 * ((t_sample)0.625));
			t_sample tap_627 = m_delay_14.read_step(((int)379));
			t_sample mul_625 = (tap_627 * ((t_sample)0.625));
			t_sample tap_633 = m_delay_13.read_step(((int)107));
			t_sample mul_631 = (tap_633 * ((t_sample)0.75));
			t_sample tap_639 = m_delay_12.read_step(((int)142));
			t_sample mul_637 = (tap_639 * ((t_sample)0.75));
			t_sample sub_635 = (mix_644 - mul_637);
			t_sample mul_636 = (sub_635 * ((t_sample)0.75));
			t_sample add_634 = (mul_636 + tap_639);
			t_sample sub_629 = (add_634 - mul_631);
			t_sample mul_630 = (sub_629 * ((t_sample)0.75));
			t_sample add_628 = (mul_630 + tap_633);
			t_sample sub_623 = (add_628 - mul_625);
			t_sample mul_624 = (sub_623 * ((t_sample)0.625));
			t_sample add_622 = (mul_624 + tap_627);
			t_sample sub_617 = (add_622 - mul_619);
			t_sample mul_618 = (sub_617 * ((t_sample)0.625));
			t_sample add_616 = (mul_618 + tap_621);
			t_sample tap_591 = m_delay_11.read_step(((int)4453));
			t_sample tap_592 = m_delay_11.read_step(((int)353));
			t_sample tap_593 = m_delay_11.read_step(((int)3627));
			t_sample tap_594 = m_delay_11.read_step(((int)1990));
			t_sample tap_584 = m_delay_10.read_step(((int)1800));
			t_sample tap_585 = m_delay_10.read_step(((int)187));
			t_sample tap_586 = m_delay_10.read_step(((int)1228));
			t_sample tap_576 = m_delay_9.read_step(((int)3720));
			t_sample tap_577 = m_delay_9.read_step(((int)1066));
			t_sample tap_578 = m_delay_9.read_step(((int)2673));
			t_sample gen_611 = ((tap_578 + tap_592) + tap_593);
			t_sample tap_556 = m_delay_8.read_step(((int)2656));
			t_sample tap_557 = m_delay_8.read_step(((int)335));
			t_sample tap_558 = m_delay_8.read_step(((int)1913));
			t_sample gen_610 = (((tap_558 + tap_577) + tap_585) + tap_594);
			t_sample tap_548 = m_delay_7.read_step(((int)3163));
			t_sample tap_549 = m_delay_7.read_step(((int)121));
			t_sample tap_550 = m_delay_7.read_step(((int)1996));
			t_sample tap_560 = m_delay_6.read_step(((int)4217));
			t_sample tap_561 = m_delay_6.read_step(((int)266));
			t_sample tap_562 = m_delay_6.read_step(((int)2974));
			t_sample tap_563 = m_delay_6.read_step(((int)2111));
			t_sample gen_609 = ((tap_561 + tap_562) + tap_550);
			t_sample gen_612 = (((tap_563 + tap_557) + tap_549) + tap_586);
			t_sample mix_666 = (tap_560 + (m_damping_18 * (m_history_3 - tap_560)));
			t_sample mix_545 = mix_666;
			t_sample mix_667 = (tap_591 + (m_damping_18 * (m_history_2 - tap_591)));
			t_sample mix_589 = mix_667;
			t_sample mul_587 = (mix_589 * m_decay_20);
			t_sample mul_543 = (mix_545 * m_decay_20);
			t_sample mul_580 = (tap_584 * ((t_sample)0.5));
			t_sample sub_581 = (mul_587 - mul_580);
			t_sample mul_579 = (sub_581 * ((t_sample)0.5));
			t_sample add_582 = (mul_579 + tap_584);
			t_sample mul_552 = (tap_556 * ((t_sample)0.5));
			t_sample sub_553 = (mul_543 - mul_552);
			t_sample mul_551 = (sub_553 * ((t_sample)0.5));
			t_sample add_554 = (mul_551 + tap_556);
			t_sample mul_546 = (tap_548 * m_decay_20);
			t_sample add_606 = (add_616 + mul_546);
			__m_cycle_21.freq(((t_sample)0.1));
			t_sample cycle_595 = __m_cycle_21(__sinedata);
			t_sample cycleindex_596 = __m_cycle_21.phase();
			t_sample mul_597 = (cycle_595 * ((int)16));
			t_sample add_598 = (mul_597 + ((int)672));
			t_sample tap_604 = m_delay_5.read_linear(add_598);
			t_sample mul_600 = (tap_604 * ((t_sample)0.7));
			t_sample add_601 = (add_606 + mul_600);
			t_sample mul_599 = (add_601 * ((t_sample)0.7));
			t_sample rsub_602 = (tap_604 - mul_599);
			t_sample mul_574 = (tap_576 * m_decay_20);
			t_sample add_605 = (mul_574 + add_616);
			__m_cycle_22.freq(((t_sample)0.07));
			t_sample cycle_564 = __m_cycle_22(__sinedata);
			t_sample cycleindex_565 = __m_cycle_22.phase();
			t_sample mul_566 = (cycle_564 * ((int)16));
			t_sample add_567 = (mul_566 + ((int)908));
			t_sample tap_573 = m_delay_4.read_linear(add_567);
			t_sample mul_569 = (tap_573 * ((t_sample)0.7));
			t_sample add_570 = (add_605 + mul_569);
			t_sample mul_568 = (add_570 * ((t_sample)0.7));
			t_sample rsub_571 = (tap_573 - mul_568);
			t_sample history_544_next_607 = fixdenorm(mix_545);
			t_sample history_588_next_608 = fixdenorm(mix_589);
			t_sample sub_528 = (gen_609 - gen_610);
			t_sample mul_526 = (sub_528 * ((t_sample)0.6));
			t_sample mix_668 = (in1 + (m_wet_19 * (mul_526 - in1)));
			t_sample out1 = mix_668;
			t_sample sub_527 = (gen_611 - gen_612);
			t_sample mul_525 = (sub_527 * ((t_sample)0.6));
			t_sample mix_669 = (in2 + (m_wet_19 * (mul_525 - in2)));
			t_sample out2 = mix_669;
			t_sample history_643_next_647 = fixdenorm(mix_644);
			m_delay_16.write(add_522);
			m_delay_15.write(sub_617);
			m_delay_12.write(sub_635);
			m_delay_13.write(sub_629);
			m_delay_14.write(sub_623);
			m_delay_11.write(rsub_602);
			m_history_2 = history_588_next_608;
			m_history_3 = history_544_next_607;
			m_delay_4.write(add_570);
			m_delay_5.write(add_601);
			m_delay_6.write(rsub_571);
			m_delay_7.write(add_554);
			m_delay_8.write(sub_553);
			m_delay_9.write(add_582);
			m_delay_10.write(sub_581);
			m_history_1 = history_643_next_647;
			m_delay_4.step();
			m_delay_5.step();
			m_delay_6.step();
			m_delay_7.step();
			m_delay_8.step();
			m_delay_9.step();
			m_delay_10.step();
			m_delay_11.step();
			m_delay_12.step();
			m_delay_13.step();
			m_delay_14.step();
			m_delay_15.step();
			m_delay_16.step();
			// assign results to output buffer;
			(*(__out1++)) = out1;
			(*(__out2++)) = out2;
			
		};
		return __exception;
		
	};
	inline void set_predelay(t_param _value) {
		m_predelay_17 = (_value < 0 ? 0 : (_value > 100 ? 100 : _value));
	};
	inline void set_damping(t_param _value) {
		m_damping_18 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_wet(t_param _value) {
		m_wet_19 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_decay(t_param _value) {
		m_decay_20 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	
} State;


///
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs

int gen_kernel_numins = 2;
int gen_kernel_numouts = 2;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 4; }

/// Assistive lables for the signal inputs and outputs

const char *gen_kernel_innames[] = { "in1", "in2" };
const char *gen_kernel_outnames[] = { "out1", "out2" };

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
		case 0: self->set_damping(value); break;
		case 1: self->set_decay(value); break;
		case 2: self->set_predelay(value); break;
		case 3: self->set_wet(value); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		case 0: *value = self->m_damping_18; break;
		case 1: *value = self->m_decay_20; break;
		case 2: *value = self->m_predelay_17; break;
		case 3: *value = self->m_wet_19; break;
		
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
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(4 * sizeof(ParamInfo));
	self->__commonstate.numparams = 4;
	// initialize parameter 0 ("m_damping_18")
	pi = self->__commonstate.params + 0;
	pi->name = "damping";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_damping_18;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 1 ("m_decay_20")
	pi = self->__commonstate.params + 1;
	pi->name = "decay";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_decay_20;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 2 ("m_predelay_17")
	pi = self->__commonstate.params + 2;
	pi->name = "predelay";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_predelay_17;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 100;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 3 ("m_wet_19")
	pi = self->__commonstate.params + 3;
	pi->name = "wet";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_wet_19;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
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


} // reverb::
