/* Copyright (C) 2009 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

//include in Core.jpp
//define EXEC_NAME and EXEC_LOOP


//***************************************************************************
//Instruction executor
//***************************************************************************
final int EXEC_NAME() throws Exception {
	int IP = mIP;
	//byte op;
	int rd,rs;
	int imm32;
	int[] regs = mRegs;
	double[] freg = mFregs;
	int[] mem_ds = mMem_ds;
	byte[] mem_cs = mMem_cs;
	int[] mem_cp = mMem_cp;

	mVM_Yield = false;
	//	printf("VM IP at %d\n",(int) IP - (int) mem_cs);

	EXEC_LOOP {
#ifndef PHONE_RELEASE
		mInstCount++;
#endif
#ifdef UPDATE_IP
		mIP = IP;
#endif
		//op = FETCH_CODEBYTE_FAST;
		switch(FETCH_CODEBYTE_FAST) {
			OPC(ADD)	FETCH_RD_RS	ARITH(RD, +, RS);	EOP;
			OPC(ADDI)	FETCH_RD_CONST	ARITH(RD, +, IMM);	EOP;
			OPC(SUB)	FETCH_RD_RS	ARITH(RD, -, RS);	EOP;
			OPC(SUBI)	FETCH_RD_CONST	ARITH(RD, -, IMM);	EOP;
			OPC(MUL)	FETCH_RD_RS	ARITH(RD, *, RS);	EOP;
			OPC(MULI)	FETCH_RD_CONST	ARITH(RD, *, IMM);	EOP;
			OPC(AND)	FETCH_RD_RS	ARITH(RD, &, RS);	EOP;
			OPC(ANDI)	FETCH_RD_CONST	ARITH(RD, &, IMM);	EOP;
			OPC(OR)		FETCH_RD_RS	ARITH(RD, |, RS);	EOP;
			OPC(ORI)	FETCH_RD_CONST	ARITH(RD, |, IMM);	EOP;
			OPC(XOR)	FETCH_RD_RS	ARITH(RD, ^, RS);	EOP;
			OPC(XORI)	FETCH_RD_CONST	ARITH(RD, ^, IMM);	EOP;
			OPC(DIVU)	FETCH_RD_RS	DIVIDEU(RD, RS);	EOP;
			OPC(DIVUI)	FETCH_RD_CONST	DIVIDEU(RD, IMM);	EOP;
			OPC(DIV)	FETCH_RD_RS	DIVIDE(RD, RS);		EOP;
			OPC(DIVI)	FETCH_RD_CONST	DIVIDE(RD, IMM);	EOP;
			OPC(SLL)	FETCH_RD_RS	ARITH(RD, <<, RS);	EOP;
			OPC(SLLI)	FETCH_RD_IMM8	ARITH(RD, <<, IMM);	EOP;
			OPC(SRA)	FETCH_RD_RS	ARITH(RD, >>, RS);	EOP;
			OPC(SRAI)	FETCH_RD_IMM8	ARITH(RD, >>, IMM);	EOP;
			OPC(SRL)	FETCH_RD_RS	ARITH(RD, >>>, RS);	EOP;
			OPC(SRLI)	FETCH_RD_IMM8	ARITH(RD, >>>, IMM);	EOP;

			OPC(NOT)	FETCH_RD_RS	RD = ~RS;	EOP;
			OPC(NEG)	FETCH_RD_RS	RD = -RS;	EOP;

			OPC(PUSH)	FETCH_RD_RS
			{
				if(rd < 2 || rd > 31 || rs < rd || rs > 31)
					BIG_PHAT_ERROR;

				for(int r=rd; r<=rs; r++) {
					REG(REG_sp) -= 4;
					WINT(REG(REG_sp), REG(r));
					DEBUG("\t0x"+Integer.toHexString(REG(r)));
				}
			}
			EOP;

			OPC(POP) FETCH_RD_RS
			{
				if(rd < 2 || rd > 31 || rs < rd || rs > 31)
					BIG_PHAT_ERROR;

				for(int r=rs; r>=rd; r--) {
					REG(r) = RINT(REG(REG_sp));
					REG(REG_sp) += 4;
					DEBUG("\t0x"+Integer.toHexString(REG(r)));
				}
			}
			EOP;

			OPC(CALLR)	FETCH_RD	CALL_RD		EOP;
			OPC(CALLI)	FETCH_IMM32	CALL_IMM	EOP;

			OPC(LDB) FETCH_RD_RS_CONST  RBYTE(RS + IMM, RD); DEBUG("\t"+RD);	EOP;
			OPC(LDH) FETCH_RD_RS_CONST  RSHORT(RS + IMM, RD); DEBUG("\t"+RD);	EOP;
			OPC(LDW) FETCH_RD_RS_CONST  RD = RINT(RS + IMM); DEBUG("\t"+RD);	EOP;

			OPC(STB) FETCH_RD_RS_CONST  WBYTE(RD + IMM, (byte)RS);  EOP;
			OPC(STH) FETCH_RD_RS_CONST  WSHORT(RD + IMM, (short)RS);	EOP;
			OPC(STW) FETCH_RD_RS_CONST  WINT(RD + IMM, RS); EOP;

			OPC(LDI)	FETCH_RD_CONST	RD = IMM;	EOP;
			OPC(LDR)	FETCH_RD_RS	RD = RS;	EOP;

			OPC(LDDR) FETCH_RD_RS WRITE_REG(rd, RS); WRITE_REG(rd+1, REG(rs+1)); EOP;
			OPC(LDDI) FETCH_RD_CONST WRITE_REG(rd, IMM); FETCH_CONST; WRITE_REG(rd+1, IMM); EOP;

			OPC(FLOATS) FETCH_FRD_RS FRD = (double)RS; EOP;
			OPC(FLOATUNS) FETCH_FRD_RS FRD = (double)(((long)RS) & 0x0ffffffff); EOP;

			OPC(FLOATD)
			{
				FETCH_FRD_RS;
				FRD = (double)ints2long(RS, REG(rs+1));
			} EOP;

			OPC(FLOATUND)
			{
				FETCH_FRD_RS;
				long l = ints2long(RS, REG(rs+1));
				if(l >= 0)
					FRD = (double)l;
				else	// This is how BigInteger.doubleValue() does it.
					FRD = Double.parseDouble(unsignedLongToHexString(l));
			} EOP;

			OPC(FSTRS) {
				FETCH_RD_FRS;
				WRITE_REG(rd, Float.floatToIntBits((float)FRS));
			} EOP;
			OPC(FSTRD) {
				FETCH_RD_FRS;
				long l = Double.doubleToLongBits(FRS);
				WRITE_REG(rd, (int)(l >> 32));
				WRITE_REG(rd+1, (int)l);
			} EOP;

			OPC(FLDRS) FETCH_FRD_RS FRD = (double)Float.intBitsToFloat(RS); EOP;
			OPC(FLDRD) FETCH_FRD_RS FRD = Double.longBitsToDouble(ints2long(RS, REG(rs+1))); EOP;

			OPC(FLDR) FETCH_FRD_FRS FRD = FRS; EOP;

			OPC(FLDIS) FETCH_FRD_CONST FRD = (double)Float.intBitsToFloat(IMM); EOP;
			OPC(FLDID) FETCH_FRD_CONST { int imm = IMM; FETCH_CONST; FRD = Double.longBitsToDouble(ints2long(imm, IMM)); } EOP;

			OPC(FIX_TRUNCS) FETCH_RD_FRS WRITE_REG(rd, (int)FRS); EOP;
			OPC(FIX_TRUNCD)
			{
				FETCH_RD_FRS;
				long l = (long)FRS;
				WRITE_REG(rd, (int)(l >> 32));
				WRITE_REG(rd+1, (int)l);
			} EOP;

			// identical to FIX_*. todo: remove these two opcodes.
			OPC(FIXUN_TRUNCS) FETCH_RD_FRS WRITE_REG(rd, (int)FRS); EOP;
			OPC(FIXUN_TRUNCD)
			{
				FETCH_RD_FRS;
				long l = (long)FRS;
				WRITE_REG(rd, (int)(l >> 32));
				WRITE_REG(rd+1, (int)l);
			} EOP;

			OPC(FSTS)
			{
				FETCH_RD_FRS_CONST
				WINT(RD + IMM, Float.floatToIntBits((float)FRS));
			} EOP;

			OPC(FSTD)
			{
				FETCH_RD_FRS_CONST
				int addr = RD + IMM;
				long l = Double.doubleToLongBits(FRS);
				WINT(addr, (int)(l >> 32));
				WINT(addr + 4, (int)l);
			} EOP;

			OPC(FLDS)
			{
				FETCH_RD_RS_CONST
				FRD = (double)Float.intBitsToFloat(RINT(RS + IMM));
			} EOP;

			OPC(FLDD)
			{
				FETCH_RD_RS_CONST
				int addr = RS + IMM;
				FRD = Double.longBitsToDouble(ints2long(RINT(addr), RINT(addr + 4)));
			} EOP;

			OPC(FADD) FETCH_FRD_FRS FRD += FRS; EOP;
			OPC(FSUB) FETCH_FRD_FRS FRD -= FRS; EOP;
			OPC(FMUL) FETCH_FRD_FRS FRD *= FRS; EOP;
			OPC(FDIV)
			{
				FETCH_FRD_FRS;
	#ifndef ALLOW_FLOAT_DIVISION_BY_ZERO
				if(FRS == 0.0)
				{
					throw new Exception("Float division by zero!");
				}
	#endif	//ALLOW_FLOAT_DIVISION_BY_ZERO
				FRD /= FRS;
			}
			EOP;

			OPC(FSQRT) FETCH_FRD_FRS FRD = Math.sqrt(FRS); EOP;
			OPC(FSIN) FETCH_FRD_FRS FRD = Math.sin(FRS); EOP;
			OPC(FCOS) FETCH_FRD_FRS FRD = Math.cos(FRS); EOP;
			OPC(FEXP) FETCH_FRD_FRS FRD = Float11.exp(FRS); EOP;
			OPC(FLOG) FETCH_FRD_FRS FRD = Float11.log(FRS); EOP;
			OPC(FPOW) FETCH_FRD_FRS FRD = Float11.pow(FRD, FRS); EOP;
			OPC(FATAN2) FETCH_FRD_FRS FRD = Float11.atan2(FRD, FRS); EOP;

			OPC(LDD)
			{
				FETCH_RD_RS_CONST
				int addr = RS + IMM;
				RD = RINT(addr);
				REG(rd+1) = RINT(addr + 4);
			}
			EOP;

			OPC(STD)
			{
				FETCH_RD_RS_CONST
				int addr = RD + IMM;
				WINT(addr, RS);
				WINT(addr + 4, REG(rs+1));
			}
			EOP;

			OPC(RET)
				IP = REG(REG_ra);
			EOP;

			OPC(JC_EQ) 	FETCH_RD_RS_ADDR	if (RD == RS)	{ JMP_IMM; } 	EOP;
			OPC(JC_NE)	FETCH_RD_RS_ADDR	if (RD != RS)	{ JMP_IMM; }	EOP;
			OPC(JC_GE)	FETCH_RD_RS_ADDR	if (RD >= RS)	{ JMP_IMM; }	EOP;
			OPC(JC_GT)	FETCH_RD_RS_ADDR	if (RD >  RS)	{ JMP_IMM; }	EOP;
			OPC(JC_LE)	FETCH_RD_RS_ADDR	if (RD <= RS)	{ JMP_IMM; }	EOP;
			OPC(JC_LT)	FETCH_RD_RS_ADDR	if (RD <  RS)	{ JMP_IMM; }	EOP;

			OPC(JC_LTU)	FETCH_RD_RS_ADDR	if(OPU(RD, <, RS))	{ JMP_IMM; }	EOP;
			OPC(JC_GEU)	FETCH_RD_RS_ADDR	if(OPU(RD, >=, RS))	{ JMP_IMM; }	EOP;
			OPC(JC_GTU)	FETCH_RD_RS_ADDR	if(OPU(RD, >, RS))	{ JMP_IMM; }	EOP;
			OPC(JC_LEU)	FETCH_RD_RS_ADDR	if(OPU(RD, <=, RS))	{ JMP_IMM; }	EOP;

			OPC(JPI)	FETCH_IMM32		JMP_IMM		EOP;

			OPC(XB)		FETCH_RD_RS		RD = (int)((byte) RS);	EOP;
			OPC(XH)		FETCH_RD_RS		RD = (int)((short) RS);	EOP;

			OPC(SYSCALL) {
				InvokeSysCall(FETCH_CODEBYTE_FAST);
#ifdef PUBLIC_DEBUG
				mLastSyscall = INVALID_SYSCALL_NUMBER;
#endif
				if(mVM_Yield)
					return IP;
			} EOP;

#ifdef GDB_DEBUG
			OPC(DBG_STAB) {
				IP--;
				mIP = IP;
				mGdbStub.exceptionHandler(0);
				IP = mIP;
			} EOP;
#else
			OPC(BREAK) {	//something of a hack
				mVM_Yield = true;
				return IP;
			} //EOP;	//unreachable statement
#endif

			OPC(CASE) FETCH_RD; FETCH_CONST; {
				int CaseStart = IMM;
				FETCH_CONST; int CaseLength = IMM;
				FETCH_CONST; int tableAddress = IMM;
				FETCH_CONST; int defaultLabel = IMM;
				int index = RD - CaseStart;
				if(index <= CaseLength && index >= 0) {
					IP = mem_ds[tableAddress + index];
				} else {
					IP = defaultLabel;
				}
			} EOP;

			default:
				//VM_State = -3;				// Bad instruction
				DEBUG_ALWAYS("Illegal instruction 0x"+Integer.toHexString(mem_cs[IP-1])+
					" @ 0x"+Integer.toHexString(IP-1)+"\n");
				BIG_PHAT_ERROR;
		}
	}
	EXEC_RETURN
}
