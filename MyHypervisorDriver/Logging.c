#include <ntddk.h>
#include <ntstrsafe.h>
#include "Common.h"
#include "Logging.h"
#include "Trace.h"
#include "GlobalVariables.h"
#include "Logging.tmh"


/* Initialize the buffer relating to log message tracing */
/* ��ʼ������־��Ϣ������صĻ����� */
BOOLEAN LogInitialize() {


	// Initialize buffers for trace message and data messages (wee have two buffers one for vmx root and one for vmx non-root)
    // ��ʼ��������Ϣ��������Ϣ�Ļ�������������������������һ������ vmx-root��һ������ vmx non-root����
	MessageBufferInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(LOG_BUFFER_INFORMATION) * 2, POOLTAG);

	if (!MessageBufferInformation)
	{
		return FALSE; //STATUS_INSUFFICIENT_RESOURCES
	}

	// Zeroing the memory
    // �����ڴ档
	RtlZeroMemory(MessageBufferInformation, sizeof(LOG_BUFFER_INFORMATION) * 2);

	// Initialize the lock for Vmx-root mode (HIGH_IRQL Spinlock)
    // ��ʼ�� Vmx-root ģʽ������HIGH_IRQL ����������
	VmxRootLoggingLock = 0;

	// Allocate buffer for messages and initialize the core buffer information 
	// Ϊ��Ϣ���仺��������ʼ�����Ļ�������Ϣ��
	for (int i = 0; i < 2; i++)
	{

		// initialize the lock
		// Actually, only the 0th buffer use this spinlock but let initialize it for both but the second buffer spinlock is useless 
		// as we use our custom spinlock.
        // ��ʼ����
        // ʵ���ϣ�ֻ�е�һ��������ʹ���������������Ϊ��һ���ԣ�����Ϊ���������������г�ʼ�������ڶ�������������������û���õģ�
        // ��Ϊ����ʹ���Զ�����������
		KeInitializeSpinLock(&MessageBufferInformation[i].BufferLock);
		KeInitializeSpinLock(&MessageBufferInformation[i].BufferLockForNonImmMessage);

		// allocate the buffer
        // ���仺������
		MessageBufferInformation[i].BufferStartAddress = ExAllocatePoolWithTag(NonPagedPool, LogBufferSize, POOLTAG);
		MessageBufferInformation[i].BufferForMultipleNonImmediateMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize, POOLTAG);

		if (!MessageBufferInformation[i].BufferStartAddress)
		{
			return FALSE; // STATUS_INSUFFICIENT_RESOURCES
		}

		// Zeroing the buffer
        // �����ڴ档
		RtlZeroMemory(MessageBufferInformation[i].BufferStartAddress, LogBufferSize);

		// Set the end address
        // ���ý�����ַ��
		MessageBufferInformation[i].BufferEndAddress = (UINT64)MessageBufferInformation[i].BufferStartAddress + LogBufferSize;
	}
}

/* Uninitialize the buffer relating to log message tracing */
/* ����ʼ������־��Ϣ������صĻ����� */
VOID LogUnInitialize()
{

	// de-allocate buffer for messages and initialize the core buffer information (for vmx-root core)
    // �ͷ���Ϣ����������ʼ�����Ļ�������Ϣ����� vmx-root ���ģ���
	for (int i = 0; i < 2; i++)
	{
		// Free each buffers
        // �ͷ�ÿ����������
		ExFreePoolWithTag(MessageBufferInformation[i].BufferStartAddress, POOLTAG);
		ExFreePoolWithTag(MessageBufferInformation[i].BufferForMultipleNonImmediateMessage, POOLTAG);
	}

	// de-allocate buffers for trace message and data messages
    // �ͷŸ�����Ϣ��������Ϣ�Ļ�������
	ExFreePoolWithTag(MessageBufferInformation, POOLTAG);
}

/* Save buffer to the pool */
/* �����������浽���� */
BOOLEAN LogSendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength)
{
	KIRQL OldIRQL;
	UINT32 Index;
	BOOLEAN IsVmxRoot;

	if (BufferLength > PacketChunkSize - 1 || BufferLength == 0)
	{
		// We can't save this huge buffer
        // �����޷���������޴�Ļ�������
		return FALSE;
	}

	// Check that if we're in vmx root-mode
    // ����Ƿ��� vmx root ģʽ��
	IsVmxRoot = GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode;

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
	if (IsVmxRoot)
	{
		// Set the index
        // ����������
		Index = 1;
		SpinlockLock(&VmxRootLoggingLock);
	}
	else
	{
		// Set the index
        // ����������
		Index = 0;
		// Acquire the lock 
		// ��ȡ����
		KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
	}

	// check if the buffer is filled to it's maximum index or not
    // ��黺�����Ƿ������������������
	if (MessageBufferInformation[Index].CurrentIndexToWrite > MaximumPacketsCapacity - 1)
	{
		// start from the begining
        // �ӿ�ͷ��ʼ��
		MessageBufferInformation[Index].CurrentIndexToWrite = 0;
	}

	// Compute the start of the buffer header
    // ���㻺����ͷ����ʼλ�á�
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	// Set the header
    // ����ͷ����
	Header->OpeationNumber = OperationCode;
	Header->BufferLength = BufferLength;
	Header->Valid = TRUE;

	/* Now it's time to fill the buffer */
    /* ��������仺������ʱ���� */

	// compute the saving index
    // ���㱣��������
	PVOID SavingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));

	// Copy the buffer
    // ���ƻ�������
	RtlCopyBytes(SavingBuffer, Buffer, BufferLength);

	// Increment the next index to write
    // ������һ��Ҫд���������
	MessageBufferInformation[Index].CurrentIndexToWrite = MessageBufferInformation[Index].CurrentIndexToWrite + 1;

	// check if there is any thread in IRP Pending state, so we can complete their request
    // ����Ƿ����κδ��� IRP ����״̬���̣߳��Ա�������ǵ�����
	if (GlobalNotifyRecord != NULL)
	{
		/* there is some threads that needs to be completed */
		// set the target pool
        /* ��һЩ��Ҫ��ɵ��߳� */
        // ����Ŀ��ء�
		GlobalNotifyRecord->CheckVmxRootMessagePool = IsVmxRoot;
		// Insert dpc to queue
        // �� DPC ������С�
		KeInsertQueueDpc(&GlobalNotifyRecord->Dpc, GlobalNotifyRecord, NULL);

		// set notify routine to null
        // ��֪ͨ��������Ϊ null��
		GlobalNotifyRecord = NULL;
	}

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
	if (IsVmxRoot)
	{
		SpinlockUnlock(&VmxRootLoggingLock);
	}
	else
	{
		// Release the lock
        // �ͷ�����
		KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
	}
}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
/* �����ķ���ֵ��ʾ��ȡ�Ƿ�ɹ������磬FALSE ��ʾû�п��õ��»���������*/
BOOLEAN LogReadBuffer(BOOLEAN IsVmxRoot, PVOID BufferToSaveMessage, UINT32* ReturnedLength) {

	KIRQL OldIRQL;
	UINT32 Index;

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
	if (IsVmxRoot)
	{
		// Set the index
        // ����������
		Index = 1;

		// Acquire the lock 
		// ��ȡ����
		SpinlockLock(&VmxRootLoggingLock);
	}
	else
	{
		// Set the index
        // ����������
		Index = 0;

		// Acquire the lock 
		// ��ȡ����
		KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
	}

	// Compute the current buffer to read
    // ����Ҫ��ȡ�ĵ�ǰ��������
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
        // û��Ҫ���͵����ݡ�
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */
	// First copy the header 
	/* ������ǵ��������ʾ�ж���Ҫ���� */
    // ���ȸ���ͷ����
	RtlCopyBytes(BufferToSaveMessage, &Header->OpeationNumber, sizeof(UINT32));


	// Second, save the buffer contents
    // ��Σ����滺�������ݡ�
	PVOID SendingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
	PVOID SavingAddress = ((UINT64)BufferToSaveMessage + sizeof(UINT32)); // Because we want to pass the header of usermode header
	RtlCopyBytes(SavingAddress, SendingBuffer, Header->BufferLength);


#if ShowMessagesOnDebugger

	// Means that show just messages
    // ��ʾֻ��ʾ��Ϣ��
	if (Header->OpeationNumber <= OPERATION_LOG_NON_IMMEDIATE_MESSAGE)
	{
		/* We're in Dpc level here so it's safe to use DbgPrint*/
		// DbgPrint limitation is 512 Byte
        /* ������ Dpc ����������Կ��԰�ȫ��ʹ�� DbgPrint */
        // DbgPrint �������� 512 �ֽڡ�
		if (Header->BufferLength > DbgPrintLimitation)
		{
			for (size_t i = 0; i <= Header->BufferLength / DbgPrintLimitation; i++)
			{
				if (i != 0)
				{
					DbgPrint("%s", (char*)((UINT64)SendingBuffer + (DbgPrintLimitation * i) - 2));
				}
				else
				{
					DbgPrint("%s", (char*)((UINT64)SendingBuffer + (DbgPrintLimitation * i)));
				}
			}
		}
		else
		{
			DbgPrint("%s", (char*)SendingBuffer);
		}

	}
#endif


	// Finally, set the current index to invalid as we sent it
    // ��󣬽���ǰ��������Ϊ��Ч����Ϊ�����Ѿ�����������
	Header->Valid = FALSE;

	// Set the length to show as the ReturnedByted in usermode ioctl funtion + size of header
    // ��Ҫ��ʾ�ĳ�������Ϊ�û�ģʽ IOCTL �����е� ReturnedByted ����ͷ���Ĵ�С��
	*ReturnedLength = Header->BufferLength + sizeof(UINT32);


	// Last step is to clear the current buffer (we can't do it once when CurrentIndexToSend is zero because
	// there might be multiple messages on the start of the queue that didn't read yet)
	// we don't free the header
    // ���һ���������ǰ�����������ǲ����� CurrentIndexToSend Ϊ��ʱһ�����������Ϊ���п�ͷ�����ж����δ��ȡ����Ϣ��
    // ���ǲ��ͷ�ͷ����
	RtlZeroMemory(SendingBuffer, Header->BufferLength);

	// Check to see whether we passed the index or not
    // ����Ƿ��Ѿ�������������
	if (MessageBufferInformation[Index].CurrentIndexToSend > MaximumPacketsCapacity - 2)
	{
		MessageBufferInformation[Index].CurrentIndexToSend = 0;
	}
	else
	{
		// Increment the next index to read
        // ������һ��Ҫ��ȡ��������
		MessageBufferInformation[Index].CurrentIndexToSend = MessageBufferInformation[Index].CurrentIndexToSend + 1;
	}

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
	if (IsVmxRoot)
	{
		SpinlockUnlock(&VmxRootLoggingLock);
	}
	else
	{
		// Release the lock
        // �ͷ�����
		KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
	}
	return TRUE;
}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
/* �����ķ���ֵ��ʾ��ȡ�Ƿ�ɹ������磬FALSE ��ʾû�п��õ��»���������*/
BOOLEAN LogCheckForNewMessage(BOOLEAN IsVmxRoot) {

	KIRQL OldIRQL;
	UINT32 Index;

	if (IsVmxRoot)
	{
		Index = 1;
	}
	else
	{
		Index = 0;
	}
	// Compute the current buffer to read
    // ����Ҫ��ȡ�ĵ�ǰ��������
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
        // û��Ҫ���͵����ݡ�
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */
    /* ������ǵ��������ʾ�ж���Ҫ���� */
	return TRUE;
}


// Send string messages and tracing for logging and monitoring
// �����ַ�����Ϣ�͸����Խ�����־��¼�ͼ�ء�
BOOLEAN LogSendMessageToQueue(UINT32 OperationCode, BOOLEAN IsImmediateMessage, BOOLEAN ShowCurrentSystemTime, const char* Fmt, ...)
{
	BOOLEAN Result;
	va_list ArgList;
	size_t WrittenSize;
	UINT32 Index;
	KIRQL OldIRQL;
	BOOLEAN IsVmxRootMode;
	int SprintfResult;
	char LogMessage[PacketChunkSize];
	char TempMessage[PacketChunkSize];
	char TimeBuffer[20] = { 0 };

	// Set Vmx State
    // ���� Vmx ״̬��
	IsVmxRootMode = GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode;

	if (ShowCurrentSystemTime)
	{
		// It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
		// it's better to use - 1
        // ʵ���ϲ���Ҫʹ�� -1������Ϊ�û�ģʽ������ܼ���һ���� null ��β�Ļ��������������ʹ�� -1��
		va_start(ArgList, Fmt);
		// We won't use this because we can't use in any IRQL
        // ���ǲ���ʹ���������Ϊ�����޷����κ� IRQL ��ʹ������
		/*Status = RtlStringCchVPrintfA(TempMessage, PacketChunkSize - 1, Fmt, ArgList);*/
		SprintfResult = vsprintf_s(TempMessage, PacketChunkSize - 1, Fmt, ArgList);
		va_end(ArgList);

		// Check if the buffer passed the limit
        // ��黺�����Ƿ񳬹������ơ�
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // ���ܻ�����̫�������޷��洢����
			return FALSE;
		}

		// Fill the above with timer
        // ʹ�ö�ʱ������������ݡ�
		TIME_FIELDS TimeFields;
		LARGE_INTEGER SystemTime, LocalTime;
		KeQuerySystemTime(&SystemTime);
		ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
		RtlTimeToTimeFields(&LocalTime, &TimeFields);

		// We won't use this because we can't use in any IRQL
        // ���ǲ���ʹ���������Ϊ�����޷����κ� IRQL ��ʹ������
		/*Status = RtlStringCchPrintfA(TimeBuffer, RTL_NUMBER_OF(TimeBuffer),
			"%02hd:%02hd:%02hd.%03hd", TimeFields.Hour,
			TimeFields.Minute, TimeFields.Second,
			TimeFields.Milliseconds);

		// Append time with previous message
		// ��ʱ�丽�ӵ���ǰ����Ϣ�С�
		Status = RtlStringCchPrintfA(LogMessage, PacketChunkSize - 1, "(%s)\t %s", TimeBuffer, TempMessage);*/

		// this function probably run without error, so there is no need to check the return value
        // �����������û�д������У�����û�б�Ҫ��鷵��ֵ��
		sprintf_s(TimeBuffer, RTL_NUMBER_OF(TimeBuffer), "%02hd:%02hd:%02hd.%03hd", TimeFields.Hour,
			TimeFields.Minute, TimeFields.Second,
			TimeFields.Milliseconds);

		// Append time with previous message
        // ��ʱ�丽�ӵ���ǰ����Ϣ�С�
		SprintfResult = sprintf_s(LogMessage, PacketChunkSize - 1, "(%s - core : %d - vmx-root? %s)\t %s", TimeBuffer, KeGetCurrentProcessorNumberEx(0), IsVmxRootMode ? "yes" : "no", TempMessage);

		// Check if the buffer passed the limit
        // ��黺�����Ƿ񳬹������ơ�
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // ���ܻ�����̫�������޷��洢����
			return FALSE;
		}


	}
	else
	{
		// It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
		// it's better to use - 1
        // ʵ���ϲ���Ҫʹ�� -1������Ϊ�û�ģʽ������ܼ���һ���� null ��β�Ļ��������������ʹ�� -1��
		va_start(ArgList, Fmt);
		// We won't use this because we can't use in any IRQL
        // ���ǲ���ʹ���������Ϊ�����޷����κ� IRQL ��ʹ������
		/* Status = RtlStringCchVPrintfA(LogMessage, PacketChunkSize - 1, Fmt, ArgList); */
		SprintfResult = vsprintf_s(LogMessage, PacketChunkSize - 1, Fmt, ArgList);
		va_end(ArgList);

		// Check if the buffer passed the limit
        // ��黺�����Ƿ񳬹������ơ�
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // ���ܻ�����̫�������޷��洢����
			return FALSE;
		}

	}
	// Use std function because they can be run in any IRQL
	// ʹ�ñ�׼��������Ϊ���ǿ������κ� IRQL �����С�
	// RtlStringCchLengthA(LogMessage, PacketChunkSize - 1, &WrittenSize);
	WrittenSize = strnlen_s(LogMessage, PacketChunkSize - 1);

	if (LogMessage[0] == '\0') {

		// nothing to write
        // û����Ҫд������ݡ�
		return FALSE;
	}
#if UseWPPTracing

	if (OperationCode == OPERATION_LOG_INFO_MESSAGE)
	{
		HypervisorTraceLevelMessage(
			TRACE_LEVEL_INFORMATION,  // ETW Level defined in evntrace.h
			HVFS_LOG_INFO,
			"%s",// Flag defined in WPP_CONTROL_GUIDS
			LogMessage);
	}
	else if (OperationCode == OPERATION_LOG_WARNING_MESSAGE)
	{
		HypervisorTraceLevelMessage(
			TRACE_LEVEL_WARNING,  // ETW Level defined in evntrace.h
			HVFS_LOG_WARNING,
			"%s",// Flag defined in WPP_CONTROL_GUIDS
			LogMessage);
	}
	else if (OperationCode == OPERATION_LOG_ERROR_MESSAGE)
	{
		HypervisorTraceLevelMessage(
			TRACE_LEVEL_ERROR,  // ETW Level defined in evntrace.h
			HVFS_LOG_ERROR,
			"%s",// Flag defined in WPP_CONTROL_GUIDS
			LogMessage);
	}
	else
	{
		HypervisorTraceLevelMessage(
			TRACE_LEVEL_NONE,  // ETW Level defined in evntrace.h
			HVFS_LOG,
			"%s",// Flag defined in WPP_CONTROL_GUIDS
			LogMessage);
	}

#else
	if (IsImmediateMessage)
	{
		return LogSendBuffer(OperationCode, LogMessage, WrittenSize);
	}
	else
	{
		// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
        // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
		if (IsVmxRootMode)
		{
			// Set the index
            // ����������
			Index = 1;
			SpinlockLock(&VmxRootLoggingLockForNonImmBuffers);
		}
		else
		{
			// Set the index
            // ����������
			Index = 0;
			// Acquire the lock 
			// ��ȡ����
			KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, &OldIRQL);
		}
		//Set the result to True
        // ���������Ϊ True��
		Result = TRUE;

		// If log message WrittenSize is above the buffer then we have to send the previous buffer
        // �����־��Ϣ�� WrittenSize �����˻������Ĵ�С����ô������Ҫ����ǰһ����������
		if ((MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer + WrittenSize) > PacketChunkSize - 1 && MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer != 0)
		{

			// Send the previous buffer (non-immediate message)
            // ����ǰһ�����������Ǽ�ʱ��Ϣ����
			Result = LogSendBuffer(OPERATION_LOG_NON_IMMEDIATE_MESSAGE,
				MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage,
				MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer);

			// Free the immediate buffer
            // �ͷż�ʱ��������
			MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer = 0;
			RtlZeroMemory(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage, PacketChunkSize);
		}

		// We have to save the message
        // ������Ҫ������Ϣ��
		RtlCopyBytes(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage +
			MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer, LogMessage, WrittenSize);

		// add the length 
		// ��ӳ��ȡ�
		MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer += WrittenSize;


		// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
        // ����Ƿ��� Vmx-root ģʽ������ǣ���ʹ�������Զ���� HIGH_IRQL ��������������ǣ���ʹ�� Windows ��������
		if (IsVmxRootMode)
		{
			SpinlockUnlock(&VmxRootLoggingLockForNonImmBuffers);
		}
		else
		{
			// Release the lock
            // �ͷ�����
			KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, OldIRQL);
		}

		return Result;
	}
#endif
}

/* Complete the IRP in IRP Pending state and fill the usermode buffers with pool data */
/* �� IRP ����״̬����� IRP����ʹ�ó���������û�ģʽ������ */
VOID LogNotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{

	PNOTIFY_RECORD NotifyRecord;
	PIRP Irp;
	UINT32 Length;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	NotifyRecord = DeferredContext;

	ASSERT(NotifyRecord != NULL); // can't be NULL // ����Ϊ NULL��
	_Analysis_assume_(NotifyRecord != NULL);

	switch (NotifyRecord->Type)
	{

	case IRP_BASED:
		Irp = NotifyRecord->Message.PendingIrp;

		if (Irp != NULL) {

			PCHAR OutBuff; // pointer to output buffer // �����������ָ�롣
			ULONG InBuffLength; // Input buffer length // ���뻺�����ĳ��ȡ�
			ULONG OutBuffLength; // Output buffer length // ����������ĳ��ȡ�
			PIO_STACK_LOCATION IrpSp;

			// Make suree that concurrent calls to notify function never occurs
            // ȷ��֪ͨ�������ᷢ���������á�
			if (!(Irp->CurrentLocation <= Irp->StackCount + 1))
			{
				LogError("Probably two or more functions called DPC for an object.");
				return;
			}

			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			InBuffLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
			OutBuffLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

			if (!InBuffLength || !OutBuffLength)
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				break;
			}

			// Check again that SystemBuffer is not null
            // �ٴμ�� SystemBuffer ��Ϊ null��
			if (!Irp->AssociatedIrp.SystemBuffer)
			{
				// Buffer is invalid
                // ��������Ч��
				return;
			}

			OutBuff = Irp->AssociatedIrp.SystemBuffer;
			Length = 0;

			// Read Buffer might be empty (nothing to send)
            // ��ȡ�Ļ���������Ϊ�գ�û��Ҫ���͵����ݣ���
			if (!LogReadBuffer(NotifyRecord->CheckVmxRootMessagePool, OutBuff, &Length))
			{
				// we have to return here as there is nothing to send here
                // ���Ǳ��������ﷵ�أ���Ϊ����û��Ҫ���͵����ݡ�
				return;
			}

			Irp->IoStatus.Information = Length;


			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		break;

	case EVENT_BASED:

		// Signal the Event created in user-mode.
        // �������û�ģʽ�д������¼��źš�
		KeSetEvent(NotifyRecord->Message.Event, 0, FALSE);

		// Dereference the object as we are done with it.
        // ȡ�����øö�����Ϊ�����Ѿ����������ʹ�á�
		ObDereferenceObject(NotifyRecord->Message.Event);

		break;

	default:
		ASSERT(FALSE);
		break;
	}

	if (NotifyRecord != NULL) {
		ExFreePoolWithTag(NotifyRecord, POOLTAG);
	}
}

/* Register a new IRP Pending thread which listens for new buffers */
/* ע��һ���µ� IRP �����̣߳����ڼ����µĻ����� */
NTSTATUS LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PNOTIFY_RECORD NotifyRecord;
	PIO_STACK_LOCATION IrpStack;
	KIRQL   OOldIrql;
	PREGISTER_EVENT RegisterEvent;

	// check if current core has another thread with pending IRP, if no then put the current thread to pending
	// otherwise return and complete thread with STATUS_SUCCESS as there is another thread waiting for message
    // ��鵱ǰ�����Ƿ�����һ�����й��� IRP ���̣߳����û�У��򽫵�ǰ�߳�����Ϊ����״̬��
    // ���򷵻ز��� STATUS_SUCCESS ����̣߳���Ϊ����һ���߳����ڵȴ���Ϣ��
	if (GlobalNotifyRecord == NULL)
	{
		IrpStack = IoGetCurrentIrpStackLocation(Irp);
		RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

		// Allocate a record and save all the event context.
        // ����һ����¼�����������¼������ġ�
		NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), POOLTAG);

		if (NULL == NotifyRecord) {
			return  STATUS_INSUFFICIENT_RESOURCES;
		}

		NotifyRecord->Type = IRP_BASED;
		NotifyRecord->Message.PendingIrp = Irp;

		KeInitializeDpc(&NotifyRecord->Dpc, // Dpc
			LogNotifyUsermodeCallback,     // DeferredRoutine
			NotifyRecord        // DeferredContext
		);

		IoMarkIrpPending(Irp);

		// check for new message (for both Vmx-root mode or Vmx non root-mode)
        // ����Ƿ�������Ϣ������ Vmx-root ģʽ�� Vmx non-root ģʽ����
		if (LogCheckForNewMessage(FALSE))
		{
			// check vmx root
            // ��� Vmx-root��
			NotifyRecord->CheckVmxRootMessagePool = FALSE;

			// Insert dpc to queue
            // �� DPC ������С�
			KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
		}
		else if (LogCheckForNewMessage(TRUE))
		{
			// check vmx non-root
            // ��� Vmx non-root��
			NotifyRecord->CheckVmxRootMessagePool = TRUE;

			// Insert dpc to queue
            // �� DPC ������С�
			KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
		}
		else
		{
			// Set the notify routine to the global structure
            // ��֪ͨ��������Ϊȫ�ֽṹ��
			GlobalNotifyRecord = NotifyRecord;
		}

		// We will return pending as we have marked the IRP pending.
        // ���ǽ����ع���״̬����Ϊ�����Ѿ������ IRP Ϊ����״̬��
		return STATUS_PENDING;
	}
	else
	{
		return STATUS_SUCCESS;
	}
}

/* Create an event-based usermode notifying mechanism*/
/* ���������¼����û�ģʽ֪ͨ���� */
NTSTATUS LogRegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PNOTIFY_RECORD NotifyRecord;
	NTSTATUS Status;
	PIO_STACK_LOCATION IrpStack;
	PREGISTER_EVENT RegisterEvent;
	KIRQL OldIrql;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

	// Allocate a record and save all the event context.
    // ����һ����¼�����������¼������ġ�
	NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), POOLTAG);

	if (NULL == NotifyRecord) {
		return  STATUS_INSUFFICIENT_RESOURCES;
	}

	NotifyRecord->Type = EVENT_BASED;

	KeInitializeDpc(&NotifyRecord->Dpc, // Dpc
		LogNotifyUsermodeCallback,     // DeferredRoutine
		NotifyRecord        // DeferredContext
	);

	// Get the object pointer from the handle. Note we must be in the context of the process that created the handle.
    // �Ӿ���л�ȡ����ָ�롣��ע�⣬���Ǳ����ڴ�������Ľ����������С�
	Status = ObReferenceObjectByHandle(RegisterEvent->hEvent,
		SYNCHRONIZE | EVENT_MODIFY_STATE,
		*ExEventObjectType,
		Irp->RequestorMode,
		&NotifyRecord->Message.Event,
		NULL
	);

	if (!NT_SUCCESS(Status)) {

		LogError("Unable to reference User-Mode Event object, Error = 0x%x", Status);
		ExFreePoolWithTag(NotifyRecord, POOLTAG);
		return Status;
	}

	// Insert dpc to the queue
    // �� DPC ������С�
	KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);

	return STATUS_SUCCESS;
}
