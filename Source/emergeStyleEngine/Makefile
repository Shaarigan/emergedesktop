LIBS += -lversion -lmsimg32 -lshlwapi -lgdi32 -lcomdlg32 -lemergeLib -lemergeGraphics

TARGET = emergeStyleEngine
SUFFIX = .dll
EXPORTS = -DEMERGESTYLEENGINE_EXPORTS

include ../functions.in
include ../globals.in
include ../lib_globals.in
include ../hg_file.in

# Export all symbols as a temporary workaround for a bug in GCC 4.5.0
LDFLAGS += -Wl,--export-all-symbols
