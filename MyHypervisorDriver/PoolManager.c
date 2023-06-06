#include <ntddk.h>
#include "PoolManager.h"
#include "Logging.h"
#include "GlobalVariables.h"
#include "PoolManager.h"
#include "Common.h"

BOOLEAN PoolManagerInitialize(){

	// Allocate global requesting variable
    // 分配全局请求变量
	RequestNewAllocation = ExAllocatePoolWithTag(NonPagedPool, sizeof(REQUEST_NEW_ALLOCATION), POOLTAG);

	if (!RequestNewAllocation)
	{
		LogError("Insufficient memory");
		return FALSE;
	}
	RtlZeroMemory(RequestNewAllocation, sizeof(REQUEST_NEW_ALLOCATION));

	ListOfAllocatedPoolsHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), POOLTAG);

	if (!ListOfAllocatedPoolsHead)
	{
		LogError("Insufficient memory");
		return FALSE;
	}
	RtlZeroMemory(ListOfAllocatedPoolsHead, sizeof(LIST_ENTRY));

	// Initialize list head 
	// 初始化链表头部
	InitializeListHead(ListOfAllocatedPoolsHead);


	// Request pages to be allocated for converting 2MB to 4KB pages
    // 请求分配页面以将2MB页面转换为4KB页面
	PoolManagerRequestAllocation(sizeof(VMM_EPT_DYNAMIC_SPLIT), 10, SPLIT_2MB_PAGING_TO_4KB_PAGE);

	// Request pages to be allocated for paged hook details
    // 请求分配页面以用于分页钩子的详细信息
	PoolManagerRequestAllocation(sizeof(EPT_HOOKED_PAGE_DETAIL), 10, TRACKING_HOOKED_PAGES);

	// Request pages to be allocated for Trampoline of Executable hooked pages
    // 请求分配页面以用于可执行钩子页面的跳板代码
	PoolManagerRequestAllocation(MAX_EXEC_TRAMPOLINE_SIZE, 10, EXEC_TRAMPOLINE);

	// Let's start the allocations
    // 让我们开始分配页面
	return PoolManagerCheckAndPerformAllocation();

}

VOID PoolManagerUninitialize() {

	PLIST_ENTRY ListTemp = 0;
	UINT64 Address = 0;
	ListTemp = ListOfAllocatedPoolsHead;

	while (ListOfAllocatedPoolsHead != ListTemp->Flink)
	{
		ListTemp = ListTemp->Flink;

		// Get the head of the record
        // 获取记录的头部
		PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

		// Free the alloocated buffer
        // 释放已分配的缓冲区
		ExFreePoolWithTag(PoolTable->Address, POOLTAG);

		// Free the record itself
        // 释放记录本身
		ExFreePoolWithTag(PoolTable, POOLTAG);

		
	}

	ExFreePoolWithTag(ListOfAllocatedPoolsHead, POOLTAG);
	ExFreePoolWithTag(RequestNewAllocation, POOLTAG);



}

/* This function should be called from vmx-root in order to get a pool from the list */
// Returns a pool address or retuns null
/* If RequestNewPool is TRUE then Size is used, otherwise Size is useless */
// Should be executed at vmx root
/* 此函数应从vmx-root中调用，以从列表中获取一个池 /
// 返回一个池地址或返回null
/ 如果RequestNewPool为TRUE，则使用Size，否则Size无效 */
// 应在vmx root中执行
UINT64 PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool , UINT32 Size)
{
	PLIST_ENTRY ListTemp = 0;
	UINT64 Address = 0;
	ListTemp = ListOfAllocatedPoolsHead;

	SpinlockLock(&LockForReadingPool);

	while (ListOfAllocatedPoolsHead != ListTemp->Flink)
	{
		ListTemp = ListTemp->Flink;

		// Get the head of the record
        // 获取记录的头部
		PPOOL_TABLE PoolTable = (PPOOL_TABLE) CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

		if (PoolTable->Intention == Intention && PoolTable->IsBusy == FALSE)
		{
			PoolTable->IsBusy = TRUE;
			Address = PoolTable->Address;
			break;
		}
	}

	SpinlockUnlock(&LockForReadingPool);

	// Check if we need additional pools e.g another pool or the pool will be available for the next use blah blah
    // 检查是否需要额外的池，例如另一个池或者池将可用于下一次使用等等
	if (RequestNewPool)
	{
		PoolManagerRequestAllocation(Size, 1, Intention);
	}

	// return Address might be null indicating there is no valid pools
    // 返回的地址可能为空，表示没有有效的池可用

	return Address;
}


/* Allocate the new pools and add them to pool table */
// Should b ecalled in vmx non-root
// This function doesn't need lock as it just calls once from PASSIVE_LEVEL
/* 分配新的池并将它们添加到池表中 */
// 应在vmx非根模式下调用
// 此函数不需要锁定，因为它只在PASSIVE_LEVEL下调用一次
BOOLEAN PoolManagerAllocateAndAddToPoolTable(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention)
{
    BOOLEAN is_finish = TRUE;
	/* If we're here then we're in vmx non-root */
    /* 如果我们在这里，则处于vmx非根模式 */
	for (size_t i = 0; i < Count; i++)
	{
		POOL_TABLE* SinglePool = ExAllocatePoolWithTag(NonPagedPool, sizeof(POOL_TABLE), POOLTAG);

		if (!SinglePool)
		{
			LogError("Insufficient memory");
            is_finish = FALSE;
            break;
		}

		RtlZeroMemory(SinglePool, sizeof(POOL_TABLE));

		// Allocate the buffer
        // 分配缓冲区
		SinglePool->Address = ExAllocatePoolWithTag(NonPagedPool, Size, POOLTAG);

		if (!SinglePool->Address)
		{
			LogError("Insufficient memory");
            is_finish = FALSE;
            break;
		}

		RtlZeroMemory(SinglePool->Address, Size);

		SinglePool->Intention = Intention;
		SinglePool->IsBusy = FALSE;
		SinglePool->ShouldBeFreed = FALSE;
		SinglePool->Size = Size;

		// Add it to the list 
		// 将其添加到列表中
		InsertHeadList(ListOfAllocatedPoolsHead, &(SinglePool->PoolsList));

	}

    return is_finish;
}


/* This function performs allocations from VMX non-root based on RequestNewAllocation */
/* 此函数基于RequestNewAllocation在VMX非根模式下执行分配 */
BOOLEAN PoolManagerCheckAndPerformAllocation() {

	BOOLEAN Result = TRUE;

	//let's make sure we're on vmx non-root and also we have new allocation
    //确保我们在vmx非根模式下，并且有新的分配
	if (!IsNewRequestForAllocationRecieved || GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
	{
		// allocation's can't be done from vmx root
        //无法从vmx根模式进行分配
		return FALSE;
	}

	PAGED_CODE();

	if (RequestNewAllocation->Size0 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size0, RequestNewAllocation->Count0, RequestNewAllocation->Intention0);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count0 = 0;
		RequestNewAllocation->Intention0 = 0;
		RequestNewAllocation->Size0 = 0;
	}
	if (RequestNewAllocation->Size1 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size1, RequestNewAllocation->Count1, RequestNewAllocation->Intention1);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count1 = 0;
		RequestNewAllocation->Intention1 = 0;
		RequestNewAllocation->Size1 = 0;
	}
	if (RequestNewAllocation->Size2 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size2, RequestNewAllocation->Count2, RequestNewAllocation->Intention2);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count2 = 0;
		RequestNewAllocation->Intention2 = 0;
		RequestNewAllocation->Size2 = 0;
	}

	if (RequestNewAllocation->Size3 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size3, RequestNewAllocation->Count3, RequestNewAllocation->Intention3);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count3 = 0;
		RequestNewAllocation->Intention3 = 0;
		RequestNewAllocation->Size3 = 0;
	}
	if (RequestNewAllocation->Size4 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size4, RequestNewAllocation->Count4, RequestNewAllocation->Intention4);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count4 = 0;
		RequestNewAllocation->Intention4 = 0;
		RequestNewAllocation->Size4 = 0;
	}
	if (RequestNewAllocation->Size5 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size5, RequestNewAllocation->Count5, RequestNewAllocation->Intention5);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count5 = 0;
		RequestNewAllocation->Intention5 = 0;
		RequestNewAllocation->Size5 = 0;
	}
	if (RequestNewAllocation->Size6 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size6, RequestNewAllocation->Count6, RequestNewAllocation->Intention6);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count6 = 0;
		RequestNewAllocation->Intention6 = 0;
		RequestNewAllocation->Size6 = 0;
	}
	if (RequestNewAllocation->Size7 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size7, RequestNewAllocation->Count7, RequestNewAllocation->Intention7);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count7 = 0;
		RequestNewAllocation->Intention7 = 0;
		RequestNewAllocation->Size7 = 0;
	}
	if (RequestNewAllocation->Size8 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size8, RequestNewAllocation->Count8, RequestNewAllocation->Intention8);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count8 = 0;
		RequestNewAllocation->Intention8 = 0;
		RequestNewAllocation->Size8 = 0;
	}
	if (RequestNewAllocation->Size9 != 0)
	{
		Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size9, RequestNewAllocation->Count9, RequestNewAllocation->Intention9);

		// Free the data for future use
        //释放数据以供将来使用
		RequestNewAllocation->Count9 = 0;
		RequestNewAllocation->Intention9 = 0;
		RequestNewAllocation->Size9 = 0;
	}
	IsNewRequestForAllocationRecieved = FALSE;
	return Result;
}


/* Request to allocate new buffers */
/* 请求分配新的缓冲区 */
BOOLEAN PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention)
{

	/* We check to find a free place to store */
    /* 我们检查以找到一个空闲的位置来存储 */
	SpinlockLock(&LockForRequestAllocation);

	if (RequestNewAllocation->Size0 == 0)
	{
		RequestNewAllocation->Count0 = Count;
		RequestNewAllocation->Intention0 = Intention;
		RequestNewAllocation->Size0 = Size;
	}
	else if (RequestNewAllocation->Size1 == 0)
	{
		RequestNewAllocation->Count1 = Count;
		RequestNewAllocation->Intention1 = Intention;
		RequestNewAllocation->Size1 = Size;
	}
	else if (RequestNewAllocation->Size2 == 0)
	{
		RequestNewAllocation->Count2 = Count;
		RequestNewAllocation->Intention2 = Intention;
		RequestNewAllocation->Size2 = Size;
	}
	else if (RequestNewAllocation->Size3 == 0)
	{
		RequestNewAllocation->Count3 = Count;
		RequestNewAllocation->Intention3 = Intention;
		RequestNewAllocation->Size3 = Size;
	}
	else if (RequestNewAllocation->Size4 == 0)
	{
		RequestNewAllocation->Count4 = Count;
		RequestNewAllocation->Intention4 = Intention;
		RequestNewAllocation->Size4 = Size;
	}
	else if (RequestNewAllocation->Size5 == 0)
	{
		RequestNewAllocation->Count5 = Count;
		RequestNewAllocation->Intention5 = Intention;
		RequestNewAllocation->Size5 = Size;
	}
	else if (RequestNewAllocation->Size6 == 0)
	{
		RequestNewAllocation->Count6 = Count;
		RequestNewAllocation->Intention6 = Intention;
		RequestNewAllocation->Size6 = Size;
	}
	else if (RequestNewAllocation->Size7 == 0)
	{
		RequestNewAllocation->Count7 = Count;
		RequestNewAllocation->Intention7 = Intention;
		RequestNewAllocation->Size7 = Size;
	}
	else if (RequestNewAllocation->Size8 == 0)
	{
		RequestNewAllocation->Count8 = Count;
		RequestNewAllocation->Intention8 = Intention;
		RequestNewAllocation->Size8 = Size;
	}
	else if (RequestNewAllocation->Size9 == 0)
	{
		RequestNewAllocation->Count9 = Count;
		RequestNewAllocation->Intention9 = Intention;
		RequestNewAllocation->Size9 = Size;

	}

	else
	{
		SpinlockUnlock(&LockForRequestAllocation);
		return FALSE;
	}
	
	// Signals to show that we have new allocations
    // 信号表明我们有新的分配
	IsNewRequestForAllocationRecieved = TRUE;

	SpinlockUnlock(&LockForRequestAllocation);
	return TRUE;

}