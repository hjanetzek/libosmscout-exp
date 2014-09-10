# based on https://github.com/tefino/blackadder-1.1/blob/master/lib/swig/java/Makefile.am
# (Liscence BSD)

#include ../swig-common.am

LIBDIR = ../../src
INC_DIR = ../../include

SWIG ?= swig
SWIGFLAGS = -c++ -I$(INC_DIR) -v  -DOSMSCOUT_API

# turn off -Wcast-qual...
CXXFLAGS = -Wall
SWIGCXXFLAGS = -v -Wno-cast-qual -std=c++11 -fno-strict-aliasing -I$(INC_DIR) -I.
SWIGLDFLAGS = -no-undefined -lmarisa

#include includes.am
JAVA_INCLUDES = -I/usr/lib/jvm/java-7-openjdk-amd64/include/ -I/usr/lib/jvm/java-7-openjdk-amd64/include/linux
JAVA_INCLUDES += -I/usr/lib/jvm/java-7-openjdk/include -I/usr/lib/jvm/java-7-openjdk/include/linux
#JAVA_INCLUDES += -I.

JAVAC ?= javac
SWIGCXXFLAGS += $(JAVA_INCLUDES)

BUILT_SOURCES = osmscout_wrap.cxx

lib_LTLIBRARIES = libosmscout_jni.la
libosmscout_jni_la_SOURCES = osmscout_wrap.cxx
libosmscout_jni_la_LIBADD = $(LIBDIR)/libosmscout.la
libosmscout_jni_la_CXXFLAGS = $(SWIGCXXFLAGS)
libosmscout_jni_la_CFLAGS =
libosmscout_jni_la_LDFLAGS = $(SWIGLDFLAGS)

SWIG_PACKAGE = org.osmscout
SWIG_OUT_SRC = gen/org/osmscout
SWIG_OUT_BIN = bin
SWIG_OUT_LIB = osmscout_jni

SWIGFLAGS += -java -outdir $(SWIG_OUT_SRC) -package $(SWIG_PACKAGE)

osmscout_wrap.cxx: *.i $(INC_DIR)/osmscout/*.h $(INC_DIR)/osmscout/util/*.h Transformer.h Visitors.h
	test -d $(SWIG_OUT_SRC) || mkdir -p $(SWIG_OUT_SRC)
	test -d $(SWIG_OUT_BIN) || mkdir -p $(SWIG_OUT_BIN)
	test -d $(SWIG_OUT_LIB) || mkdir -p $(SWIG_OUT_LIB)
	$(SWIG) $(SWIGFLAGS) osmscout.i
	$(JAVAC) $(SWIG_OUT_SRC)/*.java -d $(SWIG_OUT_BIN)

clean-local:
	rm -f osmscout_wrap.cxx
	rm -rf gen
	rm -rf $(SWIG_OUT_BIN)
	rm -rf $(SWIG_OUT_LIB)


#install-data-hook:
#	cp -r $(SWIG_PACKAGE) $(libdir)

uninstall-hook:
	test -n "$(SWIG_PACKAGE)" && rm -rf $(libdir)/$(SWIG_OUT_LIB)

# XXX: No success so far with putting the package into a jar-file (JNI
#      functions from the library not accessible in that case).
