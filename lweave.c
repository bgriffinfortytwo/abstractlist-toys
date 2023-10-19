// TCL Command to interweave multiple lists together into a single list
#include <limits.h>
#include <string.h>
#include "tcl.h"

/*
 * Forward references
 */

static Tcl_Obj* myNewLWeaveObj(const Tcl_Size objc, Tcl_Obj* const objv[]);
static void freeRep(Tcl_Obj* alObj);
static Tcl_Obj* my_LWeaveObjSetElem(Tcl_Interp *interp,
				     Tcl_Obj *lweaveObj,
				     Tcl_Size indc,
				     Tcl_Obj *const indexArray[],
				     Tcl_Obj *valueObj);
static void DupLWeaveRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static int SetLWeaveFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void UpdateStringRep(Tcl_Obj *objPtr);
static Tcl_Size my_LWeaveObjLength(Tcl_Obj *lweaveObjPtr);
static int my_LWeaveObjIndex(Tcl_Interp *interp,
			      Tcl_Obj *lweaveObj,
			      Tcl_Size index,
			      Tcl_Obj **charObjPtr);

#if 0 // Not Implemented
static int my_LWeaveObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj,
				Tcl_Obj **newObjPtr);
static int my_LWeaveReplace(Tcl_Interp *interp,
		      Tcl_Obj *listObj,
		      Tcl_Size first,
		      Tcl_Size numToDelete,
		      Tcl_Size numToInsert,
		      Tcl_Obj *const insertObjs[]);
#endif // Not Implemented

/*
 * Internal Representation of an lweave type value
 */

typedef struct LWeave {
    Tcl_Obj* *lists;     // List of lists
    Tcl_Size nlists;     // number of lists
    Tcl_Size nominalLen; // Length of each list
			 // (actual length may vary; this is the max.)
    Tcl_Size llen;       // Total list length
    Tcl_Size allocated;  // num bytes allocated to lists.
} LWeave;

/*
 * AbstractList definition of an lweave type
 */
static Tcl_ObjType lweaveType = {
    "lweave",
    freeRep,
    DupLWeaveRep,
    UpdateStringRep,
    SetLWeaveFromAny,
    TCL_OBJTYPE_V2(
	my_LWeaveObjLength,
	my_LWeaveObjIndex,
	NULL, /*ObjRange*/
	NULL, /*my_LWeaveObjReverse*/
	NULL, /*my_LWeaveGetElements*/
	my_LWeaveObjSetElem,
	NULL, /*my_LWeaveReplace,*/
	NULL) /* in operation */
};


/*
 *----------------------------------------------------------------------
 *
 * my_LWeaveObjIndex --
 *
 *	Implements the AbstractList Index function for the lweave type.  The
 *	Index function returns the value at the index position given. Caller
 *	is resposible for freeing the Obj.
 *
 * Results:
 *	TCL_OK on success. Returns a new Obj, with a 0 refcount in the
 *	supplied objPtr location. Call has ownership of the Obj.
 *
 * Side effects:
 *	Obj allocated.
 *
 *----------------------------------------------------------------------
 */

static int
my_LWeaveObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *lweaveObj,
    Tcl_Size index,
    Tcl_Obj **objPtr)
{
  LWeave *lweaveRepPtr = (LWeave*)lweaveObj->internalRep.twoPtrValue.ptr1;

  if (0 <= index && index < lweaveRepPtr->llen) {
    Tcl_Size listi = index % lweaveRepPtr->nlists;
    Tcl_Size elemi = index / lweaveRepPtr->nlists;
    Tcl_Obj *ribbon = lweaveRepPtr->lists[listi];
    Tcl_Size ribbonLen;
    Tcl_ListObjLength(interp, ribbon, &ribbonLen);
    if (elemi < ribbonLen) {
      return Tcl_ListObjIndex(interp, ribbon, elemi, objPtr);
    }
  }

  *objPtr = Tcl_NewObj();
  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * my_LWeaveObjLength --
 *
 *	Implements the AbstractList Length function for the lweave type.
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
my_LWeaveObjLength(Tcl_Obj *lweaveObjPtr)
{
    LWeave *lweaveRepPtr = (LWeave *)lweaveObjPtr->internalRep.twoPtrValue.ptr1;
    return lweaveRepPtr->llen;
}


/*
 *----------------------------------------------------------------------
 *
 * DupLWeaveRep --
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
DupLWeaveRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  LWeave *srcLWeave = (LWeave*)srcPtr->internalRep.twoPtrValue.ptr1;
  LWeave *copyLWeave = (LWeave*)Tcl_Alloc(sizeof(LWeave));

  memcpy(copyLWeave, srcLWeave, sizeof(LWeave));
  copyLWeave->lists = (Tcl_Obj**)Tcl_Alloc(srcLWeave->allocated);
  for (Tcl_Size i = 0; i < copyLWeave->nlists; i++) {
    copyLWeave->lists[i] = srcLWeave->lists[i];
    Tcl_IncrRefCount(copyLWeave->lists[i]);
  }
  copyPtr->typePtr = &lweaveType;
  return;
}

/*
 *----------------------------------------------------------------------
 *
 * SetLWeaveFromAny
 *
 *   lweave constructed from any string.
 *
 */

static int
SetLWeaveFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
  return TCL_ERROR; // Cannot create lweave from string. It's just a list.
}

/*
 * UpdateStringRep
 */
static void
UpdateStringRep(Tcl_Obj *objPtr)
{
    LWeave *lweaveRepPtr = (LWeave*)objPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Size nelements = lweaveRepPtr->llen;
    Tcl_Size ix = 0;
    int *flagPtr = (int*)Tcl_Alloc(nelements * sizeof(int));
    char *str, *rp, *start;
    Tcl_Size slen, elemlen;
    Tcl_Obj *elemObj;

    // First pass: compute string space needed.
    slen = 0;
    for (ix=0; ix < nelements; ix++) {
	my_LWeaveObjIndex(NULL, objPtr, ix, &elemObj);
	Tcl_IncrRefCount(elemObj);
	str = Tcl_GetStringFromObj(elemObj, NULL);
	elemlen = Tcl_ScanElement(str, flagPtr+ix);
	slen += (elemlen + 1); /* for space */
	Tcl_DecrRefCount(elemObj);
    }

    start = rp = Tcl_InitStringRep(objPtr, NULL, slen);

    // Second pass: create string rep
    for (ix=0; ix<nelements; ix++) {
	flagPtr[ix] |= (ix ? TCL_DONT_QUOTE_HASH : 0);
	my_LWeaveObjIndex(NULL, objPtr, ix, &elemObj);
	Tcl_IncrRefCount(elemObj);
	str = Tcl_GetStringFromObj(elemObj, NULL);
	rp += Tcl_ConvertElement(str, rp, flagPtr[ix]);
	*rp++ = ' ';
	Tcl_DecrRefCount(elemObj);
    }

    /* Set string rep length */
    (void) Tcl_InitStringRep(objPtr, NULL, rp - 1 - start);

}


/*
 *----------------------------------------------------------------------
 *
 * my_LWeaveObjSetElem --
 *
 *	Replace the element value at the given (nested) index with the
 *	valueObj provided.  If the lweave obj is shared, a new list is
 *	created conntaining the modifed element.
 *
 * Results:
 *	The modifed lweave is returned, either new or original. If the
 *	index is invalid, NULL is returned, and an error is added to the
 *	interp, if provided.
 *
 * Side effects:
 *	A new obj may be created.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
my_LWeaveObjSetElem(
    Tcl_Interp *interp,
    Tcl_Obj *lweaveObj,
    Tcl_Size indc,
    Tcl_Obj *const indexArray[],
    Tcl_Obj *valueObj)
{
    LWeave *lweaveRepPtr = (LWeave*)lweaveObj->internalRep.twoPtrValue.ptr1;
    Tcl_Size index;
    int status;
    Tcl_Obj *returnObj;

    if (indc > 1) {
        Tcl_SetObjResult(interp,
                         Tcl_ObjPrintf("Multiple indicies not supported by lweave."));
        return NULL;
    }

    status = Tcl_GetIntForIndex(interp, indexArray[0], lweaveRepPtr->llen, &index);
    if (status != TCL_OK) {
        return NULL;
    }

    returnObj = Tcl_IsShared(lweaveObj) ? Tcl_DuplicateObj(lweaveObj) : lweaveObj;
    lweaveRepPtr = (LWeave*)returnObj->internalRep.twoPtrValue.ptr1;
    returnObj->typePtr = lweaveObj->typePtr;

    if (index < 0) {
        index = 0;
    }

    Tcl_Size elemi = index % lweaveRepPtr->nlists;
    Tcl_Size listi = index / lweaveRepPtr->nlists;
    Tcl_Obj *ribbon = lweaveRepPtr->lists[listi];
    Tcl_Size ribbonLen;
    Tcl_ListObjLength(NULL, ribbon, &ribbonLen);

    if (elemi > lweaveRepPtr->nominalLen) {
        // Index out-of-range
        return NULL;
    }

    if (valueObj) {
        if (Tcl_IsShared(ribbon)) {
            Tcl_Obj *dupRibbon = Tcl_DuplicateObj(ribbon);
            Tcl_DecrRefCount(lweaveRepPtr->lists[listi]);
            ribbon = dupRibbon;
            lweaveRepPtr->lists[listi] = ribbon;
            Tcl_IncrRefCount(ribbon);
        }
        if (elemi < ribbonLen) {
            Tcl_ListObjReplace(interp, ribbon, elemi, 1, 1, &valueObj);
        } else {
            Tcl_ListObjAppendElement(interp, ribbon, valueObj);
        }
    } else {
        /* Delete at the end, (i.e. pop) */
        return NULL;
    }

    Tcl_InvalidateStringRep(returnObj);

    return returnObj;
}


/*
 *----------------------------------------------------------------------
 *
 * my_LWeaveObjReverse --
 *
 *	Creates a new Obj with the the order of the elements in the lweave
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
#if 0 // Not supported
static int
my_LWeaveObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj, Tcl_Obj **newObjPtr)
{
    Tcl_Obj *revObj = Tcl_NewObj();
    LWeave *srcRep = (LWeave*)srcObj->internalRep.twoPtrValue.ptr1;
    LWeave *revRep = (LWeave*)Tcl_Alloc(sizeof(LWeave));
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
    revObj->typePtr = &lweaveType;
    Tcl_InvalidateStringRep(revObj);
    *newObjPtr = revObj;
    return TCL_OK;
}
#endif // Not supported

/*
 *----------------------------------------------------------------------
 *
 * my_LWeaveReplace --
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
#if 0 // not supported -- TODO: rewrite this implementation
      //                        from lstring for lweave.
static int
my_LWeaveReplace(
    Tcl_Interp *interp,
    Tcl_Obj *listObj,
    Tcl_Size first,
    Tcl_Size numToDelete,
    Tcl_Size numToInsert,
    Tcl_Obj *const insertObjs[])
{
    LWeave *lweaveRep = (LWeave*)listObj->internalRep.twoPtrValue.ptr1;
    Tcl_WideInt newLen;
    Tcl_WideInt x, ix, kx;
    char *newStr;
    char *oldStr = lweaveRep->string;

    if (numToDelete < 0) numToDelete = 0;
    if (numToInsert < 0) numToInsert = 0;

    newLen = lweaveRep->strlen - numToDelete + numToInsert;

    if (newLen >= lweaveRep->allocated) {
	lweaveRep->allocated = newLen+1;
	newStr = Tcl_Alloc(lweaveRep->allocated);
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
    for(x=first, ix=0; ix<numToInsert; x++, ix++, kx++) {
	char const *svalue = Tcl_GetString(insertObjs[ix]);
	newStr[x] = svalue[0];
    }
    // Move remaining elements
    if ((first+(numToInsert-numToDelete)) < newLen) {
	for(/*x,*/ kx += (numToDelete-numToInsert); (kx <lweaveRep->strlen && x<newLen); x++, kx++) {
	    newStr[x] = oldStr[kx];
	}
    }

    // Terminate new string.
    newStr[newLen] = 0;


    if (oldStr != newStr) {
	Tcl_Free(oldStr);
    }
    lweaveRep->string = newStr;
    lweaveRep->strlen = newLen;

    /* Changes made to value, string rep no longer valid */
    Tcl_InvalidateStringRep(listObj);

    return TCL_OK;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * my_NewLWeaveObj --
 *
 *	Creates a new lweave Obj using the provide lists
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
my_NewLWeaveObj(
    Tcl_Interp *interp,
    const int objc,
    Tcl_Obj * const objv[])
{
    if (objc < 1) {
        return NULL;
    }
    return myNewLWeaveObj(objc, objv);
}


static Tcl_Obj*
myNewLWeaveObj(
    const Tcl_Size objc,
    Tcl_Obj* const objv[])
{
    Tcl_Size repSize = sizeof(LWeave);
    Tcl_Obj *lweavePtr = Tcl_NewObj();
    lweavePtr->typePtr = &lweaveType;
    LWeave* lweaveRepPtr = (LWeave*)Tcl_Alloc(repSize);
    lweaveRepPtr->nlists = objc;
    lweaveRepPtr->allocated = (sizeof(Tcl_Obj*) * lweaveRepPtr->nlists);
    lweaveRepPtr->lists = (Tcl_Obj**)Tcl_Alloc(lweaveRepPtr->allocated);
    lweaveRepPtr->nominalLen = 0;
    for (Tcl_Size i=0; i<objc; i++) {
        lweaveRepPtr->lists[i] = objv[i];
        Tcl_IncrRefCount(lweaveRepPtr->lists[i]);
        Tcl_Size llen;
        Tcl_ListObjLength(NULL,lweaveRepPtr->lists[i],&llen);
        if (lweaveRepPtr->nominalLen < llen) {
            lweaveRepPtr->nominalLen = llen;
        }
    }
    lweaveRepPtr->llen = lweaveRepPtr->nominalLen * lweaveRepPtr->nlists;
    lweavePtr->internalRep.twoPtrValue.ptr1 = lweaveRepPtr;
    if (lweaveRepPtr->nlists > 0) {
        Tcl_InvalidateStringRep(lweavePtr);
    } else {
        Tcl_InitStringRep(lweavePtr, NULL, 0);
    }

    return lweavePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * freeRep --
 *
 *	Free the value storage of the lweave Obj.
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
freeRep(Tcl_Obj* lweaveObj)
{
    LWeave *lweaveRepPtr = (LWeave*)lweaveObj->internalRep.twoPtrValue.ptr1;
    Tcl_Size i=0;
    while (i<lweaveRepPtr->nlists) {
        Tcl_DecrRefCount(lweaveRepPtr->lists[i++]);
    }
    Tcl_Free((char*)lweaveRepPtr->lists);
    Tcl_Free((char*)lweaveRepPtr);
    lweaveObj->internalRep.twoPtrValue.ptr1 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * lWeaveObjCmd --
 *
 *	Script level command that creats an lweave Obj value.
 *
 * Results:
 *	Returns and lweave Obj value in the interp results.
 *
 * Side effects:
 *	Interp results modified.
 *
 *----------------------------------------------------------------------
 */

static int
lWeaveObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
  Tcl_Obj *lweaveObj;

  if (objc < 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "lweave listA ?listB ...?");
      return TCL_ERROR;
  }

  lweaveObj = my_NewLWeaveObj(interp, objc-1, &objv[1]);

  if (lweaveObj) {
      Tcl_SetObjResult(interp, lweaveObj);
      return TCL_OK;
  }
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Lweave_Init --
 *
 *	DL load init function. Defines the "lweave" command.
 *
 * Results:
 *	"lweave" command added to the interp.
 *
 * Side effects:
 *	A new command is defined.
 *
 *----------------------------------------------------------------------
 */

int Lweave_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
        return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lweave", lWeaveObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lweave", "0.0.1");
    return TCL_OK;
}
