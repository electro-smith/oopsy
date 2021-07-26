rm -rf build_* && \
node oopsy.js writejson ../templates/oopsy_patch.cpp patch && \
node oopsy.js writejson ../templates/oopsy_field.cpp field && \
node oopsy.js writejson ../templates/oopsy_petal.cpp petal && \
node oopsy.js writejson ../templates/oopsy_pod.cpp pod && \
node oopsy.js writejson ../templates/oopsy_versio.cpp versio && \
node oopsy.js writejson ../templates/oopsy_pod.cpp seed.pod.json && \
node oopsy.js writejson ../templates/oopsy_versio.cpp seed.versio.json 