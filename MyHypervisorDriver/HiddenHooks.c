#include <ntifs.h>
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "Logging.h"
#include "Hooks.h"
#include "HypervisorRoutines.h"


/* Hook function that HooksExAllocatePoolWithTag */
/* 钩取函数，钩取ExAllocatePoolWithTag函数 */
PVOID ExAllocatePoolWithTagHook(
	POOL_TYPE	PoolType,
	SIZE_T      NumberOfBytes,
	ULONG       Tag
)
{
	LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x , Number Of Bytes = %d , Pool Type = %d ", Tag, NumberOfBytes, PoolType);
	return ExAllocatePoolWithTagOrig(PoolType, NumberOfBytes, Tag);
}

/***********************************************************************/

/* Make examples for testing hidden hooks */
/* 创建用于测试隐藏钩子的示例 */
VOID HiddenHooksTest()
{
	// Hook Test
	//	EptPageHook(KeGetCurrentThread(), NULL, NULL, TRUE, TRUE, FALSE);
	//	EptPageHook(ExAllocatePoolWithTag, ExAllocatePoolWithTagHook, (PVOID*)&ExAllocatePoolWithTagOrig, FALSE, FALSE, TRUE);

	// Unhook Tests
	//HvPerformPageUnHookSinglePage(ExAllocatePoolWithTag);
	//HvPerformPageUnHookAllPages();
	
}