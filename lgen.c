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
    Tcl_Size len;       // list length
    Tcl_Size nargs;     // Number of arguments in genFn including "index"
    Tcl_Obj *genFnObj;
} LgenSeries;

/*
 * Evaluate the generation function.
 * The provided funtion computes the value for a give index
 */
static Tcl_Obj*
lgen(
    Tcl_Obj* objPtr,
    Tcl_Size index)
{
    LgenSeries *lgenSeriesPtr = objPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Obj *elemObj = NULL;
    Tcl_Interp *intrp = lgenSeriesPtr->interp;
    Tcl_Obj *genCmd = lgenSeriesPtr->genFnObj;
    Tcl_Size endidx = lgenSeriesPtr->nargs-1;

    if (0 <= index && index < lgenSeriesPtr->len) {
	Tcl_Obj *indexObj = Tcl_NewWideIntObj(index);
	Tcl_ListObjReplace(intrp, genCmd, endidx, 1, 1, &indexObj);

	// EVAL DIRECT, just because
	int flags = TCL_EVAL_GLOBAL|TCL_EVAL_DIRECT;
	int status = Tcl_EvalObjEx(intrp, genCmd, flags);
	elemObj = Tcl_GetObjResult(intrp);
	if (status != TCL_OK) {
	    fprintf(stderr,"Error: %s\nwhile executing %s\n",
		   elemObj ? Tcl_GetString(elemObj) : "NULL",
		   Tcl_GetString(genCmd));
	}
	// Interp may be only holder of the result,
	// incr refCount to hold on to it.
	Tcl_IncrRefCount(elemObj);
    }
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

    lgenSeriesRepPtr->interp = interp;

    element = lgen(lgenSeriesObjPtr, index);
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
    Tcl_Obj *tmpstr = Tcl_NewObj();

    lgenSeriesRepPtr = objPtr->internalRep.twoPtrValue.ptr1;

    for (i=0, bytlen=0; i<lgenSeriesRepPtr->len; i++) {
	element = lgen(objPtr, i);
	if (element) {
	    if (i) {
		Tcl_AppendToObj(tmpstr," ",1);
	    }
	    Tcl_AppendObjToObj(tmpstr,element);
	}
    }

    bytlen = Tcl_GetCharLength(tmpstr);
    Tcl_InitStringRep(objPtr, Tcl_GetString(tmpstr), bytlen);
    Tcl_DecrRefCount(tmpstr);

    return;
}

/*
 *  ObjType Free Internal Rep function
 */
static void
FreeLgenInternalRep(Tcl_Obj *objPtr)
{
    LgenSeries *lgenSeries = (LgenSeries*)objPtr->internalRep.twoPtrValue.ptr1;
    if (lgenSeries->genFnObj) {
	Tcl_DecrRefCount(lgenSeries->genFnObj);
    }
    lgenSeries->interp = NULL;
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
        NULL) /* in operation */
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

    copyLgenSeries->interp = srcLgenSeries->interp;
    copyLgenSeries->nargs = srcLgenSeries->nargs;
    copyLgenSeries->len = srcLgenSeries->len;
    copyLgenSeries->genFnObj = Tcl_DuplicateObj(srcLgenSeries->genFnObj);
    Tcl_IncrRefCount(copyLgenSeries->genFnObj);
    copyPtr->typePtr = &lgenType;
    copyPtr->internalRep.twoPtrValue.ptr1 = copyLgenSeries;
    copyPtr->internalRep.twoPtrValue.ptr2 = NULL;
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

    lGenSeriesObj = Tcl_NewObj();
    repSize = sizeof(LgenSeries);
    lGenSeriesRepPtr = (LgenSeries*)Tcl_Alloc(repSize);
    lGenSeriesRepPtr->interp = interp; //Tcl_CreateInterp();
    lGenSeriesRepPtr->len = length;

    // Allocate array of *obj for cmd + index + args
    // objv  length cmd arg1 arg2 arg3 ...
    // argsv         0   1    2    3   ... index

    lGenSeriesRepPtr->nargs = objc;
    lGenSeriesRepPtr->genFnObj = Tcl_NewListObj(objc-1, objv+1);
    // Addd 0 placeholder for index
    Tcl_ListObjAppendElement(interp, lGenSeriesRepPtr->genFnObj, Tcl_NewIntObj(0));
    Tcl_IncrRefCount(lGenSeriesRepPtr->genFnObj);
    lGenSeriesObj->internalRep.twoPtrValue.ptr1 = lGenSeriesRepPtr;
    lGenSeriesObj->internalRep.twoPtrValue.ptr2 = NULL;
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
