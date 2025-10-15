//----------------------------------------------------------------------------
//
// organism.h
//
//----------------------------------------------------------------------------
//
// Copyright (c) 2006, Symantec Corporation All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// -  Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer. 
//
// -  Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution. 
//
// - Neither the name of Symantec Corp. nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission. 
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef _ORGANISM_H_

#define _ORGANISM_H_

#include "types.h"
#include "constants.h"
#include "mycon.h"
#include "disasm.h"

#include <stdio.h>

class World;

class Organism
{
public:
	Organism
	(
		World *world, 
		uint16  *myDNA, 
		uint16 dnaSize,
		uint32 startEnergy, 
		uint16 organismID,
		bool singleStep,
		const std::string & moduleInfo, 
		FILE *debugStream, 
		bool noMutate,
		CConsole *console
	);
	~Organism();
	bool alive(void);
	uint32 getEnergy(void);
	bool getDNAValue(uint16 slot, uint16 &value);
	bool setDNAValue(uint16 slot, uint16 value);
	bool execInstr(void);		// true if still alive
	void mutate(void);
	void printDisassembly(uint16 start);
	void printData(uint16 start);
	bool setXY(uint16 x, uint16 y);
	void editData(const std::string &data);
	void editRegister(const std::string &data);
	void getDisplayLines(std::vector<std::string> &lines);
	void singleStep(std::vector<std::string> &lines);
	std::string getModuleName(void)
	{
		return(m_moduleInfo);
	}
	uint16 getID(void)
	{
		return(m_organismID);
	}
	uint16 getX(void) { return(m_x); }
	uint16 getY(void)	{ return(m_y); }
	bool getOldXY(uint16 *x,uint16 *y) 
	{ 
		bool changed = m_x != m_oldX || m_y != m_oldY;

		*x = m_oldX;
		*y = m_oldY;

		m_oldX = m_x;
		m_oldY = m_y;

		return(changed); 
	}

	char getDisplayChar(void)
	{
		if (m_energy <= 0)
		{
			if (m_botName == DRONE_STRING)
				return(',');
			else
				return('.');
		}
		else
		{
			if (m_botName == DRONE_STRING)
				return('@');
			if (m_organismID < 26)
				return((char)('A'+m_organismID));
			else
				return((char)('a'+m_organismID-26));
		}
	}

private:

	void travel(void);
	void eat(void);
	void sense(void);		// sense food on current square; id put in operand1
	bool newXY(uint16 dir, uint16 *newX, uint16 *newY);
	uint32 oppositeDir(uint32 dir);
	void setValue(uint16 opcode, uint16 opNum, uint16 opValue, uint16 value);
	uint16 getValue(uint16 opcode, uint16 opNum, uint16 opValue);
	void mov(void);
	void add(void);
	void sub(void);
	void mult(void);
	void div(void);
	void mod(void);
	void andOp(void);
	void orOp(void);
	void xorOp(void);
	void shl(void);
	void shr(void);
	void test(void);
	void cmp(void);
	void jmp(bool &updateIP);
	void je(bool &updateIP);
	void jne(bool &updateIP);
	void jl(bool &updateIP);
	void jle(bool &updateIP);
	void jg(bool &updateIP);
	void jge(bool &updateIP);
	void js(bool &updateIP);
	void jns(bool &updateIP);
	void randNum(void);
	void internalPush(uint16 value);
	uint16 internalPop();
	void call(bool &updateIP);
	void ret(bool &updateIP);
	void nop(void);
	void getxy(void);
	void energy(void);
	void validateIP(void);
	void poke(void);
	void peek(void);
	void push(void);
	void pop(void);
	void release(void);
	void charge(void);
	void cksum(void);
	bool increaseEnergy(uint16 energyAmt);
	void debug(void);

private:
	World	*m_world;
	uint16	m_dna[MAX_DNA];
	uint16	m_regs[MAX_REGS];
	uint16	m_ip;	
	sint32	m_energy;
	uint16	m_organismID;
	uint16	m_x, m_y;
	bool	m_noMutate;
	std::string m_moduleInfo;
	std::string m_botName;
	FILE		*m_debugStream;
	bool		m_singleStep;
	CConsole	*m_console;
	uint16		m_goUntilIP;
	uint16		m_oldX;
	uint16		m_oldY;
	uint32		m_traceCount;
	DisAsm		*m_disasm;
};


#endif // #ifndef _ORGANISM_H_
