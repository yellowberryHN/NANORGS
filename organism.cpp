//----------------------------------------------------------------------------
//
// organism.cpp
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

#include "world.h"
#include "organism.h"
#include "disasm.h"
#include "mycon.h"

#include <string>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cctype>

using namespace std;

Organism::Organism
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
)
{
	m_ip = START_IP;
	m_world = world;
	m_energy = startEnergy;
	m_noMutate = noMutate;
	m_moduleInfo = moduleInfo;
	m_organismID = organismID;
	m_oldX = m_x = INVALID_COORD;
	m_oldY = m_y = INVALID_COORD;
	m_singleStep = singleStep;
	m_goUntilIP = INVALID_IP;
	m_traceCount = 0;

	m_console = console;

	// bot name

	int spaceOff = m_moduleInfo.find_first_of(" ,\t");
	if (spaceOff != -1)
		m_botName = m_moduleInfo.substr(0,spaceOff);
	else
		m_botName = m_moduleInfo;


	uint32 i;
	// set DNA
	for (i=0;i<dnaSize && i < MAX_DNA;i++)
		m_dna[i] = myDNA[i];
	// clear out remainder of DNA
	for (i=dnaSize;i < MAX_DNA;i++)
		m_dna[i] = 0;
	for (i=0;i<MAX_REGS;i++)
		m_regs[i] = 0;
	m_regs[SP_REG] = MAX_DNA;		// predecrement then internalPush

	m_disasm = new DisAsm(m_dna,m_regs);

	// open debug file

	m_debugStream = debugStream;
}

Organism::~Organism()
{
	if (m_debugStream != NULL)
		fclose(m_debugStream);

	if (m_disasm != NULL)
		delete m_disasm;
}


bool Organism::alive(void)
{
	return(m_energy > 0);
}

void Organism::travel(void)
{
	// op1 = direction, op2 = result register

	uint16 newX, newY;
	uint16 dir = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);

	if (newXY(dir,&newX,&newY) == false)
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
	else if (m_world->occupied(newX,newY) != NULL)
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
	else
	{
		m_oldX = m_x;
		m_oldY = m_y;
		m_x = newX;
		m_y = newY;
		m_regs[FLAGS_REG] |= FLAG_SUCCESS;
		m_energy -= TRAVEL_ENERGY;
	}
}


void Organism::eat(void)
{
	if (m_energy + FOOD_ENERGY > MAX_ORGANISM_ENERGY)
	{
		// too full!
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	if (m_world->eatFood(this,m_x,m_y) == true)
	{
		m_regs[FLAGS_REG] |= FLAG_SUCCESS;
		m_energy += FOOD_ENERGY;
	}
	else
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
}

void Organism::sense(void)		// sense food on current square; id put in operand1
{
	uint16 result = m_world->getFoodID(m_x,m_y);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],result);
	if (result != 0)
		m_regs[FLAGS_REG] |= FLAG_SUCCESS;
	else
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
}

uint32 Organism::getEnergy(void)
{
	return(m_energy);
}

bool Organism::getDNAValue(uint16 slot, uint16 & value)
{
	if (slot >= MAX_DNA)
		return(false);
	value = m_dna[slot];
	return(true);
}

bool Organism::setDNAValue(uint16 slot, uint16 value)
{
	if (slot >= MAX_DNA)
		return (false);
	m_dna[slot] = value;
	return(true);
}



bool Organism::newXY(uint16 dir, uint16 *newX, uint16 *newY)
{
	*newX = m_x;
	*newY = m_y;

	switch (dir % 4)
	{
		case DIR_NORTH:
			(*newY)--;
			break;
		case DIR_SOUTH:
			(*newY)++;
			break;
		case DIR_EAST:
			(*newX)++;
			break;
		case DIR_WEST:
			(*newX)--;
			break;
		default:
			return(false);
	}

	return(*newX < GRID_WIDTH && *newY < GRID_HEIGHT);
}

uint32 Organism::oppositeDir(uint32 dir)
{
	return(3-(dir%4));
}

// operand #0: bit 14-15: 00=register, 01=memory, 10=constant
// operand #1: bit 12-13: 00=register, 01=memory, 10=constant
// operand #2: bit 10-11: 00=register, 01=memory, 10=constant

uint16 Organism::getValue(uint16 opcode, uint16 opNum, uint16 opValue)
{
	switch ((opcode >> (14-opNum*2)) & 0x3)
	{
		case ADDR_MODE_REG:
			if (opValue < MAX_REGS)
				return(m_regs[opValue]);		// mov reg, *reg*
			else
				return(0);
		case ADDR_MODE_DNA_DIRECT:
			if (opValue < MAX_DNA)
				return(m_dna[opValue]);			// mov reg, *mem[const]*
			else
				return(0);
		case ADDR_MODE_IMMED:					
			return(opValue);					// mov reg, *const*
		case ADDR_MODE_DNA_INDEXED_DIRECT:			
			{
				uint16 reg = m_regs[opValue >> 12];		// reg is upper 4 bits of word; constrained to 0-15
				uint16 off = (opValue & OFFSET_MASK);	// offset if lower 12 bits of word
				uint16 highBit = (opcode & (1 << (11-opNum)))?OFFSET_TOP_BIT_MASK:0;
				off |= highBit;
				if (off & OFFSET_TOP_BIT_MASK)	// negative!
					off |= 0xF000;				// extend sign bits

				uint16 dnaOffset = (uint16)(reg + (sint16)off);

				if (dnaOffset < MAX_DNA)
					return(m_dna[dnaOffset]);	// mov reg, *m[reg+10]* 
				else
					return(0);
			}
		default:
			return(0);
	}
}

void Organism::setValue(uint16 opcode, uint16 opNum, uint16 opValue, uint16 value)
{
	switch ((opcode >> (14-opNum*2)) & 0x3)
	{
		case ADDR_MODE_REG:
			if (opValue < MAX_REGS)
				m_regs[opValue] = value;
			break;
		case ADDR_MODE_DNA_DIRECT:
			if (opValue < MAX_DNA)
				m_dna[opValue] = value;
			break;
		case ADDR_MODE_IMMED:
			// nop
			break;
		case ADDR_MODE_DNA_INDEXED_DIRECT:
			{
				uint16 reg = m_regs[opValue >> 12];		// reg is upper 4 bits of word
				uint16 off = (opValue & OFFSET_MASK);	// offset if lower 12 bits of word
				uint16 highBit = (opcode & (1 << (11-opNum)))?OFFSET_TOP_BIT_MASK:0;
				off |= highBit;
				if (off & OFFSET_TOP_BIT_MASK)	// negative!
					off |= 0xF000;				// extend sign bits

				uint16 dnaOffset = (uint16)(reg + (sint16)off);

				if (dnaOffset < MAX_DNA)
					m_dna[dnaOffset] = value;		// mov *m[reg+10]*, immed
			}
	}
}

bool Organism::execInstr(void)
{
	if (m_energy <= 0)
		return(false);

	validateIP();

	debug();

	bool updateIP = true;
	OPCODE oc = (OPCODE)(m_dna[m_ip] & OPCODE_MASK);


	switch (oc)
	{
		case OPCODE_MOV:	
			mov();
			break;
		case OPCODE_PUSH:
			push();
			break;
		case OPCODE_POP:
			pop();
			break;
		case OPCODE_CALL:
			call(updateIP);
			break;
		case OPCODE_RET:
			ret(updateIP);
			break;
		case OPCODE_JMP:
			jmp(updateIP);
			break;
		case OPCODE_JL:
			jl(updateIP);
			break;
		case OPCODE_JLE:
			jle(updateIP);
			break;
		case OPCODE_JG:
			jg(updateIP);
			break;
		case OPCODE_JGE:
			jge(updateIP);
			break;
		case OPCODE_JE:
			je(updateIP);
			break;
		case OPCODE_JNE:
			jne(updateIP);
			break;
		case OPCODE_JS:
			js(updateIP);
			break;
		case OPCODE_JNS:
			jns(updateIP);
			break;
		case OPCODE_ADD:
			add();
			break;
		case OPCODE_SUB:
			sub();
			break;
		case OPCODE_MULT:
			mult();
			break;
		case OPCODE_DIV:
			div();
			break;
		case OPCODE_MOD:
			mod();
			break;
		case OPCODE_AND:
			andOp();
			break;
		case OPCODE_OR:
			orOp();
			break;
		case OPCODE_XOR:
			xorOp();
			break;
		case OPCODE_CMP:
			cmp();
			break;
		case OPCODE_TEST:
			test();
			break;
		case OPCODE_GETXY:
			getxy();
			break;
		case OPCODE_ENERGY:
			energy();
			break;
		case OPCODE_TRAVEL:
			travel();
			break;
		case OPCODE_SHL:
			shl();
			break;
		case OPCODE_SHR:
			shr();
			break;
		case OPCODE_SENSE:
			sense();
			break;
		case OPCODE_EAT:
			eat();
			break;
		case OPCODE_RAND:
			randNum();
			break;
		case OPCODE_PEEK:
			peek();
			break;
		case OPCODE_POKE:
			poke();
			break;
		case OPCODE_RELEASE:
			release();
			break;
		case OPCODE_CHARGE:
			charge();
			break;
		case OPCODE_CKSUM:
			cksum();
			break;
		default:
			// treat as a nop
			break;
	}

	m_energy -= COMPUTE_ENERGY;

	// update the IP
	if (updateIP == true)
		m_ip += INSTR_SLOTS;

	return(true);
}

void Organism::mov(void)
{
	// mov dest, src
	setValue(m_dna[m_ip],0,m_dna[m_ip+1],getValue(m_dna[m_ip],1,m_dna[m_ip+2]));
}

void Organism::add(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1+operand2);
}

void Organism::sub(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1-operand2);
}

void Organism::mult(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1*operand2);
}

void Organism::div(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand2 == 0)
		return;

	uint32 result = (uint32)operand1/(uint32)operand2;

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],(uint16)result);
}

void Organism::mod(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand2 == 0)
		return;

	uint16 result = (uint16)((uint32)operand1%(uint32)operand2);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],result);
}

void Organism::andOp(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1&operand2);
}

void Organism::orOp(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1|operand2);
}

void Organism::xorOp(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1^operand2);
}

void Organism::shl(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand2 > 16)
		operand2 = 16;
	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1 << operand2);
}

void Organism::shr(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand2 > 16)
		operand2 = 16;
	setValue(m_dna[m_ip],0,m_dna[m_ip+1],operand1 >> operand2);
}

// only test and cmp actually set flags
void Organism::test(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if ((operand1 & operand2) == 0)
		m_regs[FLAGS_REG] = FLAG_EQUAL;
	else
		m_regs[FLAGS_REG] = 0;
}

void Organism::cmp(void)
{
	uint16 operand1, operand2;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	m_regs[FLAGS_REG] = 0;

	if (operand1 < operand2)
		m_regs[FLAGS_REG] |= FLAG_LESS;
	else if (operand1 > operand2)
		m_regs[FLAGS_REG] |= FLAG_GREATER;
	else if (operand1 == operand2)
		m_regs[FLAGS_REG] |= FLAG_EQUAL;
}

void Organism::jmp(bool &updateIP)
{
	uint16 delta = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	if (m_dna[m_ip] >> 14 == ADDR_MODE_IMMED)
		m_ip = m_ip + delta;
	else
		m_ip = delta;
	updateIP = false;
}

void Organism::jl(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & FLAG_LESS)
		jmp(updateIP);
	// else do nothing 
}

void Organism::jle(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & (FLAG_LESS|FLAG_EQUAL))
		jmp(updateIP);
	// else do nothing 
}

void Organism::jg(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & FLAG_GREATER)
		jmp(updateIP);
	// else do nothing 
}

void Organism::jge(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & (FLAG_GREATER|FLAG_EQUAL))
		jmp(updateIP);
	// else do nothing 
}

void Organism::je(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & FLAG_EQUAL)
		jmp(updateIP);
	// else do nothing 
}

void Organism::jne(bool &updateIP)
{
	if ((m_regs[FLAGS_REG] & FLAG_EQUAL) == 0)
		jmp(updateIP);
	// else do nothing 
}

void Organism::js(bool &updateIP)
{
	if (m_regs[FLAGS_REG] & FLAG_SUCCESS)
		jmp(updateIP);
	// else do nothing 
}

void Organism::jns(bool &updateIP)
{
	if (!(m_regs[FLAGS_REG] & FLAG_SUCCESS))
		jmp(updateIP);
	// else do nothing 
}

void Organism::internalPush(uint16 value)
{
	// predecrement then internalPush

	--m_regs[SP_REG];
	if (m_regs[SP_REG] >= MAX_DNA)
	{
		m_regs[SP_REG] = MAX_DNA-1;			// error! sp was corrupted. help the poor fool
	}
	m_dna[m_regs[SP_REG]] = value;
}

uint16 Organism::internalPop()
{
	// get value then postdecrement
	uint16 val = 0;

	if (m_regs[SP_REG] < MAX_DNA)
		val = m_dna[m_regs[SP_REG]];
	else
		m_regs[SP_REG] = MAX_DNA-1;		// error! sp was corrupted. help the poor fool

	++m_regs[SP_REG];
	return(val);
}

void Organism::call(bool &updateIP)
{
	// call dest
	// validation will occur at next iteration
	internalPush(m_ip + INSTR_SLOTS);	// internalPush next IP address on the stack to return to
	uint16 delta = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	if (m_dna[m_ip] >> 14 == ADDR_MODE_IMMED)
		m_ip = m_ip + delta;
	else
		m_ip = delta;
	updateIP = false;		// don't update IP
}

void Organism::ret(bool &updateIP)
{
	m_ip = internalPop();
	updateIP = false;		// don't update IP
}

void Organism::nop(void)
{
	// don't do anything
}

void Organism::randNum(void)
{
	uint16 operand2;

	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand2 == 0)
		return;

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],(uint16)(myrand() % operand2));
}

void Organism::getxy(void)
{
	// place x,y in two specified operands
	setValue(m_dna[m_ip],0,m_dna[m_ip+1],m_x);
	setValue(m_dna[m_ip],1,m_dna[m_ip+2],m_y);
}

void Organism::energy(void)
{
	// store energy into specified operand

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],(uint16)m_energy);
}

void Organism::validateIP(void)
{
	if (m_ip % INSTR_SLOTS != 0)
	{
		m_ip /= INSTR_SLOTS;
		++m_ip;
		m_ip *= INSTR_SLOTS;
	}

	if (m_ip > MAX_DNA-INSTR_SLOTS)
		m_ip = START_IP;
}

void Organism::mutate(void)
{
	if (m_noMutate == false)
		m_dna[myrand() % MAX_DNA] ^= myrand() % 65536;
}

void Organism::poke(void)	// direction, their slot#; sets slot of other to r0 (POKE_REG)
{
	uint16 newX, newY;
	uint16 dir = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);

	if (newXY(dir,&newX,&newY) == false)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	Organism *other = m_world->occupied(newX,newY);
	if (other == NULL)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}
	else
	{
		if (other->setDNAValue(getValue(m_dna[m_ip],1,m_dna[m_ip+2]),m_regs[POKE_REG]) == true)
			m_regs[FLAGS_REG] |= FLAG_SUCCESS;
		else
			m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
	}
}

void Organism::peek(void)	// direction/result, slot#
{
	uint16 newX, newY;
	uint16 dir = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);

	if (newXY(dir,&newX,&newY) == false)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	Organism *other = m_world->occupied(newX,newY);
	if (other == NULL)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}
	else
	{
		uint16 result;
		if (other->getDNAValue(getValue(m_dna[m_ip],1,m_dna[m_ip+2]),result) == true)
		{
			setValue(m_dna[m_ip],0,m_dna[m_ip+1],result);
			m_regs[FLAGS_REG] |= FLAG_SUCCESS;
		}
		else
		{
			m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		}
	}
}

void Organism::push(void)
{
	internalPush(getValue(m_dna[m_ip],0,m_dna[m_ip+1]));
}

void Organism::pop(void)
{
	setValue(m_dna[m_ip],0,m_dna[m_ip+1],internalPop());
}

void Organism::release(void)
{
	uint16 energyToRelease = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	if (energyToRelease > m_energy || energyToRelease <= 0)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	if (m_world->generatePower(this,energyToRelease) == true)
	{
		m_regs[FLAGS_REG] |= FLAG_SUCCESS;
	}
	else
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
	}

	// releases either way!
	m_energy -= energyToRelease;
}

void Organism::cksum(void)
{
	uint16 operand1, operand2, total = 0;

	operand1 = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	operand2 = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (operand1 >= MAX_DNA || operand2 > MAX_DNA || operand1 > operand2)
		return;

	for (uint16 i=operand1;i<operand2;i++)
		total = total + m_dna[i];

	setValue(m_dna[m_ip],0,m_dna[m_ip+1],total);
}

void Organism::charge(void)
{
	uint16 newX, newY;
	uint16 dir = getValue(m_dna[m_ip],0,m_dna[m_ip+1]);
	uint16 energyAmount = getValue(m_dna[m_ip],1,m_dna[m_ip+2]);

	if (energyAmount > m_energy)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	if (newXY(dir,&newX,&newY) == false)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	Organism *other = m_world->occupied(newX,newY);
	if (other == NULL)
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
		return;
	}

	if (other->increaseEnergy(energyAmount) == true)
	{
		m_regs[FLAGS_REG] |= FLAG_SUCCESS;
		m_energy -= energyAmount;
	}
	else
	{
		m_regs[FLAGS_REG] &= ~FLAG_SUCCESS;
	}
}

bool Organism::increaseEnergy(uint16 energyAmt)
{
	uint32 newTotal = m_energy;
	newTotal += energyAmt;
	if (newTotal < MAX_ORGANISM_ENERGY)
	{
		m_energy += energyAmt;
		return(true);
	}

	return(false);
}



void Organism::getDisplayLines(vector<string> &lines)
{
	string shortName = m_botName.substr(0,5);

	lines.clear();

	char temp[256];
	char flags[16] = {0};
	if (m_regs[FLAGS_REG]&FLAG_EQUAL)
		strcat(flags,"e");
	if (m_regs[FLAGS_REG]&FLAG_LESS)
		strcat(flags,"l");
	if (m_regs[FLAGS_REG]&FLAG_GREATER)
		strcat(flags,"g");
	if (m_regs[FLAGS_REG]&FLAG_SUCCESS)
		strcat(flags,"s");

	char orgChar = (char)m_organismID;
	if (orgChar < 26)
		orgChar += 'A';
	else
		orgChar = orgChar - 26 + 'a';

	sprintf(temp,"(%s %c) x=%d, y=%d, energy=%d, IP=%d, SP=%d, flags=%s",
			shortName.c_str(),
			orgChar,
			m_x,m_y,
			m_energy,
			m_ip,
			m_regs[SP_REG],
			flags
			);
	lines.push_back(temp);

	sprintf(temp,"R00=%5d R01=%5d R02=%5d R03=%5d R04=%5d R05=%5d R06=%5d",
		m_regs[0],
		m_regs[1],
		m_regs[2],
		m_regs[3],
		m_regs[4],
		m_regs[5],
		m_regs[6]);
	lines.push_back(temp);
	sprintf(temp,"R07=%5d R08=%5d R09=%5d R10=%5d R11=%5d R12=%5d R13=%5d",
		m_regs[7],
		m_regs[8],
		m_regs[9],
		m_regs[10],
		m_regs[11],
		m_regs[12],
		m_regs[13]);
	lines.push_back(temp);

	sprintf(temp,"%s",
			m_disasm->getCurrentLine(m_ip).c_str());
	lines.push_back(temp);
}

void Organism::debug()
{
	if (m_singleStep == false && m_debugStream == NULL)
		return;

	vector<string> lines;
	getDisplayLines(lines);

	if (m_debugStream != NULL)
	{
		for (unsigned int i=0;i<lines.size();i++)
			fprintf(m_debugStream,"%s\n",lines[i].c_str());
		fprintf(m_debugStream,"\n");
	}

	if (m_singleStep == true)
		singleStep(lines);
}

void Organism::singleStep(std::vector<std::string> &lines)
{
	if (m_traceCount > 1)
	{
		--m_traceCount;
		return;
	}
	else if (m_goUntilIP != INVALID_IP)
	{
		if (m_ip != m_goUntilIP)
			return;
		else
		{
			m_goUntilIP = INVALID_IP;
			m_world->setQuiet(false);
			m_world->redrawAll();
			m_world->showDisplay();
		}
	}

	m_traceCount = 0;
	
	for(;;)
	{
		for (unsigned int i=0;i<lines.size();i++)
		{
			m_console->gotoXY(STATUS_X,STATUS_Y+i);
			m_console->printStringOverwrite(lines[i].c_str());
		}
		m_console->gotoXY(PROMPT_X,PROMPT_Y);
		string prompt = "(u)nasm,(g)o,(s)ilentGo,(d)mp,(e)dt,(r)eg,(i)p,(q)uit,##, or [Enter]: ";
		m_console->printStringOverwrite(prompt);
		m_console->gotoXY(PROMPT_X+prompt.size(),PROMPT_Y);
		string result = m_console->getString();

		if (result.length() == 0)
		{
			// user hit enter

			return;
		}
		
		switch (toupper(result[0]))
		{
			case 'U':
				if (result.length() == 1)
					printDisassembly(m_ip);
				else
					printDisassembly((uint16)atoi(result.c_str() + 1));
				m_world->redrawAll();
				break;
			case 'G':
			case 'S':
				if (result.length() == 1)
					m_goUntilIP = GO_INDEFINITELY_IP;
				else
					m_goUntilIP = (uint16)atoi(result.c_str()+1);

				if (toupper(result[0]) == 'S')
					m_world->setQuiet(true);

				// adjust to valid IP
				while (m_goUntilIP % INSTR_SLOTS)
					m_goUntilIP = (m_goUntilIP + 1) % MAX_DNA;
				return;

			case 'D':
				if (result.length() == 1)
					printData(m_ip);
				else				
					printData((uint16)atoi(result.c_str()+1));
				m_world->redrawAll();
				break;
			case 'E':
				editData(result);
				break;
			case 'I':
				m_ip = (uint16)atoi(result.c_str()+1);
				validateIP();
				break;
			case 'R':
				editRegister(result);
				break;
			case 'Q':
				m_world->terminate();
				return;
			default:
				if (isdigit(result[0]))
				{
					m_traceCount = atoi(result.c_str());
					return;
				}
				break;
		}

		getDisplayLines(lines);
	}
}

void Organism::editData(const std::string &data)
{
	unsigned int off, val;

	if (sscanf(data.c_str() + 1, "%d %d", &off, &val) && off < MAX_DNA)
		m_dna[(uint16)off] = (uint16)val;
}

void Organism::editRegister(const std::string &data)
{
	unsigned int off, val;

	if (sscanf(data.c_str() + 1, "%d %d", &off, &val) && off < MAX_REGS)
		m_regs[(uint16)off] = (uint16)val;
}

void Organism::printDisassembly(uint16 start)
{
	std::vector<std::string> v;
	m_disasm->getDisassembly(v, start, start + UNASSEMBLE_LINES * INSTR_SLOTS);

	unsigned int i;
	for (i = 0; i < v.size(); i++)
	{
		m_console->gotoXY(STATUS_X, i + START_Y);
		m_console->printStringOverwrite(v[i]);
	}
	for (; i < UNASSEMBLE_LINES; i++)
	{
		m_console->gotoXY(STATUS_X, i + START_Y);
		m_console->printStringOverwrite("");
	}
}

void Organism::printData(uint16 start)
{
	uint16 lineNum = 0;

	for (uint16 i=start; lineNum < DATA_LINES && i < MAX_DNA; i++)
	{
		char temp[256];

		m_console->gotoXY(STATUS_X,lineNum + START_Y);
		sprintf(temp,"%04d: %5d (%04X)",i,m_dna[i],m_dna[i]);
		m_console->printStringOverwrite(temp);
		lineNum++;
	}
	for (; lineNum < DATA_LINES; lineNum++) {
		m_console->gotoXY(STATUS_X, lineNum + START_Y);
		m_console->printStringOverwrite("");
	}
}


bool Organism::setXY(uint16 x, uint16 y)
{
	if (x <= GRID_WIDTH && y <= GRID_HEIGHT)
	{
		m_x = x;
		m_y = y;
		m_oldX = m_x;
		m_oldY = m_y;
		return(true);
	}
	return(false);
}
