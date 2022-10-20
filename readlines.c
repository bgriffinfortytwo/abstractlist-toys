// TCL Command to read a file into a list of lines
#include <string.h>
#include <limits.h>
#include "tcl.h"
#include "tclDecls.h"


typedef struct ReadLines {
  const char *name;   // Name of this series
  Tcl_Obj *lines;     // Read lines
  Tcl_WideInt length; // list length
  Tcl_Channel chan;   // Open file channel
} ReadLines;


int
my_ReadLinesObjIndex(Tcl_Interp *interp, Tcl_Obj *readlinesObj, Tcl_WideInt index, Tcl_Obj **lineObjPtr)
{
  ReadLines *readlinesRepPtr = (ReadLines*)Tcl_AbstractListGetConcreteRep(readlinesObj);
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
      if (Tcl_ListObjAppendElement(interp, readlinesRepPtr->lines, lineObj) != TCL_OK) {
        return TCL_ERROR;
      }
      readlinesRepPtr->length++;
    } else {
      // EOF?
      Tcl_Close(NULL, readlinesRepPtr->chan);
      readlinesRepPtr->chan = NULL;
      break;
    }
  }

  status = Tcl_ListObjIndex(interp, readlinesRepPtr->lines, index, lineObjPtr);

  return status;
}

Tcl_WideInt
my_ReadLinesObjLength(Tcl_Obj *readLinesObjPtr)
{
  ReadLines *readLinesRepPtr = (ReadLines *)Tcl_AbstractListGetConcreteRep(readLinesObjPtr);
  return readLinesRepPtr->length;
}

static void
DupReadLinesRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  ReadLines *srcReadLines = (ReadLines*)Tcl_AbstractListGetConcreteRep(srcPtr);
  ReadLines *copyReadLines = (ReadLines*)Tcl_Alloc(sizeof(ReadLines));
  /* TODO: This is not right! */
  memcpy(copyReadLines, srcReadLines, sizeof(ReadLines));
  Tcl_AbstractListSetConcreteRep(copyPtr,copyReadLines);

  return;
}

Tcl_Obj *myNewReadLinesObj(Tcl_WideInt start, Tcl_WideInt length);
static void freeRep(Tcl_Obj* alObj);

static Tcl_AbstractListType readLinesType = {
	TCL_ABSTRACTLIST_VERSION_1,
	"readlines",
	NULL,
	DupReadLinesRep,
	my_ReadLinesObjLength,
	my_ReadLinesObjIndex,
	NULL/*ObjRange*/,
	NULL/*ObjReverse*/,
        NULL/*my_ReadLinesGetElements*/,
        freeRep,
	NULL /* use default update string */
};

Tcl_Obj *
my_NewReadLinesObj(Tcl_Interp *interp, int objc, Tcl_Obj * const objv[])
{
  Tcl_WideInt length;
  ReadLines *readLinesRepPtr;
  static const char *readLinesName = "readlines";
  size_t repSize;
  Tcl_Obj *readLinesPtr, *filenameObj;
  Tcl_Channel chan;
  char *filename;

  filenameObj = objv[0];
  Tcl_IncrRefCount(filenameObj);

  filename = Tcl_GetStringFromObj(filenameObj, NULL);

  chan = Tcl_OpenFileChannel(interp, filename, "r", 0644);

  if (chan) {
      repSize = sizeof(ReadLines);
      readLinesPtr = Tcl_AbstractListObjNew(interp, &readLinesType);
      readLinesRepPtr = (ReadLines*)Tcl_Alloc(repSize);
      readLinesRepPtr->name = readLinesName;
      readLinesRepPtr->length = 0;
      readLinesRepPtr->lines = Tcl_NewObj();
      readLinesRepPtr->chan = chan;
      Tcl_AbstractListSetConcreteRep(readLinesPtr, readLinesRepPtr);

      if (length > 0) {
	  Tcl_InvalidateStringRep(readLinesPtr);
      } else {
	  Tcl_InitStringRep(readLinesPtr, NULL, 0);
      }
  } else {
      readLinesPtr = NULL;
  }

  Tcl_DecrRefCount(filenameObj);

  return readLinesPtr;
}

static int lReadLinesObjCmd(void *clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]) {

  Tcl_Obj *readLinesObj;

  if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "filename");
      return TCL_ERROR;
  }

  readLinesObj = my_NewReadLinesObj(interp, objc-1, &objv[1]);

  if (readLinesObj) {
    // Read first line
    Tcl_Obj *lastLine;

    // stat file to get size
    // allocate memory
    // read entire file into memory
    // scan for \n using const char *Tcl_UtfFindFirst(src, ch); const char *src, int ch (unicode character)
    // build int[] list of offsets to the start of each line.

    if (my_ReadLinesObjIndex(interp, readLinesObj, INT_MAX, &lastLine) == TCL_OK) {
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
freeRep(Tcl_Obj* alObj)
{
    void *alRepPtr = Tcl_AbstractListGetConcreteRep(alObj);
    Tcl_Free((char*)alRepPtr);
    Tcl_AbstractListSetConcreteRep(alObj, NULL);
}

/* readlines.so makefile:

TCL_INSTALL = $(HOME)/tcl_tip/usr
TCL_INCLUDES = -I$(TCL_INSTALL)/include
TCL_STUBLIB = $(TCL_INSTALL)/lib/libtclstub8.7.a

CFLAGS = $(TCL_INCLUDE)
readlines.so: readlines.c $(TCL_STUBLIB)
	gcc -g -fPIC $(TCL_INCLUDES) $< -o $@ -m64  --shared $(TCL_STUBLIB)

*/
