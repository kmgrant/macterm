# GNU makefile for building the help bundle.
#
# DESIGN OVERVIEW:
# - primary rules are listed first
# - patterns (foobar-%) are used for language-specific rules
# - in general, you can use the first several variables to
#   customize/filter what this will build (e.g. language set)
# - clean rules DO NOT USE RECURSIVE DELETE; this ensures that
#   every step that is done is explicitly undone (the results are
#   both safer and more robust, if painful to set up properly)
#
# Kevin Grant (kmg@mac.com)
# March 26, 2005

DEST = $(CURDIR)/_Generated
MAIN_SRC = $(CURDIR)/Resources/
define MAIN_SRC_SUB
$(strip $(MAIN_SRC))/$(strip $(1))
endef
SHARED_SRC = $(CURDIR)/../Shared/Resources

ALL_LANGUAGES = English
MODULES := $(filter-out index,$(sort $(notdir $(subst .textile,,$(wildcard $(strip $(MAIN_SRC))/English/*.textile)))))

# use these within "xyz-%" rules where the pattern is the language (e.g. English)
define APP_HELP_LOCALIZED_ROOT
$(strip $(DEST))/$*
endef
define APP_HELP_LOCALIZED
$(strip $(DEST))/$*/Help
endef
define APP_HELP_LOCALIZED_CONTENT
$(strip $(DEST))/$*/Help/content
endef

DOCTYPER=$(CURDIR)/Tools/PrintHTMLDocType.pl
GENERATOR=$(CURDIR)/Tools/html_from_textile.py
KV_TO_HEAD=$(CURDIR)/Tools/KeyValueToHTMLHeadTags.pl
KV_TO_PLIST=$(CURDIR)/Tools/KeyValueToExactMatchProperties.pl

define SOURCE_CSS
$(call MAIN_SRC_SUB,$(1))/default.css \
$(call MAIN_SRC_SUB,$(1))/main.css
endef

define SOURCE_HTML
$(foreach PREFIX,$(strip $(MODULES)),$(strip $(MAIN_SRC))/$(1)/$(strip $(PREFIX)).html)
endef

define SOURCE_INDEX_HTML
$(foreach PREFIX,index,$(strip $(MAIN_SRC))/$(1)/$(strip $(PREFIX)).html)
endef

define SOURCE_IMG
$(wildcard $(call MAIN_SRC_SUB,$(1))/*.png) \
$(wildcard $(strip $(SHARED_SRC))/*.jpg) \
$(wildcard $(strip $(SHARED_SRC))/*.png)
endef

define SOURCE_PLIST
$(foreach PREFIX,ExactMatch,$(strip $(MAIN_SRC))/$(1)/$(strip $(PREFIX)).plist)
endef

.PHONY: all
all:
	$(begin_target)
	@echo "Building $(MODULES)"
	$(MAKE) build
	$(call open_browser_to_page,$(strip $(DEST))/English/Help/index.html)
	$(end_target)

.PHONY: clean
clean: $(foreach L,$(ALL_LANGUAGES),clean-$(L))

.PHONY: build
build: $(foreach L,$(ALL_LANGUAGES),build-$(L))

build-%:
	$(begin_target)
	install -d "$(strip $(APP_HELP_LOCALIZED))"
	$(MAKE) install-html-$*
	$(MAKE) install-images-$*
	$(MAKE) install-plist-$*
	$(MAKE) install-stylesheets-$*
	perl -pi -e 's/LAST_UPDATED/$(shell date +"%A, %B %d, %Y, %H:%M %Z")/' \
		"$(strip $(APP_HELP_LOCALIZED))/index.html"
	$(end_target)

install-html-%: $(call SOURCE_HTML,%) $(call SOURCE_INDEX_HTML,%)
	install -d "$(strip $(APP_HELP_LOCALIZED_CONTENT))/"
	cp $(call SOURCE_HTML,$*) "$(strip $(APP_HELP_LOCALIZED_CONTENT))/"
	cp $(call SOURCE_INDEX_HTML,$*) "$(strip $(APP_HELP_LOCALIZED))/"

clean-install-html-%:
	$(RM) $(foreach PAGE,$(notdir $(call SOURCE_HTML,$*)),"$(strip $(APP_HELP_LOCALIZED_CONTENT))/$(PAGE)")
	$(RM) $(foreach PAGE,$(notdir $(call SOURCE_INDEX_HTML,$*)),"$(strip $(APP_HELP_LOCALIZED))/$(PAGE)")
	-rmdir "$(strip $(APP_HELP_LOCALIZED_CONTENT))"
	$(RM) $(call SOURCE_HTML,$*)
	$(RM) $(call SOURCE_INDEX_HTML,$*)

install-images-%:
	cp $(call SOURCE_IMG,$*) "$(strip $(APP_HELP_LOCALIZED_CONTENT))/"

clean-install-images-%:
	$(RM) $(foreach IMG,$(notdir $(call SOURCE_IMG,$*)),"$(strip $(APP_HELP_LOCALIZED_CONTENT))/$(IMG)")

install-plist-%: $(call SOURCE_PLIST,%)
	cp $(call SOURCE_PLIST,$*) "$(strip $(APP_HELP_LOCALIZED))/"

clean-install-plist-%:
	$(RM) $(foreach PLIST,$(notdir $(call SOURCE_PLIST,$*)),"$(strip $(APP_HELP_LOCALIZED))/$(PLIST)")

install-stylesheets-%:
	cp $(call SOURCE_CSS,$*) "$(strip $(APP_HELP_LOCALIZED_CONTENT))/"

clean-install-stylesheets-%:
	$(RM) $(foreach CSS,$(notdir $(call SOURCE_CSS,$*)),"$(strip $(APP_HELP_LOCALIZED_CONTENT))/$(CSS)")

clean-install-index-%:
	$(RM) "$(strip $(APP_HELP_LOCALIZED))/Help.helpindex"
	$(RM) "$(strip $(APP_HELP_LOCALIZED))/Help idx"

clean-%: clean-install-images-% clean-install-stylesheets-% clean-install-plist-% clean-install-html-% clean-install-index-%
	$(begin_target)
	-rmdir "$(strip $(APP_HELP_LOCALIZED))"
	-rmdir "$(strip $(APP_HELP_LOCALIZED_ROOT))"
	$(end_target)

# "make" considers the HTML files intermediates normally,
# and would delete them if not for this .SECONDARY statement
# (this line ensures HTML files are only rebuilt if their
# original input files change)
.SECONDARY: $(foreach L,$(ALL_LANGUAGES),$(call SOURCE_HTML,$(L)) $(call SOURCE_INDEX_HTML,$(L)))

# the index page, oddly, should be formatted XHTML 1.0
# and contain Apple's special keywords, even though they
# expect HTML 4.01 (or HTML 3.2, for Panther) otherwise!
%/index.html: %/index.kv %/index.textile
	$(begin_target)
	$(DOCTYPER) XHTML1.0-UTF-8 >| $@
	echo "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head>" >> $@
	$(KV_TO_HEAD) -xml $*/index.kv >> $@
	echo "</head><body>" >> $@
	$(GENERATOR) $*/index.textile | sed 's/\>\>/\&gt;\&gt;/g' >> $@
	echo "</body></html>" >> $@
	echo >> $@
	$(end_target)

%.html: $(strip $(MAIN_SRC))/base.kv %.kv %.textile
	$(begin_target)
	$(DOCTYPER) HTML4.01 >| $@
	echo "<html><head>" >> $@
	$(KV_TO_HEAD) $< >> $@
	$(KV_TO_HEAD) $*.kv >> $@
	echo "</head><body>" >> $@
	$(GENERATOR) $*.textile | sed 's/\>\>/\&gt;\&gt;/g' >> $@
	echo "</body></html>" >> $@
	echo >> $@
	@# textile produces XHTML output, convert to HTML
	perl -pi -e 's|<br />|<br>|g' $@
	$(end_target)

%/ExactMatch.plist: %/ExactMatch.kv
	$(begin_target)
	$(KV_TO_PLIST) $< > $@
	$(end_target)

define begin_target
	@date "+      $(notdir $@): Started at %T."
endef

define end_target
	@date "+Finished $(notdir $@)."
endef

define open_browser_to_page
	echo 'open location "file://$(strip $(1))"' | osascript
endef

