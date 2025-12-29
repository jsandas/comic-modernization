VERSION="R5sw1991"
ARCHIVE_URL=https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeath${VERSION}michaelA.Denioaction.zip

REFERENCE_DIR = reference/disassembly
DJLINK = $(REFERENCE_DIR)/djlink/djlink

# Build djlink linker if needed
$(DJLINK):
	@echo "Building djlink linker..."
	@cd $(REFERENCE_DIR)/djlink && \
		g++ -O2 -Wall -std=c++98 -o djlink \
		djlink.cc fixups.cc libs.cc list.cc map.cc objs.cc \
		out.cc quark.cc segments.cc stricmp.cc symbols.cc \
		2>&1 | grep -v "warning:" || true
	@chmod +x $(DJLINK) 2>/dev/null || true

assets:
	@echo "Assets are now located in reference/assets; no extraction step."

programs:
	$(MAKE) -C programs

deriv:
	$(MAKE) -C deriv

disassembly:
	$(MAKE) -C disassembly

all: $(DJLINK)

.PHONY: all