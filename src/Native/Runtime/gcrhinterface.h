// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

//
// This header contains the definition of an interface between the GC/HandleTable portions of the Redhawk
// codebase and the regular Redhawk code. The former has all sorts of legacy environmental requirements (see
// gcrhenv.h) that we don't wish to pull into the rest of Redhawk.
//
// Since this file is included in both worlds it has no dependencies and uses a very simple subset of types
// etc. so that it will build cleanly in both. The actual implementation of the class defined here is in
// gcrhenv.cpp, since the implementation needs access to the guts of the GC/HandleTable.
//
// This is just an initial stab at the interface.
//

#ifndef __GCRHINTERFACE_INCLUDED
#define __GCRHINTERFACE_INCLUDED

#ifndef DACCESS_COMPILE
// Global data cells exported by the GC.
extern "C" unsigned char *g_ephemeral_low;
extern "C" unsigned char *g_ephemeral_high;
extern "C" unsigned char *g_lowest_address;
extern "C" unsigned char *g_highest_address;
#endif

struct alloc_context;
class MethodInfo;
struct REGDISPLAY;
class Thread;
enum GCRefKind : unsigned char;
class ICodeManager;
class EEType;

// -----------------------------------------------------------------------------------------------------------
// RtuObjectRef
// -----------------------------------------------------------------------------------------------------------
//
// READ THIS!
//
// This struct exists for type description purposes, but you must never directly refer to the object 
// reference.  The only code allowed to do this is the code inherited directly from the CLR, which all 
// includes gcrhenv.h.  If your code is outside the namespace of gcrhenv.h, direct object reference 
// manipulation is prohibited--use C# instead.
//
// To enforce this, we declare RtuObjectRef as a class with no public members.
//
class RtuObjectRef
{
#ifndef DACCESS_COMPILE
private:
#else
public:
#endif 
    TADDR pvObject;
};

typedef DPTR(RtuObjectRef) PTR_RtuObjectRef;

// -----------------------------------------------------------------------------------------------------------

// We provide various ways to enumerate GC objects or roots, each of which calls back to a user supplied
// function for each object (within the context of a garbage collection). The following function types
// describe these callbacks. Unfortunately the signatures aren't very specific: we don't want to reference
// Object* or Object** from this module, see the comment for RtuObjectRef, but this very narrow category of
// callers can't use RtuObjectRef (they really do need to drill down into the Object). The lesser evil here is
// to be a bit loose in the signature rather than exposing the Object class to the rest of Redhawk.

// Callback when enumerating objects on the GC heap or objects referenced from instance fields of another
// object. The GC dictates the shape of this signature (we're hijacking functionality originally developed for
// profiling). The real signature is:
//      int ScanFunction(Object* pObject, void* pContext)
// where:
//      return      : treated as a boolean, zero indicates the enumeration should terminate, all other values
//                    say continue
//      pObject     : pointer to the current object being scanned
//      pContext    : user context passed to the original scan function and otherwise uninterpreted
typedef int (*GcScanObjectFunction)(void*, void*);

// Callback when enumerating GC roots (stack locations, statics and handles). Similar to the callback above
// except there is no means to terminate the scan (no return value) and the root location (pointer to pointer
// to object) is returned instead of a direct pointer to the object:
//      void ScanFunction(Object** pRoot, void* pContext)
typedef void (*GcScanRootFunction)(void**, void*);

// Heap scans are scheduled by setting the following global variables and then triggering a full garbage
// collection. This requires careful synchronization. See the implementation of RedhawkGCInterface::ScanHeap
// for details.
extern GcScanObjectFunction g_pfnHeapScan;
extern void * g_pvHeapScanContext;

typedef void * GcSegmentHandle;

#define RH_LARGE_OBJECT_SIZE 85000

// A 'clump' is defined as the size of memory covered by 1 byte in the card table.  These constants are 
// verified against gcpriv.h in gcrhee.cpp.
#if (POINTER_SIZE == 8)
#define CLUMP_SIZE 0x800
#define LOG2_CLUMP_SIZE 11
#elif (POINTER_SIZE == 4)
#define CLUMP_SIZE 0x400
#define LOG2_CLUMP_SIZE 10
#else
#error unexpected pointer size
#endif

class RedhawkGCInterface
{
public:
    enum GCType
    {
        GCType_Workstation,
        GCType_Server,
    };

    // Perform any runtime-startup initialization needed by the GC, HandleTable or environmental code in
    // gcrhenv. The enum parameter is used to choose between workstation and server GC.
    // Returns true on success or false if a subsystem failed to initialize.
    // todo: figure out the final error reporting strategy
    static bool InitializeSubsystems(GCType gcType);

    // Allocate an object on the GC heap.
    //  pThread         -  current Thread
    //  cbSize          -  size in bytes of the final object
    //  uFlags          -  GC type flags (see gc.h GC_ALLOC_*)
    //  pEEType         -  type of the object
    // Returns a pointer to the object allocated or NULL on failure.
    static void* Alloc(Thread *pThread, UIntNative cbSize, UInt32 uFlags, EEType *pEEType);

    // Allocate an object on the large GC heap. Used when you want to force an allocation on the large heap
    // that wouldn't normally go there (e.g. objects containing double fields).
    //  cbSize          -  size in bytes of the final object
    //  uFlags          -  GC type flags (see gc.h GC_ALLOC_*)
    // Returns a pointer to the object allocated or NULL on failure.
    static void* AllocLarge(UIntNative cbSize, UInt32 uFlags);

    static void InitAllocContext(alloc_context * pAllocContext);
    static void ReleaseAllocContext(alloc_context * pAllocContext);

    static void WaitForGCCompletion();

    static void EnumGcRef(PTR_RtuObjectRef pRef, GCRefKind kind, void * pfnEnumCallback, void * pvCallbackData);

    static void BulkEnumGcObjRef(PTR_RtuObjectRef pRefs, UInt32 cRefs, void * pfnEnumCallback, void * pvCallbackData);

    static void EnumGcRefs(ICodeManager * pCodeManager,
                           MethodInfo * pMethodInfo, 
                           UInt32 codeOffset,
                           REGDISPLAY * pRegisterSet,
                           void * pfnEnumCallback,
                           void * pvCallbackData);

    static void EnumGcRefsInRegionConservatively(PTR_RtuObjectRef pLowerBound,
                                                 PTR_RtuObjectRef pUpperBound,
                                                 void * pfnEnumCallback,
                                                 void * pvCallbackData);

    static void GarbageCollect(UInt32 uGeneration, UInt32 uMode);

    static GcSegmentHandle RegisterFrozenSection(void * pSection, UInt32 SizeSection);
    static void UnregisterFrozenSection(GcSegmentHandle segment);

#ifdef FEATURE_GC_STRESS
    static void StressGc();
#endif // FEATURE_GC_STRESS

    // Various routines used to enumerate objects contained within a given scope (on the GC heap, as reference
    // fields of an object, on a thread stack, in a static or in one of the handle tables).
    static void ScanHeap(GcScanObjectFunction pfnScanCallback, void *pContext);
    static void ScanObject(void *pObject, GcScanObjectFunction pfnScanCallback, void *pContext);
    static void ScanStackRoots(Thread *pThread, GcScanRootFunction pfnScanCallback, void *pContext);
    static void ScanStaticRoots(GcScanRootFunction pfnScanCallback, void *pContext);
    static void ScanHandleTableRoots(GcScanRootFunction pfnScanCallback, void *pContext);

    // These three methods may only be called from a point at which the runtime is suspended.
    // Currently, this is used by the VSD infrastructure on a SyncClean::CleanUp callback
    // from the GC when a collection is complete.
    static bool IsScanInProgress();
    static GcScanObjectFunction GetCurrentScanCallbackFunction();
    static void* GetCurrentScanContext();

    // If the class library has requested it, call this method on clean shutdown (i.e. return from Main) to
    // perform a final pass of finalization where all finalizable objects are processed regardless of whether
    // they are still rooted.
    static void ShutdownFinalization();

    // Returns size GCDesc. Used by type cloning.
    static UInt32 GetGCDescSize(void * pType);

    // These methods are used to get and set the type information for the last allocation on each thread.
    static EEType * GetLastAllocEEType();
    static void SetLastAllocEEType(EEType *pEEType);

private:
    // The EEType for the last allocation.  This value is used inside of the GC allocator
    // to emit allocation ETW events with type information.  We set this value unconditionally to avoid
    // race conditions where ETW is enabled after the value is set.
    DECLSPEC_THREAD static EEType * tls_pLastAllocationEEType;
};

#endif // __GCRHINTERFACE_INCLUDED
