#pragma once
#include "Vmx.h"



//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////
// 测试VMCALL指令
#define VMCALL_TEST						0x1			// Test VMCALL
// 调用VMXOFF指令关闭虚拟化程序
#define VMCALL_VMXOFF					0x2			// Call VMXOFF to turn off the hypervisor
// 使用VMCALL指令钩住并更改EPT表的属性位
#define VMCALL_CHANGE_PAGE_ATTRIB		0x3			// VMCALL to Hook Change the attribute bits of the EPT Table
// 使用VMCALL指令使EPT失效（所有上下文）
#define VMCALL_INVEPT_ALL_CONTEXTS		0x4			// VMCALL to invalidate EPT (All Contexts)
// 使用VMCALL指令使EPT失效（单个上下文）
#define VMCALL_INVEPT_SINGLE_CONTEXT	0x5			// VMCALL to invalidate EPT (A Single Context)
// 使用VMCALL指令从钩住列表中移除所有物理地址
#define VMCALL_UNHOOK_ALL_PAGES			0x6			// VMCALL to remove a all physical addresses from hook list
// 使用VMCALL指令从钩住列表中移除单个物理地址
#define VMCALL_UNHOOK_SINGLE_PAGE		0x7			// VMCALL to remove a single physical address from hook list



//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

// Main handler for VMCALLs
// VMCALL的主处理程序
NTSTATUS VmxVmcallHandler(UINT64 VmcallNumber, UINT64 OptionalParam1, UINT64 OptionalParam2, UINT64 OptionalParam3);


// Test function which shows a message to test a successfull VMCALL
// 测试函数，显示一个消息以测试VMCALL是否成功
NTSTATUS VmcallTest(UINT64 Param1, UINT64 Param2, UINT64 Param3);