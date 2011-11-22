# GNUmakefile
#
# Builds MacTerm's application bundle by running each project
# (in all required configurations) and copying the resulting
# components into place.
#
# If you want to use an Xcode from a custom location, you can
# either open the project file from that Xcode, or set the path
# on the command line ("make DEVELOPER_DIR=...").  When run from
# Xcode the DEVELOPER_DIR is set automatically; by default on
# the command line, a DEVELOPER_DIR of /Developer is used.
#
# This is necessary to manage such things as merging multiple
# architectures/versions of Quills.framework, and installation
# of secondary executables such as the bug reporter.
#
# Kevin Grant (kmg@mac.com)
# December 14, 2007

# IMPORTANT: The first rule in the file is chosen when "make" is
# run by itself.  Xcode runs "make" for external targets.
.PHONY: first
first: bundle

.PHONY: help
help:
	@echo "Usage: make [FOR_PANTHER=no] [FOR_TIGER=no] [USE_GROWL=no]"
	@echo "       make clean"
	@echo
	@echo "You can also request specific component rules;"
	@echo "look in the GNUmakefile to see what is defined."
	@echo "These may save you some time."
	@echo
	@echo "Finally, note that many components are built using"
	@echo "Xcode, so you can often use the GUI instead.  But"
	@echo "you still need to run this makefile when ready to"
	@echo "assemble the final application bundle."
	@echo

# Xcode configuration files are "compatible enough" with GNU make
# that it is easier to simply include them to share settings.
#
# IMPORTANT: This has implications.  You can't use comments in
# the included .xcconfig file, and any #includes are ignored.
# You CAN depend on other variables, e.g. X = /foo/bar/$(Y), in
# the same way; however, you must then ensure that the dependent
# value -- "Y", in this case -- is properly defined both for
# Xcode builds and any rules in this GNUmakefile that use "X".
#
# The following should be defined by this "include":
#	SYMROOT
#	OBJROOT
#	GROWL_SDK_ROOT (if using Growl)
include $(CURDIR)/Shared/CustomPaths.xcconfig

# temporary files (subdirectories created)
#SYMROOT=<defined in Xcode config>
#OBJROOT=<defined in Xcode config>

XCODE_ENV := \
	SYMROOT=$(SYMROOT) \
	OBJROOT=$(OBJROOT)

# DEVELOPER_DIR should be set by Xcode's build environment;
# if not, or the directory does not exist, use /Developer
ifeq ($(strip $(shell /bin/test -d $(DEVELOPER_DIR) || echo "no")),no)
DEVELOPER_DIR := /Developer
endif

# tools
COPY=/bin/cp
IBTOOL=/usr/bin/ibtool
IBTOOL_COMPILE_XIB_OPTIONS=--errors --warnings --notices --output-format human-readable-text --flatten NO
MKDIR_P=/usr/bin/install -d
MOVE=/bin/mv
RMDIR=/bin/rmdir
RSYNC=/usr/bin/rsync
SYMLINK_REPLACE=/bin/ln -snf
XARGS=/usr/bin/xargs
XCB=$(DEVELOPER_DIR)/usr/bin/xcodebuild $(XCODE_ENV)

# set this to "no" if you want to disable Panther support (may
# make MacTerm easier to build if you don't care about 10.3);
# e.g. "make FOR_PANTHER=no ..."
FOR_PANTHER=yes

# set this to "no" if you want to disable Tiger support (may
# make MacTerm easier to build if you don't care about 10.4);
# e.g. "make FOR_TIGER=no ..."
FOR_TIGER=yes

# set this to "no" if you want to disable Growl support for
# notifications in MacTerm (not recommended unless you don't
# want to download the Growl SDK); see "http://www.growl.info/"
USE_GROWL=yes
#GROWL_SDK_ROOT := <defined in Xcode config>

# the PyMacTerm.framework target is platform-independent, but an
# arbitrary configuration must be given to have Xcode build it
# (and decide the output directory from which to copy)
PY_CONFIG := ForLeopardDebug

SRC_APP_PROJ_TOP := $(CURDIR)/Application
SRC_BUGREPORTER_PROJ_TOP := $(CURDIR)/BugReporter
SRC_PREFSCONVERTER_PROJ_TOP := $(CURDIR)/PrefsConverter

SRC_APP_CODE_TOP := $(SRC_APP_PROJ_TOP)/Code
SRC_APP_PYCODE_TOP := $(SRC_APP_PROJ_TOP)/PythonCode
SRC_APP_RESOURCES_TOP := $(SRC_APP_PROJ_TOP)/Resources
SRC_BUGREPORTER_BUILD_TOP := $(SYMROOT)/ForDebugging
SRC_BUGREPORTER_TOP := $(SRC_BUGREPORTER_BUILD_TOP)/BugReporter.app
SRC_PREFSCONVERTER_BUILD_TOP := $(SYMROOT)/ForDebugging
SRC_PREFSCONVERTER_TOP := $(SRC_PREFSCONVERTER_BUILD_TOP)/PrefsConverter.app
SRC_PY_TOP := $(SYMROOT)/$(PY_CONFIG)/PyMacTerm.framework
SRC_PYINVOKER_TOP_LEOPARD := $(SYMROOT)/ForLeopardDebug
SRC_PYINVOKER_TOP_TIGER := $(SYMROOT)/ForTigerDebug
SRC_PYINVOKER_TOP_PANTHER := $(SYMROOT)/ForPantherDebug
SRC_GROWL_TOP := $(GROWL_SDK_ROOT)/Frameworks/Growl.framework
SRC_QUILLS_TOP_TIGER := $(SYMROOT)/ForTigerDebug/Quills.framework
SRC_QUILLS_TOP_PANTHER := $(SYMROOT)/ForPantherDebug/Quills.framework

DEST_APP_TOP := $(CURDIR)/MacTerm.app/Contents
DEST_APP_FW_TOP := $(DEST_APP_TOP)/Frameworks
DEST_APP_MACOS_TOP := $(DEST_APP_TOP)/MacOS
DEST_APP_RESOURCES_TOP := $(DEST_APP_TOP)/Resources
DEST_BUGREPORTER_TOP := $(DEST_APP_RESOURCES_TOP)/BugReporter.app
DEST_PREFSCONVERTER_TOP := $(DEST_APP_RESOURCES_TOP)/PrefsConverter.app
DEST_PY_TOP := $(DEST_APP_FW_TOP)/PyMacTerm.framework
DEST_PY_CODE_TOP := $(DEST_PY_TOP)/Versions/A/lib/python2.3/pymacterm
DEST_QUILLS_TOP := $(DEST_APP_FW_TOP)/Quills.framework
DEST_GROWL_TOP := $(DEST_APP_FW_TOP)/Growl.framework
DEST_SWIG_TOP_LEOPARD := $(DEST_QUILLS_TOP)/Versions/B/lib/python2.5
DEST_SWIG_TOP_TIGER := $(DEST_QUILLS_TOP)/Versions/B/lib/python2.3.5
DEST_SWIG_TOP_PANTHER := $(DEST_QUILLS_TOP)/Versions/A/lib/python2.3

SWIG_WRAPPERS := _quills.so quills.py
# .pyc files can be created by Python when code runs...not important
SWIG_WRAPPERS_EXTRA := quills.pyc

PANTHER_SWIG_WRAPPERS :=
PANTHER_SWIG_WRAPPERS_EXTRA :=
ifeq ($(strip $(FOR_PANTHER)),yes)
PANTHER_SWIG_WRAPPERS := $(addprefix $(DEST_SWIG_TOP_PANTHER)/,$(SWIG_WRAPPERS))
PANTHER_SWIG_WRAPPERS_EXTRA := $(addprefix $(DEST_SWIG_TOP_PANTHER)/,$(SWIG_WRAPPERS_EXTRA))
endif

TIGER_SWIG_WRAPPERS :=
TIGER_SWIG_WRAPPERS_EXTRA :=
ifeq ($(strip $(FOR_TIGER)),yes)
TIGER_SWIG_WRAPPERS := $(addprefix $(DEST_SWIG_TOP_TIGER)/,$(SWIG_WRAPPERS))
TIGER_SWIG_WRAPPERS_EXTRA := $(addprefix $(DEST_SWIG_TOP_TIGER)/,$(SWIG_WRAPPERS_EXTRA))
endif

LEOPARD_SWIG_WRAPPERS := $(addprefix $(DEST_SWIG_TOP_LEOPARD)/,$(SWIG_WRAPPERS))
LEOPARD_SWIG_WRAPPERS_EXTRA := $(addprefix $(DEST_SWIG_TOP_LEOPARD)/,$(SWIG_WRAPPERS_EXTRA))

# space-separated dependency list for SWIG build rules,
# i.e. if any of these changes it is important to use
# SWIG to rebuild C++/Python sources and recompile
SWIG_DEPENDENCIES := \
$(wildcard $(SRC_APP_CODE_TOP)/Quills*)

# the resource list is broken into types (files or
# bundles) for convenience when writing rules that
# need different commands for recursive trees
APP_RESOURCES_FILES := \
DefaultFormat1.plist \
DefaultFormat2.plist \
DefaultFormat3.plist \
DefaultFormat4.plist \
GrowlRegistrationTicket.growlRegDict \
IconForBellOff.icns \
IconForBellOn.icns \
IconForBundle.icns \
IconForCaution.png \
IconForCommand.icns \
IconForContextMenu.icns \
IconForCustomize.icns \
IconForFullScreen.icns \
IconForHide.icns \
IconForItemAdd.icns \
IconForItemRemove.icns \
IconForKeypadArrowDown.icns \
IconForKeypadArrowLeft.icns \
IconForKeypadArrowRight.icns \
IconForKeypadArrowUp.icns \
IconForKeypadDelete.icns \
IconForKeypadEnter.icns \
IconForKeypadFind.icns \
IconForKeypadInsert.icns \
IconForKeypadPageDown.icns \
IconForKeypadPageUp.icns \
IconForKeypadSelect.icns \
IconForKillSession.icns \
IconForLEDOff.icns \
IconForLEDOn.icns \
IconForMacroSet.icns \
IconForNewSessionDefault.icns \
IconForNewSessionLogInShell.icns \
IconForNewSessionShell.icns \
IconForPrefPanelFormats.icns \
IconForPrefPanelGeneral.icns \
IconForPrefPanelKiosk.icns \
IconForPrefPanelMacros.icns \
IconForPrefPanelSessions.icns \
IconForPrefPanelTerminals.icns \
IconForPrefPanelTranslations.icns \
IconForPrefPanelWorkspaces.icns \
IconForPrint.icns \
IconForRestartSession.icns \
IconForScrollLockOff.icns \
IconForScrollLockOn.icns \
IconForSession.icns \
IconForSessionStatusActive.icns \
IconForSessionStatusDead.icns \
IconForStackWindows.icns \
IconForTerminal.icns \
English.lproj/Credits.html
APP_RESOURCES_BUNDLES := \
English.lproj/Help \
English.lproj/AlertMessages.nib \
English.lproj/ColorBox.nib \
English.lproj/GenericDialog.nib \
English.lproj/KioskPasswordDialog.nib \
English.lproj/PrefPanelFormats.nib \
English.lproj/PrefPanelMacros.nib \
English.lproj/PrefPanels.nib \
English.lproj/PrefPanelSessions.nib \
English.lproj/PrefPanelTerminals.nib \
English.lproj/PrefPanelWorkspaces.nib \
English.lproj/PrefsWindow.nib \
English.lproj/TEKWindow.nib \
English.lproj/TerminalWindow.nib
# set to a list of "English.lproj/<xyz>" paths, where there
# should be an <xyz>.xib source and implied <xyz>.nib output
XIB_PREFIXES := \
English.lproj/AddressDialogCocoa \
English.lproj/AlertMessagesModalCocoa \
English.lproj/AlertMessagesModelessCocoa \
English.lproj/ClipboardCocoa \
English.lproj/CommandLineCocoa \
English.lproj/DebugInterfaceCocoa \
English.lproj/FindDialogCocoa \
English.lproj/InfoWindowCocoa \
English.lproj/KeypadArrangeWindowCocoa \
English.lproj/KeypadControlKeysCocoa \
English.lproj/KeypadFullScreenCocoa \
English.lproj/KeypadFunctionKeysCocoa \
English.lproj/KeypadVT220KeysCocoa \
English.lproj/MainMenuCocoa \
English.lproj/PrintPreviewCocoa \
English.lproj/ServerBrowserCocoa \
English.lproj/TerminalWindowCocoa \
English.lproj/WindowTitleDialogCocoa
APP_RESOURCES_BUNDLES_GENERATED := \
$(foreach PREFIX,$(XIB_PREFIXES),$(PREFIX).nib)
APP_RESOURCES_COPIED := \
$(APP_RESOURCES_FILES) \
$(APP_RESOURCES_BUNDLES) \

# reinstalls application resources when they change;
# note that the trailing slash is significant for
# rsyncing directories in order to copy what's in them
# and not create a redundant destination directory
$(DEST_APP_RESOURCES_TOP)/%: $(SRC_APP_RESOURCES_TOP)/%
	$(MKDIR_P) $(dir $@)
	@if [ -d "$<" ] ; then \
		$(RSYNC) --archive --exclude=.svn/ --exclude=.LSOverride $</ $@ ; \
	else \
		$(RSYNC) --archive --exclude=.svn/ --exclude=.LSOverride $< $@ ; \
	fi

#
# Frameworks subdirectory
#

# Component: Quills.framework

.PHONY: install-quills-framework
install-quills-framework:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	@#-
	@# it is only necessary to build the code once per architecture,
	@# which the Tiger build handles; however, a (PPC-only) Panther
	@# build is done because 10.3 doesn't support universal binaries
ifeq ($(strip $(FOR_PANTHER)),yes)
	$(call banner,BEGIN PANTHER BUILD)
	$(XCB) -project Application.xcodeproj -target Quills.framework -configuration ForPantherDebug
endif
	$(call banner,BEGIN TIGER BUILD)
	@# Tiger and Leopard share this configuration...
	$(XCB) -project Application.xcodeproj -target Quills.framework -configuration ForTigerDebug
	@# merge in the built frameworks to one root; start with the
	@# "preferred" version so that the framework structure
	@# generated by Xcode is already basically set up
	$(RSYNC) --archive $(SRC_QUILLS_TOP_TIGER)/ $(DEST_QUILLS_TOP)
	$(MKDIR_P) $(DEST_QUILLS_TOP)/Versions/A
	$(RSYNC) --archive $(SRC_QUILLS_TOP_PANTHER)/Versions/A/ $(DEST_QUILLS_TOP)/Versions/A
	@# add remaining parts
	$(MAKE) install-swig-wrappers
	$(SYMLINK_REPLACE) Versions/B/Resources $(DEST_QUILLS_TOP)/

.PHONY: clean-quills-framework
clean-quills-framework:
	$(MAKE) clean-swig-wrappers
ifeq ($(strip $(FOR_PANTHER)),yes)
	$(XCB) clean -project Application.xcodeproj -target Quills.framework -configuration ForPantherDebug
endif
	@# note: the majority of Quills isn't built specifically for Leopard,
	@# so the Tiger clean applies to both Tiger and Leopard
	$(XCB) clean -project Application.xcodeproj -target Quills.framework -configuration ForTigerDebug
	@# avoid accidental over-deletion
	@if [ "x$(DEST_QUILLS_TOP)" = "x" ] ; then \
		echo "DEST_QUILLS_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(RM) -R $(DEST_QUILLS_TOP)

$(SYMROOT)/ForPantherDebug/_quills.so \
$(SYMROOT)/ForPantherDebug/quills.py: $(SWIG_DEPENDENCIES)
	$(call banner,build SWIG wrapper: Panther)
	$(XCB) -project Application.xcodeproj -target PythonWrapper -configuration ForPantherDebug

# reinstalls a SWIG wrapper when it changes
$(DEST_SWIG_TOP_PANTHER)/%: $(SYMROOT)/ForPantherDebug/%
	$(MKDIR_P) $(dir $@)
	$(COPY) $< $@

$(SYMROOT)/ForTigerDebug/_quills.so \
$(SYMROOT)/ForTigerDebug/quills.py: $(SWIG_DEPENDENCIES)
	$(call banner,build SWIG wrapper: Tiger)
	$(XCB) -project Application.xcodeproj -target PythonWrapper -configuration ForTigerDebug

# reinstalls a SWIG wrapper when it changes
$(DEST_SWIG_TOP_TIGER)/%: $(SYMROOT)/ForTigerDebug/%
	$(MKDIR_P) $(dir $@)
	$(COPY) $< $@

$(SYMROOT)/ForLeopardDebug/_quills.so \
$(SYMROOT)/ForLeopardDebug/quills.py: $(SWIG_DEPENDENCIES)
	$(call banner,build SWIG wrapper: Leopard)
	@# Python versions are different on each Mac OS, so the lightweight
	@# SWIG wrapper code *does* have to be compiled for each separately
	$(XCB) -project Application.xcodeproj -target PythonWrapper -configuration ForLeopardDebug

# reinstalls a SWIG wrapper when it changes
$(DEST_SWIG_TOP_LEOPARD)/%: $(SYMROOT)/ForLeopardDebug/%
	$(MKDIR_P) $(dir $@)
	$(COPY) $< $@

.PHONY: install-swig-wrappers
install-swig-wrappers: \
$(PANTHER_SWIG_WRAPPERS) \
$(TIGER_SWIG_WRAPPERS) \
$(LEOPARD_SWIG_WRAPPERS)
	@echo

.PHONY: clean-swig-wrappers
clean-swig-wrappers:
ifeq ($(strip $(FOR_PANTHER)),yes)
	$(XCB) clean -project Application.xcodeproj -target PythonWrapper -configuration ForPantherDebug
	$(RM) $(SYMROOT)/ForPantherDebug/quills.py
endif
ifeq ($(strip $(FOR_TIGER)),yes)
	$(XCB) clean -project Application.xcodeproj -target PythonWrapper -configuration ForTigerDebug
	$(RM) $(SYMROOT)/ForTigerDebug/quills.py
endif
	$(XCB) clean -project Application.xcodeproj -target PythonWrapper -configuration ForLeopardDebug
	$(RM) $(SYMROOT)/ForLeopardDebug/quills.py
	echo '$(PANTHER_SWIG_WRAPPERS) $(PANTHER_SWIG_WRAPPERS_EXTRA)' | $(XARGS) $(RM)
	-$(RMDIR) $(DEST_SWIG_TOP_PANTHER)
	echo '$(TIGER_SWIG_WRAPPERS) $(TIGER_SWIG_WRAPPERS_EXTRA)' | $(XARGS) $(RM)
	-$(RMDIR) $(DEST_SWIG_TOP_TIGER)
	echo '$(LEOPARD_SWIG_WRAPPERS) $(LEOPARD_SWIG_WRAPPERS_EXTRA)' | $(XARGS) $(RM)
	-$(RMDIR) $(DEST_SWIG_TOP_LEOPARD)
	@echo

# Component: PyMacTerm.framework

.PHONY: install-py-framework
install-py-framework:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project Application.xcodeproj -target PyMacTerm.framework -configuration $(PY_CONFIG)
	@# avoid rsyncing from /
	@if [ "x$(SRC_PY_TOP)" = "x" ] ; then \
		echo "SRC_PY_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_PY_TOP))
	$(RSYNC) --archive $(SRC_PY_TOP)/ $(DEST_PY_TOP)
	echo "# automatically-generated Python file with basic version data" > $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "major_version = $$ENV{MY_MAJOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "minor_version = $$ENV{MY_MINOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "subminor_version = $$ENV{MY_SUBMINOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "alpha_beta = @{[ chr(39) ]}$$ENV{MY_ALPHA_BETA}@{[ chr(39) ]}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "build_number = $$ENV{MY_BUILD_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(CURDIR)/VersionInfo.sh perl -e 'print "prefs_version = $$ENV{MY_PREFS_VERSION}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	@# validate generated code...
	python $(DEST_PY_CODE_TOP)/versions.py

.PHONY: clean-py-framework
clean-py-framework:
	$(XCB) clean -project Application.xcodeproj -target PyMacTerm.framework -configuration $(PY_CONFIG)
	@# avoid accidental over-deletion
	@if [ "x$(DEST_PY_TOP)" = "x" ] ; then \
		echo "DEST_PY_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(RM) $(DEST_PY_CODE_TOP)/versions.py
	$(RM) -R $(DEST_PY_TOP)

# Component: Growl.framework

.PHONY: install-growl-framework
install-growl-framework:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	@if [ ! -d "$(GROWL_SDK_ROOT)" ] ; then \
		echo "(current working directory: '$(CURDIR)')" >&2 ; \
		echo "first download the Growl SDK <http://growl.info/> and install in subdirectory '$(GROWL_SDK_ROOT)''" >&2 ; \
		exit 1 ; \
	fi
	@# avoid rsyncing from /
	@if [ "x$(SRC_GROWL_TOP)" = "x" ] ; then \
		echo "SRC_GROWL_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_GROWL_TOP))
	$(RSYNC) --archive '$(SRC_GROWL_TOP)/' $(DEST_GROWL_TOP)

.PHONY: clean-growl-framework
clean-growl-framework:
	@# avoid accidental over-deletion
	@if [ "x$(DEST_GROWL_TOP)" = "x" ] ; then \
		echo "DEST_GROWL_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(RM) -R $(DEST_GROWL_TOP)

# Composite: Frameworks

ifeq ($(strip $(USE_GROWL)),yes)
INSTALL_GROWL := install-growl-framework
CLEAN_GROWL := clean-growl-framework
else
INSTALL_GROWL :=
CLEAN_GROWL :=
endif

.PHONY: install-frameworks
install-frameworks: \
$(INSTALL_GROWL) \
install-py-framework \
install-quills-framework
	@# keep in sync with the matching clean rule
	@echo

.PHONY: clean-frameworks
clean-frameworks: \
$(CLEAN_GROWL) \
clean-quills-framework \
clean-py-framework
	-$(RMDIR) $(DEST_APP_FW_TOP)

#
# MacOS subdirectory
#

# reinstalls Python programs when their code changes
$(DEST_APP_MACOS_TOP)/%: $(SRC_APP_PYCODE_TOP)/%
	$(MKDIR_P) $(dir $@)
	$(COPY) $< $@

# This rule sets up virtual paths in the bundle that point to (or
# create wrappers for) interpreter executables.  For more on the
# requirements of these, see the "MacOS/MacTerm" (main) script.
.PHONY: install-executables
install-executables: \
$(DEST_APP_MACOS_TOP)/MacTerm \
$(DEST_APP_MACOS_TOP)/RunApplication.py
	@# keep in sync with the matching clean rule
	$(MKDIR_P) $(DEST_APP_MACOS_TOP)
	$(SYMLINK_REPLACE) /usr/bin/python $(DEST_APP_MACOS_TOP)/MacTerm_python2.x
	@# note that Tiger's Python 2.3.5 is in the same path as Panther's Python 2.3.0
	$(SYMLINK_REPLACE) /System/Library/Frameworks/Python.framework/Versions/2.3/bin/python $(DEST_APP_MACOS_TOP)/MacTerm_python2.3.5
	$(SYMLINK_REPLACE) /System/Library/Frameworks/Python.framework/Versions/2.3/bin/python $(DEST_APP_MACOS_TOP)/MacTerm_python2.3
	$(XCB) -project Application.xcodeproj -target PythonInvoker -configuration ForLeopardDebug
	$(COPY) $(SRC_PYINVOKER_TOP_LEOPARD)/PythonInvoker_debug $(DEST_APP_MACOS_TOP)/MacTerm_python2.5_wrap

.PHONY: clean-executables
clean-executables:
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm
	$(RM) $(DEST_APP_MACOS_TOP)/RunApplication.py
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm_python2.x
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm_python2.3.5
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm_python2.3
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm_python2.5_wrap
	-$(RMDIR) $(DEST_APP_MACOS_TOP)
	$(XCB) clean -project Application.xcodeproj -target PythonInvoker -configuration ForLeopardDebug

#
# Resources subdirectory
#

# Component: Localized .strings Files

.PHONY: install-app-dot-strings
install-app-dot-strings: $(SRC_APP_CODE_TOP)/UIStrings.cp
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(MKDIR_P) $(DEST_APP_RESOURCES_TOP)/English.lproj
	/usr/bin/genstrings $(SRC_APP_CODE_TOP)/UIStrings.cp -o $(DEST_APP_RESOURCES_TOP)/English.lproj

.PHONY: clean-app-dot-strings
clean-app-dot-strings:
	$(foreach FILE,$(wildcard $(DEST_APP_RESOURCES_TOP)/English.lproj/*.strings),$(RM) $(FILE) ;)

# Component: Help

$(DEST_APP_RESOURCES_TOP)/English.lproj/Help:
	$(call banner,$@)
	$(CURDIR)/HTMLHelpBuild.command
	$(MKDIR_P) $(dir $@)
	$(RSYNC) --archive $(CURDIR)/HTMLHelp/_Generated/English/Help/ $@

# Component: Bug Reporter

.PHONY: install-bug-reporter
install-bug-reporter:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project BugReporter.xcodeproj
	@# avoid rsyncing from /
	@if [ "x$(SRC_BUGREPORTER_TOP)" = "x" ] ; then \
		echo "SRC_BUGREPORTER_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_BUGREPORTER_TOP))
	$(RSYNC) --archive $(SRC_BUGREPORTER_TOP)/ $(DEST_BUGREPORTER_TOP)

.PHONY: clean-bug-reporter
clean-bug-reporter:
	$(XCB) clean -project BugReporter.xcodeproj
	$(RM) -R $(DEST_BUGREPORTER_TOP)

# Component: Preferences Converter

.PHONY: install-prefs-converter
install-prefs-converter:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project PrefsConverter.xcodeproj
	@# avoid rsyncing from /
	@if [ "x$(SRC_PREFSCONVERTER_TOP)" = "x" ] ; then \
		echo "SRC_PREFSCONVERTER_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_PREFSCONVERTER_TOP))
	$(RSYNC) --archive $(SRC_PREFSCONVERTER_TOP)/ $(DEST_PREFSCONVERTER_TOP)

.PHONY: clean-prefs-converter
clean-prefs-converter:
	$(XCB) clean -project PrefsConverter.xcodeproj
	$(RM) -R $(DEST_PREFSCONVERTER_TOP)

# Component: Miscellaneous Resources (NIBs, Images, etc.)

.PHONY: install-resources
install-resources: \
$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_COPIED))
	$(call banner,$@)
	@# build any NIBs that are based on XIBs
	$(foreach PREFIX,$(XIB_PREFIXES),$(call compile_xib_named,$(PREFIX),$(SRC_APP_RESOURCES_TOP),$(DEST_APP_RESOURCES_TOP)))
	@# keep in sync with the matching clean rule
	$(MKDIR_P) $(DEST_APP_RESOURCES_TOP)/English.lproj
	$(MAKE) install-app-dot-strings
	$(MAKE) install-bug-reporter
	$(MAKE) install-prefs-converter
	@# make a special exception for Growl, which requires a filename with spaces
	$(MOVE) $(DEST_APP_RESOURCES_TOP)/GrowlRegistrationTicket.growlRegDict \
		'$(DEST_APP_RESOURCES_TOP)/Growl Registration Ticket.growlRegDict'

.PHONY: clean-resources
clean-resources:
	@# undo the special Growl exception
	@if [ -r "$(DEST_APP_RESOURCES_TOP)/Growl Registration Ticket.growlRegDict" ] ; then \
			$(MOVE) '$(DEST_APP_RESOURCES_TOP)/Growl Registration Ticket.growlRegDict' \
				$(DEST_APP_RESOURCES_TOP)/GrowlRegistrationTicket.growlRegDict ; \
	fi
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_FILES))' | $(XARGS) $(RM)
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_BUNDLES))' | $(XARGS) $(RM) -R
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_BUNDLES_GENERATED))' | $(XARGS) $(RM) -R
	$(MAKE) clean-prefs-converter
	$(MAKE) clean-bug-reporter
	$(MAKE) clean-app-dot-strings
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP)/English.lproj
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP)

#
# DefaultPreferences.plist file
#

$(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes often and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-DefaultPreferences.plist $@
	env BUILT_PRODUCTS_DIR="$(DEST_APP_RESOURCES_TOP)" INFOPLIST_PATH=DefaultPreferences.plist \
		$(CURDIR)/VersionInfo.sh $(CURDIR)/Tools/AutoDatePropertyList.sh

#
# Info.plist file
#

$(DEST_APP_TOP)/Info.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes daily and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-Application-Info.plist $@
	env BUILT_PRODUCTS_DIR="$(DEST_APP_TOP)" INFOPLIST_PATH=Info.plist \
		$(CURDIR)/VersionInfo.sh $(CURDIR)/Tools/AutoDatePropertyList.sh

#
# PkgInfo file
#

$(DEST_APP_TOP)/PkgInfo: FORCE
	$(MKDIR_P) $(dir $@)
	/bin/echo -n 'APPLKevG' >| $@

#
# Utilities
#

# compile a XIB to a NIB without using Xcode;
# $(1) is the path to the .xib file that will be the source
# $(2) is the path to the .nib bundle that will be created or overwritten
define compile_xib
	$(IBTOOL) $(IBTOOL_COMPILE_XIB_OPTIONS) '$(1)' --compile '$(2)'

endef

# helper for compiling based on prefix from a source directory and
# dumping results to a destination directory
# $(1) is the prefix (e.g. "abc" implies "abc.xib" source and "abc.nib" output)
# $(2) is the source directory
# $(3) is the output directory
define compile_xib_named
	$(call compile_xib,$(addprefix $(2)/,$(1).xib),$(addprefix $(3)/,$(1).nib))

endef

# print a line in huge text (on VT100 terminals)
define banner
	@printf "\033#3$(1)\n"
	@printf "\033#4$(1)\n"
	@printf "\033#5"

endef

#
# Primary Rules
#

.PHONY: bundle
bundle:
	@# just comment-out anything you don't care about; MacTerm
	@# still works with most of its components missing
	$(MAKE) $(DEST_APP_TOP)/Info.plist
	$(MAKE) $(DEST_APP_TOP)/PkgInfo
	$(MAKE) $(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist
	$(MAKE) install-frameworks
	$(MAKE) install-executables
	$(MAKE) install-resources
	@echo
	@echo "Build complete."

.PHONY: clean
clean:
	$(MAKE) clean-resources
	$(MAKE) clean-executables
	$(MAKE) clean-frameworks
	$(RM) $(DEST_APP_TOP)/PkgInfo
	$(RM) $(DEST_APP_TOP)/Info.plist
	$(RM) $(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist
	-$(RMDIR) $(SRC_PREFSCONVERTER_BUILD_TOP)
	-$(RMDIR) $(SRC_BUGREPORTER_BUILD_TOP)
	-$(RMDIR) $(SYMROOT)/ForLeopardDebug
	-$(RMDIR) $(SYMROOT)/ForTigerDebug
	-$(RMDIR) $(SYMROOT)/ForPantherDebug
	-$(RMDIR) $(DEST_APP_TOP)
	-$(RMDIR) $(dir $(DEST_APP_TOP))
	@echo
	@echo "NOTE: Useful Xcode intermediate files in '$(SYMROOT)'"
	@echo "(such as code indices) have not been removed."
	@echo
	@echo "Clean complete."

FORCE:
