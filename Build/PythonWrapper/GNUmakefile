BUILD_TOP := $(CURDIR)/..
# the TARGET_BUILD_DIR variable should be set by Xcode;
# but the fallback is also useful if running "make" directly
ifeq ($(strip $(TARGET_BUILD_DIR)),)
TARGET_BUILD_DIR := $(BUILD_TOP)/..
endif

# miscellaneous
COMMA       :=,

# set compiler
CC          := gcc
CXX         := g++
CXXSHARED   := g++ -bundle -undefined suppress -flat_namespace

# set Python installation
PYTHONINC := -I/System/Library/Frameworks/Python.framework/Versions/2.3/include/python2.3 \
             -I/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config

# compiler setup
CFLAGS      := -Wno-long-double
CXXFLAGS    :=
STDLIBPATH  :=
APPLIBPATH  := -F$(TARGET_BUILD_DIR)
LIBPATH     := $(APPLIBPATH) $(STDLIBPATH)
STDLIBS     :=
APPLIBS     := -lm -framework Quills
LIBS        := $(APPLIBS) $(STDLIBS)
INCLUDES   = -I$(BUILD_TOP)/Application/Code -I$(BUILD_TOP)/Shared/Code

# set this flag appropriately if you care about Intel versus PowerPC
UNIVERSAL   := yes
CFLAGS_ARCH :=
ifeq ($(strip $(UNIVERSAL)),yes)
CFLAGS_ARCH  := -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc
LDFLAGS_ARCH := -arch i386 -arch ppc
endif

CFLAGS     += $(CFLAGS_ARCH)
LDFLAGS    += $(LDFLAGS_ARCH)

# SWIG setup
SWIG       = /opt/swig/bin/swig
SWIGOPT    = $(INCLUDES)

# target setup
BASENAME   = Quills
INTERFACE  = $(CURDIR)/$(BASENAME).i
IWRAP      = $(INTERFACE:.i=_wrap.i)
# Python requires ".so" despite Mac OS X convention
DYLIBSWIG  = $(CURDIR)/_$(BASENAME).so
IDEPFILES  := $(wildcard $(BUILD_TOP)/Application/Code/Quills*.h)

# build setup
ICXXSRCS   = $(IWRAP:.i=.cxx)
OBJS       = $(IWRAP:.i=.o)



.PHONY: all
all: $(DYLIBSWIG)

$(ICXXSRCS): $(INTERFACE) $(IDEPFILES)
	$(SWIG) -c++ -python $(SWIGOPT) $(INTERFACE)

%.o: %.cxx
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $^ $(INCLUDES) $(PYTHONINC) -o $@

$(DYLIBSWIG): $(OBJS)
	$(CXXSHARED) $(CFLAGS_ARCH) $(LIBPATH) $(OBJS) $(LIBS) -o $@

.PHONY: clean
clean:
	rm -f $(ICXXSRCS) $(OBJS) $(DYLIBSWIG)
	rm -f $(BASENAME).py

