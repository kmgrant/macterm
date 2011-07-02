# GNU makefile for building the release notes and the PAD file
# that tells shareware sites about new releases.
#
# The release notes are published online and referred to by the
# application's Check for Updates command.
#
# Kevin Grant (kmg@mac.com)
# October 12, 2006

# determine application version
# IMPORTANT: read from a built Info.plist only;
# if you don't build the bundle first, this won't work
VERSION_INFO := $(shell cd $(CURDIR)/.. ; ./VersionInfo.sh ; cd $(CURDIR))
MAJOR := $(word 1,$(VERSION_INFO))
MINOR := $(word 2,$(VERSION_INFO))
SUBMINOR := $(word 3,$(VERSION_INFO))
PRERELEASE := $(word 4,$(VERSION_INFO))
BUILD := $(word 5,$(VERSION_INFO))
MAJOR_MINOR_SUB := $(strip $(MAJOR)).$(strip $(MINOR)).$(strip $(SUBMINOR))

DEST = $(CURDIR)
MAIN_SRC = $(CURDIR)/Resources/
define MAIN_SRC_SUB
$(strip $(MAIN_SRC))/$*
endef

ALL_LANGUAGES = English

GENERATOR=$(CURDIR)/generate_all.py
define GENERATOR_SUB
$(CURDIR)/Resources/$*/release_notes.py
endef

.PHONY: all
all:
	$(begin_target)
	@echo "Building $(MODULES)"
	$(MAKE) build
	$(call open_browser_to_page,$(strip $(DEST))/index.html)
	$(call open_browser_to_page,$(strip $(DEST))/daily.html)
	$(end_target)

.PHONY: build
build: $(foreach L,$(ALL_LANGUAGES),build-$(L))

.PHONY: clean
clean: $(foreach L,$(ALL_LANGUAGES),clean-$(L))



#$(strip $(MAIN_SRC_SUB))/template-index.html  $(strip $(MAIN_SRC_SUB))/template-version.html
#		$(strip $(MAIN_SRC_SUB))/template-pad.plist  $(strip $(MAIN_SRC_SUB))/release-notes.py
build-%:
	$(begin_target)
	install -d "$(strip $(DEST))"
	cd $(DEST) && env RELEASE_NOTES_SRC='$(strip $(MAIN_SRC_SUB))' RELEASE_NOTES_DEST='$(strip $(DEST))' $(GENERATOR)
	perl -pi -e 's/LAST_UPDATED/$(shell date +"%A, %B %d, %Y, %H:%M %Z")/' \
		"$(strip $(DEST))/index.html"
	cp $(strip $(MAIN_SRC_SUB))/template-pad.plist $(strip $(DEST))/pad.plist
	perl -pi -e 's/VERSION_SUBSTITUTION/$(MAJOR).$(MINOR).$(SUBMINOR).$(PRERELEASE).$(BUILD)/' \
		"$(strip $(DEST))/pad.plist"
	perl -pi -e 's/LAST_UPDATED_MM/$(shell date +"%m")/'  "$(strip $(DEST))/pad.plist"
	perl -pi -e 's/LAST_UPDATED_DD/$(shell date +"%d")/'  "$(strip $(DEST))/pad.plist"
	perl -pi -e 's/LAST_UPDATED_YYYY/$(shell date +"%Y")/'  "$(strip $(DEST))/pad.plist"
	perl -pi -e "s|RELEASE_NOTES_ONE_STRING|$(shell $(GENERATOR_SUB) plain_text $(MAJOR_MINOR_SUB))|"  "$(strip $(DEST))/pad.plist"
	$(end_target)

clean-%:
	$(begin_target)
	$(RM) $(strip $(DEST))/3.*.html  $(strip $(DEST))/index.html  $(strip $(DEST))/pad.plist
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

