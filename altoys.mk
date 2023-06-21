# Define location of tcl install to find include and lib files
# necessary for building Tcl packages.
# File must contain definitions for these macros:
#
#    TCL_INCLUDE_SPEC = -I$(path-to-tcl-install)/include
#    TCL_STUB_LIB_PATH = $(path-to-tcl-install)/lib/libtclstub.a
#    TCL_STUB_LIB_SPEC = -L$(path-to-tcl-install)/lib -ltclstub
include tclConfig.mk


DEFS		= -DPACKAGE_NAME=\"poly\" -DPACKAGE_TARNAME=\"poly\" -DPACKAGE_VERSION=\"0.1\" -DPACKAGE_STRING=\"poly\ 0.1\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_INTPTR_T=1 -DHAVE_PTRDIFF_T=1 -DUSE_THREAD_ALLOC=1 -D_REENTRANT=1 -D_THREAD_SAFE=1 -DTCL_THREADS=1 -DMODULE_SCOPE=extern\ __attribute__\(\(__visibility__\(\"hidden\"\)\)\) -DHAVE_HIDDEN=1 -DHAVE_CAST_TO_UNION=1 -D_LARGEFILE64_SOURCE=1 -DTCL_WIDE_INT_IS_LONG=1 -DUSE_TCL_STUBS=1

INCLUDES = $(TCL_INCLUDE_SPEC)
#CFLAGS = -pipe -O2 -fomit-frame-pointer -DNDEBUG -Wall -fPIC $(INCLUDES) $(DEFS)
CFLAGS = -pipe -g -DNDEBUG -Wall -fPIC $(INCLUDES) $(DEFS)
LIBS = $(TCL_STUB_LIB_SPEC)

TARGETS = fib.so poly.so readlines.so lstring.so lgen.so

.so:.c
	gcc -g -fPIC $(CFLAGS) $< -o $@ -m64  --shared $(TCL_STUB_LIB_PATH)

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

fib.so: fib.c $(TCL_STUB_LIB_PATH)
	gcc -g $(CFLAGS) $< -o $@ -m64 $(LIBS) --shared

poly.so: poly.c $(TCL_STUB_LIB_PATH)
	gcc -g $(CFLAGS) $< -o $@ -m64 $(LIBS) --shared

readlines.so: readlines.c $(TCL_STUB_LIB_PATH)
	gcc -g $(CFLAGS) $< -o $@ -m64 $(LIBS) --shared

lstring.so: lstring.c $(TCL_STUBLIB)
	gcc -g $(CFLAGS) $< -o $@ -m64 $(LIBS) --shared

lgen.so: lgen.c $(TCL_STUBLIB)
	gcc -g $(CFLAGS) $< -o $@ -m64 $(LIBS) --shared


test: all all.tcl $(TARGETS)
	$(TCLSH) all.tcl
