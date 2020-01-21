/******************************************************************************
*
* Copyright (C) 2009 - 2015 Xilinx, Inc. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xil_testmem.c
*
* Contains the memory test utility functions.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver    Who    Date    Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a hbm  08/25/09 First release
* </pre>
*
*****************************************************************************/

/***************************** Include Files ********************************/
#include "ik_testmem.h"
#include "xil_io.h"
#include "xil_assert.h"
#include "microblaze_sleep.h"
#include "AxiTimerHelper.h"
#include <stdlib.h>
#include <stdio.h>

/************************** Constant Definitions ****************************/
/************************** Function Prototypes *****************************/

#define XPAR_AXI_TIMER_DEVICE_ID 		(XPAR_AXI_TIMER_0_DEVICE_ID) // Vivado 2015.2.1

// TIMER Instance
XTmrCtr timer_dev;



static u32 RotateLeft(u32 Input, u8 Width);

/* define ROTATE_RIGHT to give access to this functionality */
/* #define ROTATE_RIGHT */
#ifdef ROTATE_RIGHT
static u32 RotateRight(u32 Input, u8 Width);
#endif /* ROTATE_RIGHT */


/*****************************************************************************/
/**
*
* @brief    Perform a destructive 32-bit wide memory test.
*
* @param    Addr: pointer to the region of memory to be tested.
* @param    Words: length of the block.
* @param    Pattern: constant used for the constant pattern test, if 0,
*           0xDEADBEEF is used.
* @param    Subtest: test type selected. See xil_testmem.h for possible
*	        values.
*
* @return
*           - 0 is returned for a pass
*           - 1 is returned for a failure
*
* @note
* Used for spaces where the address range of the region is smaller than
* the data width. If the memory range is greater than 2 ** Width,
* the patterns used in XIL_TESTMEM_WALKONES and XIL_TESTMEM_WALKZEROS will
* repeat on a boundry of a power of two making it more difficult to detect
* addressing errors. The XIL_TESTMEM_INCREMENT and XIL_TESTMEM_INVERSEADDR
* tests suffer the same problem. Ideally, if large blocks of memory are to be
* tested, break them up into smaller regions of memory to allow the test
* patterns used not to repeat over the region tested.
*
*****************************************************************************/
u64 countOnes64(u64 num)
{
	u32 ret = 0;
	u64 one = 0x0000000000000001;
	u32 I;
	for(I = 0; I < 64; I++)
	{
		if(one & num)
			ret++;
		num = num >> 1;
	}

	return ret;
}

u32 countOnes32(u32 num)
{
	u32 ret = 0;
	u32 one = 0x00000001;
	u32 I;
	for(I = 0; I < 32; I++)
	{
		if(one & num)
			ret++;
		num = num >> 1;
	}

	return ret;
}

u32 countZeros32(u32 num)
{
	u32 ret = 0;
	u32 inv_num = ~num;
	//print("num: 0x"); putnum((unsigned)num); print("\n\r");
	//print("inv_num: 0x"); putnum((unsigned)inv_num); print("\n\r");
	u32 one = 0x00000001;
	u32 I;
	for(I = 0; I < 32; I++)
	{
		if(one & inv_num)
			ret++;
		inv_num = inv_num >> 1;
	}

	return ret;
}

u32 countOnes8(u8 num)
{
	u32 ret = 0;
	u8 one = 0x01;
	u32 I;
	for(I = 0; I < 8; I++)
	{
		if(one & num)
			ret++;
		num = num >> 1;
	}
	return ret;
}


u64 refresh_disable(u32 buffer_ms, u32 wait_ms, AxiTimerHelper* timer)
{

	unsigned disable_base = 0x44A00000;
	u32* base_ptr = (u32*)disable_base;

	u64 disabled_ms = 0;
	u32 wait_ms_temp = wait_ms;

	MB_Sleep(buffer_ms);
	*(base_ptr) = 1U;

	while(wait_ms_temp > 30*1000)
	{
		startTimer(timer);
		MB_Sleep(30*1000);
		wait_ms_temp -= 30*1000;
		stopTimer(timer);
		disabled_ms += getElapsedTimerInMiliSeconds(timer);
	}

	startTimer(timer);
	MB_Sleep(wait_ms_temp);
	stopTimer(timer);
	disabled_ms += getElapsedTimerInMiliSeconds(timer);


	*(base_ptr) = 0U;
	MB_Sleep(buffer_ms);

	return disabled_ms;
}


unsigned long int IK_ValidateMem8(u8 *Addr, u32 Words, u8 Pattern, u8 Subtest, u32 Wait_ms)
{
	u32 I;
	u32 j;
	u8 Val;
	u8 FirtVal;
	u8 WordMem8;
	unsigned long int Fail_bitCount;
	unsigned long int Fail_byteCount;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirtVal = XIL_TESTMEM_INIT_VALUE;

	/*
	 * select the proper Subtest(s)
	 */

	MB_Sleep(Wait_ms);

	// Test All 1s
	if(Subtest == XIL_TESTMEM_ALL1) {
		Fail_bitCount = 0;
		Fail_byteCount = 0;
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			Val = 0xff;
			WordMem8 = *(Addr+I);


			if (WordMem8 != Val) {
				u8 res_xor = Val ^ WordMem8;
				//print("Value: 0x"); putnum((unsigned)Val); print("\n\r");
				//print("WordMem8: 0x"); putnum((unsigned)WordMem8); print("\n\r");
				//print("res_xor: 0x"); putnum((unsigned)res_xor); print("\n\r");
				//print("countOnes8: 0x"); putnum((unsigned)countOnes8(res_xor)); print("\n\r");

				Fail_bitCount += countOnes8(res_xor);
				Fail_byteCount++;
			}
		}

		return Fail_bitCount;
	}

	// Test All 0s
	if(Subtest == XIL_TESTMEM_ALL0) {
		Fail_bitCount = 0;
		Fail_byteCount = 0;

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			Val = 0x00;
			WordMem8 = *(Addr+I);


			if (WordMem8 != Val) {
				u8 res_xor = Val ^ WordMem8;

				//print("Value: 0x"); putnum((unsigned)Val); print("\n\r");
				//print("WordMem8: 0x"); putnum((unsigned)WordMem8); print("\n\r");
				//print("res_xor: 0x"); putnum((unsigned)res_xor); print("\n\r");
				//print("countOnes8: 0x"); putnum((unsigned)countOnes8(res_xor)); print("\n\r");

				Fail_bitCount += countOnes8(res_xor);
				Fail_byteCount++;
			}
		}

		return Fail_bitCount;
	}
}

void IK_TestMem32(u32 *Addr, u32 Words, u8 Pattern, u8 Subtest, u32 Wait_ms, u32 round)
{
	u32 I;
	u32 j;
	u32 Val;
	u32 FirtVal;
	u32 WordMem32;
	u64 Fail_bitCount;
	u64 Unfail_bitCount;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);


	AxiTimerHelper timer;
	timerInitialize(&timer);
	u64 disabled_ms;

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirtVal = XIL_TESTMEM_INIT_VALUE;

	/*
	 * select the proper Subtest(s)
	 */

	// Test All 1s
	if(Subtest == XIL_TESTMEM_ALL1) {
		Fail_bitCount = 0;
		Unfail_bitCount = 0;
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			Val = 0xffffffff;
			*(Addr+I) = Val;
		}
		/*
		 * Restore the reference 'Val' to the
		 * initial value
		 */
		Val = FirtVal;
		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the incrementing reference
		 * Val
		 */

		disabled_ms = refresh_disable(1000, Wait_ms, &timer);

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			Val = 0xffffffff;
			WordMem32 = *(Addr+I);

			u32 res_xor = Val ^ WordMem32;
			if (WordMem32 != Val) {
				//print("Value: 0x"); putnum((unsigned)Val); print("\n\r");
				//print("WordMem32: 0x"); putnum((unsigned)WordMem32); print("\n\r");
				//print("res_xor: 0x"); putnum((unsigned)res_xor); print("\n\r");
				//print("countOnes32: 0x"); putnum((unsigned)countOnes32(res_xor)); print("\n\r");
				u32 temp_fail = countOnes32(res_xor);
				Fail_bitCount += temp_fail;
				//Unfail_bitCount += (32 - temp_fail);
			}
			//else
				//Unfail_bitCount += countZeros32(res_xor);
		}
	}


	/*
	// Test All 0s
	if(Subtest == XIL_TESTMEM_ALL0) {
		Fail_bitCount = 0;

		for (I = 0U; I < Words; I++) {
			Val = 0x00000000;
			*(Addr+I) = Val;
		}

		Val = FirtVal;


		refresh_disable(1000, Wait_ms, &timer);

		for (I = 0U; I < Words; I++) {
			Val = 0x00000000;
			WordMem32 = *(Addr+I);


			if (WordMem32 != Val) {
				u32 res_xor = Val ^ WordMem32;

				//print("Value: 0x"); putnum((unsigned)Val); print("\n\r");
				//print("WordMem32: 0x"); putnum((unsigned)WordMem32); print("\n\r");
				//print("res_xor: 0x"); putnum((unsigned)res_xor); print("\n\r");
				//print("countOnes32: 0x"); putnum((unsigned)countOnes32(res_xor)); print("\n\r");

				Fail_bitCount += countOnes32(res_xor);
			}
		}
	}
	*/

}
void IK_TestMem64_FailCount(u64 *Addr, u32 Words, u8 Subtest, u64 Wait_ms, retentionTestResult* result)
{
	u32 I;
	u32 j;
	u64 Val;
	u64 FirstVal;
	u64 WordMem64;
	u64 FailCount = 0;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);


	AxiTimerHelper timer;
	timerInitialize(&timer);
	u64 disabled_ms;

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirstVal = XIL_TESTMEM_INIT_VALUE;

	/*
	 * select the proper Subtest(s)
	 */

	// Test All 1s
	if(Subtest == XIL_TESTMEM_ALL1) {
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */
		//printf("here\n");
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			Val = 0xffffffffffffffff;
			*(Addr+I) = Val;
		}

		disabled_ms = refresh_disable(1000, Wait_ms, &timer);

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			Val = 0xffffffffffffffff;
			WordMem64 = *(Addr+I);

			//printf("%u, %u, %x, %llx, %llx, %llu\n", (unsigned)round, (unsigned)Subtest, (unsigned)I, (unsigned long long)Val, (unsigned long long)WordMem64, (unsigned long long)disabled_ms);

			if (WordMem64 != Val) {
				u64 res_xor = WordMem64 ^ Val;
				u64 temp = countOnes64(res_xor);
				FailCount += temp;
				// print: round_num, pattern, 64bit_word_num, ref_val, read_val, disabled_time
				//printf("%u, %u, %.8x, %.16llx, %.16llx, %llu\n", (unsigned)round, (unsigned)Subtest, (unsigned)I, (unsigned long long)Val, (unsigned long long)WordMem64, (unsigned long long)disabled_ms);
			}
		}

		result->disabled_ms = disabled_ms;
		result->failed_bit_count = FailCount;

		return;
	}

	if(Subtest == XIL_TESTMEM_WALK1)
	{
		// round < 8
		//FirstVal = (0x0101010101010101) << round;
		FirstVal = (0x0101010101010101);
		Val = FirstVal;

		for (I = 0U; I < Words; I++) {
			*(Addr+I) = Val;
			if(Val & 0x8000000000000000)
				Val = (Val << 1) + 1;
			else
				Val = Val << 1;
		}
//		printf("enter refresh_disable");
		disabled_ms = refresh_disable(1000, Wait_ms, &timer);
//		printf("exit refresh_diable");
		Val = FirstVal;
		for (I = 0U; I < Words; I++) {
			WordMem64 = *(Addr+I);
			if (WordMem64 != Val) {
				u64 res_xor = WordMem64 ^ Val;
				u64 temp = countOnes64(res_xor);
				FailCount += temp;				// print: round_num, pattern, 64bit_word_num, ref_val, read_val, disabled_time
				//printf("%u, %u, %.8x, %.16llx, %.16llx, %llu\n", (unsigned)round, (unsigned)Subtest, (unsigned)I, (unsigned long long)Val, (unsigned long long)WordMem64, (unsigned long long)disabled_ms);
			}

			if(Val & 0x8000000000000000)
				Val = (Val << 1) + 1;
			else
				Val = Val << 1;
		}

		result->disabled_ms = disabled_ms;
		result->failed_bit_count = FailCount;

		return;
	}
}

s32 Xil_TestMem32(u32 *Addr, u32 Words, u32 Pattern, u8 Subtest)
{
	u32 I;
	u32 j;
	u32 Val;
	u32 FirtVal;
	u32 WordMem32;
	s32 Status = 0;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= (u8)XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirtVal = XIL_TESTMEM_INIT_VALUE;


	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INCREMENT)) {
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */
		for (I = 0U; I < Words; I++) {
			*(Addr+I) = Val;
			Val++;
		}

		/*
		 * Restore the reference 'Val' to the
		 * initial value
		 */
		Val = FirtVal;

		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the incrementing reference
		 * Val
		 */

		for (I = 0U; I < Words; I++) {
			WordMem32 = *(Addr+I);

			if (WordMem32 != Val) {
				Status = -1;
				goto End_Label;
			}

			Val++;
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKONES)) {
		/*
		 * set up to cycle through all possible initial
		 * test Patterns for walking ones test
		 */

		for (j = 0U; j < (u32)32; j++) {
			/*
			 * Generate an initial value for walking ones test
			 * to test for bad data bits
			 */

			Val = (1U << j);

			/*
			 * START walking ones test
			 * Write a one to each data bit indifferent locations
			 */

			for (I = 0U; I < (u32)32; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = (u32) RotateLeft(Val, 32U);
			}

			/*
			 * Restore the reference 'val' to the
			 * initial value
			 */
			Val = 1U << j;

			/* Read the values from each location that was
			 * written */
			for (I = 0U; I < (u32)32; I++) {
				/* read memory location */

				WordMem32 = *(Addr+I);

				if (WordMem32 != Val) {
					Status = -1;
					goto End_Label;
				}

				Val = (u32)RotateLeft(Val, 32U);
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKZEROS)) {
		/*
		 * set up to cycle through all possible
		 * initial test Patterns for walking zeros test
		 */

		for (j = 0U; j < (u32)32; j++) {

			/*
			 * Generate an initial value for walking ones test
			 * to test for bad data bits
			 */

			Val = ~(1U << j);

			/*
			 * START walking zeros test
			 * Write a one to each data bit indifferent locations
			 */

			for (I = 0U; I < (u32)32; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = ~((u32)RotateLeft(~Val, 32U));
			}

			/*
			 * Restore the reference 'Val' to the
			 * initial value
			 */

			Val = ~(1U << j);

			/* Read the values from each location that was
			 * written */
			for (I = 0U; I < (u32)32; I++) {
				/* read memory location */
				WordMem32 = *(Addr+I);
				if (WordMem32 != Val) {
					Status = -1;
					goto End_Label;
				}
				Val = ~((u32)RotateLeft(~Val, 32U));
			}

		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INVERSEADDR)) {
		/* Fill the memory with inverse of address */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			Val = (u32) (~((INTPTR) (&Addr[I])));
			*(Addr+I) = Val;
		}

		/*
		 * Check every word within the words
		 * of tested memory
		 */

		for (I = 0U; I < Words; I++) {
			/* Read the location */
			WordMem32 = *(Addr+I);
			Val = (u32) (~((INTPTR) (&Addr[I])));

			if ((WordMem32 ^ Val) != 0x00000000U) {
				Status = -1;
				goto End_Label;
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_FIXEDPATTERN)) {
		/*
		 * Generate an initial value for
		 * memory testing
		 */

		if (Pattern == (u32)0) {
			Val = 0xDEADBEEFU;
		}
		else {
			Val = Pattern;
		}

		/*
		 * Fill the memory with fixed Pattern
		 */

		for (I = 0U; I < Words; I++) {
			/* write memory location */
			*(Addr+I) = Val;
		}

		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the fixed Pattern
		 */

		for (I = 0U; I < Words; I++) {

			/* read memory location */

			WordMem32 = *(Addr+I);
			if (WordMem32 != Val) {
				Status = -1;
				goto End_Label;
			}
		}
	}

End_Label:
	return Status;
}

/*****************************************************************************/
/**
*
* @brief    Perform a destructive 16-bit wide memory test.
*
* @param    Addr: pointer to the region of memory to be tested.
* @param    Words: length of the block.
* @param    Pattern: constant used for the constant Pattern test, if 0,
*           0xDEADBEEF is used.
* @param    Subtest: type of test selected. See xil_testmem.h for possible
*	        values.
*
* @return
*
*           - -1 is returned for a failure
*           - 0 is returned for a pass
*
* @note
* Used for spaces where the address range of the region is smaller than
* the data width. If the memory range is greater than 2 ** Width,
* the patterns used in XIL_TESTMEM_WALKONES and XIL_TESTMEM_WALKZEROS will
* repeat on a boundry of a power of two making it more difficult to detect
* addressing errors. The XIL_TESTMEM_INCREMENT and XIL_TESTMEM_INVERSEADDR
* tests suffer the same problem. Ideally, if large blocks of memory are to be
* tested, break them up into smaller regions of memory to allow the test
* patterns used not to repeat over the region tested.
*
*****************************************************************************/
s32 Xil_TestMem16(u16 *Addr, u32 Words, u16 Pattern, u8 Subtest)
{
	u32 I;
	u32 j;
	u16 Val;
	u16 FirtVal;
	u16 WordMem16;
	s32 Status = 0;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirtVal = XIL_TESTMEM_INIT_VALUE;

	/*
	 * selectthe proper Subtest(s)
	 */

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INCREMENT)) {
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			*(Addr+I) = Val;
			Val++;
		}
		/*
		 * Restore the reference 'Val' to the
		 * initial value
		 */
		Val = FirtVal;

		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the incrementing reference val
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem16 = *(Addr+I);
			if (WordMem16 != Val) {
				Status = -1;
				goto End_Label;
			}
			Val++;
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKONES)) {
		/*
		 * set up to cycle through all possible initial test
		 * Patterns for walking ones test
		 */

		for (j = 0U; j < (u32)16; j++) {
			/*
			 * Generate an initial value for walking ones test
			 * to test for bad data bits
			 */

			Val = (u16)((u32)1 << j);
			/*
			 * START walking ones test
			 * Write a one to each data bit indifferent locations
			 */

			for (I = 0U; I < (u32)16; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = (u16)RotateLeft(Val, 16U);
			}
			/*
			 * Restore the reference 'Val' to the
			 * initial value
			 */
			Val = (u16)((u32)1 << j);
			/* Read the values from each location that was written */
			for (I = 0U; I < (u32)16; I++) {
				/* read memory location */
				WordMem16 = *(Addr+I);
				if (WordMem16 != Val) {
					Status = -1;
					goto End_Label;
				}
				Val = (u16)RotateLeft(Val, 16U);
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKZEROS)) {
		/*
		 * set up to cycle through all possible initial
		 * test Patterns for walking zeros test
		 */

		for (j = 0U; j < (u32)16; j++) {
			/*
			 * Generate an initial value for walking ones
			 * test to test for bad
			 * data bits
			 */

			Val = ~(1U << j);
			/*
			 * START walking zeros test
			 * Write a one to each data bit indifferent locations
			 */

			for (I = 0U; I < (u32)16; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = ~((u16)RotateLeft(~Val, 16U));
			}
			/*
			 * Restore the reference 'Val' to the
			 * initial value
			 */
			Val = ~(1U << j);
			/* Read the values from each location that was written */
			for (I = 0U; I < (u32)16; I++) {
				/* read memory location */
				WordMem16 = *(Addr+I);
				if (WordMem16 != Val) {
					Status = -1;
					goto End_Label;
				}
				Val = ~((u16)RotateLeft(~Val, 16U));
			}

		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INVERSEADDR)) {
		/* Fill the memory with inverse of address */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			Val = (u16) (~((INTPTR)(&Addr[I])));
			*(Addr+I) = Val;
		}
		/*
		 * Check every word within the words
		 * of tested memory
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem16 = *(Addr+I);
			Val = (u16) (~((INTPTR) (&Addr[I])));
			if ((WordMem16 ^ Val) != 0x0000U) {
				Status = -1;
				goto End_Label;
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_FIXEDPATTERN)) {
		/*
		 * Generate an initial value for
		 * memory testing
		 */
		if (Pattern == (u16)0) {
			Val = 0xDEADU;
		}
		else {
			Val = Pattern;
		}

		/*
		 * Fill the memory with fixed pattern
		 */

		for (I = 0U; I < Words; I++) {
			/* write memory location */
			*(Addr+I) = Val;
		}

		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the fixed pattern
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem16 = *(Addr+I);
			if (WordMem16 != Val) {
				Status = -1;
				goto End_Label;
			}
		}
	}

End_Label:
	return Status;
}


/*****************************************************************************/
/**
*
* @brief    Perform a destructive 8-bit wide memory test.
*
* @param    Addr: pointer to the region of memory to be tested.
* @param    Words: length of the block.
* @param    Pattern: constant used for the constant pattern test, if 0,
*           0xDEADBEEF is used.
* @param    Subtest: type of test selected. See xil_testmem.h for possible
*	        values.
*
* @return
*           - -1 is returned for a failure
*           - 0 is returned for a pass
*
* @note
* Used for spaces where the address range of the region is smaller than
* the data width. If the memory range is greater than 2 ** Width,
* the patterns used in XIL_TESTMEM_WALKONES and XIL_TESTMEM_WALKZEROS will
* repeat on a boundry of a power of two making it more difficult to detect
* addressing errors. The XIL_TESTMEM_INCREMENT and XIL_TESTMEM_INVERSEADDR
* tests suffer the same problem. Ideally, if large blocks of memory are to be
* tested, break them up into smaller regions of memory to allow the test
* patterns used not to repeat over the region tested.
*
*****************************************************************************/
s32 Xil_TestMem8(u8 *Addr, u32 Words, u8 Pattern, u8 Subtest)
{
	u32 I;
	u32 j;
	u8 Val;
	u8 FirtVal;
	u8 WordMem8;
	s32 Status = 0;

	Xil_AssertNonvoid(Words != (u32)0);
	Xil_AssertNonvoid(Subtest <= XIL_TESTMEM_MAXTEST);
	Xil_AssertNonvoid(Addr != NULL);

	/*
	 * variable initialization
	 */
	Val = XIL_TESTMEM_INIT_VALUE;
	FirtVal = XIL_TESTMEM_INIT_VALUE;

	/*
	 * select the proper Subtest(s)
	 */

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INCREMENT)) {
		/*
		 * Fill the memory with incrementing
		 * values starting from 'FirtVal'
		 */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			*(Addr+I) = Val;
			Val++;
		}
		/*
		 * Restore the reference 'Val' to the
		 * initial value
		 */
		Val = FirtVal;
		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the incrementing reference
		 * Val
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem8 = *(Addr+I);
			if (WordMem8 != Val) {
				Status = -1;
				goto End_Label;
			}
			Val++;
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKONES)) {
		/*
		 * set up to cycle through all possible initial
		 * test Patterns for walking ones test
		 */

		for (j = 0U; j < (u32)8; j++) {
			/*
			 * Generate an initial value for walking ones test
			 * to test for bad data bits
			 */
			Val = (u8)((u32)1 << j);
			/*
			 * START walking ones test
			 * Write a one to each data bit indifferent locations
			 */
			for (I = 0U; I < (u32)8; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = (u8)RotateLeft(Val, 8U);
			}
			/*
			 * Restore the reference 'Val' to the
			 * initial value
			 */
			Val = (u8)((u32)1 << j);
			/* Read the values from each location that was written */
			for (I = 0U; I < (u32)8; I++) {
				/* read memory location */
				WordMem8 = *(Addr+I);
				if (WordMem8 != Val) {
					Status = -1;
					goto End_Label;
				}
				Val = (u8)RotateLeft(Val, 8U);
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_WALKZEROS)) {
		/*
		 * set up to cycle through all possible initial test
		 * Patterns for walking zeros test
		 */

		for (j = 0U; j < (u32)8; j++) {
			/*
			 * Generate an initial value for walking ones test to test
			 * for bad data bits
			 */
			Val = ~(1U << j);
			/*
			 * START walking zeros test
			 * Write a one to each data bit indifferent locations
			 */
			for (I = 0U; I < (u32)8; I++) {
				/* write memory location */
				*(Addr+I) = Val;
				Val = ~((u8)RotateLeft(~Val, 8U));
			}
			/*
			 * Restore the reference 'Val' to the
			 * initial value
			 */
			Val = ~(1U << j);
			/* Read the values from each location that was written */
			for (I = 0U; I < (u32)8; I++) {
				/* read memory location */
				WordMem8 = *(Addr+I);
				if (WordMem8 != Val) {
					Status = -1;
					goto End_Label;
				}

				Val = ~((u8)RotateLeft(~Val, 8U));
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_INVERSEADDR)) {
		/* Fill the memory with inverse of address */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			Val = (u8) (~((INTPTR) (&Addr[I])));
			*(Addr+I) = Val;
		}

		/*
		 * Check every word within the words
		 * of tested memory
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem8 = *(Addr+I);
			Val = (u8) (~((INTPTR) (&Addr[I])));
			if ((WordMem8 ^ Val) != 0x00U) {
				Status = -1;
				goto End_Label;
			}
		}
	}

	if((Subtest == XIL_TESTMEM_ALLMEMTESTS) || (Subtest == XIL_TESTMEM_FIXEDPATTERN)) {
		/*
		 * Generate an initial value for
		 * memory testing
		 */

		if (Pattern == (u8)0) {
			Val = 0xA5U;
		}
		else {
			Val = Pattern;
		}
		/*
		 * Fill the memory with fixed Pattern
		 */
		for (I = 0U; I < Words; I++) {
			/* write memory location */
			*(Addr+I) = Val;
		}
		/*
		 * Check every word within the words
		 * of tested memory and compare it
		 * with the fixed Pattern
		 */

		for (I = 0U; I < Words; I++) {
			/* read memory location */
			WordMem8 = *(Addr+I);
			if (WordMem8 != Val) {
				Status = -1;
				goto End_Label;
			}
		}
	}

End_Label:
	return Status;
}


/*****************************************************************************/
/**
*
* @brief   Rotates the provided value to the left one bit position
*
* @param    Input is value to be rotated to the left
* @param    Width is the number of bits in the input data
*
* @return
*           The resulting unsigned long value of the rotate left
*
*
*****************************************************************************/
static u32 RotateLeft(u32 Input, u8 Width)
{
	u32 Msb;
	u32 ReturnVal;
	u32 WidthMask;
	u32 MsbMask;
	u32 LocalInput = Input;

	/*
	 * set up the WidthMask and the MsbMask
	 */

	MsbMask = 1U << (Width - 1U);

	WidthMask = (MsbMask << (u32)1) - (u32)1;

	/*
	 * set the Width of the Input to the correct width
	 */

	LocalInput = LocalInput & WidthMask;

	Msb = LocalInput & MsbMask;

	ReturnVal = LocalInput << 1U;

	if (Msb != 0x00000000U) {
		ReturnVal = ReturnVal | (u32)0x00000001;
	}

	ReturnVal = ReturnVal & WidthMask;

	return ReturnVal;

}

#ifdef ROTATE_RIGHT
/*****************************************************************************/
/**
*
* @brief    Rotates the provided value to the right one bit position
*
* @param    Input: value to be rotated to the right
* @param    Width: number of bits in the input data
*
* @return
*           The resulting u32 value of the rotate right
*
*****************************************************************************/
static u32 RotateRight(u32 Input, u8 Width)
{
	u32 Lsb;
	u32 ReturnVal;
	u32 WidthMask;
	u32 MsbMask;
	u32 LocalInput = Input;
	/*
	 * set up the WidthMask and the MsbMask
	 */

	MsbMask = 1U << (Width - 1U);

	WidthMask = (MsbMask << 1U) - 1U;

	/*
	 * set the width of the input to the correct width
	 */

	LocalInput = LocalInput & WidthMask;

	ReturnVal = LocalInput >> 1U;

	Lsb = LocalInput & 0x00000001U;

	if (Lsb != 0x00000000U) {
		ReturnVal = ReturnVal | MsbMask;
	}

	ReturnVal = ReturnVal & WidthMask;

	return ReturnVal;

}
#endif /* ROTATE_RIGHT */
