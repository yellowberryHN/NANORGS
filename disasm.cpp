//----------------------------------------------------------------------------
//
// disasm.cpp
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

#ifdef WIN32
#pragma warning(disable:4786)
#endif // #ifdef WIN32

#include <stdio.h>
#include <string>
#include <cstring>

#include "compiler.h"
#include "constants.h"
#include "types.h"
#include "disasm.h"

using namespace std;

std::string DisAsm::getRegString(int regNum)
{
	if (regNum == SP_REG)
		return("sp");
	else if (regNum == FLAGS_REG)
		return("flags");

	char temp[256];
	sprintf(temp,"r%d",regNum);
	return(temp);
}

std::string DisAsm::getOperandString(int instrLocation, int opNum, std::string &value, bool relative)
{
	uint16 opcode = m_dna[instrLocation];
	uint16 opValue = m_dna[instrLocation+opNum+1];
	char temp[256], temp2[256] = {0};

	switch ((opcode >> (14-opNum*2)) & 0x3)
	{
		case ADDR_MODE_REG:
			if (opValue < MAX_REGS)
			{
				strcpy(temp, getRegString(opValue).c_str());
				if (m_regs != NULL)
					sprintf(temp2,"%s = %d",temp,m_regs[opValue]);
				value = temp2;
				return(temp);
			}
			else
			{
				strcpy(temp, getRegString(opValue).c_str());
				sprintf(temp2,"%s = <invalid>",temp);
				value = temp2;
				return(temp);
			}
			break;
		case ADDR_MODE_DNA_DIRECT:
			if (opValue < MAX_DNA)
			{
				sprintf(temp,"[%d]",opValue);
				sprintf(temp2,"[%d] = %d",opValue,m_dna[opValue]);
				value = temp2;
				return(temp);
			}
			else
			{
				sprintf(temp,"[%d]",opValue);
				sprintf(temp2,"[%d] = <invalid>",opValue);
				value = temp2;
				return(temp);
			}
			break;

		case ADDR_MODE_IMMED:					
			if (relative)
				sprintf(temp,"%d",(uint16)(instrLocation+opValue));
			else
				sprintf(temp,"%d",opValue);
			value = "";
			return(temp);
		case ADDR_MODE_DNA_INDEXED_DIRECT:			
			{
				uint16 reg = opValue >> 12;				// reg is upper 4 bits of word
				uint16 off = (opValue & OFFSET_MASK);	// offset if lower 12 bits of word
				uint16 highBit = (opcode & (1 << (11-opNum)))?OFFSET_TOP_BIT_MASK:0;
				off |= highBit;
				if (off & OFFSET_TOP_BIT_MASK)	// negative!
					off |= 0xF000;				// extend sign bits
				uint16 dnaOffset = 0;

				if (m_regs != NULL)
					dnaOffset = (uint16)(m_regs[reg]+(sint16)off);


				if (off & 0x8000)
				{
					sprintf(temp,"[%s%d]",getRegString(reg).c_str(),(sint16)off);
					if (m_regs != NULL)
					{
						if (dnaOffset < MAX_DNA)
							sprintf(temp2,"[%d] = %d",dnaOffset,m_dna[dnaOffset]);
						else
							sprintf(temp2,"[%d] = <invalid>",dnaOffset);
					}
					value = temp2;
				}
				else if (off != 0)
				{
					sprintf(temp,"[%s+%d]",getRegString(reg).c_str(),(sint16)off);
					if (m_regs != NULL)
					{
						if (dnaOffset < MAX_DNA)
							sprintf(temp2,"[%d] = %d",dnaOffset,m_dna[dnaOffset]);
						else
							sprintf(temp2,"[%d] = <invalid>",dnaOffset);
					}
					value = temp2;
				}
				else
				{
					sprintf(temp, "[%s]",getRegString(reg).c_str());
					if (m_regs != NULL)
					{
						if (dnaOffset < MAX_DNA)
							sprintf(temp2,"[%d] = %d",dnaOffset,m_dna[dnaOffset]);
						else
							sprintf(temp2,"[%d] = <invalid>",dnaOffset);
					}
					value = temp2;
				}
			}
			return(temp);
		default:
			value = "<invalid>";
			return("<invalid>");
	}

}

std::string DisAsm::getCurrentInstructionString(int location, bool appendValues)
{
	OPCODE oc = (OPCODE)(m_dna[location] & OPCODE_MASK);
	string instr;
	int numOperands = 2;		// default
	bool invalid = false;
	bool relative = false;

	if (location > MAX_DNA-INSTR_SLOTS)
		return("<ip out of range>");

	switch (oc)
	{
		case OPCODE_MOV:	
			instr = "mov";
			break;
		case OPCODE_PUSH:
			instr = "push";
			numOperands = 1;
			break;
		case OPCODE_POP:
			instr = "pop";
			numOperands = 1;
			break;
		case OPCODE_CALL:
			instr = "call";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_RET:
			instr = "ret";
			numOperands = 0;
			break;
		case OPCODE_JMP:
			instr = "jmp";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JL:
			instr = "jl";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JLE:
			instr = "jle";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JG:
			instr = "jg";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JGE:
			instr = "jge";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JE:
			instr = "je";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JNE:
			instr = "jne";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JS:
			instr = "js";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_JNS:
			instr = "jns";
			numOperands = 1;
			relative = true;
			break;
		case OPCODE_ADD:
			instr = "add";
			break;
		case OPCODE_SUB:
			instr = "sub";
			break;
		case OPCODE_MULT:
			instr = "mult";
			break;
		case OPCODE_DIV:
			instr = "div";
			break;
		case OPCODE_AND:
			instr = "and";
			break;
		case OPCODE_OR:
			instr = "or";
			break;
		case OPCODE_XOR:
			instr = "xor";
			break;
		case OPCODE_CMP:
			instr = "cmp";
			break;
		case OPCODE_TEST:
			instr = "test";
			break;
		case OPCODE_GETXY:
			instr = "getxy";
			break;
		case OPCODE_ENERGY:
			instr = "energy";
			numOperands = 1;
			break;
		case OPCODE_TRAVEL:
			instr = "travel";
			numOperands = 1;
			break;
		case OPCODE_SHL:
			instr = "shl";
			break;
		case OPCODE_SHR:
			instr = "shr";
			break;
		case OPCODE_SENSE:
			instr = "sense";
			numOperands = 1;		// sense food!
			break;
		case OPCODE_EAT:
			instr = "eat";
			numOperands = 0;
			break;
		case OPCODE_MOD:
			instr = "mod";
			break;
		case OPCODE_RAND:
			instr = "rand";
			break;
		case OPCODE_CHARGE:
			instr = "charge";
			break;
		case OPCODE_RELEASE:
			instr = "release";
			numOperands = 1;
			break;
		case OPCODE_PEEK:
			instr = "peek";
			break;
		case OPCODE_POKE:
			instr = "poke";
			break;
		case OPCODE_CKSUM:
			instr = "cksum";
			break;
		case OPCODE_NOP:
			instr = "nop";
			numOperands = 0;
			break;

		default:
			// treat as a data
			instr = "data";
			invalid = true;
			break;
	}

	string values;

	if (invalid)
	{
		char temp[256];

		instr += " { ";
		for (int i=0;i<3;i++)
		{
			sprintf(temp,"%d ",m_dna[location+i]);
			instr += temp;
		}
		instr += "}";
	}
	else
	{
		for (int i=0;i<numOperands;i++)
		{
			string value;

			instr = instr + " " + getOperandString(location, i, value,relative);
			if (i < numOperands-1)
				instr += ",";

			if (appendValues && value.size())
			{
				values += value;
				if (i < numOperands - 1)
					values += ", ";
			}
		}

		uint32 len = values.size();
		if (len >= 2)
		{
			if (values.substr(len-2) == ", ")
				values = values.substr(0,len-2);
		}
	}

	if (values.size())
	{
		while (instr.size() < MAX_INSTR_STRING_WIDTH)
			instr += " ";
		instr = instr + "// (" + values + ")";
	}

	return(instr);
}


void DisAsm::getDisassembly(std::vector<std::string> &disasm, uint16 start, uint16 end)
{
	while (start % INSTR_SLOTS != 0)
		start++; 

	disasm.clear();

	for (uint16 i=start; i < MAX_DNA && i < end;i+=INSTR_SLOTS)
	{
		char temp[256], temp2[256];
		sprintf(temp,"%04d  %s  ",i,getCurrentInstructionString(i,false).c_str());
		while (strlen(temp) < MAX_INSTR_STRING_WIDTH)
			strcat(temp," ");
		
		sprintf(temp2,"%s (%04X %04X %04X)",temp,m_dna[i],m_dna[i+1],m_dna[i+2]);
		disasm.push_back(temp2);
	}
}


