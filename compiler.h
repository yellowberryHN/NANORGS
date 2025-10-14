//----------------------------------------------------------------------------
//
// compiler.h
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

#ifndef _COMPILER_H_
#define _COMPILER_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cctype>

#include "constants.h"
#include "types.h"

class OrganismBinary
{
public:
	OrganismBinary(uint16 arr[],uint16 length, std::string moduleInfo)
	{
		for (int i=0;i<length;i++)
			m_arr[i] = arr[i];
		m_length = length;
		m_moduleInfo = moduleInfo;
	}

	void getProgram(uint16 arr[])
	{
		for (int i=0;i<m_length;i++)
			arr[i] = m_arr[i];
	}

	uint16 getProgramSize(void)
	{
		return(m_length);
	}

	std::string getModuleInfo(void)
	{
		return(m_moduleInfo);
	}

	std::string getModuleName(void)
	{
		size_t index = m_moduleInfo.find_first_of(" \t,");
		if (index == -1)
			return(m_moduleInfo);
		return(m_moduleInfo.substr(0,index));
	}


private:
	std::string m_moduleInfo;
	uint16		m_arr[MAX_DNA];
	uint16		m_length;
};

class Operand
{
public:
	
	void constructImmediate(uint16 operand)
	{
		m_addrMode = ADDR_MODE_IMMED;
		m_label = "";
		m_offset = operand;
	}

	void constructRegister(uint16 reg)
	{
		m_addrMode = ADDR_MODE_REG;
		m_label = "";
		m_reg = reg;
	}


	void constructImmediate(const std::string &label)
	{
		m_addrMode = ADDR_MODE_IMMED;
		m_label = label;				// offset
	}

	void constructIndexedDirect(uint16 reg, uint16 offset = 0)
	{
		m_addrMode = ADDR_MODE_DNA_INDEXED_DIRECT;
		m_label = "";
		m_offset = offset;
		m_reg = reg;
	}

	void constructIndexedDirect(uint16 reg, const std::string &label)
	{
		m_addrMode = ADDR_MODE_DNA_INDEXED_DIRECT;
		m_label = label;
		m_offset = 0;
		m_reg = reg;
	}

	void constructDirect(const std::string &label)
	{
		m_addrMode = ADDR_MODE_DNA_DIRECT;
		m_label = label;
	}

	void constructDirect(uint16 offset)
	{
		m_addrMode = ADDR_MODE_DNA_DIRECT;
		m_offset = offset;
		m_label = "";
	}

	bool resolve(std::map<std::string,uint16> & labelMap)
	{
		if (m_label.length())
		{
			std::map<std::string,uint16>::iterator it;
			it = labelMap.find(m_label);
			if (it != labelMap.end())
			{
				m_offset = (*it).second;
				return(true);
			}
			else
				return(false);
		}

		return(true);
	}

	bool isLValue(void) const
	{
		return(m_addrMode == ADDR_MODE_REG || m_addrMode == ADDR_MODE_DNA_DIRECT ||
			   m_addrMode == ADDR_MODE_DNA_INDEXED_DIRECT);
	}

	uint16 getAddrMode(void) const
	{
		return(m_addrMode);
	}
	uint16 getOffset(void) const
	{
		return(m_offset);
	}
	uint16 getRegister(void) const
	{
		return(m_reg);
	}

	uint16 getOperand(uint16 *highOperandBit) const
	{
		*highOperandBit = 0;
		switch (m_addrMode)
		{
			case ADDR_MODE_REG:
				return(m_reg);
			case ADDR_MODE_DNA_DIRECT:
				return(m_offset);
			case ADDR_MODE_DNA_INDEXED_DIRECT:
				*highOperandBit = (m_offset & OFFSET_TOP_BIT_MASK)?1:0;
				return((m_reg<<12)|(m_offset&OFFSET_MASK));		// offset is lower 12 bits, reg is upper 4
			case ADDR_MODE_IMMED:
				return(m_offset);
		}

		return(0);
	}

	std::string getString(void)
	{
		return(m_label);
	}

private:
	std::string	m_label;
	uint16		m_addrMode;
	uint16		m_reg, m_offset;
};

class Instr
{
public:
	void setOpcode(uint16 opcode, const std::string &opcodeString)
	{
		m_opcode = opcode;
		m_opcodeString = opcodeString;
		m_instrSize = INSTR_SLOTS;
		m_align = true;
		m_relative = false;
		switch((OPCODE)m_opcode)
		{
			// 2 operand instructions whose first operand is an l-value
			case OPCODE_MOV:	
			case OPCODE_ADD:
			case OPCODE_SUB:
			case OPCODE_MULT:
			case OPCODE_DIV:
			case OPCODE_AND:
			case OPCODE_OR:
			case OPCODE_XOR:
			case OPCODE_SHL:
			case OPCODE_SHR:
			case OPCODE_MOD:
			case OPCODE_RAND:
			case OPCODE_PEEK:
			case OPCODE_CKSUM:
				m_maxOperands = 2;
				m_lvalueMask = OP1_LVALUE;
				break;
			
			// 2 operand instructions where neither operand needs to be an l-value
			case OPCODE_CMP:
			case OPCODE_TEST:
			case OPCODE_CHARGE:
			case OPCODE_POKE:
				m_maxOperands = 2;
				m_lvalueMask = 0;
				break;

			// 2 operand instructions where both operands need to be an l-value
			case OPCODE_GETXY:
				m_maxOperands = 2;
				m_lvalueMask = OP1_LVALUE|OP2_LVALUE;
				break;

			// 1 operand instructions where operand need not be an l-value
			case OPCODE_PUSH:
			case OPCODE_TRAVEL:
			case OPCODE_RELEASE:
				m_maxOperands = 1;
				m_lvalueMask = 0;
				break;

			// 1 operand instructions where operand need not be an l-value
			case OPCODE_CALL:
			case OPCODE_JMP:
			case OPCODE_JL:
			case OPCODE_JLE:
			case OPCODE_JG:
			case OPCODE_JGE:
			case OPCODE_JE:
			case OPCODE_JNE:
			case OPCODE_JS:
			case OPCODE_JNS:
				m_maxOperands = 1;
				m_lvalueMask = 0;
				m_relative = true;
				break;

			// 1 operand instructions where first operand must be an l-value
			case OPCODE_POP:
			case OPCODE_ENERGY:
			case OPCODE_SENSE:
				m_maxOperands = 1;
				m_lvalueMask = OP1_LVALUE;
				break;

			// 0 operand instructions 
			case OPCODE_RET:
			case OPCODE_EAT:
			case OPCODE_NOP:
				m_maxOperands = 0;
				m_lvalueMask = 0;
				break;

			// N operands
			case OPCODE_DATA:
				m_maxOperands = MAX_DATA_ITEMS;
				m_lvalueMask = 0;
				m_instrSize = 0;
				m_align = false;
				break;

			// label case (an empty instruction)
			case OPCODE_LABEL:
				m_instrSize = 0;
				m_lvalueMask = 0;
				m_maxOperands = 0;
				m_align = false;
				break;
		}
	}

	void setLineNum(uint32 lineNum)
	{
		m_lineNum = lineNum;
	}

	uint32 getLineNum(void)
	{
		return(m_lineNum);
	}

	// returns # of words
	uint16 getInstrSize(void)  
	{
		return(m_instrSize);
	}

	bool requiresAlignment(void)
	{
		return(m_align);
	}

	uint16 getOpcode(void)
	{
		return((uint16)m_opcode);
	}

	bool addOperand(const Operand &op, std::string &error)
	{
		if (m_opcode == OPCODE_DATA)
		{
			if (op.isLValue() == true)
			{
				char temp[256];
				sprintf(temp,"non-constant/label in data section on line %d",m_lineNum);
				error = temp;
				return(false);
			}
			m_operands.push_back(op);
			m_instrSize++;
			return(true);
		}

		if (m_operands.size() + 1 > m_maxOperands)	
		{
			char temp[256];
			sprintf(temp,"the %s instruction only has %d operands on line %d",
				m_opcodeString.c_str(),m_maxOperands,m_lineNum);
			error = temp;
			return(false);		// too many operands
		}

		switch (m_operands.size())
		{
			case 0:	// first operand being added
				if ((m_lvalueMask & OP1_LVALUE) && !op.isLValue())
				{
					char temp[256];
					sprintf(temp,"first operand of the %s instruction on line %d must be an l-value",
						m_opcodeString.c_str(),getLineNum());
					error = temp;
					return(false);
				}
				break;
			case 1:	// second operand being added
				if ((m_lvalueMask & OP2_LVALUE) && !op.isLValue())
				{
					char temp[256];
					sprintf(temp,"second operand of the %s instruction on line %d must be an l-value",
						m_opcodeString.c_str(),getLineNum());
					error = temp;
					return(false);
				}
				break;
			default:
				return(false);
		}
		m_operands.push_back(op);
		return(true);
	}

	bool resolve(std::map<std::string,uint16> & labelMap, std::string &error)
	{
		for (unsigned int  i=0;i<m_operands.size();i++)
		{
			if (m_operands[i].resolve(labelMap) == false)
			{
				char temp[256];
				sprintf(temp,"invalid label %s found in instruction on line %d",
						m_operands[i].getString().c_str(),
						getLineNum());
				error = temp;
				return(false);
			}
		}
		return(true);
	}
	void serialize(uint16 arr[], uint16 offset)
	{
		if (m_instrSize == 0)	// nothing to do!
			return;

		arr += offset;			// write data here

		if (m_opcode == OPCODE_DATA)
		{
			uint16 temp;

			for (unsigned int  i=0;i<m_operands.size();i++)
				arr[i] = m_operands[i].getOperand(&temp);
		}
		else
		{
			arr[0] = m_opcode;
			arr[1] = 0;				// reset
			arr[2] = 0;				// reset
			for (unsigned int  i=0;i<m_operands.size();i++)
			{
				uint16 highBit = 0;	

				arr[0] |= (m_operands[i].getAddrMode() << (14-i*2));
				arr[i+1] = m_operands[i].getOperand(&highBit);
				arr[0] |= (highBit << (11-i));	// or in high bit for reg+indexed addressing modes
				// jmp label is implemented as a relative jump...
				if (m_relative == true && m_operands[i].getAddrMode() == ADDR_MODE_IMMED)
					arr[i+1] = arr[i+1] - offset;
			}
		}
	}

	uint32 getNumOperands(void)
	{
		return(m_maxOperands);
	}

	uint32 getLength(void)
	{
		if (m_opcode == OPCODE_DATA)
			return(m_operands.size());
		else
			return(INSTR_SLOTS);
	}

	std::string getString(void)
	{
		return(m_opcodeString);
	}

private:
	uint32						m_lineNum;
	uint16						m_opcode;
	uint16						m_instrSize;		// in words
	std::vector<Operand>		m_operands;
	uint32						m_maxOperands;
	uint32						m_lvalueMask;
	std::string					m_opcodeString;
	bool						m_align;
	bool						m_relative;
};

class Token
{
public:

	Token()
	{
		m_tokenType = TOKEN_INVALID;
	}

	bool setToken(const std::string &token,uint32 lineNum)
	{
		char temp[256];

		m_tokenType = TOKEN_INVALID;

		m_lineNum = lineNum;

		strcpy(temp,token.c_str());
		strupr(temp);
		m_token = temp;
		if (isalpha(m_token[0]) || m_token[0] == '_')
		{
			if (m_token[0] == 'R')
			{
				unsigned int i;
				for (i=1;i<m_token.length();i++)
					if (!isdigit(m_token[i]))
						break;
				if (i == m_token.length())
				{
					// register: r##
					unsigned int regNum;
					sscanf(m_token.c_str()+1,"%d",&regNum);
					if (regNum > MAX_USER_REGS)
						return(false);
					m_regNum = (uint16)regNum;
					m_tokenType = TOKEN_REGISTER;
					return(true);
				}
			}

			if (m_token == "SP")
			{
				m_tokenType = TOKEN_REGISTER;
				m_regNum = SP_REG;
				return(true);
			}

			if (m_token == "flags")
			{
				m_tokenType = TOKEN_REGISTER;
				m_regNum = FLAGS_REG;
				return(true);
			}


			// either a label or an opcode

			if (isOpcode())
				m_tokenType = TOKEN_OPCODE;
			else
				m_tokenType = TOKEN_LABEL;
			return(true);
		}
		else if (isdigit(m_token[0]))
		{
			if (m_token[0] == '0' && m_token[1] == 'X')
			{
				unsigned int i;
				for (i=2;i<m_token.length();i++)
					if (!isdigit(m_token[i]) && (m_token[i] < 'A' || m_token[i] > 'F'))
						break;
				if (i != m_token.length())			// invalid token!
					return(false);

				unsigned int val;

				sscanf(m_token.c_str()+2,"%x",&val);
				if (val > MAX_WORD_VALUE)
					return(false);
				m_immed = (uint16) val;
			}
			else
			{
				unsigned int i;
				for (i=0;i<m_token.length();i++)
					if (!isdigit(m_token[i]))
						break;
				if (i != m_token.length())			// invalid token!
					return(false);

				unsigned int val;

				sscanf(m_token.c_str(),"%d",&val);
				if (val > MAX_WORD_VALUE)
					return(false);

				m_immed = (uint16)val;
			}
			m_tokenType = TOKEN_IMMED;
		}
		else if (m_token == EOF_TOKEN_STRING)
		{
			m_tokenType = TOKEN_EOF;
			return(true);
		}
		else if (m_token.size() == 1)
		{
			// single-character tokens use their own ascii value as their token type
			m_tokenType = m_token[0];
			return(true);
		}
		else
		{
			// error
			return(false);
		}

		return(true);
	}

	uint32 getLineNum(void)
	{
		return(m_lineNum);
	}

	uint16 getTokenType(void)
	{
		return(m_tokenType);
	}

	uint16 getRegisterNum(void)
	{
		return(m_regNum);
	}

	uint16 getImmediateValue(void)
	{
		return(m_immed);
	}

	std::string getString(void)
	{
		return(m_token);
	}

	uint16 getOpcode(void)
	{
		return((uint16)m_opcode);
	}


private:

	bool isOpcode()
	{
		if (m_token == "MOV")
		{
			m_opcode = OPCODE_MOV;
			return(true);
		}

		if (m_token == "PUSH")
		{
			m_opcode = OPCODE_PUSH;
			return(true);
		}

		if (m_token == "POP")
		{
			m_opcode = OPCODE_POP;
			return(true);
		}

		if (m_token == "CALL")
		{
			m_opcode = OPCODE_CALL;
			return(true);
		}

		if (m_token == "RET")
		{
			m_opcode = OPCODE_RET;
			return(true);
		}

		if (m_token == "JMP")
		{
			m_opcode = OPCODE_JMP;
			return(true);
		}

		if (m_token == "JL")
		{
			m_opcode = OPCODE_JL;
			return(true);
		}

		if (m_token == "JLE")
		{
			m_opcode = OPCODE_JLE;
			return(true);
		}

		if (m_token == "JG")
		{
			m_opcode = OPCODE_JG;
			return(true);
		}

		if (m_token == "JGE")
		{
			m_opcode = OPCODE_JGE;
			return(true);
		}

		if (m_token == "JE")
		{
			m_opcode = OPCODE_JE;
			return(true);
		}

		if (m_token == "JNE")
		{
			m_opcode = OPCODE_JNE;
			return(true);
		}

		if (m_token == "JS")
		{
			m_opcode = OPCODE_JS;
			return(true);
		}

		if (m_token == "JNS")
		{
			m_opcode = OPCODE_JNS;
			return(true);
		}

		if (m_token == "ADD")
		{
			m_opcode = OPCODE_ADD;
			return(true);
		}

		if (m_token == "SUB")
		{
			m_opcode = OPCODE_SUB;
			return(true);
		}

		if (m_token == "MULT")
		{
			m_opcode = OPCODE_MULT;
			return(true);
		}

		if (m_token == "DIV")
		{
			m_opcode = OPCODE_DIV;
			return(true);
		}

		if (m_token == "AND")
		{
			m_opcode = OPCODE_AND;
			return(true);
		}

		if (m_token == "OR")
		{
			m_opcode = OPCODE_OR;
			return(true);
		}

		if (m_token == "XOR")
		{
			m_opcode = OPCODE_XOR;
			return(true);
		}

		if (m_token == "CMP")
		{
			m_opcode = OPCODE_CMP;
			return(true);
		}

		if (m_token == "TEST")
		{
			m_opcode = OPCODE_TEST;
			return(true);
		}

		if (m_token == "GETXY")
		{
			m_opcode = OPCODE_GETXY;
			return(true);
		}

		if (m_token == "ENERGY")
		{
			m_opcode = OPCODE_ENERGY;
			return(true);
		}

		if (m_token == "TRAVEL")
		{
			m_opcode = OPCODE_TRAVEL;
			return(true);
		}

		if (m_token == "SHL")
		{
			m_opcode = OPCODE_SHL;
			return(true);
		}

		if (m_token == "SHR")
		{
			m_opcode = OPCODE_SHR;
			return(true);
		}

		if (m_token == "SENSE")
		{
			m_opcode = OPCODE_SENSE;
			return(true);
		}

		if (m_token == "EAT")
		{
			m_opcode = OPCODE_EAT;
			return(true);
		}

		if (m_token == "NOP")
		{
			m_opcode = OPCODE_NOP;
			return(true);
		}

		if (m_token == "DATA")
		{
			m_opcode = OPCODE_DATA;
			return(true);
		}

		if (m_token == "MOD")
		{
			m_opcode = OPCODE_MOD;
			return(true);
		}

		if (m_token == "RAND")
		{
			m_opcode = OPCODE_RAND;
			return(true);
		}

		if (m_token == "RELEASE")
		{
			m_opcode = OPCODE_RELEASE;
			return(true);
		}

		if (m_token == "CHARGE")
		{
			m_opcode = OPCODE_CHARGE;
			return(true);
		}

		if (m_token == "PEEK")
		{
			m_opcode = OPCODE_PEEK;
			return(true);
		}

		if (m_token == "POKE")
		{
			m_opcode = OPCODE_POKE;
			return(true);
		}

		if (m_token == "CKSUM")
		{
			m_opcode = OPCODE_CKSUM;
			return(true);
		}

		return(false);
	}

private:
	std::string m_token;
	uint16		m_tokenType;
	uint16		m_regNum;
	uint16		m_immed;
	OPCODE		m_opcode;
	uint32		m_lineNum;
};

class Compiler
{
public:
	Compiler()
	{
		m_success = false;
	}

	bool compile(const std::string &fileName, std::string &error);

	OrganismBinary *getProgram(void);

	void reset(void)
	{
		m_success = false;
		m_tokens.clear();
		m_labelTable.clear();
		m_instrs.clear();
		m_totalProgramSize = 0;
		m_moduleInfo = "";
		m_labelsSoFar.clear();
	}

private:

	bool enoughTokens(int numRequired)
	{
		return (m_curToken + numRequired - 1 < m_tokens.size());
	}

	bool tokenize(FILE *stream, std::string &error);
	Operand *register_offset(std::string &error);
	Operand *index(std::string &error);
	Operand *memory(std::string &error);
	Operand *immediate(std::string &error);
	Operand *operand(std::string &error);
	bool label(std::string &error);
	bool two_operands(std::string & error);
	bool one_operands(std::string & error);
	bool no_operands(std::string & error);
	bool instruction(std::string &error);
	bool statement(std::string &error);
	bool statement_list(std::string &error);
	bool parse(std::string &error);
	bool eof(std::string &error);
	bool resolveLabels(std::string &error);
	bool processComma(std::string &error);
	bool buildLabelTable(void);
	bool data(std::string &error);
	//bool moduleInfo(void);
	
private:

	bool								m_success;
	std::vector<Token>					m_tokens;
	std::map<std::string,uint16>		m_labelTable;
	std::vector<Instr>					m_instrs;
	uint32								m_lineNum;
	uint32								m_curToken;
	uint16								m_totalProgramSize;
	std::string							m_moduleInfo;
	std::set<std::string>				m_labelsSoFar;
};

#endif // #ifndef _COMPILER_H_
