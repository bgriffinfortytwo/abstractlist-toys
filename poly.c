// TCL Command to compute the polynomial y = a0 + a1*x + a2*x^2 ... for index x.
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "tclDecls.h"

#ifndef Tcl_Double
#define Tcl_Double double
#endif

Tcl_Double
poly(
    Tcl_Size x,
    int ac,
    Tcl_Double a[])
{
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
    Tcl_Size len;     // list length
    int n;            // Number of coeficents
    Tcl_Double a[1];  // coeficents
} PolySeries;


int
my_PolySeriesObjIndex(Tcl_Interp *interp, Tcl_Obj *polySeriesObjPtr, Tcl_Size index, Tcl_Obj **elemPtr)
{
    PolySeries *polySeriesRepPtr;
    Tcl_Double element;

    polySeriesRepPtr = polySeriesObjPtr->internalRep.twoPtrValue.ptr1;

    if (index < 0 || index >= polySeriesRepPtr->len)
	return TCL_ERROR;

    // poly(Tcl_WideInt x, int ac, Tcl_Double a[]) {
    element = poly(index, polySeriesRepPtr->n, polySeriesRepPtr->a);
    *elemPtr = Tcl_NewDoubleObj(element);
    return TCL_OK;
}

void
UpdateStringRep(Tcl_Obj *objPtr)
{
    PolySeries *polySeriesRepPtr;
    Tcl_Double element;
    Tcl_Size i;
    size_t bytlen;

    polySeriesRepPtr = objPtr->internalRep.twoPtrValue.ptr1;

    for (i=0, bytlen=0; i<polySeriesRepPtr->len; i++) {
      char tmp[TCL_DOUBLE_SPACE+2];
      size_t l;
      element = poly(i, polySeriesRepPtr->n, polySeriesRepPtr->a);
      tmp[0] = 0;
      Tcl_PrintDouble(NULL,element,tmp);
      l=strlen(tmp);
      if ((bytlen + l) < 0) {
        break; // overflow
      }
      bytlen += l;
    }

    bytlen += polySeriesRepPtr->len; // Space for each separator
    char *p = Tcl_InitStringRep(objPtr, NULL, bytlen);
    Tcl_Obj *eleObj;

    for (i = 0; i < polySeriesRepPtr->len; i++) {
	if (my_PolySeriesObjIndex(NULL, objPtr, i, &eleObj) == TCL_OK) {
	    Tcl_Size slen;
	    char *str = Tcl_GetStringFromObj(eleObj, &slen);
	    strcpy(p, str);
	    p[slen] = ' ';
	    p += slen+1;
	    Tcl_DecrRefCount(eleObj);
	} // else TODO: report error here?
    }

    if (bytlen > 0) objPtr->bytes[bytlen-1] = '\0';
    objPtr->length = bytlen-1;

    return;
}

Tcl_Size
my_PolySeriesObjLength(Tcl_Obj *objPtr)
{
    PolySeries *polySeriesRepPtr = (PolySeries *)objPtr->internalRep.twoPtrValue.ptr1;
    return polySeriesRepPtr->len;
}

static void
FreePolyInternalRep(Tcl_Obj *objPtr)
{
    PolySeries *polySeries = (PolySeries*)objPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Free(polySeries);
    objPtr->internalRep.twoPtrValue.ptr1 = 0;
}

static void
DupPolySeriesRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);

static Tcl_ObjType polyType = {
    "polyseries",
    FreePolyInternalRep,
    DupPolySeriesRep,
    UpdateStringRep,
    NULL,
    TCL_OBJTYPE_V2(
	my_PolySeriesObjLength,
	my_PolySeriesObjIndex,
	NULL, /* slice */
	NULL, /* reverse */
	NULL, /* get elements */
        NULL, /* set element */
        NULL, /* replace */
        NULL  /* get double */)
};

static void
DupPolySeriesRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
    PolySeries *srcPolySeries = (PolySeries*)srcPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Size repSize = sizeof(PolySeries) + (srcPolySeries->n * sizeof(double));
    PolySeries *copyPolySeries = (PolySeries*)Tcl_Alloc(repSize);
    copyPtr->typePtr = &polyType;
    memcpy(copyPolySeries, srcPolySeries, repSize);
    copyPtr->internalRep.twoPtrValue.ptr1 = copyPolySeries;
    return;
}

Tcl_Obj *
my_NewPolySeriesObj(
    int objc,
    Tcl_Obj * const objv[])
{
    Tcl_WideInt length;
    Tcl_Double coeficent, *a;
    int i, n = objc - 1;
    PolySeries *polySeriesRepPtr;
    size_t repSize;
    Tcl_Obj *polySeriesObj;

    if (Tcl_GetWideIntFromObj(NULL, objv[0], &length) != TCL_OK
	|| length < 0) {
	return NULL;
    }

    polySeriesObj = Tcl_NewObj();
    repSize = sizeof(PolySeries) + (n * sizeof(Tcl_WideInt));
    polySeriesRepPtr = (PolySeries*)Tcl_Alloc(repSize);
    polySeriesRepPtr->len = length;
    polySeriesRepPtr->n = n;
    a = &polySeriesRepPtr->a[0];
    for (i=0; i< n; i++) {
	if (Tcl_GetDoubleFromObj(NULL, objv[i+1], &coeficent) != TCL_OK) {
	    return NULL;
	}
	a[i] = coeficent;
    }
    polySeriesObj->internalRep.twoPtrValue.ptr1 = polySeriesRepPtr;
    polySeriesObj->typePtr = &polyType;

    if (length > 0) {
	Tcl_InvalidateStringRep(polySeriesObj);
    } else {
	Tcl_InitStringRep(polySeriesObj, NULL, 0);
    }

    return polySeriesObj;
}

static int
lPolyObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
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
