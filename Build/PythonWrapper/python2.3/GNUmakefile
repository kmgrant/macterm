# variables that are set by Xcode: TARGET_BUILD_DIR, ARCHS, SDKROOT

BUILD_TOP := $(CURDIR)/../..
# the TARGET_BUILD_DIR variable should be set by Xcode;
# but the fallback is also useful if running "make" directly
ifeq ($(strip $(TARGET_BUILD_DIR)),)
TARGET_BUILD_DIR := $(BUILD_TOP)/Application/ForDebugging
endif

# miscellaneous
COMMA       :=,

# set compiler
CC          := gcc
CXX         := g++
CXXSHARED   := g++ -bundle -undefined suppress -flat_namespace

# set a target SDK; it is crucial that this be consistent for all dependencies in this file
ifeq ($(strip $(SDKROOT)),)
SDKROOT     := /Developer/SDKs/MacOSX10.4u.sdk
endif

# set Python installation
PYTHONINC := -I$(SDKROOT)/System/Library/Frameworks/Python.framework/Versions/2.3/include/python2.3/

# compiler setup
CFLAGS      := -Wno-long-double
CXXFLAGS    :=
STDLIBPATH  :=
APPLIBPATH  := -F$(TARGET_BUILD_DIR)
LIBPATH     := $(APPLIBPATH) $(STDLIBPATH)
STDLIBS     :=
APPLIBS     := -lm -framework Quills
LIBS        := $(APPLIBS) $(STDLIBS)
INCLUDES    = -I$(BUILD_TOP)/Application/Code -I$(BUILD_TOP)/Shared/Code

CFLAGS_ARCH  := -isysroot $(SDKROOT) $(foreach ARCH,$(ARCHS),-arch $(ARCH))
LDFLAGS_ARCH := $(foreach ARCH,$(ARCHS),-arch $(ARCH))

CFLAGS     += $(CFLAGS_ARCH)
LDFLAGS    += $(LDFLAGS_ARCH)

# SWIG setup
SWIG       = /opt/swig-1.3.29/bin/swig
SWIGOPT    = $(INCLUDES) -outdir .

# target setup
BASENAME       = Quills
INTERFACE_DIR  = ../
INTERFACE_NAME = $(BASENAME).i
INTERFACE      = $(INTERFACE_DIR)/$(INTERFACE_NAME)
SWIGOPT        += -I$(INTERFACE_DIR)
IWRAP          = $(BASENAME)_wrap.i
# Python requires ".so" despite Mac OS X convention
DYLIBSWIG      = _$(BASENAME).so
IDEPFILES      := $(wildcard $(BUILD_TOP)/Application/Code/Quills*.h)

# build setup
ICXXSRCS   = $(BASENAME)_wrap.cxx
OBJS       = $(BASENAME)_wrap.o



.PHONY: all
all: $(DYLIBSWIG)

$(ICXXSRCS): $(INTERFACE) $(IDEPFILES)
	$(SWIG) -c++ -python $(SWIGOPT) $(INTERFACE_NAME)

%.o: %.cxx
	@# add missing headers that SWIG somehow misses
	echo "#include <string.h>" > $^.NEW
	cat $^ >> $^.NEW
	mv $^.NEW $^
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $^ $(INCLUDES) $(PYTHONINC) -o $@

$(DYLIBSWIG): $(OBJS)
	$(CXXSHARED) $(CFLAGS_ARCH) $(LIBPATH) $(OBJS) $(LIBS) -o $@

.PHONY: clean
clean:
	rm -f $(ICXXSRCS) $(OBJS) $(DYLIBSWIG)
	rm -f $(BASENAME).py

