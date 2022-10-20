// TCL Command to find n'th fibonacci Number
#include <math.h>
#include "tcl.h"
#include "tclDecls.h"

//static const Tcl_ObjType *tclAbstractListType;

typedef struct FibonacciSeries {
  Tcl_WideInt start;
  Tcl_WideInt length;
  const char *name;
} FibonacciSeries;

static Tcl_Obj *newFibSeriesObj(Tcl_WideInt start, Tcl_WideInt length);

static Tcl_WideInt
fib(Tcl_WideInt n) {
  static double phi = (1 + sqrt(5)) / 2;
  return round(pow(phi, n) / sqrt(5));
}

static Tcl_Obj *Tcl_NewFibSeriesObj(int objc, Tcl_Obj *const objv[]);

    static void
DupFibonocciRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
    FibonacciSeries *srcFibSeries = (FibonacciSeries*)Tcl_AbstractListGetConcreteRep(srcPtr);
    FibonacciSeries *copyFibSeries = (FibonacciSeries*)Tcl_AbstractListGetConcreteRep(copyPtr);

    copyFibSeries->name = "fibseries";
    copyFibSeries->start = srcFibSeries->start;
    copyFibSeries->length = srcFibSeries->length;
}

Tcl_WideInt FibonocciObjLength(Tcl_Obj *fibObj)
{
  FibonacciSeries *fibSeriesRepPtr = (FibonacciSeries *)Tcl_AbstractListGetConcreteRep(fibObj);
  return fibSeriesRepPtr->length;
}

int FibonocciObjIndex(Tcl_Interp *interp, Tcl_Obj *fibObj, Tcl_WideInt index, Tcl_Obj **elemObjPtr)
{
    FibonacciSeries *fibSeriesRepPtr;
    Tcl_WideInt element;

    fibSeriesRepPtr = Tcl_AbstractListGetConcreteRep(fibObj);

    if (index < 0 || index >= fibSeriesRepPtr->length)
	return TCL_ERROR;

    /* List[i] = Start + (Step * i) */
    element = fib(index+fibSeriesRepPtr->start);
    *elemObjPtr = Tcl_NewWideIntObj(element);
    return TCL_OK;
}

int
FibonocciObjRange(
    Tcl_Interp *interp,  /* For error reporting */
    Tcl_Obj *fibObj,	 /* List object to take a range from. */
    Tcl_WideInt fromIdx, /* Index of first element to include. */
    Tcl_WideInt toIdx,	 /* Index of last element to include. */
    Tcl_Obj **newObjPtr) /* return value */
{
    FibonacciSeries *fibSeriesRepPtr = Tcl_AbstractListGetConcreteRep(fibObj);
    Tcl_WideInt start;
    Tcl_WideInt len = (toIdx - fromIdx);

    start = fibSeriesRepPtr->start + fromIdx;
    *newObjPtr = newFibSeriesObj(start, len);

    return TCL_OK;
}

int FibonocciObjReverse(Tcl_Interp *interp, Tcl_Obj *fibObj, Tcl_Obj **newObjPtr)
{
    FibonacciSeries *fibSeriesRepPtr = Tcl_AbstractListGetConcreteRep(fibObj);

    return TCL_ERROR; // Not supported
}

int FibonocciGetElements(
    Tcl_Interp *interp,		/* Used to report errors if not NULL. */
    Tcl_Obj *fibObj,		/* ArithSeries object for which an element
				 * array is to be returned. */
    int *objcPtr,		/* Where to store the count of objects
				 * referenced by objv. */
    Tcl_Obj ***objvPtr)		/* Where to store the pointer to an array of
				 * pointers to the list's objects. */
{
    printf("GetElements not yet implemented\n");
    return TCL_ERROR;
}


void
FreeFibonocciRep(Tcl_Obj* fibObj)
{
    FibonacciSeries *fibSeriesRepPtr = Tcl_AbstractListGetConcreteRep(fibObj);
    Tcl_Free((char*)fibSeriesRepPtr);
    Tcl_AbstractListSetConcreteRep(fibObj, NULL);
}

static void
UpdateStringOfArithSeries(Tcl_Obj *fibObj)
{
}

static Tcl_AbstractListType fibType = {
	TCL_ABSTRACTLIST_VERSION_1,
	"fibonocci",
	Tcl_NewFibSeriesObj,
	DupFibonocciRep,
	FibonocciObjLength,
	FibonocciObjIndex,
	FibonocciObjRange,
	NULL/*FibonocciObjReverse*/,
        FibonocciGetElements,
        FreeFibonocciRep,
	NULL /* use default update string */
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
    static const char *fibSeriesName = "fibonacciseries";
    if (length < 0) return NULL; /* Invalid range error */

    fibSeriesPtr = Tcl_AbstractListObjNew(NULL, &fibType);

    fibSeriesRepPtr = (FibonacciSeries*)Tcl_Alloc(sizeof (FibonacciSeries));
    fibSeriesRepPtr->start = start;
    fibSeriesRepPtr->length = length;

    Tcl_AbstractListSetConcreteRep(fibSeriesPtr, fibSeriesRepPtr);
    Tcl_InvalidateStringRep(fibSeriesPtr);

    return fibSeriesPtr;
}

static int lFibObjCmd(void *clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]) {
  if (objc == 3) {
    Tcl_Obj *obj2[2];
    obj2[0] = objv[1];
    obj2[1] = objv[2];
    Tcl_Obj *seriesObj = Tcl_NewFibSeriesObj(2, obj2);
    Tcl_SetObjResult(interp, seriesObj);
    return TCL_OK;
  }
  Tcl_WrongNumArgs(interp, 1, objv, "start length");
  return TCL_ERROR;
}

int Fib_Init(Tcl_Interp *interp) {
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
