# GNU makefile for building code documentation.
#
# Kevin Grant (kmg@mac.com)
# March 26, 2005

# if "brew" is used ("brew install doxygen", "brew install graphviz")
# then the required tools will be in /usr/local/bin
PATH := /usr/local/bin:$(PATH)
OUTPUT_DIR_APP := $(CURDIR)/docs-c++/
OUTPUT_DIR_PY := $(CURDIR)/docs-python/

# this makes minor tweaks to the appearance of source files in C++
# or Objective-C before they are processed by Doxygen; the number
# of lines dumped from the filter should not change
FILTER_COMMAND_CODE := $(CURDIR)/../Tools/DoxygenCodeFilter.pl

# this assumes you have installed "doxypy" in the indicated location;
# you can download it (GPL'd) from https://github.com/Feneric/doxypypy
# or use "pip install doxypy"
FILTER_COMMAND_PY := python /usr/local/bin/doxypy.py

# determine application version
# IMPORTANT: read from a built Info.plist only;
# if you don't build the bundle first, this won't work
VERSION_INFO := $(shell $(CURDIR)/../VersionInfo.sh)
MAJOR := $(word 1,$(VERSION_INFO))
MINOR := $(word 2,$(VERSION_INFO))
SUBMINOR := $(word 3,$(VERSION_INFO))
ABR := $(word 4,$(VERSION_INFO))
BUILD := $(word 5,$(VERSION_INFO))

YYYYMMDD := $(shell date "+%Y-%m-%d")
LAST_UPDATED_DATE := $(shell date "+%A, %B %e, %Y")

DOXYGEN := doxygen
DOXYGEN_CONF_TEMPLATE := $(CURDIR)/Resources/Doxygen.conf.in
TMP_CONF := /tmp/Doxygen.conf

PY_SRC := $(CURDIR)/../MacTerm.app/Contents/Frameworks/MacTermQuills.framework/Versions/B/lib/python3.9/Quills.py
ifeq ($(strip $(PY_SRC)),)
$(warning Failed to find "Quills.py"; Python generation will have no effect!)
endif
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
	@# C++ API
	$(begin_target)
	$(config_for_app)
	install -d "$(strip $(OUTPUT_DIR_APP))"
	@# generate HTML from C++ source
	$(DOXYGEN) "$(strip $(TMP_CONF))"
	@# throw away configuration file (no longer needed)
	$(RM) "$(strip $(TMP_CONF))"
	@# post-processing
	$(custom_index_for_app)
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
	$(custom_index_for_py)
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
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_APP)/*.html),$(call post_process_html,$(FILE)))

.PHONY: postprocess_py
postprocess_py:
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_PY)/*.html),$(call post_process_html,$(FILE)))

.PHONY: index
index:
	$(custom_index_for_app)
	$(custom_index_for_py)

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
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_APP)/*.html),$(call post_process_html,$(FILE)))
	$(foreach FILE,$(wildcard $(OUTPUT_DIR_PY)/*.html),$(call post_process_html,$(FILE)))

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
	@# substitute values appropriate for the C++ documentation
	@echo "Creating Doxygen configuration file for C++ by substituting template variables..."
	$(RM) "$(strip $(TMP_CONF))"
	cp "$(strip $(DOXYGEN_CONF_TEMPLATE))" "$(strip $(TMP_CONF))"
	@# replace template variables with actual values
	perl -pi -e "\
			s|===PROJECT===|\"MacTerm C++ Code\"|; \
			s|===FILTER_YES_NO===||; \
			s|===FILTER_COMMAND===|$(strip $(FILTER_COMMAND_CODE))|; \
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
			s|===PROJECT===|\"Quills - MacTerm Python API\"|; \
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
		"$(strip $(STYLE_SRC))/doxygen.css" \
		"$(strip $(STYLE_SRC))/override.css" \
		"$(strip $(IMG_SRC))/LogoBlurFull.jpg" \
		"$(strip $(1))/"

endef

define custom_index_for_app
	@# replace Doxygen-generated index page with something a bit nicer
	@echo "Creating custom index file by substituting template variables..."
	cp "$(strip $(DOC_SRC))/index-app.html.in" "$(strip $(OUTPUT_DIR_APP))/index.html"
	perl -pi -e "s|===VERSION===|$(MAJOR).$(MINOR).$(SUBMINOR)$(ABR), build $(BUILD)|" \
			"$(strip $(OUTPUT_DIR_APP))/index.html"

endef

define custom_index_for_py
	@# replace Doxygen-generated index page with something a bit nicer
	@echo "Creating custom index file by substituting template variables..."
	cp "$(strip $(DOC_SRC))/index-py.html.in" "$(strip $(OUTPUT_DIR_PY))/index.html"
	perl -pi -e "s|===VERSION===|$(MAJOR).$(MINOR).$(SUBMINOR)$(ABR), build $(BUILD)|" \
			"$(strip $(OUTPUT_DIR_PY))/index.html"

endef

define open_browser_to_page
	open "file://$(strip $(1))"
endef

define post_process_html
	@# based on what Doxygen 1.8.13 generates
	@echo "Post-processing Doxygen-generated files..."
	perl -pi -e "\
			s|===YYYYMMDD===|$(YYYYMMDD)|; \
			s|===LAST_UPDATED_DATE===|$(LAST_UPDATED_DATE)|; \
			"\
			"$(1)"

endef

