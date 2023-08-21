// TCL Command to read a file into a list of lines
#include <string.h>
#include <limits.h>
#include "tcl.h"
#include "tclDecls.h"


typedef struct ReadLines {
    Tcl_Obj *lines;   // Read lines
    Tcl_Size length;  // list length
    Tcl_Channel chan; // Open file channel
} ReadLines;


int
my_ReadLinesObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *readlinesObj,
    Tcl_Size index,
    Tcl_Obj **lineObjPtr)
{
    ReadLines *readlinesRepPtr = (ReadLines*)readlinesObj->internalRep.twoPtrValue.ptr1;
    Tcl_Obj *lineObj;
    int status;

    if (index < 0) {
	return TCL_ERROR;
    }

    while (readlinesRepPtr->chan != NULL && index >= readlinesRepPtr->length) {
	int slen;
	lineObj = Tcl_NewObj();
	slen = Tcl_GetsObj(readlinesRepPtr->chan, lineObj);
	if (slen >= 0) {
	    if (Tcl_ListObjAppendElement(interp, readlinesRepPtr->lines, lineObj)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    readlinesRepPtr->length++;
	} else {
	    // EOF?
	    Tcl_Close(NULL, readlinesRepPtr->chan);
	    readlinesRepPtr->chan = NULL;
	    if (index > readlinesRepPtr->length) {
		index = readlinesRepPtr->length-1;
	    }
	    Tcl_IncrRefCount(readlinesRepPtr->lines);
	    break;
	}
    }

    status = Tcl_ListObjIndex(interp, readlinesRepPtr->lines, index, lineObjPtr);

    return status;
}

Tcl_Size
my_ReadLinesObjLength(Tcl_Obj *readLinesObj)
{
    ReadLines *readLinesRepPtr =
	(ReadLines*)readLinesObj->internalRep.twoPtrValue.ptr1;
    return readLinesRepPtr->length;
}

static void
UpdateStringRep(Tcl_Obj *objPtr)
{
    ReadLines *readlinesRepPtr = (ReadLines*)objPtr->internalRep.twoPtrValue.ptr1;
    Tcl_Size strlen;
    const char *str = Tcl_GetStringFromObj(readlinesRepPtr->lines, &strlen);
    Tcl_InitStringRep(objPtr, str, strlen);
    Tcl_InvalidateStringRep(readlinesRepPtr->lines); /* don't need 2 copies */
}

Tcl_Obj *myNewReadLinesObj(Tcl_Size start, Tcl_WideInt length);
static void freeRep(Tcl_Obj* alObj);
static void DupReadLinesRep(Tcl_Obj *srcObj, Tcl_Obj *copyObj);

static Tcl_ObjType readLinesType = {
    "readlines",
    freeRep,
    DupReadLinesRep,
    UpdateStringRep, /* use default update string */
    NULL,
    TCL_OBJTYPE_V2(
	my_ReadLinesObjLength,
	my_ReadLinesObjIndex,
	NULL, /* ObjRange */
	NULL, /* ObjReverse */
        NULL, /* my_ReadLinesGetElements */
	NULL, /* setElements */
	NULL, /* replace */
        NULL) /* in operation */
};

static void
DupReadLinesRep(Tcl_Obj *srcObj, Tcl_Obj *copyObj)
{
    ReadLines *srcReadLines = (ReadLines*)srcObj->internalRep.twoPtrValue.ptr1;
    ReadLines *copyReadLines = (ReadLines*)Tcl_Alloc(sizeof(ReadLines));
    copyObj->internalRep.twoPtrValue.ptr1 = copyReadLines;
    /* TODO: This is not right! */
    memcpy(copyReadLines, srcReadLines, sizeof(ReadLines));
    Tcl_IncrRefCount(copyReadLines->lines);
    copyObj->typePtr = &readLinesType;
    return;
}

Tcl_Obj *
my_NewReadLinesObj(Tcl_Interp *interp, int objc, Tcl_Obj * const objv[])
{
    Tcl_Size length = 0;
    ReadLines *readLinesRepPtr;
    Tcl_Size repSize;
    Tcl_Obj *readLinesObj, *filenameObj;
    Tcl_Channel chan;
    char *filename;

    filenameObj = objv[0];
    Tcl_IncrRefCount(filenameObj);

    filename = Tcl_GetStringFromObj(filenameObj, NULL);

    chan = Tcl_OpenFileChannel(interp, filename, "r", 0644);

    if (chan) {
	repSize = sizeof(ReadLines);
	readLinesObj = Tcl_NewObj();
	readLinesObj->typePtr = &readLinesType;
	readLinesRepPtr = (ReadLines*)Tcl_Alloc(repSize);
	readLinesRepPtr->length = 0;
	readLinesRepPtr->lines = Tcl_NewObj();
	readLinesRepPtr->chan = chan;
	readLinesObj->internalRep.twoPtrValue.ptr1 = readLinesRepPtr;
    } else {
	return NULL;
    }

    // Read in the file
    while (readLinesRepPtr->chan && !Tcl_Eof(readLinesRepPtr->chan)) {
	Tcl_Obj *lineObj;
	int status = my_ReadLinesObjIndex(interp, readLinesObj, length, &lineObj);
	if (status == TCL_OK) {
	    length++;
	} else {
	    break;
	}
    }
    if (readLinesRepPtr->chan) {
	if (Tcl_Close(interp, readLinesRepPtr->chan) != TCL_OK) {
	    return NULL;
	}
    }
    if (length > 0) {
	Tcl_InvalidateStringRep(readLinesObj);
    } else {
	Tcl_InitStringRep(readLinesObj, NULL, 0);
    }

    Tcl_DecrRefCount(filenameObj);

    return readLinesObj;
}

static int lReadLinesObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
  Tcl_Obj *readLinesObj;

  if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "filename");
      return TCL_ERROR;
  }

  readLinesObj = my_NewReadLinesObj(interp, objc-1, &objv[1]);

  if (readLinesObj) {
    // Read first line
//    Tcl_Obj *lastLine;

    // stat file to get size
    // allocate memory
    // read entire file into memory
    // scan for \n using const char *Tcl_UtfFindFirst(src, ch); const char *src, int ch (unicode character)
    // build int[] list of offsets to the start of each line.

    if (1 /*my_ReadLinesObjIndex(interp, readLinesObj, INT_MAX, &lastLine) == TCL_OK*/) {
	Tcl_SetObjResult(interp, readLinesObj);
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
  }
  return TCL_ERROR;
}

int Readlines_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lreadlines", lReadLinesObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "lreadlines", "1.0.0");
    return TCL_OK;
}

static void
freeRep(Tcl_Obj* rlObj)
{
    ReadLines *rlRepPtr = (ReadLines*)rlObj->internalRep.twoPtrValue.ptr1;
    if (rlRepPtr->chan) {
	Tcl_Close(NULL, rlRepPtr->chan);
	rlRepPtr->chan = NULL;
    }
    /* Free the lines here */
    Tcl_DecrRefCount(rlRepPtr->lines);
    Tcl_Free((char*)rlRepPtr);
    rlObj->internalRep.twoPtrValue.ptr1 = NULL;
}

/* readlines.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)
readlines.so: readlines.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

*/
