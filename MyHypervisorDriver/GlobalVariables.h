#pragma once
#include <ntddk.h>
#include  "Vmx.h"
#include  "Logging.h"
#include  "PoolManager.h"

/* Here we put global variables that are used more or less in all part of our hypervisor (not all of them) */
/* 在这里我们放置那些在我们的虚拟化程序的各个部分中或多或少使用的全局变量（并非全部） */
//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////


// Save the state and variables related to each to logical core
// 保存与每个逻辑核心相关的状态和变量
VIRTUAL_MACHINE_STATE* GuestState;

// Save the state and variables related to EPT
// 保存与EPT相关的状态和变量
EPT_STATE* EptState;

// Save the state of the thread that waits for messages to deliver to user-mode
// 保存等待将消息传递给用户模式的线程的状态
NOTIFY_RECORD* GlobalNotifyRecord;

// Support for execute-only pages (indicating that data accesses are not allowed while instruction fetches are allowed).
// 支持仅执行页面（指示不允许数据访问，但允许指令获取）。
BOOLEAN ExecuteOnlySupport;

// Client Allowed to send IOCTL to the drive
// 允许客户端向驱动发送IOCTL请求。
BOOLEAN AllowIOCTLFromUsermode;
