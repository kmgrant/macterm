# GNU makefile for building code documentation.
#
# Kevin Grant (kmg@mac.com)
# March 26, 2005

PATH := /opt/doxygen/bin:/Applications/Doxygen.app/Contents/Resources:/Applications/GraphViz.app/Contents/MacOS:$(PATH)
OUTPUT_DIR_APP := $(CURDIR)/docs-c++/
OUTPUT_DIR_PY := $(CURDIR)/docs-python/

# this assumes you have installed "doxypy" in the indicated location;
# you can download it (GPL'd) from http://code.foosel.org/doxypy
FILTER_COMMAND_PY := python /opt/doxypy/bin/doxypy.py

# determine MacTelnet version
# IMPORTANT: read from a built MacTelnet.app's Info.plist;
# if you don't build MacTelnet first, this won't work
VERSION_INFO := $(shell $(CURDIR)/../VersionInfo.sh)
MAJOR := $(word 1,$(VERSION_INFO))
MINOR := $(word 2,$(VERSION_INFO))
SUBMINOR := $(word 3,$(VERSION_INFO))
ABR := $(word 4,$(VERSION_INFO))
BUILD := $(word 5,$(VERSION_INFO))

DOXYGEN := doxygen
DOXYGEN_CONF_TEMPLATE := $(CURDIR)/Resources/Doxygen.conf.in
TMP_CONF := /tmp/Doxygen.conf

PY_SRC := $(CURDIR)/../MacTelnet.app/Contents/Frameworks/Quills.framework/Versions/B/lib/python2.5/Quills.py
APP_INCL := $(CURDIR)/../Application/Code/
APP_SRC := $(CURDIR)/../Application/Code/
DOC_SRC := $(CURDIR)/Resources/
IMG_SRC := $(CURDIR)/../Shared/Resources/
SHARED_INCL := $(CURDIR)/../Shared/Code/
SHARED_SRC := $(CURDIR)/../Shared/Code/
STYLE_SRC := $(CURDIR)/Resources/

.PHONY: all
all:
	$(begin_target)
	$(MAKE) build_app
	$(MAKE) build_py
	$(end_target)

.PHONY: build_app
build_app:
	@# MacTelnet (C++) API
	$(begin_target)
	$(config_for_app)
	install -d "$(strip $(OUTPUT_DIR_APP))"
	@# generate HTML from C++ source
	$(DOXYGEN) "$(strip $(TMP_CONF))"
	@# throw away configuration file (no longer needed)
	$(RM) "$(strip $(TMP_CONF))"
	@# post-processing
	$(xcustom_index)
	@# note that postprocess depends on $(wildcard), which is resolved at
	@# the time the makefile is parsed; a recursive "make" ensures the
	@# wildcard is parsed only after Doxygen has actually created files!
	$(MAKE) postprocess_app
	$(call copy_resources, $(OUTPUT_DIR_APP))
	@# show the results in a browser!
	$(MAKE) view_app
	$(end_target)

.PHONY: build_py
build_py:
	@# Quills (Python) API
	$(begin_target)
	$(config_for_py)
	install -d "$(strip $(OUTPUT_DIR_PY))"
	@# generate HTML from Python source
	$(DOXYGEN) "$(strip $(TMP_CONF))"
	@# throw away configuration file (no longer needed)
	$(RM) "$(strip $(TMP_CONF))"
	@# post-processing
	$(xcustom_index)
	@# note that postprocess depends on $(wildcard), which is resolved at
	@# the time the makefile is parsed; a recursive "make" ensures the
	@# wildcard is parsed only after Doxygen has actually created files!
	$(MAKE) postprocess_py
	$(call copy_resources, $(OUTPUT_DIR_PY))
	@# show the results in a browser!
	$(MAKE) view_py
	$(end_target)

.PHONY: view_app
view_app:
	$(call open_browser_to_page,$(strip $(OUTPUT_DIR_APP))/index.html)

.PHONY: view_py
view_py:
	$(call open_browser_to_page,$(strip $(OUTPUT_DIR_PY))/index.html)

.PHONY: postprocess_app
postprocess_app:
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_APP)/*.html),$(call strip_unwanted_sections,$(FILE)))

.PHONY: postprocess_py
postprocess_py:
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_PY)/*.html),$(call strip_unwanted_sections,$(FILE)))

.PHONY: index
index:
	$(custom_index)

.PHONY: clean_app
clean_app:
	$(begin_target)
	$(RM) "$(OUTPUT_DIR_APP)"/*
	-rmdir "$(OUTPUT_DIR_APP)"
	$(end_target)

.PHONY: clean_py
clean_py:
	$(begin_target)
	$(RM) "$(OUTPUT_DIR_PY)"/*
	-rmdir "$(OUTPUT_DIR_PY)"
	$(end_target)

.PHONY: clean
clean:
	$(MAKE) clean_py
	$(MAKE) clean_app

.PHONY: test_strip
test_strip:
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_APP)/*.html),$(call strip_unwanted_sections,$(FILE)))
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_PY)/*.html),$(call strip_unwanted_sections,$(FILE)))

define begin_target
	@date "+      $(notdir $@): Started at %T."
endef

define end_target
	@date "+Finished $(notdir $@)."
endef

define config_common
	@# update the Doxygen configuration file with settings common to
	@# any type of documentation being created for this project
	@echo "Substituting common template variables..."
	perl -pi -e "\
			s|===HTML_SOURCE===|$(strip $(DOC_SRC))|; \
			s|===VERSION===|$(MAJOR).$(MINOR).$(SUBMINOR)$(ABR), build $(BUILD)|; \
			"\
			"$(strip $(TMP_CONF))"

endef

define config_for_app
	@# create the Doxygen configuration file that will actually be used;
	@# substitute values appropriate for MacTelnet C++ documentation
	@echo "Creating Doxygen configuration file for MacTelnet C++ by substituting template variables..."
	$(RM) "$(strip $(TMP_CONF))"
	cp "$(strip $(DOXYGEN_CONF_TEMPLATE))" "$(strip $(TMP_CONF))"
	@# replace template variables with actual values
	perl -pi -e "\
			s|===PROJECT===|\"MacTelnet C++ Code\"|; \
			s|===FILTER_YES_NO===|NO|; \
			s|===FILTER_COMMAND===||; \
			s|===APP_SOURCE===|$(strip $(APP_SRC))|; \
			s|===APP_INCL===|$(strip $(APP_INCL))|; \
			s|===OUTPUT_DIR===|$(strip $(OUTPUT_DIR_APP))|; \
			s|===SHARED_SOURCE===|$(strip $(SHARED_SRC))|; \
			s|===SHARED_INCL===|$(strip $(SHARED_INCL))|; \
			"\
			"$(strip $(TMP_CONF))"
	$(config_common)

endef

define config_for_py
	@# create the Doxygen configuration file that will actually be used;
	@# substitute values appropriate for Python (Quills) documentation
	@echo "Creating Doxygen configuration file for Quills Python by substituting template variables..."
	$(RM) "$(strip $(TMP_CONF))"
	cp "$(strip $(DOXYGEN_CONF_TEMPLATE))" "$(strip $(TMP_CONF))"
	@# make sure this command exists
	@$(FILTER_COMMAND_PY) --help > /dev/null 2>&1 || (echo "failed: $(FILTER_COMMAND_PY) (maybe not installed; see FILTER_COMMAND_PY variable)" && exit 1)
	@# replace template variables with actual values
	perl -pi -e "\
			s|===PROJECT===|\"Quills - MacTelnet Python API\"|; \
			s|===FILTER_YES_NO===|YES|; \
			s|===FILTER_COMMAND===|\"$(FILTER_COMMAND_PY)\"|; \
			s|===APP_SOURCE===|$(strip $(PY_SRC))|; \
			s|===APP_INCL===||; \
			s|===OUTPUT_DIR===|$(strip $(OUTPUT_DIR_PY))|; \
			s|===SHARED_SOURCE===||; \
			s|===SHARED_INCL===||; \
			"\
			"$(strip $(TMP_CONF))"
	$(config_common)

endef

# $(1) = destination directory
define copy_resources
	@# copy in style sheets and the images that they refer to
	@echo "Copying images and style sheets..."
	cp \
		"$(strip $(STYLE_SRC))/code.css" \
		"$(strip $(STYLE_SRC))/tabs.css" \
		"$(strip $(STYLE_SRC))/default.css" \
		"$(strip $(IMG_SRC))/LogoBlurFull.jpg" \
		"$(strip $(IMG_SRC))/Metal4x60.png" \
		"$(strip $(IMG_SRC))/SubtleBorderTop.png" \
		"$(strip $(IMG_SRC))/TabLeft.png" \
		"$(strip $(IMG_SRC))/TabRight.png" \
		"$(strip $(IMG_SRC))/TabBottom.png" \
		"$(strip $(IMG_SRC))/Translucent64x64.png" \
		"$(strip $(IMG_SRC))/White40Translucent64x64.png" \
		"$(strip $(IMG_SRC))/White60Translucent64x64.png" \
		"$(strip $(1))/"

endef

define custom_index
	@# replace Doxygen-generated index page with something a bit nicer
	@echo "Creating custom index file by substituting template variables..."
	cp "$(strip $(DOC_SRC))/index.html.in" "$(strip $(OUTPUT_DIR))/index.html"
	perl -pi -e "s|===VERSION===|$(MAJOR).$(MINOR).$(SUBMINOR)$(ABR), build $(BUILD)|" \
			"$(strip $(OUTPUT_DIR))/index.html"

endef

define open_browser_to_page
	echo 'open location "file://$(strip $(1))"' | osascript
endef

define strip_unwanted_sections
	@# based on what Doxygen 1.4.7 generates
	@echo "Stripping unwanted parts from Doxygen-generated files..."
	@# The command below "bumps down" heading tags generated by Doxygen; where Doxygen says
	@# "<h1>" it becomes "<h3>", etc. since "Resources/header.html.in" chooses to create
	@# base "<h1>" and "<h2>" headings of its own.  A "<!-- template -->" suffix comment is
	@# placed after all header tags from "header.html.in", so that they can be avoided here.
	perl -pi -e "\
			s|<h4>([^<]+)</h4>(?!<\!\-\- template)|<h6>\1</h6>|g; \
			s|<h3>([^<]+)</h3>(?!<\!\-\- template)|<h5>\1</h5>|g; \
			s|<h2>([^<]+)</h2>(?!<\!\-\- template)|<h4>\1</h4>|g; \
			s|<h1>([^<]+)</h1>(?!<\!\-\- template)|<h3>\1</h3>|g; \
			s|(\"memname\">[^\:]+\:\:)|\1<br>|g; \
			s|(\"memname\"><a.*?</a>)|\1<br>|g; \
			s|(\"memname\">\s*[^<]\S+)(\s+)|\1<br>\2|g; \
			s|<td>\(</td>|<td valign=\"bottom\">(</td>|g; \
			s|(<td class=\"paramtype\")|\1 valign=\"bottom\"|g; \
			s|(<td class=\"paramname\")|\1 valign=\"bottom\"|g; \
			s|<td>(\&nbsp\;)*?\)(\&nbsp\;)*?</td>|<td valign=\"bottom\">)</td>|g; \
			"\
			"$(1)"

endef

