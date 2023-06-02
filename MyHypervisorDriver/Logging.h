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
    // ����֪ͨ�ص��������������������м�飨Vmx��ģʽ����Vmx�Ǹ�ģʽ��
	BOOLEAN			CheckVmxRootMessagePool; // Set so that notify callback can understand where to check (Vmx root or Vmx non-root)
} NOTIFY_RECORD, * PNOTIFY_RECORD;


// Message buffer structure
// ��Ϣ�������ṹ��
typedef struct _BUFFER_HEADER {
    // ����ID���͸��û�ģʽ
	UINT32 OpeationNumber;	// Operation ID to user-mode
    // ʵ�ʳ���
	UINT32 BufferLength;	// The actual length
    // ȷ���������Ƿ���Ч�Է��ͻ�����Ч
	BOOLEAN Valid;			// Determine whether the buffer was valid to send or not
} BUFFER_HEADER, * PBUFFER_HEADER;

// Core-specific buffers
typedef struct _LOG_BUFFER_INFORMATION {
    // ����������ʼ��ַ
	UINT64 BufferStartAddress;						// Start address of the buffer
    // �������Ľ�����ַ
	UINT64 BufferEndAddress;						// End address of the buffer

	// �����ۻ���������Ϣ�Ļ���������ʼ��ַ
	UINT64 BufferForMultipleNonImmediateMessage;	// Start address of the buffer for accumulating non-immadiate messages
    // �����ۻ���������Ϣ�Ļ������ĵ�ǰ��С
	UINT32 CurrentLengthOfNonImmBuffer;				// the current size of the buffer for accumulating non-immadiate messages

	// �����������ڱ����Զ��еķ���
	KSPIN_LOCK BufferLock;							// SpinLock to protect access to the queue
    // �����������ڱ����Է�������Ϣ���еķ���
	KSPIN_LOCK BufferLockForNonImmMessage;			// SpinLock to protect access to the queue of non-imm messages

	// ��ǰҪ���͸��û�ģʽ�Ļ���������
	UINT32 CurrentIndexToSend;						// Current buffer index to send to user-mode
    // ��ǰҪд������Ϣ�Ļ���������
	UINT32 CurrentIndexToWrite;						// Current buffer index to write new messages

} LOG_BUFFER_INFORMATION, * PLOG_BUFFER_INFORMATION;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

// Global Variable for buffer on all cores
// �����к����ϵĻ�������ȫ�ֱ���
LOG_BUFFER_INFORMATION* MessageBufferInformation;

// Vmx-root lock for logging
// ������־��¼��Vmx����
volatile LONG VmxRootLoggingLock;

// Vmx-root lock for logging
// ������־��¼��Vmx����
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
