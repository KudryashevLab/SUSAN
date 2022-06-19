SRC:=$(wildcard *.cpp)
all: $(SRC)
	@for FILE in $? ; do \
	mex CXXFLAGS='-fexceptions -fPIC -fno-omit-frame-pointer -pthread -std=c++14' -R2018a -I../../src -I../../../dependencies/eigen $${FILE} -lut -lmwservices ; \
	done
