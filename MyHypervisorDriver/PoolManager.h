#pragma once
#include <ntddk.h>



//////////////////////////////////////////////////
//                   Definition	    			//
//////////////////////////////////////////////////
#define NumberOfPreAllocatedBuffers				10



//////////////////////////////////////////////////
//                    Enums		    			//
//////////////////////////////////////////////////

typedef enum {
	TRACKING_HOOKED_PAGES,
	EXEC_TRAMPOLINE,
	SPLIT_2MB_PAGING_TO_4KB_PAGE,

} POOL_ALLOCATION_INTENTION;


//////////////////////////////////////////////////
//                   Structures		   			//
//////////////////////////////////////////////////

typedef struct _POOL_TABLE
{
    // 应该是列表的起始部分，因为我们将其计算为起始地址
	UINT64 Address; // Should be the start of the list as we compute it as the start address
	SIZE_T  Size;
	POOL_ALLOCATION_INTENTION Intention;
	LIST_ENTRY PoolsList;
	BOOLEAN  IsBusy;
	BOOLEAN  ShouldBeFreed;

} POOL_TABLE, * PPOOL_TABLE;


typedef struct _REQUEST_NEW_ALLOCATION
{
	SIZE_T Size0;
	UINT32 Count0;
	POOL_ALLOCATION_INTENTION Intention0;

	SIZE_T Size1;
	UINT32 Count1;
	POOL_ALLOCATION_INTENTION Intention1;

	SIZE_T Size2;
	UINT32 Count2;
	POOL_ALLOCATION_INTENTION Intention2;

	SIZE_T Size3;
	UINT32 Count3;
	POOL_ALLOCATION_INTENTION Intention3;

	SIZE_T Size4;
	UINT32 Count4;
	POOL_ALLOCATION_INTENTION Intention4;

	SIZE_T Size5;
	UINT32 Count5;
	POOL_ALLOCATION_INTENTION Intention5;

	SIZE_T Size6;
	UINT32 Count6;
	POOL_ALLOCATION_INTENTION Intention6;

	SIZE_T Size7;
	UINT32 Count7;
	POOL_ALLOCATION_INTENTION Intention7;

	SIZE_T Size8;
	UINT32 Count8;
	POOL_ALLOCATION_INTENTION Intention8;

	SIZE_T Size9;
	UINT32 Count9;
	POOL_ALLOCATION_INTENTION Intention9;

} REQUEST_NEW_ALLOCATION, * PREQUEST_NEW_ALLOCATION;



//////////////////////////////////////////////////
//                   Variables	    			//
//////////////////////////////////////////////////

// 如果有人想要从VMX根模式进行分配，则将其请求添加到此结构中
REQUEST_NEW_ALLOCATION* RequestNewAllocation;		// If sb wants allocation from vmx root, adds it's request to this structure
volatile LONG LockForRequestAllocation;
volatile LONG LockForReadingPool;
// 当有新的分配时，我们设置它
BOOLEAN IsNewRequestForAllocationRecieved;			// We set it when there is a new allocation
// 从所有池中创建一个列表
PLIST_ENTRY ListOfAllocatedPoolsHead;				// Create a list from all pools


//////////////////////////////////////////////////
//                   Functions		  			//
//////////////////////////////////////////////////

// Initializes the Pool Manager and pre-allocate some pools
// 初始化池管理器并预先分配一些池
BOOLEAN PoolManagerInitialize();

// Should be called in PASSIVE_LEVEL (vmx non-root), it tries to see whether a new pool request is available, if availabe then allocates it
// 应该在PASSIVE_LEVEL（vmx非根）中调用，它尝试查看是否有新的池请求可用，如果可用则进行分配
BOOLEAN PoolManagerCheckAndPerformAllocation();

// If we have request to allocate new pool, we can call this function (should be called from vmx-root), it stores the requests 
// somewhere then when it's safe (IRQL PASSIVE_LEVEL) it allocates the requested pool
// 如果我们有请求分配新的池，可以调用此函数（应该从vmx根模式调用），它将请求存储在某个地方，然后在安全的时候（IRQL PASSIVE_LEVEL）分配所请求的池
BOOLEAN PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);

// From vmx-root if we need a safe pool address immediately we call it, it also request a new pool if we set RequestNewPool to TRUE
// next time it's safe the pool will be allocated
// 如果我们需要立即获得一个安全的池地址，我们可以从vmx根模式调用它，如果我们将RequestNewPool设置为TRUE，它还会请求一个新的池，下次安全时将分配池
UINT64 PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);

// De-allocate all the allocated pools
// 释放所有已分配的池
VOID PoolManagerUninitialize();