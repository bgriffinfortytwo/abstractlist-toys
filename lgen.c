/*
** lgen - Derived from TIP 192 - Lazy Lists
** Generate a list using a command provided as argument(s).
** The command computes the value for a given index.
*/

#include <math.h>
#include <string.h>
#include "tcl.h"
#include "tclDecls.h"

/*
 * Internal rep for the Generate Series
 */
typedef struct LgenSeries {
    Tcl_Interp *interp; // used to evaluate gen script
    Tcl_Size len;     // list length
    Tcl_Size nargs;   // size of argv
    Tcl_Obj **argsv;  // function and args
} LgenSeries;

/*
 * Evaluate the generation function.
 * The provided funtion computes the value for a give index
 */
static Tcl_Obj*
lgen(
    LgenSeries *lgenSeriesPtr,
    Tcl_Size index)
{
    Tcl_Obj *elemObj = NULL;
    Tcl_Interp *intrp = lgenSeriesPtr->interp;
    Tcl_InterpState state = Tcl_SaveInterpState(intrp, TCL_OK);
    Tcl_Size nargs = lgenSeriesPtr->nargs;
    Tcl_Obj **argsv = lgenSeriesPtr->argsv;
    int status;
    if (0 <= index && index < lgenSeriesPtr->len) {
	Tcl_Obj *indexObj = Tcl_NewWideIntObj(index);
	argsv[1] = indexObj;
	Tcl_IncrRefCount(argsv[1]);
	int flags = 0;
	status = Tcl_EvalObjv(intrp, nargs, argsv, flags);
	elemObj = (status == TCL_OK)
	    ? Tcl_GetObjResult(intrp)
	    : Tcl_NewObj();
	// Interp may be only holder of the result,
	// bump refCount to hold on to it.
	Tcl_IncrRefCount(elemObj);
	Tcl_DecrRefCount(argsv[1]);
	argsv[1] = NULL;
    }
    Tcl_RestoreInterpState(intrp, state);
    return elemObj;
}

/*
 *  Abstract List Length function
 */
static Tcl_Size
lgenSeriesObjLength(Tcl_Obj *objPtr)
{
    LgenSeries *lgenSeriesRepPtr = (LgenSeries *)objPtr->internalRep.twoPtrValue.ptr1;
    return lgenSeriesRepPtr->len;
}

/*
 *  Abstract List Index function
 */
static int
lgenSeriesObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *lgenSeriesObjPtr,
    Tcl_Size index,
    Tcl_Obj **elemPtr)
{
    LgenSeries *lgenSeriesRepPtr;
    Tcl_Obj *element;

    lgenSeriesRepPtr = lgenSeriesObjPtr->internalRep.twoPtrValue.ptr1;

    if (index < 0 || index >= lgenSeriesRepPtr->len)
	return TCL_ERROR;

    if (lgenSeriesRepPtr->interp == NULL && interp == NULL) {
	return TCL_ERROR;
    }
    if (lgenSeriesRepPtr->interp == NULL) {
	lgenSeriesRepPtr->interp = interp;
    }

    element = lgen(lgenSeriesRepPtr, index);

    if (element) {
	*elemPtr = element;
    } else {
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *  ObjType String function
 */
static void
UpdateStringRep(Tcl_Obj *objPtr)
{
    LgenSeries *lgenSeriesRepPtr;
    Tcl_Obj *element;
    Tcl_Size i;
    size_t bytlen;
    Tcl_Size slen;
    char *str;

    lgenSeriesRepPtr = objPtr->internalRep.twoPtrValue.ptr1;

    for (i=0, bytlen=0; i<lgenSeriesRepPtr->len; i++) {
	element = lgen(lgenSeriesRepPtr, i);
	Tcl_IncrRefCount(element);
	Tcl_GetStringFromObj(element,&slen);
	if ((bytlen + slen) < 0) {
	    break; // overflow
	}
	Tcl_DecrRefCount(element);
	bytlen += slen;
    }

    bytlen += lgenSeriesRepPtr->len; // Space for each separator
    char *p = Tcl_InitStringRep(objPtr, NULL, bytlen);

    for (i = 0; i < lgenSeriesRepPtr->len; i++) {
	if (lgenSeriesObjIndex(NULL, objPtr, i, &element) == TCL_OK) {
	    Tcl_IncrRefCount(element);
	    str = Tcl_GetStringFromObj(element, &slen);
	    strcpy(p, str);
	    p[slen] = ' ';
	    p += slen+1;
	    Tcl_DecrRefCount(element);
	} // else TODO: report error here?
    }

    if (bytlen > 0) objPtr->bytes[bytlen-1] = '\0';
    objPtr->length = bytlen-1;

    return;
}

/*
 *  ObjType Free Internal Rep function
 */
static void
FreeLgenInternalRep(Tcl_Obj *objPtr)
{
    LgenSeries *lgenSeries = (LgenSeries*)objPtr->internalRep.twoPtrValue.ptr1;
    for (Tcl_Size i = 0;i < lgenSeries->nargs; i++) {
	if (lgenSeries->argsv[i]) {
	    Tcl_DecrRefCount(lgenSeries->argsv[i]);
	}
    }
    Tcl_Free(lgenSeries->argsv);
    Tcl_Free(lgenSeries);
    objPtr->internalRep.twoPtrValue.ptr1 = 0;
}

static void DupLgenSeriesRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);

/*
 *  Abstract List ObjType definition
 */

static Tcl_ObjType lgenType = {
    "lgenseries",
    FreeLgenInternalRep,
    DupLgenSeriesRep,
    UpdateStringRep,
    NULL, /* SetFromAnyProc */
    TCL_OBJTYPE_V2(
	lgenSeriesObjLength,
	lgenSeriesObjIndex,
	NULL, /* slice */
	NULL, /* reverse */
	NULL, /* get elements */
        NULL, /* set element */
        NULL, /* replace */
        NULL  /* get double */)
};

/*
 *  ObjType Duplicate Internal Rep Function
 */
static void
DupLgenSeriesRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *copyPtr)
{
    LgenSeries *srcLgenSeries = (LgenSeries*)srcPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Size repSize = sizeof(LgenSeries);
    LgenSeries *copyLgenSeries = (LgenSeries*)Tcl_Alloc(repSize);
    Tcl_Size i;

    copyLgenSeries->interp = srcLgenSeries->interp;
    copyLgenSeries->nargs = srcLgenSeries->nargs;
    copyLgenSeries->len = srcLgenSeries->len;
    copyLgenSeries->argsv = (Tcl_Obj**)Tcl_Alloc(copyLgenSeries->nargs * sizeof(Tcl_Obj*));
    for (i=0; i<copyLgenSeries->nargs; i++) {
	copyLgenSeries->argsv[i] = srcLgenSeries->argsv[i];
	if (copyLgenSeries->argsv[i] != NULL) {
	    Tcl_IncrRefCount(copyLgenSeries->argsv[i]);
	}
    }
    copyPtr->typePtr = &lgenType;
    copyPtr->internalRep.twoPtrValue.ptr1 = copyLgenSeries;
    return;
}

/*
 *  Create a new lgen Tcl_Obj
 */
Tcl_Obj *
newLgenObj(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    Tcl_WideInt length;
    Tcl_Obj **argsv;
    int i, argsc = objc;
    LgenSeries *lGenSeriesRepPtr;
    Tcl_Size repSize;
    Tcl_Obj *lGenSeriesObj;

    if (objc < 2) {
	return NULL;
    }

    if (Tcl_GetWideIntFromObj(NULL, objv[0], &length) != TCL_OK
	|| length < 0) {
	return NULL;
    }

    // Allocate array of *obj for cmd + index + args
    // objv  length cmd arg1 arg2 arg3 ...
    // argsv index   0   2    3    4    5   ...
    argsv = (Tcl_Obj**)Tcl_Alloc( argsc * sizeof(Tcl_Obj*) );
    argsv[0] = objv[1];
    Tcl_IncrRefCount(argsv[0]);
    argsv[1] = NULL; // filled in later with index value
    for (i=2; i<argsc; i++) {
	argsv[i] = objv[i];
	Tcl_IncrRefCount(argsv[i]);
    }
    lGenSeriesObj = Tcl_NewObj();
    repSize = sizeof(LgenSeries);
    lGenSeriesRepPtr = (LgenSeries*)Tcl_Alloc(repSize);
    lGenSeriesRepPtr->interp = interp;
    lGenSeriesRepPtr->len = length;
    lGenSeriesRepPtr->argsv = argsv;
    lGenSeriesRepPtr->nargs = argsc;
    lGenSeriesObj->internalRep.twoPtrValue.ptr1 = lGenSeriesRepPtr;
    lGenSeriesObj->typePtr = &lgenType;

    if (length > 0) {
	Tcl_InvalidateStringRep(lGenSeriesObj);
    } else {
	Tcl_InitStringRep(lGenSeriesObj, NULL, 0);
    }

    return lGenSeriesObj;
}

/*
 *  The [lgen] command
 */
static int
lGenObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    Tcl_Obj *genObj = newLgenObj(interp, objc-1, &objv[1]);
    if (genObj) {
	Tcl_SetObjResult(interp, genObj);
	return TCL_OK;
    }
    Tcl_WrongNumArgs(interp, 1, objv, "length cmd ?args?");
    return TCL_ERROR;
}

/*
 *  lgen package init
 */
int Lgen_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lgen", lGenObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lgen", "1.0");
    return TCL_OK;
}

/* lgen.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)
lgen.so: lgen.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) lgen.c -o lgen.so -m64  --shared $(TCL_STUBLIB)

*/
