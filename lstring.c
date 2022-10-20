// TCL Command to present a string as a list
#include <string.h>
#include <limits.h>
#include "tcl.h"
#include "tclDecls.h"


typedef struct LString {
    char *string;          // NULL terminated utf-8 string
    Tcl_WideInt strlen;    // num bytes in string
} LString;


int
my_LStringObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_WideInt index,
    Tcl_Obj **charObjPtr)
{
  LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);

  if (0 <= index && index < lstringRepPtr->strlen) {
      char cchar[2];
      cchar[0] = lstringRepPtr->string[index];
      cchar[1] = 0;
      *charObjPtr = Tcl_NewStringObj(cchar,1);
  } else {
      *charObjPtr = Tcl_NewObj();
  }

  return TCL_OK;
}

Tcl_WideInt
my_LStringObjLength(Tcl_Obj *lstringObjPtr)
{
    LString *lstringRepPtr = (LString *)Tcl_AbstractListGetConcreteRep(lstringObjPtr);
    return lstringRepPtr->strlen;
}

static void
DupLStringRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  LString *srcLString = (LString*)Tcl_AbstractListGetConcreteRep(srcPtr);
  LString *copyLString = (LString*)Tcl_Alloc(sizeof(LString));

  memcpy(copyLString, srcLString, sizeof(LString));
  copyLString->string = (char*)Tcl_Alloc(srcLString->strlen+1);
  strcpy(copyLString->string, srcLString->string);
  Tcl_AbstractListSetConcreteRep(copyPtr,copyLString);

  return;
}

Tcl_Obj *myNewLStringObj(Tcl_WideInt start,
			 Tcl_WideInt length);
static void freeRep(Tcl_Obj* alObj);
static Tcl_Obj* my_LStringObjSetElem(Tcl_Interp *interp,
				     Tcl_Obj *listPtr,
				     Tcl_Obj *indicies,
				     Tcl_Obj *valueObj);
int my_LStringObjReverse(Tcl_Interp *interp,
			  Tcl_Obj *srcObj,
			  Tcl_Obj **newObjPtr);

static Tcl_AbstractListType lstringType = {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	NULL/*ObjRange*/,
	my_LStringObjReverse,
        NULL/*my_LStringGetElements*/,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem /* use default update string */
};

Tcl_Obj *
my_NewLStringObj(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    LString *lstringRepPtr;
    size_t repSize;
    Tcl_Obj *lstringPtr;
    const char *string;

    if (objc != 1) {
	return NULL;
    }
    string = Tcl_GetString(objv[0]);

    repSize = sizeof(LString);
    lstringPtr = Tcl_AbstractListObjNew(interp, &lstringType);
    lstringRepPtr = (LString*)Tcl_Alloc(repSize);
    lstringRepPtr->strlen = strlen(string);
    lstringRepPtr->string = (char*)Tcl_Alloc(lstringRepPtr->strlen+1);
    strcpy(lstringRepPtr->string, string);
    Tcl_AbstractListSetConcreteRep(lstringPtr, lstringRepPtr);
    if (lstringRepPtr->strlen > 0) {
	Tcl_InvalidateStringRep(lstringPtr);
    } else {
	Tcl_InitStringRep(lstringPtr, NULL, 0);
    }

    return lstringPtr;
}

static int
lLStringObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
  Tcl_Obj *lstringObj;

  if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "string");
      return TCL_ERROR;
  }

  lstringObj = my_NewLStringObj(interp, objc-1, &objv[1]);

  if (lstringObj) {
      Tcl_SetObjResult(interp, lstringObj);
      return TCL_OK;
  }
  return TCL_ERROR;
}

int Lstring_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lstring", lLStringObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lstring", "1.0.0");
    return TCL_OK;
}

static void
freeRep(Tcl_Obj* lstringObj)
{
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    if (lstringRepPtr->string) {
	Tcl_Free(lstringRepPtr->string);
    }
    Tcl_Free((char*)lstringRepPtr);
    Tcl_AbstractListSetConcreteRep(lstringObj, NULL);
}

static Tcl_Obj*
my_LStringObjSetElem(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_Obj *indicies,
    Tcl_Obj *valueObj)
{
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    int indc;
    Tcl_Obj **indv;
    int index;
    const char *newvalue;
    int status;
    Tcl_Obj *returnObj;

    if (Tcl_ListObjGetElements(interp, indicies, &indc, &indv) != TCL_OK) {
	return NULL;
    }

    if (indc > 1) {
	Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("Multiple indicies not supported by lstring."));
	return NULL;
    }

    status = Tcl_GetIntForIndex(interp, indv[0], lstringRepPtr->strlen, &index);
    if (status != TCL_OK) {
	return NULL;
    }

    if (index > lstringRepPtr->strlen) {
	// TODO: need error messsage
	return NULL;
    }

    returnObj = Tcl_IsShared(lstringObj) ? Tcl_DuplicateObj(lstringObj) : lstringObj;
    lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(returnObj);

    if (index == lstringRepPtr->strlen) {
	char *newstr = Tcl_Realloc(lstringRepPtr->string, index+1);
	lstringRepPtr->strlen = index+1;
	newstr[index] = 0;
	newstr[index+1] = 0;
    }

    newvalue = Tcl_GetString(valueObj);
    lstringRepPtr->string[index] = newvalue[0];

    Tcl_InvalidateStringRep(returnObj);

    return returnObj;
}

int my_LStringObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj, Tcl_Obj **newObjPtr)
{
    Tcl_Obj *revObj = Tcl_AbstractListObjNew(interp, &lstringType);
    LString *srcRep = (LString*)Tcl_AbstractListGetConcreteRep(srcObj);
    LString *revRep = (LString*)Tcl_Alloc(sizeof(LString));
    Tcl_WideInt i, len;
    char *srcp, *dstp, *endp;
    len = srcRep->strlen;
    revRep->strlen = len;
    revRep->string = (char*)Tcl_Alloc(len+1);
    srcp = srcRep->string;
    endp = &srcRep->string[len];
    dstp = &revRep->string[len];
    *dstp-- = 0;
    while (srcp < endp) {
	*dstp-- = *srcp++;
    }
    Tcl_AbstractListSetConcreteRep(revObj, revRep);
    *newObjPtr = revObj;
    return TCL_OK;
}

/* readlines.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)
lstring.so: lstring.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

*/
