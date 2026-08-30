# Crux/Granny SDL2 reimplementation
#
# Usage:
#   make            build ./crux + ./dumpprog
#   make run        build, then run against ../  (where ADVENT.IDX/.RES live)
#   make clean
#
# Dev/debug runs (so common runs go through `make`, easy to allowlist):
#   make rp P=vvi2:0        build + execute scene:prog with opcode trace (filtered)
#   make rpall P=vvi2:0     same, full unfiltered trace
#   make dprog P="vvi2 0"   static opcode dump of a scene program
#   make theme T=vvi        dry-walk + stream a music theme (sequencer trace)
#   make themedump T=vvi    dump a theme file's tables
#   make menu [F=3]         render the menu to menu.png (F = hover flower index)
#   make click F=3          click a menu flower (verb-0 handler), traced
#
# SDL2 is located via pkg-config (works for Homebrew/apt installs).

CXX      := clang++
# -MMD -MP emit per-object .d files of header dependencies so editing a header
# (e.g. changing a class layout) rebuilds every .cpp that includes it. Without
# this, stale objects cause ABI mismatches across translation units.
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -g -MMD -MP $(shell pkg-config --cflags sdl2)
LDFLAGS  := $(shell pkg-config --libs sdl2)

SRCS := main.cpp ResArchive.cpp Framebuffer.cpp Display.cpp Scm.cpp Sprite.cpp Bani.cpp \
        ScmPlayer.cpp Scene.cpp RunProg.cpp OpNames.cpp Timer.cpp Audio.cpp Anim.cpp Cursor.cpp Area.cpp Inventory.cpp HelpBlit.cpp Palette.cpp Text.cpp Theme.cpp Gv.cpp Sentence.cpp TextRender.cpp Slider.cpp
OBJS := $(SRCS:.cpp=.o)
# dumpprog is a standalone tool (no SDL): just the resource loader + scene parser.
DUMP_OBJS := dumpprog.o Scene.o ResArchive.o
# Self-checks (link the anim subsystem + its deps against the real archive).
TEST_OBJS := test_dumpqueue.o Inventory.o Anim.o Area.o HelpBlit.o Bani.o Sprite.o Framebuffer.o ResArchive.o
TESTBIN := test_dumpqueue
DEPS := $(OBJS:.o=.d) dumpprog.d test_dumpqueue.d
BIN  := crux
DUMPBIN := dumpprog

# The game data lives one directory up (granny-re/ADVENT.IDX, ADVENT.RES).
DATADIR ?= ..

all: $(BIN) $(DUMPBIN)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(DUMPBIN): $(DUMP_OBJS)
	$(CXX) $(DUMP_OBJS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

run: $(BIN)
	./$(BIN) $(DATADIR)

# Headless so these never block on a window; HUSH keeps a value-less env unset.
HEADLESS := SDL_VIDEODRIVER=dummy

# Execute one scene program with opcode trace, filtered to the key lines.
rp: $(BIN)
	@RP_TRACE=1 RUN_PROG=$(P) $(HEADLESS) ./$(BIN) $(DATADIR) 2>&1 \
		| grep -iE "exec program|UNIMPL|completed|Bad commad" || true

# Same, but the full unfiltered trace.
rpall: $(BIN)
	@RP_TRACE=1 RUN_PROG=$(P) $(HEADLESS) ./$(BIN) $(DATADIR)

# Static opcode dump of a scene program (no execution).  P="<scene> <prog|all>"
dprog: $(DUMPBIN)
	@./$(DUMPBIN) $(DATADIR) $(P)

# Dry-walk a music theme's cue sequence, then stream-test it.
theme: $(BIN)
	@THEME_PLAY=$(T) $(HEADLESS) ./$(BIN) $(DATADIR) 2>&1 \
		| grep -iE "Theme '|PLAY seg|SETVOL|LOOP|STOP|THEME:" || true

# Dump a theme file's tables (segments / commands / labels).
themedump: $(BIN)
	@THEME_DUMP=$(T) $(HEADLESS) ./$(BIN) $(DATADIR)

# Render the menu to menu.png; pass F=<n> to hover that flower.
menu: $(BIN)
	@MENU_DUMP=1 $(if $(F),MENU_HOVER=$(F)) $(HEADLESS) ./$(BIN) $(DATADIR) >/dev/null 2>&1
	@sips -s format png menu.ppm --out menu.png >/dev/null 2>&1 && rm -f menu.ppm && echo "wrote menu.png"

# Click a menu flower (runs its verb-0 handler), traced.
click: $(BIN)
	@RP_TRACE=1 MENU_CLICK=$(F) $(HEADLESS) ./$(BIN) $(DATADIR) 2>&1 \
		| grep -iE "menu click|exec program|UNIMPL|play SCM" || true

# Run the self-checks.
test: $(TESTBIN)
	@./$(TESTBIN) $(DATADIR)

$(TESTBIN): $(TEST_OBJS)
	$(CXX) $(TEST_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(DEPS) dumpprog.o test_dumpqueue.o $(BIN) $(DUMPBIN) $(TESTBIN)

.PHONY: all run rp rpall dprog theme themedump menu click test clean
