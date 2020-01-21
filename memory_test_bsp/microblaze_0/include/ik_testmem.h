#ifndef XIL_TESTMEM_H	/* prevent circular inclusions */
#define XIL_TESTMEM_H	/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#include "xil_types.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/

/* xutil_memtest defines */

#define XIL_TESTMEM_INIT_VALUE	1U

/** @name Memory subtests
 * @{
 */
/**
 * See the detailed description of the subtests in the file description.
 */
#define XIL_TESTMEM_ALLMEMTESTS     0x00U
#define XIL_TESTMEM_INCREMENT       0x01U
#define XIL_TESTMEM_WALKONES        0x02U
#define XIL_TESTMEM_WALKZEROS       0x03U
#define XIL_TESTMEM_INVERSEADDR     0x04U
#define XIL_TESTMEM_FIXEDPATTERN    0x05U



#define XIL_TESTMEM_ALL1			0x00U
#define XIL_TESTMEM_ALL0			0x01U
#define XIL_TESTMEM_WALK1	 		0x02U
#define XIL_TESTMEM_WALK0			0x03U
#define XIL_TESTMEM_RANDOM			0x04U
#define XIL_TESTMEM_INVRANDOM		0x05U
#define XIL_TESTMEM_MAXTEST         XIL_TESTMEM_WALK0




typedef struct
{
	u64 disabled_ms;
	u64 failed_bit_count;
	//u64 unfailed_bit_count;
}retentionTestResult;

/* @} */

/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

/* xutil_testmem prototypes */

extern s32 Xil_TestMem32(u32 *Addr, u32 Words, u32 Pattern, u8 Subtest);
extern s32 Xil_TestMem16(u16 *Addr, u32 Words, u16 Pattern, u8 Subtest);
extern s32 Xil_TestMem8(u8 *Addr, u32 Words, u8 Pattern, u8 Subtest);
extern void IK_TestMem32(u32 *Addr, u32 Words, u8 Pattern, u8 Subtest, u32 Wait_ms, u32 round);
extern void IK_TestMem64_PrintAll(u64 *Addr, u32 Words, u8 Subtest, u32 Wait_ms, u32 round, u32 seed);
extern void IK_TestMem64_FailCount(u64 *Addr, u32 Words, u8 Subtest, u64 Wait_ms, retentionTestResult* result);

extern unsigned long int IK_ValidateMem8(u8 *Addr, u32 Words, u8 Pattern, u8 Subtest, u32 Wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
