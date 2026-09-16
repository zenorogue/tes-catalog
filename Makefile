tescat.wasm: tescat.cpp table-upto5.cpp table-arcm.cpp table.cpp Makefile
	em++ -DEMS tescat.cpp -std=c++17 -o tescat.js -s EXPORTED_FUNCTIONS="['_doit', '_playit', '_close_gfx', '_render', '_activ', '_options']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'FS', 'requestFullscreen']" \
        -sALLOW_MEMORY_GROWTH -sFETCH -sGROWABLE_ARRAYBUFFERS=0 -sUSE_ZLIB=1 -Wno-invalid-offsetof -sSTACK_SIZE=1mb -sDISABLE_EXCEPTION_CATCHING=0 \
	-O2 #-g -O0 -sASSERTIONS=2 -gsource-map -fsanitize=address
