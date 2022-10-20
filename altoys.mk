TCL_INSTALL = $(HOME)/tcl_core/usr_default
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)

TARGETS = fib.so poly.so readlines.so lstring.so

.so:.c
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

all: $(TARGETS)

clean: $(TARGETS)
	rm -f $^

fib.so: fib.c $(TCL_STUBLIB)

poly.so: poly.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

readlines.so: readlines.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

lstring.so: lstring.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)
