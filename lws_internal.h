/** ****************************************************************************
*
* 	\file	lwo_internal.h
*
* 	\brief	Header file for internal declarations of LwS.
*
******************************************************************************/
/*
*  LwS version 0.1
*
*  2026-04-12
*
*  Copyright (c) 2021 Ari Suomi
*
*------------------------------------------------------------------------------
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3. The name of the author may not be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS AND ANY EXPRESS OR IMPLIED
*  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
*  EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
*  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
*  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
*  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef LWS_INTERNAL_H_INCLUDED
#define LWS_INTERNAL_H_INCLUDED

/*******************************************************************************
;
;	I N C L U D E S
;
;-----------------------------------------------------------------------------*/

#include "lwl/lwl_dlList.h"

/*******************************************************************************
;
;	M A C R O S
;
;-----------------------------------------------------------------------------*/

/** ****************************************************************************
*
* 	\brief 	Get pointer to struct from pointer to struct member.
*
* 	\param	structType_	Struct data type.
* 	\param	membName_	Struct member name.
* 	\param	membPtr_	Pointer to struct member.
*
* 	\return	Pointer to struct.
*
******************************************************************************/
#define lws__getStructPtr(structType_, membName_, membPtr_) \
	((structType_ *)(void *)(((char *)(membPtr_)) - offsetof(structType_, membName_)))

/*******************************************************************************
;
;	T Y P E S
;
;-----------------------------------------------------------------------------*/

/*******************************************************************************
;
;	D A T A
;
;-----------------------------------------------------------------------------*/

const lwl_DlListCmp lws__taskCompare;

/*******************************************************************************
;
;	F U N C T I O N S
;
;-----------------------------------------------------------------------------*/

bool lws__taskPrioCompare(
	const lwl_DlListNode * pNode1,
	const lwl_DlListNode * pNode2,
	const void *		   pParam
);

#endif
