# GNUmakefile
#
# Builds MacTerm's application bundle by running each project
# (in all required configurations) and copying the resulting
# components into place.
#
# In addition, the rule "make official" (not automatic) should
# be used at release time to sign the bundle that is going to be
# made available for general use.
#
# If you want to use an Xcode from a custom location, you can
# either open the project file from that Xcode, or set the path
# on the command line ("make DEVELOPER_DIR=...").  When run from
# Xcode the DEVELOPER_DIR is set automatically; by default on
# the command line, a DEVELOPER_DIR of /Developer is used.
#
# The build is done this way so that it is more straightforward
# to perform extra steps such as merging multiple architectures
# and versions of frameworks and installing secondary executables
# like the bug reporter and preferences converter.
#
# Kevin Grant (kmg@mac.com)
# December 14, 2007

MAKEFILE_DIR := $(dir $(firstword $(MAKEFILE_LIST)))
SHELL = /bin/bash
SRC_SHARED_TOP := $(MAKEFILE_DIR)/Shared

# IMPORTANT: The first rule in the file is chosen when "make" is
# run by itself.  Xcode runs "make" for external targets.
.PHONY: first
first: bundle

.PHONY: help
help:
	@echo "Usage: make [USE_GROWL=no]"
	@echo "       make official BUNDLE=/path/to/MacTerm.app
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
include $(SRC_SHARED_TOP)/CustomPaths.xcconfig

# The following should be defined by this "include":
#   CODE_SIGN_IDENTITY
#   DEVELOPER_ID_APP
#   DEVELOPMENT_TEAM
# It may also define signing-related environment variables.
include $(MAKEFILE_DIR)/Shared/Signature.xcconfig

# temporary files (subdirectories created)
#SYMROOT=<defined in Xcode config>
#OBJROOT=<defined in Xcode config>

XCODE_ENV := \
	SYMROOT=$(SYMROOT) \
	OBJROOT=$(OBJROOT)

# DEVELOPER_DIR should be set by Xcode's build environment;
# if not, or the directory does not exist, use Xcode's root
ifeq ($(strip $(shell /bin/test -d $(DEVELOPER_DIR) || echo "no")),no)
DEVELOPER_DIR := /Applications/Xcode.app/Contents/Developer
endif

# choose a build configuration...
BUILD_CONF := ForDebugging
#BUILD_CONF := ForRelease

# tools
CHMOD=/bin/chmod
CODE_SIGN=/usr/bin/codesign
COPY=/bin/cp
IBTOOL=$(DEVELOPER_DIR)/usr/bin/ibtool
# the ibtool options below might be useful for debugging
#IBTOOL_COMPILE_XIB_OPTIONS=--errors --warnings --notices --output-format human-readable-text --flatten NO
# the ibtool options below are meant to minimize space in released bundles
IBTOOL_COMPILE_XIB_OPTIONS=--errors --warnings --notices --output-format binary1 --flatten YES
MKDIR_P=/usr/bin/install -d
MOVE=/bin/mv
RMDIR=/bin/rmdir
RSYNC=/usr/bin/rsync
SPCTL=/usr/sbin/spctl
SYMLINK_REPLACE=/bin/ln -snf
XARGS=/usr/bin/xargs
XCB=$(DEVELOPER_DIR)/usr/bin/xcodebuild $(XCODE_ENV)

# set this to "no" if you want to disable Growl support for
# notifications in MacTerm (not recommended unless you don't
# want to download the Growl SDK); see "http://www.growl.info/"
USE_GROWL=yes
#GROWL_SDK_ROOT := <defined in Xcode config>

# the PyMacTerm.framework target is platform-independent, but an
# arbitrary configuration must be given to have Xcode build it
# (and decide the output directory from which to copy)
PY_CONFIG := $(BUILD_CONF)

SRC_APP_PROJ_TOP := $(MAKEFILE_DIR)/Application
SRC_BUGREPORTER_PROJ_TOP := $(MAKEFILE_DIR)/BugReporter
SRC_CALLPYCLIENT_PROJ_TOP := $(MAKEFILE_DIR)/CallPythonClient
SRC_PREFSCONVERTER_PROJ_TOP := $(MAKEFILE_DIR)/PrefsConverter
SRC_PRINTPREVIEW_PROJ_TOP := $(MAKEFILE_DIR)/PrintPreview
#SRC_SHARED_TOP is set at the beginning

SRC_APP_CODE_TOP := $(SRC_APP_PROJ_TOP)/Code
SRC_APP_PYCODE_TOP := $(SRC_APP_PROJ_TOP)/PythonCode
SRC_APP_RESOURCES_TOP := $(SRC_APP_PROJ_TOP)/Resources
SRC_BUGREPORTER_BUILD_TOP := $(SYMROOT)/$(BUILD_CONF)
SRC_BUGREPORTER_TOP := $(SRC_BUGREPORTER_BUILD_TOP)/BugReporter.app
SRC_CALLPYCLIENT_BUILD_TOP := $(SYMROOT)/$(BUILD_CONF)
SRC_CALLPYCLIENT_TOP := $(SRC_CALLPYCLIENT_BUILD_TOP)/CallPythonClient.xpc
SRC_PREFSCONVERTER_BUILD_TOP := $(SYMROOT)/$(BUILD_CONF)
SRC_PREFSCONVERTER_TOP := $(SRC_PREFSCONVERTER_BUILD_TOP)/PrefsConverter.app
SRC_PRINTPREVIEW_BUILD_TOP := $(SYMROOT)/$(BUILD_CONF)
SRC_PRINTPREVIEW_TOP := $(SRC_PRINTPREVIEW_BUILD_TOP)/PrintPreview.app
SRC_SHARED_CODE_TOP := $(SRC_SHARED_TOP)/Code
SRC_SHARED_RESOURCES_TOP := $(SRC_SHARED_TOP)/Resources
SRC_PY_TOP := $(SYMROOT)/$(PY_CONFIG)/PyMacTerm.framework
SRC_PYINVOKER_TOP_LEOPARD := $(SYMROOT)/$(BUILD_CONF)
SRC_GROWL_TOP := $(GROWL_SDK_ROOT)/Framework/Growl.framework
SRC_QUILLS_TOP := $(SYMROOT)/$(BUILD_CONF)/MacTermQuills.framework

DEST_APP_TOP := $(MAKEFILE_DIR)/MacTerm.app/Contents
DEST_APP_FW_TOP := $(DEST_APP_TOP)/Frameworks
DEST_APP_MACOS_TOP := $(DEST_APP_TOP)/MacOS
DEST_APP_RESOURCES_TOP := $(DEST_APP_TOP)/Resources
DEST_APP_XPCSERVICES_TOP := $(DEST_APP_TOP)/XPCServices
DEST_BUGREPORTER_TOP := $(DEST_APP_RESOURCES_TOP)/BugReporter.app
DEST_CALLPYCLIENT_TOP := $(DEST_APP_XPCSERVICES_TOP)/net.macterm.helpers.CallPythonClient.xpc
DEST_PREFSCONVERTER_TOP := $(DEST_APP_RESOURCES_TOP)/PrefsConverter.app
DEST_PRINTPREVIEW_TOP := $(DEST_APP_RESOURCES_TOP)/PrintPreview.app
DEST_PY_TOP := $(DEST_APP_FW_TOP)/PyMacTerm.framework
DEST_PY_CODE_TOP := $(DEST_PY_TOP)/Versions/A/lib/python2.6/pymacterm
DEST_QUILLS_TOP := $(DEST_APP_FW_TOP)/MacTermQuills.framework
DEST_GROWL_TOP := $(DEST_APP_FW_TOP)/Growl.framework
DEST_SWIG_TOP_LEOPARD := $(DEST_QUILLS_TOP)/Versions/B/lib/python2.6

SWIG_WRAPPERS := _quills.so quills.py
# .pyc files can be created by Python when code runs...not important
SWIG_WRAPPERS_EXTRA := quills.pyc

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
Application.sb \
CursorCrosshairs.png \
CursorCrosshairs@2x.png \
CursorIBeam.png \
CursorIBeam@2x.png\
CursorIBeamSmall.png \
CursorIBeamSmall@2x.png \
CursorMoveCursor.png \
CursorMoveCursor@2x.png \
CursorMoveCursorSmall.png \
CursorMoveCursorSmall@2x.png \
DefaultFormat1.plist \
DefaultFormat2.plist \
DefaultFormat3.plist \
DefaultFormat4.plist \
GlyphForPatternDarkGray.png \
GlyphForPatternLightGray.png \
GlyphForPatternMediumGray.png \
GrowlRegistrationTicket.growlRegDict \
IconForBellOff.icns \
IconForBellOn.icns \
IconForBundle.icns \
IconForCaution.icns \
IconForCommand.icns \
IconForContextMenu.icns \
IconForContextMenuSegment.icns \
IconForContextMenuSegment@2x.icns \
IconForCustomize.icns \
IconForFullScreen.icns \
IconForFullScreenDisable.icns \
IconForHide.icns \
IconForItemAdd.icns \
IconForItemAddMenuSegment.icns \
IconForItemAddMenuSegment@2x.icns \
IconForItemRemove.icns \
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
IconForPrefs.icns \
IconForPrint.icns \
IconForRefresh.png \
IconForRestartSession.icns \
IconForSearch.icns \
IconForSearchKey.icns \
IconForScrollLockOff.icns \
IconForScrollLockOn.icns \
IconForSession.icns \
IconForSessionStatusActive.icns \
IconForSessionStatusDead.icns \
IconForStackWindows.icns \
IconForTerminal.icns \
PreferencesSearch.plist \
English.lproj/Credits.html
APP_RESOURCES_BUNDLES := \
MacTerm.help \
English.lproj/TerminalWindow.nib
# set to a list of "English.lproj/<xyz>" paths, where there
# should be an <xyz>.xib source and implied <xyz>.nib output
APP_XIB_PREFIXES := \
English.lproj/AddressDialogCocoa \
English.lproj/AlertMessagesCocoa \
English.lproj/ApplicationTouchBarCocoa \
English.lproj/ClipboardCocoa \
English.lproj/CommandLineCocoa \
English.lproj/DebugInterfaceCocoa \
English.lproj/FindDialogCocoa \
English.lproj/GenericDialogCocoa \
English.lproj/GenericPanelNumberedListCocoa \
English.lproj/GenericPanelTabsCocoa \
English.lproj/InfoWindowCocoa \
English.lproj/InfoWindowTouchBarCocoa \
English.lproj/KeypadArrangeWindowCocoa \
English.lproj/KeypadControlKeysCocoa \
English.lproj/KeypadFullScreenCocoa \
English.lproj/KeypadFunctionKeysCocoa \
English.lproj/KeypadVT220KeysCocoa \
English.lproj/MainMenuCocoa \
English.lproj/PrefPanelFormatGeneralCocoa \
English.lproj/PrefPanelFormatStandardColorsCocoa \
English.lproj/PrefPanelFullScreenCocoa \
English.lproj/PrefPanelGeneralNotificationsCocoa \
English.lproj/PrefPanelGeneralOptionsCocoa \
English.lproj/PrefPanelGeneralSpecialCocoa \
English.lproj/PrefPanelMacrosCocoa \
English.lproj/PrefPanelSessionDataFlowCocoa \
English.lproj/PrefPanelSessionGraphicsCocoa \
English.lproj/PrefPanelSessionKeyboardCocoa \
English.lproj/PrefPanelSessionResourceCocoa \
English.lproj/PrefPanelTerminalEmulationCocoa \
English.lproj/PrefPanelTerminalOptionsCocoa \
English.lproj/PrefPanelTerminalScreenCocoa \
English.lproj/PrefPanelTranslationsCocoa \
English.lproj/PrefPanelWorkspaceOptionsCocoa \
English.lproj/PrefPanelWorkspaceWindowsCocoa \
English.lproj/PrefsWindowCocoa \
English.lproj/PrefsWindowTouchBarCocoa \
English.lproj/ResizeInfoCocoa \
English.lproj/ServerBrowserCocoa \
English.lproj/TerminalViewCocoa \
English.lproj/TerminalWindowCocoa \
English.lproj/TerminalWindowTouchBarCocoa \
English.lproj/VectorWindowCocoa \
English.lproj/VectorWindowTouchBarCocoa
SHARED_XIB_PREFIXES := \
English.lproj/WindowTitleDialogCocoa
APP_RESOURCES_BUNDLES_GENERATED := \
$(foreach PREFIX,$(APP_XIB_PREFIXES) $(SHARED_XIB_PREFIXES),$(PREFIX).nib)
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

# Component: MacTermQuills.framework

.PHONY: install-quills-framework
install-quills-framework:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project Application.xcodeproj -target MacTermQuills.framework -configuration $(BUILD_CONF)
	@# merge in the built frameworks to one root; start with the
	@# "preferred" version so that the framework structure
	@# generated by Xcode is already basically set up
	$(RSYNC) --archive $(SRC_QUILLS_TOP)/ $(DEST_QUILLS_TOP)
	# "Versions/A" is the legacy PowerPC-only Panther build that is no longer created
	@# add remaining parts
	$(MAKE) install-swig-wrappers
	$(SYMLINK_REPLACE) Versions/B/Resources $(DEST_QUILLS_TOP)/

.PHONY: clean-quills-framework
clean-quills-framework:
	$(MAKE) clean-swig-wrappers
	$(XCB) clean -project Application.xcodeproj -target MacTermQuills.framework -configuration $(BUILD_CONF)
	@# avoid accidental over-deletion
	@if [ "x$(DEST_QUILLS_TOP)" = "x" ] ; then \
		echo "DEST_QUILLS_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(RM) -R $(DEST_QUILLS_TOP)

$(SYMROOT)/$(BUILD_CONF)/_quills.so \
$(SYMROOT)/$(BUILD_CONF)/quills.py: $(SWIG_DEPENDENCIES)
	$(call banner,build SWIG wrapper: $(BUILD_CONF))
	@# if Python versions are different on each Mac OS, the lightweight
	@# SWIG wrapper code *does* have to be compiled for each separately
	$(XCB) -project Application.xcodeproj -target PythonWrapper -configuration $(BUILD_CONF)

# reinstalls a SWIG wrapper when it changes
$(DEST_SWIG_TOP_LEOPARD)/%: $(SYMROOT)/$(BUILD_CONF)/%
	$(MKDIR_P) $(dir $@)
	$(COPY) $< $@

.PHONY: install-swig-wrappers
install-swig-wrappers: \
$(LEOPARD_SWIG_WRAPPERS)
	@echo

.PHONY: clean-swig-wrappers
clean-swig-wrappers:
	$(XCB) clean -project Application.xcodeproj -target PythonWrapper -configuration $(BUILD_CONF)
	$(RM) $(SYMROOT)/$(BUILD_CONF)/quills.py
	echo '$(LEOPARD_SWIG_WRAPPERS) $(LEOPARD_SWIG_WRAPPERS_EXTRA)' | $(XARGS) $(RM)
	-$(RMDIR) $(DEST_SWIG_TOP_LEOPARD) 2>/dev/null
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
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "major_version = $$ENV{MY_MAJOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "minor_version = $$ENV{MY_MINOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "subminor_version = $$ENV{MY_SUBMINOR_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "alpha_beta = @{[ chr(39) ]}$$ENV{MY_ALPHA_BETA}@{[ chr(39) ]}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "build_number = $$ENV{MY_BUILD_NUMBER}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
	$(MAKEFILE_DIR)/VersionInfo.sh perl -e 'print "prefs_version = $$ENV{MY_PREFS_VERSION}\n"' >> $(DEST_PY_CODE_TOP)/versions.py
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
	@# (note: this rule has a check to avoid accidental rsyncing from '/')
	@if [ ! -d "$(GROWL_SDK_ROOT)" ] ; then \
		echo "WARNING: Support for Growl is NOT being compiled in, due to a missing framework." >&2 ; \
		echo "Current working directory: '$(MAKEFILE_DIR)';" >&2 ; \
		echo "first download the Growl SDK <http://growl.info/> and install in subdirectory '$(GROWL_SDK_ROOT)'." >&2 ; \
	else \
		if [ "x$(SRC_GROWL_TOP)" = "x" ] ; then \
			echo "SRC_GROWL_TOP variable unset" >&2 ; \
			exit 1 ; \
		fi ; \
		$(MKDIR_P) $(dir $(DEST_GROWL_TOP)) ; \
		$(RSYNC) --archive '$(SRC_GROWL_TOP)/' $(DEST_GROWL_TOP) ; \
		$(CHMOD) -R +w $(DEST_GROWL_TOP) ; \
	fi

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
	-$(RMDIR) $(DEST_APP_FW_TOP) 2>/dev/null

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
	$(XCB) -project Application.xcodeproj -target PythonInvoker -configuration $(BUILD_CONF)
	$(COPY) $(SRC_PYINVOKER_TOP_LEOPARD)/PythonInvoker* $(DEST_APP_MACOS_TOP)/MacTerm_python2.6_wrap

.PHONY: clean-executables
clean-executables:
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm
	$(RM) $(DEST_APP_MACOS_TOP)/RunApplication.py
	$(RM) $(DEST_APP_MACOS_TOP)/MacTerm_python2.6_wrap
	-$(RMDIR) $(DEST_APP_MACOS_TOP) 2>/dev/null
	$(XCB) clean -project Application.xcodeproj -target PythonInvoker -configuration $(BUILD_CONF)

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
	$(COPY) $(SRC_APP_RESOURCES_TOP)/English.lproj/PreferencesSearch.strings $(DEST_APP_RESOURCES_TOP)/English.lproj

.PHONY: clean-app-dot-strings
clean-app-dot-strings:
	$(foreach FILE,$(wildcard $(DEST_APP_RESOURCES_TOP)/English.lproj/*.strings),$(RM) $(FILE) ;)

# Component: Help

$(MAKEFILE_DIR)/_Generated/MacTerm.help:
	$(call banner,$@)
	$(MAKEFILE_DIR)/HTMLHelpBuild.command

$(DEST_APP_RESOURCES_TOP)/MacTerm.help: $(MAKEFILE_DIR)/_Generated/MacTerm.help
	$(call banner,$@)
	$(MKDIR_P) $(dir $@)
	$(RSYNC) --archive $(MAKEFILE_DIR)/_Generated/MacTerm.help/ $@

# Component: Bug Reporter

.PHONY: install-bug-reporter
install-bug-reporter:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project BugReporter.xcodeproj -configuration $(BUILD_CONF)
	@# avoid rsyncing from /
	@if [ "x$(SRC_BUGREPORTER_TOP)" = "x" ] ; then \
		echo "SRC_BUGREPORTER_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_BUGREPORTER_TOP))
	$(RSYNC) --archive $(SRC_BUGREPORTER_TOP)/ $(DEST_BUGREPORTER_TOP)

.PHONY: clean-bug-reporter
clean-bug-reporter:
	$(XCB) clean -project BugReporter.xcodeproj -configuration $(BUILD_CONF)
	$(RM) -R $(DEST_BUGREPORTER_TOP)

# Component: Canvas Client

.PHONY: install-callpy-client
install-callpy-client:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project CallPythonClient.xcodeproj -configuration $(BUILD_CONF)
	@# avoid rsyncing from /
	@if [ "x$(SRC_CALLPYCLIENT_TOP)" = "x" ] ; then \
		echo "SRC_CALLPYCLIENT_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_CALLPYCLIENT_TOP))
	$(RSYNC) --archive $(SRC_CALLPYCLIENT_TOP)/ $(DEST_CALLPYCLIENT_TOP)

.PHONY: clean-callpy-client
clean-callpy-client:
	$(XCB) clean -project CallPythonClient.xcodeproj -configuration $(BUILD_CONF)
	$(RM) -R $(DEST_CALLPYCLIENT_TOP)

# Component: Preferences Converter

.PHONY: install-prefs-converter
install-prefs-converter:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project PrefsConverter.xcodeproj -configuration $(BUILD_CONF)
	@# avoid rsyncing from /
	@if [ "x$(SRC_PREFSCONVERTER_TOP)" = "x" ] ; then \
		echo "SRC_PREFSCONVERTER_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_PREFSCONVERTER_TOP))
	$(RSYNC) --archive $(SRC_PREFSCONVERTER_TOP)/ $(DEST_PREFSCONVERTER_TOP)

.PHONY: clean-prefs-converter
clean-prefs-converter:
	$(XCB) clean -project PrefsConverter.xcodeproj -configuration $(BUILD_CONF)
	$(RM) -R $(DEST_PREFSCONVERTER_TOP)

# Component: Print Preview

.PHONY: install-print-preview
install-print-preview:
	$(call banner,$@)
	@# keep in sync with the matching clean rule
	$(XCB) -project PrintPreview.xcodeproj -configuration $(BUILD_CONF)
	@# avoid rsyncing from /
	@if [ "x$(SRC_PRINTPREVIEW_TOP)" = "x" ] ; then \
		echo "SRC_PRINTPREVIEW_TOP variable unset" >&2 ; \
		exit 1 ; \
	fi
	$(MKDIR_P) $(dir $(DEST_PRINTPREVIEW_TOP))
	$(RSYNC) --archive $(SRC_PRINTPREVIEW_TOP)/ $(DEST_PRINTPREVIEW_TOP)

.PHONY: clean-print-preview
clean-print-preview:
	$(XCB) clean -project PrintPreview.xcodeproj -configuration $(BUILD_CONF)
	$(RM) -R $(DEST_PRINTPREVIEW_TOP)

# Component: Miscellaneous Resources (NIBs, Images, etc.)

.PHONY: install-resources
install-resources: \
$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_COPIED))
	$(call banner,$@)
	@# build any NIBs that are based on XIBs
	$(foreach PREFIX,$(APP_XIB_PREFIXES),$(call compile_xib_named,$(PREFIX),$(SRC_APP_RESOURCES_TOP),$(DEST_APP_RESOURCES_TOP)))
	$(foreach PREFIX,$(SHARED_XIB_PREFIXES),$(call compile_xib_named,$(PREFIX),$(SRC_SHARED_RESOURCES_TOP),$(DEST_APP_RESOURCES_TOP)))
	@# keep in sync with the matching clean rule
	$(MKDIR_P) $(DEST_APP_RESOURCES_TOP)/English.lproj
	$(MAKE) install-app-dot-strings
	$(MAKE) install-bug-reporter
	$(MAKE) install-callpy-client
	$(MAKE) install-prefs-converter
	$(MAKE) install-print-preview
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
	$(MAKE) clean-print-preview
	$(MAKE) clean-prefs-converter
	$(MAKE) clean-callpy-client
	$(MAKE) clean-bug-reporter
	$(MAKE) clean-app-dot-strings
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP)/English.lproj 2>/dev/null
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP) 2>/dev/null

#
# DefaultPreferences.plist file
#

$(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes often and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-DefaultPreferences.plist $@
	env BUILT_PRODUCTS_DIR="$(DEST_APP_RESOURCES_TOP)" INFOPLIST_PATH=DefaultPreferences.plist \
		$(MAKEFILE_DIR)/VersionInfo.sh $(MAKEFILE_DIR)/Tools/AutoDatePropertyList.sh

#
# Info.plist file
#

$(DEST_APP_TOP)/Info.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes daily and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-Application-Info.plist $@
	env BUILT_PRODUCTS_DIR="$(DEST_APP_TOP)" INFOPLIST_PATH=Info.plist \
		$(MAKEFILE_DIR)/VersionInfo.sh $(MAKEFILE_DIR)/Tools/AutoDatePropertyList.sh

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

# sign a file with a "Developer ID Application" and verify the result
define sign_with_dev_id_app
	$(call banner,code sign $(1))
	$(CODE_SIGN) --deep --force --sign "$(DEVELOPER_ID_APP)" $(1)
	$(CODE_SIGN) --display --verbose=4 $(1)
	$(SPCTL) --assess --verbose=4 --raw $(1)
	$(SPCTL) --assess --verbose=4 $(1)
	
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
	$(RM) $(DEST_APP_TOP)/Info.plist
	$(RM) $(DEST_APP_TOP)/PkgInfo
	$(RM) $(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist
	-$(RMDIR) $(SRC_PRINTPREVIEW_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_PREFSCONVERTER_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_CALLPYCLIENT_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_BUGREPORTER_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SYMROOT)/$(BUILD_CONF) 2>/dev/null
	-$(RMDIR) $(DEST_APP_TOP) 2>/dev/null
	-$(RMDIR) $(dir $(DEST_APP_TOP)) 2>/dev/null
	@echo
	@echo "NOTE: Useful Xcode intermediate files in '$(SYMROOT)'"
	@echo "(such as code indices) have not been removed."
	@echo
	@echo "Clean complete."

.PHONY: official
official:
	@if [ "x$(BUNDLE)" = "x" ] ; then \
		echo "BUNDLE=/path/to/MacTerm.app must be set" >&2 ; \
		exit 1; \
	fi
	@# The existing signature for Growl.framework causes problems
	@# when signing the bundle and I could find no work-around
	@# except to delete that signature and then sign everything
	@# (including the copy of Growl.framework being used) under
	@# the same developer identity.
	$(RM) -R $(BUNDLE)/Contents/Frameworks/Growl.framework/Versions/Current/_CodeSignature
	$(call sign_with_dev_id_app, $(BUNDLE))

FORCE:
