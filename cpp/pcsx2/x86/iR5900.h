// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "Config.h"
#include "R5900.h"
#include "R5900_Profiler.h"
#include "VU.h"
#include "iCore.h"

#include "common/emitter/x86emitter.h"

// Register containing a pointer to our fastmem (4GB) area
// On ARM64 builds (including macOS Apple Silicon), this is defined in
// arm64/VixlHelpers.h to an AArch64 register. Only define the x86 variant
// when we're not on Android and not compiling for ARM64.
#if !defined(__ANDROID__) && !defined(__aarch64__) && !defined(_M_ARM64)
#define RFASTMEMBASE x86Emitter::rbp
#endif

// iPSX2: Enable recompiler modules when Real Rec is requested
#if defined(iPSX2_REAL_REC) && iPSX2_REAL_REC
    #ifndef IPSX2_NO_SHIFT_RECOMPILE
    #define SHIFT_RECOMPILE
    #endif
    #ifndef IPSX2_NO_MULTDIV_RECOMPILE
    #define MULTDIV_RECOMPILE
    #endif
#endif

extern u32 maxrecmem;
extern u32 pc;             // recompiler pc
extern int g_branch;       // set for branch
extern u32 target;         // branch target
extern u32 s_nBlockCycles; // cycles of current block recompiling
extern bool s_nBlockInterlocked; // Current block has VU0 interlocking

//////////////////////////////////////////////////////////////////////////////////////////
//

#define REC_FUNC(f) \
	void rec##f() \
	{ \
		recCall(Interp::f); \
	}

#define REC_FUNC_DEL(f, delreg) \
	void rec##f() \
	{ \
		if ((delreg) > 0) \
			_deleteEEreg(delreg, 1); \
		recCall(Interp::f); \
	}

#define REC_SYS(f) \
	void rec##f() \
	{ \
		recBranchCall(Interp::f); \
	}

#define REC_SYS_DEL(f, delreg) \
	void rec##f() \
	{ \
		if ((delreg) > 0) \
			_deleteEEreg(delreg, 1); \
		recBranchCall(Interp::f); \
	}

extern bool g_recompilingDelaySlot;

// Used for generating backpatch thunks for fastmem.
u8* recBeginThunk();
u8* recEndThunk();

// used when processing branches
bool TrySwapDelaySlot(u32 rs, u32 rt, u32 rd, bool allow_loadstore);
void SaveBranchState();
void LoadBranchState();

void recompileNextInstruction(bool delayslot, bool swapped_delay_slot);
void SetBranchReg(u32 reg);
void SetBranchImm(u32 imm);

void iFlushCall(int flushtype);
void recBranchCall(void (*func)());
void recCall(void (*func)());
u32 scaleblockcycles_clear();

namespace R5900
{
	namespace Dynarec
	{
		extern void recDoBranchImm(u32 branchTo, a64::Label* jmpSkip, bool isLikely = false, bool swappedDelaySlot = false);
	} // namespace Dynarec
} // namespace R5900

////////////////////////////////////////////////////////////////////
// Constant Propagation - From here to the end of the header!

#define GPR_IS_CONST1(reg) (EE_CONST_PROP && (reg) < 32 && (g_cpuHasConstReg & (1 << (reg))))
#define GPR_IS_CONST2(reg1, reg2) (EE_CONST_PROP && (g_cpuHasConstReg & (1 << (reg1))) && (g_cpuHasConstReg & (1 << (reg2))))
#define GPR_IS_DIRTY_CONST(reg) (EE_CONST_PROP && (reg) < 32 && (g_cpuHasConstReg & (1 << (reg))) && (!(g_cpuFlushedConstReg & (1 << (reg)))))
#define GPR_SET_CONST(reg) \
	{ \
		if ((reg) < 32) \
		{ \
			g_cpuHasConstReg |= (1 << (reg)); \
			g_cpuFlushedConstReg &= ~(1 << (reg)); \
		} \
	}

#define GPR_DEL_CONST(reg) \
	{ \
		if ((reg) < 32) \
			g_cpuHasConstReg &= ~(1 << (reg)); \
	}

alignas(16) extern GPR_reg64 g_cpuConstRegs[32];
extern u32 g_cpuHasConstReg, g_cpuFlushedConstReg;

// finds where the GPR is stored and moves lower 32 bits to EAX
void _eeMoveGPRtoR(const a64::Register& to, int fromgpr, bool allow_preload = true);
void _eeMoveGPRtoM(const a64::MemOperand& to, int fromgpr); // 32-bit only

void _eeFlushAllDirty();
void _eeOnWriteReg(int reg, int signext);

// totally deletes from const, xmm, and mmx entries
// if flush is 1, also flushes to memory
// if 0, only flushes if not an xmm reg (used when overwriting lower 64bits of reg)
void _deleteEEreg(int reg, int flush);
void _deleteEEreg128(int reg);

void _flushEEreg(int reg, bool clear = false);

int _eeTryRenameReg(int to, int from, int fromx86, int other, int xmminfo);

//////////////////////////////////////
// Templates for code recompilation //
//////////////////////////////////////

typedef void (*R5900FNPTR)();
typedef void (*R5900FNPTR_INFO)(int info);

#define EERECOMPILE_CODE0(fn, xmminfo) \
	void rec##fn(void) \
	{ \
		EE::Profiler.EmitOp(eeOpcode::fn); \
		eeRecompileCode0(rec##fn##_const, rec##fn##_consts, rec##fn##_constt, rec##fn##_, (xmminfo)); \
	}
#define EERECOMPILE_CODERC0(fn, xmminfo) \
	void rec##fn(void) \
	{ \
		EE::Profiler.EmitOp(eeOpcode::fn); \
		eeRecompileCodeRC0(rec##fn##_const, rec##fn##_consts, rec##fn##_constt, rec##fn##_, (xmminfo)); \
	}

#define EERECOMPILE_CODEX(codename, fn, xmminfo) \
	void rec##fn(void) \
	{ \
		EE::Profiler.EmitOp(eeOpcode::fn); \
		codename(rec##fn##_const, rec##fn##_, (xmminfo)); \
	}

#define EERECOMPILE_CODEI(codename, fn, xmminfo) \
	void rec##fn(void) \
	{ \
		EE::Profiler.EmitOp(eeOpcode::fn); \
		codename(rec##fn##_const, rec##fn##_, (xmminfo)); \
	}

//
// MMX/XMM caching helpers
//

// rd = rs op rt
void eeRecompileCodeRC0(R5900FNPTR constcode, R5900FNPTR_INFO constscode, R5900FNPTR_INFO consttcode, R5900FNPTR_INFO noconstcode, int xmminfo);
// rt = rs op imm16
void eeRecompileCodeRC1(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode, int xmminfo);
// rd = rt op sa
void eeRecompileCodeRC2(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode, int xmminfo);

// [iPSX2 FPU bisect] -DIPSX2_FPU_INTERP_<OP>=1 runs that one FPU op through the interpreter
// (same fallback as building without FPU_RECOMPILE); 0 = normal JIT. See fpu-bisect.yml.
#ifndef IPSX2_FPU_INTERP_ABS_S
#define IPSX2_FPU_INTERP_ABS_S 0
#endif
#ifndef IPSX2_FPU_INTERP_ADD_S
#define IPSX2_FPU_INTERP_ADD_S 0
#endif
#ifndef IPSX2_FPU_INTERP_ADDA_S
#define IPSX2_FPU_INTERP_ADDA_S 0
#endif
#ifndef IPSX2_FPU_INTERP_C_EQ
#define IPSX2_FPU_INTERP_C_EQ 0
#endif
#ifndef IPSX2_FPU_INTERP_C_LE
#define IPSX2_FPU_INTERP_C_LE 0
#endif
#ifndef IPSX2_FPU_INTERP_C_LT
#define IPSX2_FPU_INTERP_C_LT 0
#endif
#ifndef IPSX2_FPU_INTERP_DIV_S
#define IPSX2_FPU_INTERP_DIV_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MADD_S
#define IPSX2_FPU_INTERP_MADD_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MADDA_S
#define IPSX2_FPU_INTERP_MADDA_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MAX_S
#define IPSX2_FPU_INTERP_MAX_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MIN_S
#define IPSX2_FPU_INTERP_MIN_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MOV_S
#define IPSX2_FPU_INTERP_MOV_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MSUB_S
#define IPSX2_FPU_INTERP_MSUB_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MSUBA_S
#define IPSX2_FPU_INTERP_MSUBA_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MUL_S
#define IPSX2_FPU_INTERP_MUL_S 0
#endif
#ifndef IPSX2_FPU_INTERP_MULA_S
#define IPSX2_FPU_INTERP_MULA_S 0
#endif
#ifndef IPSX2_FPU_INTERP_NEG_S
#define IPSX2_FPU_INTERP_NEG_S 0
#endif
#ifndef IPSX2_FPU_INTERP_SUB_S
#define IPSX2_FPU_INTERP_SUB_S 0
#endif
#ifndef IPSX2_FPU_INTERP_SUBA_S
#define IPSX2_FPU_INTERP_SUBA_S 0
#endif
#ifndef IPSX2_FPU_INTERP_SQRT_S
#define IPSX2_FPU_INTERP_SQRT_S 0
#endif
#ifndef IPSX2_FPU_INTERP_RSQRT_S
#define IPSX2_FPU_INTERP_RSQRT_S 0
#endif
#ifndef IPSX2_FPU_INTERP_CVT_S
#define IPSX2_FPU_INTERP_CVT_S 0
#endif
#ifndef IPSX2_FPU_INTERP_CVT_W
#define IPSX2_FPU_INTERP_CVT_W 0
#endif
// [iPSX2 FPU verify] -DIPSX2_FPU_VERIFY=1: every FPU op is JIT-compiled as normal, then re-run by the
// interpreter on a snapshot of the pre-op FPU state. Mismatches go to Documents/fpu_verify.txt.
// The JIT result is what the game keeps, so gameplay is unchanged apart from speed.
#ifndef IPSX2_FPU_VERIFY
#define IPSX2_FPU_VERIFY 0
#endif
void iPSX2_FpuVerifyBefore();
void iPSX2_FpuVerifyAfter(void (*interp)(), const char* name);
void iPSX2_FpuVerifyEmitBefore();
void iPSX2_FpuVerifyEmitAfter(void (*interp)(), const char* name);

#define FPURECOMPILE_CONSTCODE(fn, xmminfo) \
	void rec##fn(void) \
	{ \
		if (IPSX2_FPU_INTERP_##fn) \
		{ \
			iFlushCall(FLUSH_INTERPRETER); \
			armEmitCall(reinterpret_cast<void*>((uptr)R5900::Interpreter::OpcodeImpl::COP1::fn)); \
			return; \
		} \
		if (IPSX2_FPU_VERIFY) \
			iPSX2_FpuVerifyEmitBefore(); \
		if (CHECK_FPU_FULL) \
			eeFPURecompileCode(DOUBLE::rec##fn##_xmm, R5900::Interpreter::OpcodeImpl::COP1::fn, xmminfo); \
		else \
			eeFPURecompileCode(rec##fn##_xmm, R5900::Interpreter::OpcodeImpl::COP1::fn, xmminfo); \
		if (IPSX2_FPU_VERIFY) \
			iPSX2_FpuVerifyEmitAfter(R5900::Interpreter::OpcodeImpl::COP1::fn, #fn); \
	}

// rd = rs op rt (all regs need to be in xmm)
int eeRecompileCodeXMM(int xmminfo);
void eeFPURecompileCode(R5900FNPTR_INFO xmmcode, R5900FNPTR fpucode, int xmminfo);

// [STEP2] Flight Recorder Struct (Shared)
struct Step2FlightRecEntry {
    u32 kind;       // 1=READ, 2=BRANCH
    u32 guest_pc;   // The PC being compiled
    u32 addr;       // Read Address OR Branch Target
    u32 val;        // Read Value OR Branch Taken
    u32 extra1;     // RS value or 0
    u32 extra2;     // RT value or 0
    u32 pc_m4;
    u32 pc_m8;
};
extern volatile u32 g_step2_idx;
extern volatile Step2FlightRecEntry g_step2_ring[];
// [DIAG] ROM_READ_DIAG_V1
struct RomReadDiagEntry {
    u32 pc;
    u32 va32;
    u32 phys32;
    u32 val32;
    u64 host_ptr;
    u32 path_id; // 1=FastMem
    u32 _pad;
};
extern volatile RomReadDiagEntry g_rom_diag_ring[];
extern volatile u32 g_rom_diag_idx;

extern "C" void Step2_CheckDump();

