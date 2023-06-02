#pragma once
#include "Definitions.h"


//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

typedef struct _NOTIFY_RECORD {
	NOTIFY_TYPE     Type;
	union {
		PKEVENT     Event;
		PIRP        PendingIrp;
	} Message;
	KDPC            Dpc;
    // 设置通知回调函数可以理解在哪里进行检查（Vmx根模式还是Vmx非根模式）
	BOOLEAN			CheckVmxRootMessagePool; // Set so that notify callback can understand where to check (Vmx root or Vmx non-root)
} NOTIFY_RECORD, * PNOTIFY_RECORD;


// Message buffer structure
// 消息缓冲区结构体
typedef struct _BUFFER_HEADER {
    // 操作ID发送给用户模式
	UINT32 OpeationNumber;	// Operation ID to user-mode
    // 实际长度
	UINT32 BufferLength;	// The actual length
    // 确定缓冲区是否有效以发送或者无效
	BOOLEAN Valid;			// Determine whether the buffer was valid to send or not
} BUFFER_HEADER, * PBUFFER_HEADER;

// Core-specific buffers
typedef struct _LOG_BUFFER_INFORMATION {
    // 缓冲区的起始地址
	UINT64 BufferStartAddress;						// Start address of the buffer
    // 缓冲区的结束地址
	UINT64 BufferEndAddress;						// End address of the buffer

	// 用于累积非立即消息的缓冲区的起始地址
	UINT64 BufferForMultipleNonImmediateMessage;	// Start address of the buffer for accumulating non-immadiate messages
    // 用于累积非立即消息的缓冲区的当前大小
	UINT32 CurrentLengthOfNonImmBuffer;				// the current size of the buffer for accumulating non-immadiate messages

	// 自旋锁，用于保护对队列的访问
	KSPIN_LOCK BufferLock;							// SpinLock to protect access to the queue
    // 自旋锁，用于保护对非立即消息队列的访问
	KSPIN_LOCK BufferLockForNonImmMessage;			// SpinLock to protect access to the queue of non-imm messages

	// 当前要发送给用户模式的缓冲区索引
	UINT32 CurrentIndexToSend;						// Current buffer index to send to user-mode
    // 当前要写入新消息的缓冲区索引
	UINT32 CurrentIndexToWrite;						// Current buffer index to write new messages

} LOG_BUFFER_INFORMATION, * PLOG_BUFFER_INFORMATION;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

// Global Variable for buffer on all cores
// 在所有核心上的缓冲区的全局变量
LOG_BUFFER_INFORMATION* MessageBufferInformation;

// Vmx-root lock for logging
// 用于日志记录的Vmx根锁
volatile LONG VmxRootLoggingLock;

// Vmx-root lock for logging
// 用于日志记录的Vmx根锁
volatile LONG VmxRootLoggingLockForNonImmBuffers;

//////////////////////////////////////////////////
//					Illustration				//
//////////////////////////////////////////////////

/*
A core buffer is like this , it's divided into MaximumPacketsCapacity chucks,
each chunk has PacketChunkSize + sizeof(BUFFER_HEADER) size

			 _________________________
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|						  |
			|						  |
			|						  |
			|						  |
			|			.			  |
			|			.			  |
			|			.			  |
			|						  |
			|						  |
			|						  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|

*/

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN LogInitialize();
VOID LogUnInitialize();
BOOLEAN LogSendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength);
BOOLEAN LogReadBuffer(BOOLEAN IsVmxRoot, PVOID BufferToSaveMessage, UINT32* ReturnedLength);
BOOLEAN LogCheckForNewMessage(BOOLEAN IsVmxRoot);
BOOLEAN LogSendMessageToQueue(UINT32 OperationCode, BOOLEAN IsImmediateMessage, BOOLEAN ShowCurrentSystemTime, const char* Fmt, ...);
VOID LogNotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
NTSTATUS LogRegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
