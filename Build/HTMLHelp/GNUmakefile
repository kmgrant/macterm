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

# determine MacTelnet version
# IMPORTANT: read from a built MacTelnet.app's Info.plist;
# if you don't build MacTelnet first, this won't work
VERSION_INFO := $(shell cd $(CURDIR)/.. ; ./VersionInfo.sh ; cd $(CURDIR))
MAJOR := $(word 1,$(VERSION_INFO))
MINOR := $(word 2,$(VERSION_INFO))
SUBMINOR := $(word 3,$(VERSION_INFO))
PRERELEASE := $(word 4,$(VERSION_INFO))
BUILD := $(word 5,$(VERSION_INFO))

DEST = $(CURDIR)
MAIN_SRC = $(CURDIR)/Resources/
define MAIN_SRC_SUB
$(strip $(MAIN_SRC))/$(strip $(1))
endef
SHARED_SRC = $(CURDIR)/../Shared/Resources

ALL_LANGUAGES = English
MODULES := $(sort $(notdir $(subst .textile,,$(wildcard $(strip $(MAIN_SRC))/English/*.textile))))

MACTELNET_HELP_BUNDLE = MacTelnet.help
MACTELNET_HELP = $(strip $(DEST))/$(strip $(MACTELNET_HELP_BUNDLE))
MACTELNET_HELP_CONTENTS = $(strip $(MACTELNET_HELP))/Contents
MACTELNET_HELP_RES = $(strip $(MACTELNET_HELP_CONTENTS))/Resources
define MACTELNET_HELP_RES_SUB
$(strip $(MACTELNET_HELP_RES))/$*.lproj
endef

DOCTYPER=$(CURDIR)/Tools/PrintHTMLDocType.pl
GENERATOR=$(CURDIR)/Tools/html_from_textile.py
KV_TO_HEAD=$(CURDIR)/Tools/KeyValueToHTMLHeadTags.pl

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

.PHONY: all
all:
	$(begin_target)
	@echo "Building $(MODULES)"
	install -d "$(strip $(MACTELNET_HELP_CONTENTS))"
	echo "BNDLhbwr" >| "$(strip $(MACTELNET_HELP_CONTENTS))/PkgInfo"
	cp "$(strip $(MAIN_SRC))/MacTelnetHelp-Info.plist" "$(strip $(MACTELNET_HELP_CONTENTS))/Info.plist"
	$(MAKE) build
	$(call open_browser_to_page,$(strip $(MACTELNET_HELP_RES))/English.lproj/index.html)
	$(end_target)

.PHONY: clean
clean: $(foreach L,$(ALL_LANGUAGES),clean-$(L))
	$(begin_target)
	-rmdir "$(strip $(MACTELNET_HELP_RES))"
	$(RM) "$(strip $(MACTELNET_HELP_CONTENTS))/PkgInfo"
	$(RM) "$(strip $(MACTELNET_HELP_CONTENTS))/Info.plist"
	-rmdir "$(strip $(MACTELNET_HELP_CONTENTS))"
	-rmdir "$(strip $(MACTELNET_HELP))"
	$(end_target)

.PHONY: build
build: $(foreach L,$(ALL_LANGUAGES),build-$(L))

# this rule works only on Tiger
.PHONY: index
index: $(foreach L,$(ALL_LANGUAGES),install-index-$(L))

build-%: $(strip $(MAIN_SRC))/MacTelnetHelp-Info.plist
	$(begin_target)
	install -d "$(strip $(MACTELNET_HELP_RES_SUB))"
	$(MAKE) install-html-$*
	$(MAKE) install-images-$*
	$(MAKE) install-stylesheets-$*
	perl -pi -e 's/LAST_UPDATED/$(shell date +"%A, %B %d, %Y, %H:%M %Z")/' \
		"$(strip $(MACTELNET_HELP_RES_SUB))/index.html"
	$(MAKE) install-index-$*
	$(end_target)

install-html-%: $(call SOURCE_HTML,%)
	cp $(call SOURCE_HTML,$*) "$(strip $(MACTELNET_HELP_RES_SUB))/"

clean-install-html-%:
	$(RM) $(foreach PAGE,$(notdir $(call SOURCE_HTML,$*)),"$(strip $(MACTELNET_HELP_RES_SUB))/$(PAGE)")

install-images-%:
	cp $(call SOURCE_IMG,$*) "$(strip $(MACTELNET_HELP_RES_SUB))/"

clean-install-images-%:
	$(RM) $(foreach IMG,$(notdir $(call SOURCE_IMG,$*)),"$(strip $(MACTELNET_HELP_RES_SUB))/$(IMG)")

install-stylesheets-%:
	cp $(call SOURCE_CSS,$*) "$(strip $(MACTELNET_HELP_RES_SUB))/"

clean-install-stylesheets-%:
	$(RM) $(foreach CSS,$(notdir $(call SOURCE_CSS,$*)),"$(strip $(MACTELNET_HELP_RES_SUB))/$(CSS)")

# this rule works only on Tiger
install-index-%:
	cd /Developer/Applications/Utilities/Help\ Indexer.app/Contents/MacOS/ && \
	./Help\ Indexer "$(strip $(MACTELNET_HELP_RES_SUB))" \
		-Tokenizer 1 -ShowProgress YES -UseRemoteRoot NO \
		-PreferNetworkFiles NO -LogStyle 2 -IndexAnchors YES \
		-PantherIndexing YES
	cd /Developer/Applications/Utilities/Help\ Indexer.app/Contents/Resources/ && \
	./Unarchiver -v "$(strip $(MACTELNET_HELP_RES_SUB))/$*.lproj.helpindex"

clean-install-index-%:
	$(RM) "$(strip $(MACTELNET_HELP_RES_SUB))/$*.lproj.helpindex"
	$(RM) "$(strip $(MACTELNET_HELP_RES_SUB))/$*.lproj idx"

clean-%: clean-install-images-% clean-install-stylesheets-% clean-install-html-% clean-install-index-%
	$(begin_target)
	-rmdir "$(strip $(MACTELNET_HELP_RES_SUB))"
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

define begin_target
	@date "+      $(notdir $@): Started at %T."
endef

define end_target
	@date "+Finished $(notdir $@)."
endef

define open_browser_to_page
	echo 'open location "file://$(strip $(1))"' | osascript
endef

