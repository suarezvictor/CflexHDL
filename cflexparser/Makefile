PYVER=$(shell python3 --version | sed "s/Python //")
PARSER_C=clangparser$(PYVER).c

all: clangparser.so

$(PARSER_C):
	@cython -3  --embed private/clangparser.py -o clangparser.cxx
	$(CC) -fpreprocessed -dD -E -P $(shell python3-config --cflags) clangparser.cxx > $(PARSER_C)
	@rm clangparser.cxx

clangparser.so: $(PARSER_C)
	$(CC) -o clangparser.so -shared $(PARSER_C) -fPIC $(shell python3-config --cflags --ldflags)

clean:
	rm -Rf clangparser.so __pycache__ private/__pycache__
