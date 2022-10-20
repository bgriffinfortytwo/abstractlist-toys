// TCL Command to compute the polynomial y = a0 + a1*x + a2*x^2 ... for index x.
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "tclDecls.h"

#ifndef Tcl_Double
#define Tcl_Double double
#endif

Tcl_Double
poly(Tcl_WideInt x, int ac, Tcl_Double a[]) {
  int i = 0;
  Tcl_Double y;
  if (ac == 0) {
    return 0.0;
  }
  y = a[i];
  i++;
  while (i<ac) {
    y += a[i] * pow(x,i);
    i++;
  }
  return y;
}

typedef struct PolySeries {
  const char *name;   // Name of this series
  Tcl_WideInt length; // list length
  int n;              // Number of coeficents
  Tcl_Double a[1];    // coeficents
} PolySeries;


int
my_PolySeriesObjIndex(Tcl_Interp *interp, Tcl_Obj *polySeriesObjPtr, Tcl_WideInt index, Tcl_Obj **elemPtr)
{
    PolySeries *polySeriesRepPtr;
    Tcl_Double element, doubleidx;
    static const Tcl_ObjType *abstractListType = NULL;

    polySeriesRepPtr = Tcl_AbstractListGetConcreteRep(polySeriesObjPtr);

    if (index < 0 || index >= polySeriesRepPtr->length)
	return TCL_ERROR;

    // poly(Tcl_WideInt x, int ac, Tcl_Double a[]) {
    element = poly(index, polySeriesRepPtr->n, polySeriesRepPtr->a);
    doubleidx = index;
    *elemPtr = Tcl_ObjPrintf("%g %g", doubleidx, element);
    return TCL_OK;
}

Tcl_WideInt
my_PolySeriesObjLength(Tcl_Obj *polySeriesObjPtr)
{
  PolySeries *polySeriesRepPtr = (PolySeries *)Tcl_AbstractListGetConcreteRep(polySeriesObjPtr);
  return polySeriesRepPtr->length;
}

static void
DupPolySeriesRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  PolySeries *srcPolySeries = (PolySeries*)Tcl_AbstractListGetConcreteRep(srcPtr);
  PolySeries *copyPolySeries = (PolySeries*)Tcl_AbstractListGetConcreteRep(copyPtr);
  size_t repSize = sizeof(PolySeries) + (srcPolySeries->n * sizeof(Tcl_WideInt));
  memcpy(copyPolySeries, srcPolySeries, repSize);
  return;
}

Tcl_Obj *my_NewPolySeriesObj(int objc, Tcl_Obj * const objv[]);

static Tcl_AbstractListType polyType = {
	TCL_ABSTRACTLIST_VERSION_1,
	"polyseries",
	my_NewPolySeriesObj,
	DupPolySeriesRep,
	my_PolySeriesObjLength,
	my_PolySeriesObjIndex,
	NULL, /* range */
	NULL, /* reverse */
	NULL, /* get elements */
	NULL  /* update string */
};

Tcl_Obj *
my_NewPolySeriesObj(int objc, Tcl_Obj * const objv[])
{
  Tcl_WideInt length;
  Tcl_Double coeficent, *a;
  int i, n = objc - 1;
  PolySeries *polySeriesRepPtr;
  size_t repSize;
  Tcl_Obj *polySeriesPtr;

  if (Tcl_GetWideIntFromObj(NULL, objv[0], &length) != TCL_OK ||
      length < 0) {
    return NULL;
  }

  polySeriesPtr = Tcl_AbstractListObjNew(NULL, &polyType);
  repSize = sizeof(PolySeries) + (n * sizeof(Tcl_WideInt));
  polySeriesRepPtr = (PolySeries*)Tcl_Alloc(repSize);
  polySeriesRepPtr->length = length;
  polySeriesRepPtr->n = n;
  a = &polySeriesRepPtr->a[0];
  for (i=0; i< n; i++) {
    if (Tcl_GetDoubleFromObj(NULL, objv[i+1], &coeficent) != TCL_OK) {
      return NULL;
    }
    a[i] = coeficent;
  }
  Tcl_AbstractListSetConcreteRep(polySeriesPtr, polySeriesRepPtr);

  if (length > 0) {
    Tcl_InvalidateStringRep(polySeriesPtr);
  } else {
    Tcl_InitStringRep(polySeriesPtr, NULL, 0);
  }

  return polySeriesPtr;
}

static int lPolyObjCmd(void *clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]) {

  Tcl_Obj *seriesObj = my_NewPolySeriesObj(objc-1, &objv[1]);
  if (seriesObj) {
    Tcl_SetObjResult(interp, seriesObj);
    return TCL_OK;
  }
  Tcl_WrongNumArgs(interp, 1, objv, "length a0 ?a1 ... an?");
  return TCL_ERROR;
}

int Poly_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lpoly", lPolyObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lpoly", "1.0.0");
    return TCL_OK;
}

/* poly.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)
poly.so: poly.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) poly.c -o poly.so -m64  --shared $(TCL_STUBLIB)

*/
