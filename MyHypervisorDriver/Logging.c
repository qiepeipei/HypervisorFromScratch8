#include <ntddk.h>
#include <ntstrsafe.h>
#include "Common.h"
#include "Logging.h"
#include "Trace.h"
#include "GlobalVariables.h"
#include "Logging.tmh"


/* Initialize the buffer relating to log message tracing */
/* 初始化与日志消息跟踪相关的缓冲区 */
BOOLEAN LogInitialize() {


	// Initialize buffers for trace message and data messages (wee have two buffers one for vmx root and one for vmx non-root)
    // 初始化跟踪消息和数据消息的缓冲区（我们有两个缓冲区，一个用于 vmx-root，一个用于 vmx non-root）。
	MessageBufferInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(LOG_BUFFER_INFORMATION) * 2, POOLTAG);

	if (!MessageBufferInformation)
	{
		return FALSE; //STATUS_INSUFFICIENT_RESOURCES
	}

	// Zeroing the memory
    // 清零内存。
	RtlZeroMemory(MessageBufferInformation, sizeof(LOG_BUFFER_INFORMATION) * 2);

	// Initialize the lock for Vmx-root mode (HIGH_IRQL Spinlock)
    // 初始化 Vmx-root 模式的锁（HIGH_IRQL 自旋锁）。
	VmxRootLoggingLock = 0;

	// Allocate buffer for messages and initialize the core buffer information 
	// 为消息分配缓冲区并初始化核心缓冲区信息。
	for (int i = 0; i < 2; i++)
	{

		// initialize the lock
		// Actually, only the 0th buffer use this spinlock but let initialize it for both but the second buffer spinlock is useless 
		// as we use our custom spinlock.
        // 初始化锁
        // 实际上，只有第一个缓冲区使用这个自旋锁，但为了一致性，我们为两个缓冲区都进行初始化，但第二个缓冲区的自旋锁是没有用的，
        // 因为我们使用自定义自旋锁。
		KeInitializeSpinLock(&MessageBufferInformation[i].BufferLock);
		KeInitializeSpinLock(&MessageBufferInformation[i].BufferLockForNonImmMessage);

		// allocate the buffer
        // 分配缓冲区。
		MessageBufferInformation[i].BufferStartAddress = ExAllocatePoolWithTag(NonPagedPool, LogBufferSize, POOLTAG);
		MessageBufferInformation[i].BufferForMultipleNonImmediateMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize, POOLTAG);

		if (!MessageBufferInformation[i].BufferStartAddress)
		{
			return FALSE; // STATUS_INSUFFICIENT_RESOURCES
		}

		// Zeroing the buffer
        // 清零内存。
		RtlZeroMemory(MessageBufferInformation[i].BufferStartAddress, LogBufferSize);

		// Set the end address
        // 设置结束地址。
		MessageBufferInformation[i].BufferEndAddress = (UINT64)MessageBufferInformation[i].BufferStartAddress + LogBufferSize;
	}
}

/* Uninitialize the buffer relating to log message tracing */
/* 反初始化与日志消息跟踪相关的缓冲区 */
VOID LogUnInitialize()
{

	// de-allocate buffer for messages and initialize the core buffer information (for vmx-root core)
    // 释放消息缓冲区并初始化核心缓冲区信息（针对 vmx-root 核心）。
	for (int i = 0; i < 2; i++)
	{
		// Free each buffers
        // 释放每个缓冲区。
		ExFreePoolWithTag(MessageBufferInformation[i].BufferStartAddress, POOLTAG);
		ExFreePoolWithTag(MessageBufferInformation[i].BufferForMultipleNonImmediateMessage, POOLTAG);
	}

	// de-allocate buffers for trace message and data messages
    // 释放跟踪消息和数据消息的缓冲区。
	ExFreePoolWithTag(MessageBufferInformation, POOLTAG);
}

/* Save buffer to the pool */
/* 将缓冲区保存到池中 */
BOOLEAN LogSendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength)
{
	KIRQL OldIRQL;
	UINT32 Index;
	BOOLEAN IsVmxRoot;

	if (BufferLength > PacketChunkSize - 1 || BufferLength == 0)
	{
		// We can't save this huge buffer
        // 我们无法保存这个巨大的缓冲区。
		return FALSE;
	}

	// Check that if we're in vmx root-mode
    // 检查是否处于 vmx root 模式。
	IsVmxRoot = GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode;

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
	if (IsVmxRoot)
	{
		// Set the index
        // 设置索引。
		Index = 1;
		SpinlockLock(&VmxRootLoggingLock);
	}
	else
	{
		// Set the index
        // 设置索引。
		Index = 0;
		// Acquire the lock 
		// 获取锁。
		KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
	}

	// check if the buffer is filled to it's maximum index or not
    // 检查缓冲区是否填满到其最大索引。
	if (MessageBufferInformation[Index].CurrentIndexToWrite > MaximumPacketsCapacity - 1)
	{
		// start from the begining
        // 从开头开始。
		MessageBufferInformation[Index].CurrentIndexToWrite = 0;
	}

	// Compute the start of the buffer header
    // 计算缓冲区头的起始位置。
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	// Set the header
    // 设置头部。
	Header->OpeationNumber = OperationCode;
	Header->BufferLength = BufferLength;
	Header->Valid = TRUE;

	/* Now it's time to fill the buffer */
    /* 现在是填充缓冲区的时候了 */

	// compute the saving index
    // 计算保存索引。
	PVOID SavingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));

	// Copy the buffer
    // 复制缓冲区。
	RtlCopyBytes(SavingBuffer, Buffer, BufferLength);

	// Increment the next index to write
    // 增加下一个要写入的索引。
	MessageBufferInformation[Index].CurrentIndexToWrite = MessageBufferInformation[Index].CurrentIndexToWrite + 1;

	// check if there is any thread in IRP Pending state, so we can complete their request
    // 检查是否有任何处于 IRP 挂起状态的线程，以便完成它们的请求。
	if (GlobalNotifyRecord != NULL)
	{
		/* there is some threads that needs to be completed */
		// set the target pool
        /* 有一些需要完成的线程 */
        // 设置目标池。
		GlobalNotifyRecord->CheckVmxRootMessagePool = IsVmxRoot;
		// Insert dpc to queue
        // 将 DPC 插入队列。
		KeInsertQueueDpc(&GlobalNotifyRecord->Dpc, GlobalNotifyRecord, NULL);

		// set notify routine to null
        // 将通知函数设置为 null。
		GlobalNotifyRecord = NULL;
	}

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
	if (IsVmxRoot)
	{
		SpinlockUnlock(&VmxRootLoggingLock);
	}
	else
	{
		// Release the lock
        // 释放锁。
		KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
	}
}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
/* 函数的返回值表示读取是否成功（例如，FALSE 表示没有可用的新缓冲区）。*/
BOOLEAN LogReadBuffer(BOOLEAN IsVmxRoot, PVOID BufferToSaveMessage, UINT32* ReturnedLength) {

	KIRQL OldIRQL;
	UINT32 Index;

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
	if (IsVmxRoot)
	{
		// Set the index
        // 设置索引。
		Index = 1;

		// Acquire the lock 
		// 获取锁。
		SpinlockLock(&VmxRootLoggingLock);
	}
	else
	{
		// Set the index
        // 设置索引。
		Index = 0;

		// Acquire the lock 
		// 获取锁。
		KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
	}

	// Compute the current buffer to read
    // 计算要读取的当前缓冲区。
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
        // 没有要发送的内容。
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */
	// First copy the header 
	/* 如果我们到达这里，表示有东西要发送 */
    // 首先复制头部。
	RtlCopyBytes(BufferToSaveMessage, &Header->OpeationNumber, sizeof(UINT32));


	// Second, save the buffer contents
    // 其次，保存缓冲区内容。
	PVOID SendingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
	PVOID SavingAddress = ((UINT64)BufferToSaveMessage + sizeof(UINT32)); // Because we want to pass the header of usermode header
	RtlCopyBytes(SavingAddress, SendingBuffer, Header->BufferLength);


#if ShowMessagesOnDebugger

	// Means that show just messages
    // 表示只显示消息。
	if (Header->OpeationNumber <= OPERATION_LOG_NON_IMMEDIATE_MESSAGE)
	{
		/* We're in Dpc level here so it's safe to use DbgPrint*/
		// DbgPrint limitation is 512 Byte
        /* 我们在 Dpc 级别这里，所以可以安全地使用 DbgPrint */
        // DbgPrint 的限制是 512 字节。
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
    // 最后，将当前索引设置为无效，因为我们已经发送了它。
	Header->Valid = FALSE;

	// Set the length to show as the ReturnedByted in usermode ioctl funtion + size of header
    // 将要显示的长度设置为用户模式 IOCTL 函数中的 ReturnedByted 加上头部的大小。
	*ReturnedLength = Header->BufferLength + sizeof(UINT32);


	// Last step is to clear the current buffer (we can't do it once when CurrentIndexToSend is zero because
	// there might be multiple messages on the start of the queue that didn't read yet)
	// we don't free the header
    // 最后一步是清除当前缓冲区（我们不能在 CurrentIndexToSend 为零时一次性清除，因为队列开头可能有多个尚未读取的消息）
    // 我们不释放头部。
	RtlZeroMemory(SendingBuffer, Header->BufferLength);

	// Check to see whether we passed the index or not
    // 检查是否已经超过了索引。
	if (MessageBufferInformation[Index].CurrentIndexToSend > MaximumPacketsCapacity - 2)
	{
		MessageBufferInformation[Index].CurrentIndexToSend = 0;
	}
	else
	{
		// Increment the next index to read
        // 增加下一个要读取的索引。
		MessageBufferInformation[Index].CurrentIndexToSend = MessageBufferInformation[Index].CurrentIndexToSend + 1;
	}

	// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
    // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
	if (IsVmxRoot)
	{
		SpinlockUnlock(&VmxRootLoggingLock);
	}
	else
	{
		// Release the lock
        // 释放锁。
		KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
	}
	return TRUE;
}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
/* 函数的返回值表示读取是否成功（例如，FALSE 表示没有可用的新缓冲区）。*/
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
    // 计算要读取的当前缓冲区。
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
        // 没有要发送的内容。
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */
    /* 如果我们到达这里，表示有东西要发送 */
	return TRUE;
}


// Send string messages and tracing for logging and monitoring
// 发送字符串消息和跟踪以进行日志记录和监控。
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
    // 设置 Vmx 状态。
	IsVmxRootMode = GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode;

	if (ShowCurrentSystemTime)
	{
		// It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
		// it's better to use - 1
        // 实际上不需要使用 -1，但因为用户模式代码可能假设一个以 null 结尾的缓冲区，所以最好使用 -1。
		va_start(ArgList, Fmt);
		// We won't use this because we can't use in any IRQL
        // 我们不会使用这个，因为我们无法在任何 IRQL 中使用它。
		/*Status = RtlStringCchVPrintfA(TempMessage, PacketChunkSize - 1, Fmt, ArgList);*/
		SprintfResult = vsprintf_s(TempMessage, PacketChunkSize - 1, Fmt, ArgList);
		va_end(ArgList);

		// Check if the buffer passed the limit
        // 检查缓冲区是否超过了限制。
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // 可能缓冲区太大，我们无法存储它。
			return FALSE;
		}

		// Fill the above with timer
        // 使用定时器填充上述内容。
		TIME_FIELDS TimeFields;
		LARGE_INTEGER SystemTime, LocalTime;
		KeQuerySystemTime(&SystemTime);
		ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
		RtlTimeToTimeFields(&LocalTime, &TimeFields);

		// We won't use this because we can't use in any IRQL
        // 我们不会使用这个，因为我们无法在任何 IRQL 中使用它。
		/*Status = RtlStringCchPrintfA(TimeBuffer, RTL_NUMBER_OF(TimeBuffer),
			"%02hd:%02hd:%02hd.%03hd", TimeFields.Hour,
			TimeFields.Minute, TimeFields.Second,
			TimeFields.Milliseconds);

		// Append time with previous message
		// 将时间附加到先前的消息中。
		Status = RtlStringCchPrintfA(LogMessage, PacketChunkSize - 1, "(%s)\t %s", TimeBuffer, TempMessage);*/

		// this function probably run without error, so there is no need to check the return value
        // 这个函数可能没有错误运行，所以没有必要检查返回值。
		sprintf_s(TimeBuffer, RTL_NUMBER_OF(TimeBuffer), "%02hd:%02hd:%02hd.%03hd", TimeFields.Hour,
			TimeFields.Minute, TimeFields.Second,
			TimeFields.Milliseconds);

		// Append time with previous message
        // 将时间附加到先前的消息中。
		SprintfResult = sprintf_s(LogMessage, PacketChunkSize - 1, "(%s - core : %d - vmx-root? %s)\t %s", TimeBuffer, KeGetCurrentProcessorNumberEx(0), IsVmxRootMode ? "yes" : "no", TempMessage);

		// Check if the buffer passed the limit
        // 检查缓冲区是否超过了限制。
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // 可能缓冲区太大，我们无法存储它。
			return FALSE;
		}


	}
	else
	{
		// It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
		// it's better to use - 1
        // 实际上不需要使用 -1，但因为用户模式代码可能假设一个以 null 结尾的缓冲区，所以最好使用 -1。
		va_start(ArgList, Fmt);
		// We won't use this because we can't use in any IRQL
        // 我们不会使用这个，因为我们无法在任何 IRQL 中使用它。
		/* Status = RtlStringCchVPrintfA(LogMessage, PacketChunkSize - 1, Fmt, ArgList); */
		SprintfResult = vsprintf_s(LogMessage, PacketChunkSize - 1, Fmt, ArgList);
		va_end(ArgList);

		// Check if the buffer passed the limit
        // 检查缓冲区是否超过了限制。
		if (SprintfResult == -1)
		{
			// Probably the buffer is large that we can't store it
            // 可能缓冲区太大，我们无法存储它。
			return FALSE;
		}

	}
	// Use std function because they can be run in any IRQL
	// 使用标准函数，因为它们可以在任何 IRQL 中运行。
	// RtlStringCchLengthA(LogMessage, PacketChunkSize - 1, &WrittenSize);
	WrittenSize = strnlen_s(LogMessage, PacketChunkSize - 1);

	if (LogMessage[0] == '\0') {

		// nothing to write
        // 没有需要写入的内容。
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
        // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
		if (IsVmxRootMode)
		{
			// Set the index
            // 设置索引。
			Index = 1;
			SpinlockLock(&VmxRootLoggingLockForNonImmBuffers);
		}
		else
		{
			// Set the index
            // 设置索引。
			Index = 0;
			// Acquire the lock 
			// 获取锁。
			KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, &OldIRQL);
		}
		//Set the result to True
        // 将结果设置为 True。
		Result = TRUE;

		// If log message WrittenSize is above the buffer then we have to send the previous buffer
        // 如果日志消息的 WrittenSize 超过了缓冲区的大小，那么我们需要发送前一个缓冲区。
		if ((MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer + WrittenSize) > PacketChunkSize - 1 && MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer != 0)
		{

			// Send the previous buffer (non-immediate message)
            // 发送前一个缓冲区（非即时消息）。
			Result = LogSendBuffer(OPERATION_LOG_NON_IMMEDIATE_MESSAGE,
				MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage,
				MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer);

			// Free the immediate buffer
            // 释放即时缓冲区。
			MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer = 0;
			RtlZeroMemory(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage, PacketChunkSize);
		}

		// We have to save the message
        // 我们需要保存消息。
		RtlCopyBytes(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage +
			MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer, LogMessage, WrittenSize);

		// add the length 
		// 添加长度。
		MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer += WrittenSize;


		// Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock, if not we use the windows spinlock
        // 检查是否处于 Vmx-root 模式，如果是，则使用我们自定义的 HIGH_IRQL 自旋锁，如果不是，则使用 Windows 自旋锁。
		if (IsVmxRootMode)
		{
			SpinlockUnlock(&VmxRootLoggingLockForNonImmBuffers);
		}
		else
		{
			// Release the lock
            // 释放锁。
			KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, OldIRQL);
		}

		return Result;
	}
#endif
}

/* Complete the IRP in IRP Pending state and fill the usermode buffers with pool data */
/* 在 IRP 挂起状态下完成 IRP，并使用池数据填充用户模式缓冲区 */
VOID LogNotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{

	PNOTIFY_RECORD NotifyRecord;
	PIRP Irp;
	UINT32 Length;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	NotifyRecord = DeferredContext;

	ASSERT(NotifyRecord != NULL); // can't be NULL // 不能为 NULL。
	_Analysis_assume_(NotifyRecord != NULL);

	switch (NotifyRecord->Type)
	{

	case IRP_BASED:
		Irp = NotifyRecord->Message.PendingIrp;

		if (Irp != NULL) {

			PCHAR OutBuff; // pointer to output buffer // 输出缓冲区的指针。
			ULONG InBuffLength; // Input buffer length // 输入缓冲区的长度。
			ULONG OutBuffLength; // Output buffer length // 输出缓冲区的长度。
			PIO_STACK_LOCATION IrpSp;

			// Make suree that concurrent calls to notify function never occurs
            // 确保通知函数不会发生并发调用。
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
            // 再次检查 SystemBuffer 不为 null。
			if (!Irp->AssociatedIrp.SystemBuffer)
			{
				// Buffer is invalid
                // 缓冲区无效。
				return;
			}

			OutBuff = Irp->AssociatedIrp.SystemBuffer;
			Length = 0;

			// Read Buffer might be empty (nothing to send)
            // 读取的缓冲区可能为空（没有要发送的内容）。
			if (!LogReadBuffer(NotifyRecord->CheckVmxRootMessagePool, OutBuff, &Length))
			{
				// we have to return here as there is nothing to send here
                // 我们必须在这里返回，因为这里没有要发送的内容。
				return;
			}

			Irp->IoStatus.Information = Length;


			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		break;

	case EVENT_BASED:

		// Signal the Event created in user-mode.
        // 发出在用户模式中创建的事件信号。
		KeSetEvent(NotifyRecord->Message.Event, 0, FALSE);

		// Dereference the object as we are done with it.
        // 取消引用该对象，因为我们已经完成了它的使用。
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
/* 注册一个新的 IRP 挂起线程，用于监听新的缓冲区 */
NTSTATUS LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PNOTIFY_RECORD NotifyRecord;
	PIO_STACK_LOCATION IrpStack;
	KIRQL   OOldIrql;
	PREGISTER_EVENT RegisterEvent;

	// check if current core has another thread with pending IRP, if no then put the current thread to pending
	// otherwise return and complete thread with STATUS_SUCCESS as there is another thread waiting for message
    // 检查当前核心是否有另一个具有挂起 IRP 的线程，如果没有，则将当前线程设置为挂起状态；
    // 否则返回并以 STATUS_SUCCESS 完成线程，因为有另一个线程正在等待消息。
	if (GlobalNotifyRecord == NULL)
	{
		IrpStack = IoGetCurrentIrpStackLocation(Irp);
		RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

		// Allocate a record and save all the event context.
        // 分配一个记录并保存所有事件上下文。
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
        // 检查是否有新消息（对于 Vmx-root 模式或 Vmx non-root 模式）。
		if (LogCheckForNewMessage(FALSE))
		{
			// check vmx root
            // 检查 Vmx-root。
			NotifyRecord->CheckVmxRootMessagePool = FALSE;

			// Insert dpc to queue
            // 将 DPC 插入队列。
			KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
		}
		else if (LogCheckForNewMessage(TRUE))
		{
			// check vmx non-root
            // 检查 Vmx non-root。
			NotifyRecord->CheckVmxRootMessagePool = TRUE;

			// Insert dpc to queue
            // 将 DPC 插入队列。
			KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
		}
		else
		{
			// Set the notify routine to the global structure
            // 将通知函数设置为全局结构。
			GlobalNotifyRecord = NotifyRecord;
		}

		// We will return pending as we have marked the IRP pending.
        // 我们将返回挂起状态，因为我们已经标记了 IRP 为挂起状态。
		return STATUS_PENDING;
	}
	else
	{
		return STATUS_SUCCESS;
	}
}

/* Create an event-based usermode notifying mechanism*/
/* 创建基于事件的用户模式通知机制 */
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
    // 分配一个记录并保存所有事件上下文。
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
    // 从句柄中获取对象指针。请注意，我们必须在创建句柄的进程上下文中。
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
    // 将 DPC 插入队列。
	KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);

	return STATUS_SUCCESS;
}
