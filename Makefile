tescat.wasm: tescat.cpp table-upto5.cpp table-arcm.cpp table.cpp
	em++ -DEMS tescat.cpp -std=c++17 -o tescat.js -s EXPORTED_FUNCTIONS="['_doit']" -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" -s TOTAL_MEMORY=134217728
