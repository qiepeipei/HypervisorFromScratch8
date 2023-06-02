#pragma once
#include <ntddk.h>
#include  "Vmx.h"
#include  "Logging.h"
#include  "PoolManager.h"

/* Here we put global variables that are used more or less in all part of our hypervisor (not all of them) */
/* ���������Ƿ�����Щ�����ǵ����⻯����ĸ��������л�����ʹ�õ�ȫ�ֱ���������ȫ���� */
//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////


// Save the state and variables related to each to logical core
// ������ÿ���߼�������ص�״̬�ͱ���
VIRTUAL_MACHINE_STATE* GuestState;

// Save the state and variables related to EPT
// ������EPT��ص�״̬�ͱ���
EPT_STATE* EptState;

// Save the state of the thread that waits for messages to deliver to user-mode
// ����ȴ�����Ϣ���ݸ��û�ģʽ���̵߳�״̬
NOTIFY_RECORD* GlobalNotifyRecord;

// Support for execute-only pages (indicating that data accesses are not allowed while instruction fetches are allowed).
// ֧�ֽ�ִ��ҳ�棨ָʾ���������ݷ��ʣ�������ָ���ȡ����
BOOLEAN ExecuteOnlySupport;

// Client Allowed to send IOCTL to the drive
// ����ͻ�������������IOCTL����
BOOLEAN AllowIOCTLFromUsermode;
