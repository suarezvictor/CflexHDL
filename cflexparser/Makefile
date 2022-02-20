
all: clangparser.so

clangparser.c:
	@cython -3  --embed private/clangparser.py -o clangparser.cxx
	$(CC) -fpreprocessed -dD -E -P $(shell python3-config --cflags) clangparser.cxx > clangparser.c
	@rm clangparser.cxx

clangparser.so: clangparser.c
	$(CC) -o clangparser.so -shared clangparser.c -fPIC $(shell python3-config --cflags --ldflags)

clean:
	rm -Rf clangparser.so __pycache__ private/__pycache__
