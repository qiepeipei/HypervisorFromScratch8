#pragma once
#include "Vmx.h"
#include "Ept.h"


//////////////////////////////////////////////////
//                   Structures		   			//
//////////////////////////////////////////////////

typedef struct _INVEPT_DESC
{
	UINT64 EptPointer;
	UINT64  Reserved;
}INVEPT_DESC, * PINVEPT_DESC;




//////////////////////////////////////////////////
//                 Functions	    			//
//////////////////////////////////////////////////

// Invept Functions
// Invept函数（用于使EPT失效）
unsigned char Invept(UINT32 Type, INVEPT_DESC* Descriptor);
unsigned char InveptAllContexts();
unsigned char InveptSingleContext(UINT64 EptPonter);