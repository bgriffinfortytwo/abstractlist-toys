// TCL Command to find n'th fibonacci Number
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "tclDecls.h"

//static const Tcl_ObjType *tclAbstractListType;

typedef struct FibonacciSeries {
  Tcl_WideInt start;
  Tcl_WideInt length;
} FibonacciSeries;

static Tcl_Obj *newFibSeriesObj(Tcl_WideInt start, Tcl_WideInt length);

static Tcl_WideInt
fib(Tcl_WideInt n) {
  double phi = (1 + sqrt(5.0)) / 2.0;
  return round(pow(phi, n) / sqrt(5.0));
}

static Tcl_Obj *Tcl_NewFibSeriesObj(int objc, Tcl_Obj *const objv[]);

static void
DupFibonocciRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
    FibonacciSeries *srcFibSeries =
	(FibonacciSeries*)srcPtr->internalRep.twoPtrValue.ptr1;
    FibonacciSeries *copyFibSeries =
	(FibonacciSeries*)copyPtr->internalRep.twoPtrValue.ptr1;

    copyFibSeries->start = srcFibSeries->start;
    copyFibSeries->length = srcFibSeries->length;
}

static Tcl_Size
FibonocciObjLength(Tcl_Obj *fibObj)
{
  FibonacciSeries *fibSeriesRepPtr =
      (FibonacciSeries *)fibObj->internalRep.twoPtrValue.ptr1;
  return fibSeriesRepPtr->length;
}

static int
FibonocciObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *fibObj,
    Tcl_Size index,
    Tcl_Obj **elemObjPtr)
{
    FibonacciSeries *fibSeriesRepPtr = fibObj->internalRep.twoPtrValue.ptr1;
    Tcl_WideInt element = fib(index+fibSeriesRepPtr->start);
    *elemObjPtr = Tcl_NewWideIntObj(element);
    return TCL_OK;
}

static int
FibonocciObjRange(
    Tcl_Interp *interp,  /* For error reporting */
    Tcl_Obj *fibObj,     /* List object to take a range from. */
    Tcl_Size fromIdx,    /* Index of first element to include. */
    Tcl_Size toIdx,      /* Index of last element to include. */
    Tcl_Obj **newObjPtr) /* return value */
{
    FibonacciSeries *fibSeriesRepPtr = fibObj->internalRep.twoPtrValue.ptr1;
    Tcl_WideInt start;
    Tcl_WideInt len = (toIdx - fromIdx);

    start = fibSeriesRepPtr->start + fromIdx;
    *newObjPtr = newFibSeriesObj(start, len);

    return TCL_OK;
}

#if 0
static int
FibonocciObjReverse(
    Tcl_Interp *interp,
    Tcl_Obj *fibObj,
    Tcl_Obj **newObjPtr)
{
    FibonacciSeries *fibSeriesRepPtr = fibObj->internalRep.twoPtrValue.ptr1;
    (void)fibSeriesRepPtr;
    return TCL_ERROR; // Not supported
}

static int
FibonocciGetElements(
    Tcl_Interp *interp,         /* Used to report errors if not NULL. */
    Tcl_Obj *fibObj,            /* ArithSeries object for which an element
                                 * array is to be returned. */
    Tcl_Size *objcPtr,          /* Where to store the count of objects
                                 * referenced by objv. */
    Tcl_Obj ***objvPtr)         /* Where to store the pointer to an array of
                                 * pointers to the list's objects. */
{
    printf("GetElements not yet implemented\n");
    return TCL_ERROR;
}
#endif


static void
FreeFibonocciRep(Tcl_Obj* fibObj)
{
    FibonacciSeries *fibSeriesRepPtr = fibObj->internalRep.twoPtrValue.ptr1;
    Tcl_Free((char*)fibSeriesRepPtr);
    fibObj->internalRep.twoPtrValue.ptr1 = NULL;
}

static void
UpdateStringOfSeries(Tcl_Obj *fibObj)
{
    FibonacciSeries *fibSeriesRepPtr = fibObj->internalRep.twoPtrValue.ptr1;
    Tcl_WideInt element;
    Tcl_Size i;
    Tcl_Size bytlen;

    for (i=0, bytlen=0; i<fibSeriesRepPtr->length; i++) {
	Tcl_Size l;
	double d;
	element = fib(i+fibSeriesRepPtr->start);
	d = (double)element;
	l = d>0 ? log10(d)+1 : d<0 ? log10((0-d))+2 : 1;

	if ((bytlen + l) < 0) {
	    break; // overflow
	}
	bytlen += l;
    }

    bytlen += fibSeriesRepPtr->length; // Space for each separator
    char *p = Tcl_InitStringRep(fibObj, NULL, bytlen);
    Tcl_Obj *eleObj;

    for (i = 0; i < fibSeriesRepPtr->length; i++) {
	if (FibonocciObjIndex(NULL, fibObj, i, &eleObj) == TCL_OK) {
	    Tcl_Size slen;
	    char *str = Tcl_GetStringFromObj(eleObj, &slen);
	    strcpy(p, str);
	    p[slen] = ' ';
	    p += slen+1;
	    Tcl_DecrRefCount(eleObj);
	} // else TODO: report error here?
    }

    if (bytlen > 0) fibObj->bytes[bytlen-1] = '\0';
    fibObj->length = bytlen-1;

    return;
}

static Tcl_ObjType fibType = {
    "fibonocci",
    FreeFibonocciRep,
    DupFibonocciRep,
    UpdateStringOfSeries,
    NULL,
    TCL_OBJTYPE_V2(
	FibonocciObjLength,
	FibonocciObjIndex,
	FibonocciObjRange,
	NULL, /*FibonocciObjReverse*/
        NULL, /*FibonocciGetElements*/
	NULL, /* SetElem */
	NULL, /* Replace */
        NULL) /* in operation */
};

static Tcl_Obj *
Tcl_NewFibSeriesObj(int objc, Tcl_Obj *const objv[])
{
  Tcl_WideInt start, length;
  if (objc != 2) {
    return NULL;
  }
  if (Tcl_GetWideIntFromObj(NULL, objv[0], &start) != TCL_OK) {
    return NULL;
  }
  if (Tcl_GetWideIntFromObj(NULL, objv[1], &length) != TCL_OK) {
    return NULL;
  }
  return newFibSeriesObj(start, length);
}

static Tcl_Obj *
newFibSeriesObj(Tcl_WideInt start, Tcl_WideInt length) {
    Tcl_Obj *fibSeriesPtr;
    FibonacciSeries *fibSeriesRepPtr;

    if (length < 0) return NULL; /* Invalid range error */

    fibSeriesPtr = Tcl_NewObj();

    fibSeriesRepPtr = (FibonacciSeries*)Tcl_Alloc(sizeof (FibonacciSeries));
    fibSeriesRepPtr->start = start;
    fibSeriesRepPtr->length = length;

    fibSeriesPtr->internalRep.twoPtrValue.ptr1 = fibSeriesRepPtr;
    fibSeriesPtr->typePtr = &fibType;
    Tcl_InvalidateStringRep(fibSeriesPtr);

    return fibSeriesPtr;
}

static int
lFibObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    if (objc == 3) {
	Tcl_Obj *seriesObj = Tcl_NewFibSeriesObj(2, &objv[1]);
	Tcl_SetObjResult(interp, seriesObj);
	return TCL_OK;
    }
    Tcl_WrongNumArgs(interp, 1, objv, "start length");
    return TCL_ERROR;
}

int
Fib_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lfib", lFibObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lfib", "1.0.0");
    return TCL_OK;
}

/* fib.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)

all: fib.so

fib.so: fib.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) fib.c -o fib.so -m64  --shared $(TCL_STUBLIB)

*/
