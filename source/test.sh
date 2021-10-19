rm -rf build_* && \
node oopsy.js ../templates/oopsy_patch.cpp patch && \
node oopsy.js ../templates/oopsy_field.cpp field && \
node oopsy.js ../templates/oopsy_petal.cpp petal && \
node oopsy.js ../templates/oopsy_pod.cpp pod && \
node oopsy.js ../templates/oopsy_versio.cpp versio && \
node oopsy.js ../templates/oopsy_pod.cpp seed.pod.json && \
node oopsy.js ../templates/oopsy_versio.cpp seed.versio.json && \
node oopsy.js ../templates/oopsy_bluemchen.cpp bluemchen && \
node oopsy.js ../templates/oopsy_nehcmeulb.cpp nehcmeulb && \
node oopsy.js ../examples/dattoro.cpp ../examples/giga.cpp ../examples/crossover.cpp ../examples/midside.cpp && \
node oopsy.js ../examples/midi_noteouts.cpp ../examples/midi_drum_ins.cpp ../examples/midi_drum_outs.cpp ../examples/midi_control_ins.cpp ../examples/midi_control_outs.cpp ../examples/midi_transport_ins.cpp  ../examples/midi_transport_outs.cpp && \
node oopsy.js ../examples/sdcard.cpp