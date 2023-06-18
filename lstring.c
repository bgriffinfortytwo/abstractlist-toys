// TCL Command to present a string as a list
#include <string.h>
#include <limits.h>
#include "tcl.h"

/*
 * Forward references
 */

static Tcl_Obj* myNewLStringObj(const char *string, Tcl_Size length);
static void freeRep(Tcl_Obj* alObj);
static Tcl_Obj* my_LStringObjSetElem(Tcl_Interp *interp,
				     Tcl_Obj *lstringObj,
				     Tcl_Size indc,
				     Tcl_Obj *const indexArray[],
				     Tcl_Obj *valueObj);
static void DupLStringRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static int SetLStringFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void UpdateStringRep(Tcl_Obj *objPtr);
static Tcl_Size my_LStringObjLength(Tcl_Obj *lstringObjPtr);
static int my_LStringObjIndex(Tcl_Interp *interp,
			      Tcl_Obj *lstringObj,
			      Tcl_Size index,
			      Tcl_Obj **charObjPtr);
static int my_LStringObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj,
				Tcl_Obj **newObjPtr);
static int my_LStringReplace(Tcl_Interp *interp,
		      Tcl_Obj *listObj,
		      Tcl_Size first,
		      Tcl_Size numToDelete,
		      Tcl_Size numToInsert,
		      Tcl_Obj *const insertObjs[]);

/*
 * Internal Representation of an lstring type value
 */

typedef struct LString {
    char *string;          // NULL terminated utf-8 string
    Tcl_Size strlen;    // num bytes in string
    Tcl_Size allocated; // num bytes allocated
} LString;

/*
 * AbstractList definition of an lstring type
 */
static Tcl_ObjType lstringType = {
    "lstring",
    freeRep,
    DupLStringRep,
    UpdateStringRep,
    SetLStringFromAny,
    TCL_OBJTYPE_V2(
	my_LStringObjLength,
	my_LStringObjIndex,
	NULL, /*ObjRange*/
	my_LStringObjReverse,
        NULL, /*my_LStringGetElements*/
	my_LStringObjSetElem,
	my_LStringReplace)
};




/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjIndex --
 *
 *	Implements the AbstractList Index function for the lstring type.  The
 *	Index function returns the value at the index position given. Caller
 *	is resposible for freeing the Obj.
 *
 * Results:
 *	TCL_OK on success. Returns a new Obj, with a 0 refcount in the
 *	supplied charObjPtr location. Call has ownership of the Obj.
 *
 * Side effects:
 *	Obj allocated.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_Size index,
    Tcl_Obj **charObjPtr)
{
  LString *lstringRepPtr = (LString*)lstringObj->internalRep.twoPtrValue.ptr1;

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


/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjLength --
 *
 *	Implements the AbstractList Length function for the lstring type.
 *	The Length function returns the number of elements in the list.
 *
 * Results:
 *	WideInt number of elements in the list.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Size
my_LStringObjLength(Tcl_Obj *lstringObjPtr)
{
    LString *lstringRepPtr = (LString *)lstringObjPtr->internalRep.twoPtrValue.ptr1;
    return lstringRepPtr->strlen;
}


/*
 *----------------------------------------------------------------------
 *
 * DupLStringRep --
 *
 *	Replicates the internal representation of the src value, and storing
 *	it in the copy
 *
 * Results:
 *	void
 *
 * Side effects:
 *	Modifies the rep of the copyObj.
 *
 *----------------------------------------------------------------------
 */

static void
DupLStringRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  LString *srcLString = (LString*)srcPtr->internalRep.twoPtrValue.ptr1;
  LString *copyLString = (LString*)Tcl_Alloc(sizeof(LString));

  memcpy(copyLString, srcLString, sizeof(LString));
  copyLString->string = (char*)Tcl_Alloc(srcLString->allocated);
  strcpy(copyLString->string, srcLString->string);
  copyPtr->internalRep.twoPtrValue.ptr1 = copyLString;
  copyPtr->typePtr = &lstringType;

  return;
}

/*
 *----------------------------------------------------------------------
 *
 * SetLStringFromAny
 *
 *   lstring can be constructed from any string.
 *
 */

static int
SetLStringFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_Size repSize = sizeof(LString);
    char *string = objPtr->bytes; //Tcl_GetString(objPtr);
    LString *lstringRepPtr = (LString*)Tcl_Alloc(repSize);
    lstringRepPtr->strlen = strlen(string);
    lstringRepPtr->allocated = lstringRepPtr->strlen + 1;
    lstringRepPtr->string = (char*)Tcl_Alloc(lstringRepPtr->allocated);
    strncpy(lstringRepPtr->string, string, lstringRepPtr->strlen);
    objPtr->internalRep.twoPtrValue.ptr1 = lstringRepPtr;
    objPtr->typePtr = &lstringType;
    if (lstringRepPtr->strlen > 0) {
	Tcl_InvalidateStringRep(objPtr);
    } else {
	Tcl_InitStringRep(objPtr, NULL, 0);
    }
    return TCL_OK;
}

/*
 * UpdateStringRep
 *
 *    Is duplication of data(?)
 */
static void
UpdateStringRep(Tcl_Obj *objPtr)
{
    LString *lstringRepPtr = (LString*)objPtr->internalRep.twoPtrValue.ptr1;
    Tcl_InitStringRep(objPtr, lstringRepPtr->string, lstringRepPtr->strlen);
}


/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjSetElem --
 *
 *	Replace the element value at the given (nested) index with the
 *	valueObj provided.  If the lstring obj is shared, a new list is
 *	created conntaining the modifed element.
 *
 * Results:
 *	The modifed lstring is returned, either new or original. If the
 *	index is invalid, NULL is returned, and an error is added to the
 *	interp, if provided.
 *
 * Side effects:
 *	A new obj may be created.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
my_LStringObjSetElem(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_Size indc,
    Tcl_Obj *const indexArray[],
    Tcl_Obj *valueObj)
{
    LString *lstringRepPtr = (LString*)lstringObj->internalRep.twoPtrValue.ptr1;
    Tcl_Size index;
    const char *newvalue;
    int status;
    Tcl_Obj *returnObj;

    if (indc > 1) {
	Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("Multiple indicies not supported by lstring."));
	return NULL;
    }

    status = Tcl_GetIntForIndex(interp, indexArray[0], lstringRepPtr->strlen, &index);
    if (status != TCL_OK) {
	return NULL;
    }

    returnObj = Tcl_IsShared(lstringObj) ? Tcl_DuplicateObj(lstringObj) : lstringObj;
    lstringRepPtr = (LString*)returnObj->internalRep.twoPtrValue.ptr1;
    returnObj->typePtr = lstringObj->typePtr;

    if (index < 0) {
	index = 0;
    }
    if (index >= lstringRepPtr->strlen) {
	index = lstringRepPtr->strlen;
	lstringRepPtr->strlen++;
	lstringRepPtr->string = (char*)Tcl_Realloc(lstringRepPtr->string, lstringRepPtr->strlen+1);
    }

    newvalue = Tcl_GetString(valueObj);
    lstringRepPtr->string[index] = newvalue[0];

    Tcl_InvalidateStringRep(returnObj);

    return returnObj;
}


/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjReverse --
 *
 *	Creates a new Obj with the the order of the elements in the lstring
 *	value reversed, where first is last and last is first, etc.
 *
 * Results:
 *	A new Obj is assigned to newObjPtr. Returns TCL_OK
 *
 * Side effects:
 *	A new Obj is created.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj, Tcl_Obj **newObjPtr)
{
    Tcl_Obj *revObj = Tcl_NewObj();
    LString *srcRep = (LString*)srcObj->internalRep.twoPtrValue.ptr1;
    LString *revRep = (LString*)Tcl_Alloc(sizeof(LString));
    Tcl_Size len;
    char *srcp, *dstp, *endp;
    len = srcRep->strlen;
    revRep->strlen = len;
    revRep->allocated = len+1;
    revRep->string = (char*)Tcl_Alloc(revRep->allocated);
    srcp = srcRep->string;
    endp = &srcRep->string[len];
    dstp = &revRep->string[len];
    *dstp-- = 0;
    while (srcp < endp) {
	*dstp-- = *srcp++;
    }
    revObj->internalRep.twoPtrValue.ptr1 = revRep;
    revObj->typePtr = &lstringType;
    *newObjPtr = revObj;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringReplace --
 *
 *	Delete and/or Insert elements in the list, starting at index first.
 *	See more details in the comments below. This should not be called with
 *	a Shared Obj.
 *
 * Results:
 *	The value of the listObj is modified.
 *
 * Side effects:
 *	The string rep is invalidated.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringReplace(
    Tcl_Interp *interp,
    Tcl_Obj *listObj,
    Tcl_Size first,
    Tcl_Size numToDelete,
    Tcl_Size numToInsert,
    Tcl_Obj *const insertObjs[])
{
    LString *lstringRep = (LString*)listObj->internalRep.twoPtrValue.ptr1;
    Tcl_WideInt newLen;
    Tcl_WideInt x, ix, kx;
    char *newStr;
    char *oldStr = lstringRep->string;

    if (numToDelete < 0) numToDelete = 0;
    if (numToInsert < 0) numToInsert = 0;

    newLen = lstringRep->strlen - numToDelete + numToInsert;

    if (newLen >= lstringRep->allocated) {
	lstringRep->allocated = newLen+1;
	newStr = Tcl_Alloc(lstringRep->allocated);
	newStr[newLen] = 0;
    } else {
	newStr = oldStr;
    }

    /* Tcl_ListObjReplace replaces zero or more elements of the list
     * referenced by listPtr with the objc values in the array referenced by
     * objv.
     *
     * If listPtr does not point to a list value, Tcl_ListObjReplace
     * will attempt to convert it to one; if the conversion fails, it returns
     * TCL_ERROR and leaves an error message in the interpreter's result value
     * if interp is not NULL. Otherwise, it returns TCL_OK after replacing the
     * values.
     *
     *    * If objv is NULL, no new elements are added.
     *
     *    * If the argument first is zero or negative, it refers to the first
     *      element.
     *
     *    * If first is greater than or equal to the number of elements in the
     *      list, then no elements are deleted; the new elements are appended
     *      to the list. count gives the number of elements to replace.
     *
     *    * If count is zero or negative then no elements are deleted; the new
     *      elements are simply inserted before the one designated by first.
     *      Tcl_ListObjReplace invalidates listPtr's old string representation.
     *
     *    * The reference counts of any elements inserted from objv are
     *      incremented since the resulting list now refers to them. Similarly,
     *      the reference counts for any replaced values are decremented.
     */

    // copy 0 to first-1
    if (newStr != oldStr) {
	strncpy(newStr, oldStr, first);
    }

    // move front elements to keep
    for(x=0, kx=0; x<newLen && kx<first; kx++, x++) {
	newStr[x] = oldStr[kx];
    }
    // Insert new elements into new string
    for(x=first, ix=0; ix<numToInsert; x++, ix++) {
	char const *svalue = Tcl_GetString(insertObjs[ix]);
	newStr[x] = svalue[0];
    }
    // Move remaining elements
    if ((first+numToDelete) < newLen) {
	for(/*x,*/ kx=first+numToDelete; (kx <lstringRep->strlen && x<newLen); x++, kx++) {
	    newStr[x] = oldStr[kx];
	}
    }

    // Terminate new string.
    newStr[newLen] = 0;


    if (oldStr != newStr) {
	Tcl_Free(oldStr);
    }
    lstringRep->string = newStr;
    lstringRep->strlen = newLen;

    /* Changes made to value, string rep no longer valid */
    Tcl_InvalidateStringRep(listObj);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * my_NewLStringObj --
 *
 *	Creates a new lstring Obj using the string value of objv[0]
 *
 * Results:
 *	results
 *
 * Side effects:
 *	side effects
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
my_NewLStringObj(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    const char *string;
    Tcl_Size length;
    if (objc != 1) {
	return NULL;
    }
    string = Tcl_GetStringFromObj(objv[0],&length);
    return myNewLStringObj(string, length);
}


static Tcl_Obj*
myNewLStringObj(
    const char *string,
    Tcl_Size length)
{
    Tcl_Size repSize = sizeof(LString);
    Tcl_Obj *lstringPtr = Tcl_NewObj();
    lstringPtr->typePtr = &lstringType;
    LString* lstringRepPtr = (LString*)Tcl_Alloc(repSize);
    lstringRepPtr->strlen = length;
    lstringRepPtr->allocated = lstringRepPtr->strlen + 1;
    lstringRepPtr->string = (char*)Tcl_Alloc(lstringRepPtr->allocated);
    strncpy(lstringRepPtr->string, string, length);
    lstringPtr->internalRep.twoPtrValue.ptr1 = lstringRepPtr;
    if (lstringRepPtr->strlen > 0) {
	Tcl_InvalidateStringRep(lstringPtr);
    } else {
	Tcl_InitStringRep(lstringPtr, NULL, 0);
    }

    return lstringPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * freeRep --
 *
 *	Free the value storage of the lstring Obj.
 *
 * Results:
 *	void
 *
 * Side effects:
 *	Memory free'd.
 *
 *----------------------------------------------------------------------
 */

static void
freeRep(Tcl_Obj* lstringObj)
{
    LString *lstringRepPtr = (LString*)lstringObj->internalRep.twoPtrValue.ptr1;
    if (lstringRepPtr->string) {
	Tcl_Free(lstringRepPtr->string);
    }
    Tcl_Free((char*)lstringRepPtr);
    lstringObj->internalRep.twoPtrValue.ptr1 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * lLStringObjCmd --
 *
 *	Script level command that creats an lstring Obj value.
 *
 * Results:
 *	Returns and lstring Obj value in the interp results.
 *
 * Side effects:
 *	Interp results modified.
 *
 *----------------------------------------------------------------------
 */

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

/*
 *----------------------------------------------------------------------
 *
 * Lstring_Init --
 *
 *	DL load init function. Defines the "lstring" command.
 *
 * Results:
 *	"lstring" command added to the interp.
 *
 * Side effects:
 *	A new command is defined.
 *
 *----------------------------------------------------------------------
 */

int Lstring_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lstring", lLStringObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lstring", "1.0.0");
    return TCL_OK;
}
