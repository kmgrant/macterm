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
	@echo "Usage: make"
	@echo "       make official BUNDLE=/path/to/MacTerm.app"
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
ACTOOL=$(DEVELOPER_DIR)/usr/bin/actool
ACTOOL_COMPILE_CAR_OPTIONS=--errors --warnings --notices --output-format xml1 --minimum-deployment-target 10.10 --platform macosx
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
TIFFUTIL=/usr/bin/tiffutil
XARGS=/usr/bin/xargs
XCB=$(DEVELOPER_DIR)/usr/bin/xcodebuild $(XCODE_ENV)

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
SRC_QUILLS_TOP := $(SYMROOT)/$(BUILD_CONF)/MacTermQuills.framework

DEST_APP_TOP := $(MAKEFILE_DIR)/MacTerm.app
DEST_APP_CONTENTS_TOP := $(DEST_APP_TOP)/Contents
DEST_APP_FW_TOP := $(DEST_APP_CONTENTS_TOP)/Frameworks
DEST_APP_MACOS_TOP := $(DEST_APP_CONTENTS_TOP)/MacOS
DEST_APP_RESOURCES_TOP := $(DEST_APP_CONTENTS_TOP)/Resources
DEST_APP_XPCSERVICES_TOP := $(DEST_APP_CONTENTS_TOP)/XPCServices
DEST_BUGREPORTER_TOP := $(DEST_APP_RESOURCES_TOP)/BugReporter.app
DEST_CALLPYCLIENT_TOP := $(DEST_APP_XPCSERVICES_TOP)/net.macterm.helpers.CallPythonClient.xpc
DEST_PREFSCONVERTER_TOP := $(DEST_APP_RESOURCES_TOP)/PrefsConverter.app
DEST_PRINTPREVIEW_TOP := $(DEST_APP_RESOURCES_TOP)/PrintPreview.app
DEST_PY_TOP := $(DEST_APP_FW_TOP)/PyMacTerm.framework
DEST_PY_CODE_TOP := $(DEST_PY_TOP)/Versions/A/lib/python2.6/pymacterm
DEST_QUILLS_TOP := $(DEST_APP_FW_TOP)/MacTermQuills.framework
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
DefaultFormat1.plist \
DefaultFormat2.plist \
DefaultFormat3.plist \
DefaultFormat4.plist \
DefaultFormat5.plist \
DefaultFormat6.plist \
DefaultFormat7.plist \
DefaultFormat8.plist \
DefaultFormat9.plist \
DefaultFormat10.plist \
GlyphForPatternDarkGray.png \
GlyphForPatternLightGray.png \
GlyphForPatternMediumGray.png \
IconForBundle.icns \
IconForCommand.icns \
IconForContextMenu.icns \
IconForLEDOff.icns \
IconForLEDOn.icns \
IconForMacroSet.icns \
IconForPrefs.icns \
IconForSession.icns \
IconForSessionStatusActive.icns \
IconForSessionStatusDead.icns \
IconForTerminal.icns \
PreferencesSearch.plist \
Base.lproj/Credits.html
APP_RESOURCES_BUNDLES := \
MacTerm.help
# set to a list of files in "MacTermQuills.framework/Resources"
# that should move to "MacTerm.app/Contents/Resources" (used for
# Xcode-generated images)
APP_RESOURCES_PREFIXES_MULTI_DPI_PNG := \
CursorBlackCrosshairs \
CursorBlackIBeam \
CursorBlackIBeamSmall \
CursorBlackMoveCursor \
CursorBlackMoveCursorSmall \
CursorCrosshairs \
CursorIBeam \
CursorIBeamSmall \
CursorMoveCursor \
CursorMoveCursorSmall \
CursorWhiteCrosshairs \
CursorWhiteIBeam \
CursorWhiteIBeamSmall \
CursorWhiteMoveCursor \
CursorWhiteMoveCursorSmall
# set to a list of "Base.lproj/<xyz>" paths, where there
# should be an <xyz>.xib source and implied <xyz>.nib output
APP_XIB_PREFIXES := \
Base.lproj/AlertMessagesCocoa \
Base.lproj/ApplicationTouchBarCocoa \
Base.lproj/ClipboardCocoa \
Base.lproj/CommandLineCocoa \
Base.lproj/FindDialogCocoa \
Base.lproj/GenericDialogCocoa \
Base.lproj/GenericPanelNumberedListCocoa \
Base.lproj/GenericPanelTabsCocoa \
Base.lproj/InfoWindowCocoa \
Base.lproj/InfoWindowTouchBarCocoa \
Base.lproj/MainMenuCocoa \
Base.lproj/PrefPanelFormatGeneralCocoa \
Base.lproj/PrefPanelFormatStandardColorsCocoa \
Base.lproj/PrefPanelMacrosCocoa \
Base.lproj/PrefPanelSessionDataFlowCocoa \
Base.lproj/PrefPanelSessionKeyboardCocoa \
Base.lproj/PrefPanelSessionResourceCocoa \
Base.lproj/PrefPanelTranslationsCocoa \
Base.lproj/PrefPanelWorkspaceWindowsCocoa \
Base.lproj/PrefsWindowCocoa \
Base.lproj/PrefsWindowTouchBarCocoa \
Base.lproj/ServerBrowserCocoa \
Base.lproj/TerminalRootCocoa \
Base.lproj/TerminalScrollableRootViewCocoa \
Base.lproj/TerminalViewCocoa \
Base.lproj/TerminalWindowCocoa \
Base.lproj/TerminalWindowTouchBarCocoa \
Base.lproj/VectorWindowCocoa \
Base.lproj/VectorWindowTouchBarCocoa
SHARED_XIB_PREFIXES := \
Base.lproj/WindowTitleDialogCocoa
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
	@if [ -d '$<' ] ; then \
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
	$(MAKE) install-combined-hi-res-images
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

# moves Xcode-generated ".tiff" files to the main bundle
.PHONY: install-combined-hi-res-images
install-combined-hi-res-images:
	$(foreach PREFIX,$(APP_RESOURCES_PREFIXES_MULTI_DPI_PNG),$(TIFFUTIL) -cathidpicheck $(SRC_APP_RESOURCES_TOP)/$(PREFIX).png $(SRC_APP_RESOURCES_TOP)/$(PREFIX)@2x.png -out $(DEST_APP_RESOURCES_TOP)/$(PREFIX).tiff ;)

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

# Composite: Frameworks

.PHONY: install-frameworks
install-frameworks: \
install-py-framework \
install-quills-framework
	@# keep in sync with the matching clean rule
	@echo

.PHONY: clean-frameworks
clean-frameworks: \
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
	$(MKDIR_P) $(DEST_APP_RESOURCES_TOP)/Base.lproj
	/usr/bin/genstrings $(SRC_APP_CODE_TOP)/UIStrings.cp -o $(DEST_APP_RESOURCES_TOP)/Base.lproj
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Base.lproj/PreferencesSearch.strings $(DEST_APP_RESOURCES_TOP)/Base.lproj

.PHONY: clean-app-dot-strings
clean-app-dot-strings:
	$(foreach FILE,$(wildcard $(DEST_APP_RESOURCES_TOP)/Base.lproj/*.strings),$(RM) $(FILE) ;)

# Component: Help

$(MAKEFILE_DIR)/_Generated/MacTerm.help:
	$(call banner,$@)
	$(MAKEFILE_DIR)/HTMLHelpBuild.command

$(DEST_APP_RESOURCES_TOP)/MacTerm.help: $(MAKEFILE_DIR)/_Generated/MacTerm.help
	$(call banner,$@)
	$(MKDIR_P) $(dir $@)
	$(RSYNC) --archive $(MAKEFILE_DIR)/_Generated/MacTerm.help/ $@

# Component: Combined Asset Catalog

$(MAKEFILE_DIR)/_Generated/Merged.xcassets: FORCE
	$(call banner,$@)
	$(MKDIR_P) $@
	@# the "Contents.json" top-level file appears in all ".xcassets" sources but it is always the same
	$(RSYNC) --archive $(SRC_APP_RESOURCES_TOP)/ApplicationMedia.xcassets/ $@
	$(RSYNC) --archive $(SRC_SHARED_RESOURCES_TOP)/SharedMedia.xcassets/ $@

$(DEST_APP_RESOURCES_TOP)/Assets.car: FORCE
	$(MAKE) $(MAKEFILE_DIR)/_Generated/Merged.xcassets
	$(call banner,$@)
	$(MKDIR_P) $(dir $@)
	$(ACTOOL) $(ACTOOL_COMPILE_CAR_OPTIONS) --compile $(dir $@) $(MAKEFILE_DIR)/_Generated/Merged.xcassets

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
$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_COPIED)) \
$(DEST_APP_RESOURCES_TOP)/Assets.car
	$(call banner,$@)
	@# build any NIBs that are based on XIBs
	$(foreach PREFIX,$(APP_XIB_PREFIXES),$(call compile_xib_named,$(PREFIX),$(SRC_APP_RESOURCES_TOP),$(DEST_APP_RESOURCES_TOP)))
	$(foreach PREFIX,$(SHARED_XIB_PREFIXES),$(call compile_xib_named,$(PREFIX),$(SRC_SHARED_RESOURCES_TOP),$(DEST_APP_RESOURCES_TOP)))
	@# keep in sync with the matching clean rule
	$(MKDIR_P) $(DEST_APP_RESOURCES_TOP)/Base.lproj
	$(MAKE) install-app-dot-strings
	$(MAKE) install-bug-reporter
	$(MAKE) install-callpy-client
	$(MAKE) install-prefs-converter
	$(MAKE) install-print-preview

.PHONY: clean-resources
clean-resources:
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_FILES))' | $(XARGS) $(RM)
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_BUNDLES))' | $(XARGS) $(RM) -R
	echo '$(addprefix $(DEST_APP_RESOURCES_TOP)/,$(APP_RESOURCES_BUNDLES_GENERATED))' | $(XARGS) $(RM) -R
	$(RM) $(DEST_APP_RESOURCES_TOP)/Assets.car
	$(MAKE) clean-print-preview
	$(MAKE) clean-prefs-converter
	$(MAKE) clean-callpy-client
	$(MAKE) clean-bug-reporter
	$(MAKE) clean-app-dot-strings
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP)/Base.lproj 2>/dev/null
	-$(RMDIR) $(DEST_APP_RESOURCES_TOP) 2>/dev/null

#
# DefaultPreferences.plist file
#

$(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes often and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-DefaultPreferences.plist $@
	env BUILT_PRODUCTS_DIR='$(DEST_APP_RESOURCES_TOP)' INFOPLIST_PATH=DefaultPreferences.plist \
		$(MAKEFILE_DIR)/VersionInfo.sh $(MAKEFILE_DIR)/Tools/AutoDatePropertyList.sh

#
# Info.plist file
#

$(DEST_APP_CONTENTS_TOP)/Info.plist: FORCE
	$(MKDIR_P) $(dir $@)
	@# this is always regenerated because it changes daily and is a tiny file
	$(COPY) $(SRC_APP_RESOURCES_TOP)/Template-Application-Info.plist $@
	env BUILT_PRODUCTS_DIR='$(DEST_APP_CONTENTS_TOP)' INFOPLIST_PATH=Info.plist \
		$(MAKEFILE_DIR)/VersionInfo.sh $(MAKEFILE_DIR)/Tools/AutoDatePropertyList.sh

#
# PkgInfo file
#

$(DEST_APP_CONTENTS_TOP)/PkgInfo: FORCE
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
	$(call banner,code sign '$(strip $(1))')
	$(CODE_SIGN) --deep --force --sign '$(DEVELOPER_ID_APP)' --timestamp '$(strip $(1))'
	$(CODE_SIGN) --display --verbose=4 '$(strip $(1))'
	$(CODE_SIGN) --verify --deep --strict --verbose=2 '$(strip $(1))'
	$(SPCTL) --assess --verbose=4 --raw '$(strip $(1))'
	$(SPCTL) --assess --verbose=4 '$(strip $(1))'
	
endef

#
# Primary Rules
#

.PHONY: bundle
bundle:
	@# just comment-out anything you don't care about; MacTerm
	@# still works with most of its components missing
	$(MAKE) $(DEST_APP_CONTENTS_TOP)/Info.plist
	$(MAKE) $(DEST_APP_CONTENTS_TOP)/PkgInfo
	$(MAKE) $(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist
	$(MAKE) install-frameworks
	$(MAKE) install-executables
	$(MAKE) install-resources
	env BUNDLE='$(DEST_APP_TOP)' $(MAKE) official
	@echo
	@echo "Build complete."

.PHONY: clean
clean:
	$(MAKE) clean-resources
	$(MAKE) clean-executables
	$(MAKE) clean-frameworks
	$(RM) $(DEST_APP_CONTENTS_TOP)/Info.plist
	$(RM) $(DEST_APP_CONTENTS_TOP)/PkgInfo
	$(RM) $(DEST_APP_RESOURCES_TOP)/DefaultPreferences.plist
	-$(RMDIR) $(SRC_PRINTPREVIEW_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_PREFSCONVERTER_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_CALLPYCLIENT_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SRC_BUGREPORTER_BUILD_TOP) 2>/dev/null
	-$(RMDIR) $(SYMROOT)/$(BUILD_CONF) 2>/dev/null
	-$(RMDIR) $(DEST_APP_CONTENTS_TOP) 2>/dev/null
	-$(RMDIR) $(DEST_APP_TOP) 2>/dev/null
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
	$(call sign_with_dev_id_app, $(BUNDLE))

FORCE:
