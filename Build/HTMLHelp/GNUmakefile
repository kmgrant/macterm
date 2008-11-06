# GNU makefile for building MacTelnet.help.
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
# Kevin Grant (kevin@ieee.org)
# March 26, 2005

DEST = $(CURDIR)
MAIN_SRC = $(CURDIR)/Resources/
define MAIN_SRC_SUB
$(strip $(MAIN_SRC))/$(strip $(1))
endef
SHARED_SRC = $(CURDIR)/../Shared/Resources

ALL_LANGUAGES = English
MODULES := $(sort $(notdir $(subst .textile,,$(wildcard $(strip $(MAIN_SRC))/English/*.textile))))

# use within "xyz-%" rules where the pattern is the language (e.g. English)
define MACTELNET_HELP_LOCALIZED
$(strip $(DEST))/$*/Help
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

# this rule works only on Tiger
.PHONY: index
index: $(foreach L,$(ALL_LANGUAGES),install-index-$(L))

build-%:
	$(begin_target)
	install -d "$(strip $(MACTELNET_HELP_LOCALIZED))"
	$(MAKE) install-html-$*
	$(MAKE) install-images-$*
	$(MAKE) install-plist-$*
	$(MAKE) install-stylesheets-$*
	perl -pi -e 's/LAST_UPDATED/$(shell date +"%A, %B %d, %Y, %H:%M %Z")/' \
		"$(strip $(MACTELNET_HELP_LOCALIZED))/index.html"
	$(MAKE) install-index-$*
	$(end_target)

install-html-%: $(call SOURCE_HTML,%)
	cp $(call SOURCE_HTML,$*) "$(strip $(MACTELNET_HELP_LOCALIZED))/"

clean-install-html-%:
	$(RM) $(foreach PAGE,$(notdir $(call SOURCE_HTML,$*)),"$(strip $(MACTELNET_HELP_LOCALIZED))/$(PAGE)")

install-images-%:
	cp $(call SOURCE_IMG,$*) "$(strip $(MACTELNET_HELP_LOCALIZED))/"

clean-install-images-%:
	$(RM) $(foreach IMG,$(notdir $(call SOURCE_IMG,$*)),"$(strip $(MACTELNET_HELP_LOCALIZED))/$(IMG)")

install-plist-%: $(call SOURCE_PLIST,%)
	cp $(call SOURCE_PLIST,$*) "$(strip $(MACTELNET_HELP_LOCALIZED))/"

clean-install-plist-%:
	$(RM) $(foreach PLIST,$(notdir $(call SOURCE_PLIST,$*)),"$(strip $(MACTELNET_HELP_LOCALIZED))/$(PLIST)")

install-stylesheets-%:
	cp $(call SOURCE_CSS,$*) "$(strip $(MACTELNET_HELP_LOCALIZED))/"

clean-install-stylesheets-%:
	$(RM) $(foreach CSS,$(notdir $(call SOURCE_CSS,$*)),"$(strip $(MACTELNET_HELP_LOCALIZED))/$(CSS)")

# this rule works only on Tiger
install-index-%:
	cd /Developer/Applications/Utilities/Help\ Indexer.app/Contents/MacOS/ && \
	./Help\ Indexer "$(strip $(MACTELNET_HELP_LOCALIZED))" \
		-Tokenizer 1 -ShowProgress YES -UseRemoteRoot NO \
		-PreferNetworkFiles NO -LogStyle 2 -IndexAnchors YES \
		-PantherIndexing YES
	cd /Developer/Applications/Utilities/Help\ Indexer.app/Contents/Resources/ && \
	./Unarchiver -v "$(strip $(MACTELNET_HELP_LOCALIZED))/Help.helpindex"

clean-install-index-%:
	$(RM) "$(strip $(MACTELNET_HELP_LOCALIZED))/Help.helpindex"
	$(RM) "$(strip $(MACTELNET_HELP_LOCALIZED))/Help idx"

clean-%: clean-install-images-% clean-install-stylesheets-% clean-install-plist-% clean-install-html-% clean-install-index-%
	$(begin_target)
	-rmdir "$(strip $(MACTELNET_HELP_LOCALIZED))"
	$(end_target)

# "make" considers the HTML files intermediates normally,
# and would delete them if not for this .SECONDARY statement
# (this line ensures HTML files are only rebuilt if their
# original input files change)
.SECONDARY: $(foreach L,$(ALL_LANGUAGES),$(call SOURCE_HTML,$(L)))

%.html: $(strip $(MAIN_SRC))/base.kv %.kv %.textile
	$(begin_target)
	$(DOCTYPER) HTML4.01 >| $@
	echo "<html><head>" >> $@
	$(KV_TO_HEAD) $< >> $@
	$(KV_TO_HEAD) $*.kv >> $@
	echo "</head><body>" >> $@
	$(GENERATOR) $*.textile >> $@
	echo "</body></html>" >> $@
	echo >> $@
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

