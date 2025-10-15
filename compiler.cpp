//----------------------------------------------------------------------------
//
// compiler.cpp
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
#include <ctype.h>
#include <string>
#include <cstring>
#include <map>

#include "compiler.h"
#include "constants.h"

using namespace std;

bool Compiler::compile(const std::string &fileName, std::string &error)
{
	// reset label table, etc...

	m_labelTable.clear();
	m_instrs.clear();
	m_tokens.clear();
	m_success = false;
	m_totalProgramSize = 0;

	FILE * stream = fopen(fileName.c_str(),"rt");
	if (stream == NULL)
	{
		error = "unable to open file (" + fileName + ")";
		return(false);
	}

	// retrieve information line first (without the tokenizer)

	char infoLine[256+1] = {0};
	if (fgets(infoLine,256,stream) == NULL)
	{
		error = "missing information line";
		fclose(stream);
		return(false);
	}

	m_moduleInfo = "";

	_strupr(infoLine);
	if (strncmp(infoLine,"INFO:",strlen("INFO:")) == 0)
	{
		char *ptr = infoLine + strlen("INFO:");
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
			++ptr;

		char *ptr2 = strchr(infoLine,'\n');
		if (ptr2 != NULL)
			*ptr2 = 0;

		m_moduleInfo = ptr;

	}

	if (m_moduleInfo.size() == 0)
	{
		error = "missing or invalid information line";
		fclose(stream);
		return(false);
	}

	// tokenization now starts AFTER the information line

	m_lineNum = 2;		// information line is #1
	if (tokenize(stream,error) == false)
	{
		char temp[256];
		sprintf(temp,"%d",m_lineNum);
		error = error + (string)" on line "+temp;

		fclose(stream);
		return(false);
	}

	fclose(stream);

	if (parse(error) == false)
	{
		return(false);
	}

	// figure out the offsets of all the labels
	buildLabelTable();

	// now go resolve label references in instructions
	if (resolveLabels(error) == false)
	{
		return(false);
	}

	m_success = true;

	return(true);
}

bool Compiler::resolveLabels(std::string &error)
{
	for (uint16 i=0;i<m_instrs.size();i++)
	{
		if (m_instrs[i].resolve(m_labelTable,error) == false)
			return(false);
	}

	return(true);
}


bool Compiler::tokenize(FILE *stream, std::string &error)
{
	// tokens: labels, ":", ",", instruction names, "[", "]", numbers, "+", "-", registers
	// comments followed by ; or /

	char temp[MAX_TOKEN_LEN+1] = {0};
	int cc = 0; // charCount
	Token t;

	while(!feof(stream))
	{
		if (cc == MAX_TOKEN_LEN)
		{
			error = (string)"token exceeds maximum length ("+temp+")";
			return(false);
		}

		int ch = fgetc(stream);

		if (ch == EOF)
		{
			if (cc > 0)
			{
				// did we have a token right before the EOF?  If so, process...
				temp[cc] = 0;
				if (t.setToken(temp,m_lineNum) == false)
				{
					error = (string)"invalid token ("+temp+")";
					return(false);
				}
				m_tokens.push_back(t);
			}
			break;
		}

		if (cc == 0)		// starting a new token
		{
			switch (ch)
			{
				case '\n':
				case '\r':
					m_lineNum++;
					break;
				case ':':
				case ',':
				case '[':
				case ']':
				case '+':
				case '-':
				case '{':
				case '}':
					{
						temp[0] = (char)ch;
						temp[1] = 0;
						t.setToken(temp,m_lineNum);
						m_tokens.push_back(t);
						cc = 0;
						break;
					}
				case ';':
				case '/':
					{
						while (!feof(stream) && fgetc(stream) != '\n')
							;
						m_lineNum++;
						break;
					}
				case ' ':
				case '\t':
					break;
				default:
					temp[cc++] = (char)ch;
					break;
			}
		}
		else
		{
			switch(ch)
			{
				case '\n':
				case '\r':
					{
						temp[cc] = 0;
						if (t.setToken(temp,m_lineNum) == false)
						{
							error = (string)"invalid token ("+temp+")";
							return(false);
						}
						m_tokens.push_back(t);
						cc = 0;
						m_lineNum++;
						break;
					}
				case ' ':
				case '\t':
					{
						temp[cc] = 0;
						if (t.setToken(temp,m_lineNum) == false)
						{
							error = (string)"invalid token ("+temp+(string)")";
							return(false);
						}
						m_tokens.push_back(t);
						cc = 0;
						break;
					}

				case ':':
				case ',':
				case '[':
				case ']':
				case '+':
				case '-':
				case '{':
				case '}':
					{
						ungetc(ch,stream);
						temp[cc] = 0;
						if (t.setToken(temp,m_lineNum) == false)
						{
							error = (string)"invalid token ("+temp+")";
							return(false);
						}
						m_tokens.push_back(t);
						cc = 0;
						break;
					}
				case ';':
				case '/':
					{
						while (!feof(stream) && fgetc(stream) != '\n')
							;
						m_lineNum++;
						break;
					}
				default:
					temp[cc++] = (char)ch;
					break;
			}
		}
	}

	// end of file
	t.setToken(EOF_TOKEN_STRING,0);
	m_tokens.push_back(t);

	return(true);
}

/*
program bnf:
------------

program = module_info statement_list 
module_info = info ':' .* \n
statement_list = statement | statement statement_list
statement = label | instruction | data
label = LABEL ':'
instruction = two_operands | one_operands | no_operands
no_operands = opcode
one_operand = opcode operand
two_operand = opcode operand operand
operand = register | immediate | memory
memory = '[' index ']'
index = register_offset | register | label | constant 
register_offset = label op register | register op offset
op = '+' | '-'
data = "DATA" '{' wvalues '}'
wvalues = immed | immed wvalues

*/

Operand *Compiler::register_offset(std::string &error)
{
	if (!enoughTokens(3))
		return(NULL);

	if (m_tokens[m_curToken].getTokenType() == TOKEN_REGISTER)
	{
		uint16 nextType = m_tokens[m_curToken+1].getTokenType();
		if (nextType == '+' || nextType == '-')
		{
			if (m_tokens[m_curToken+2].getTokenType() == TOKEN_IMMED)
			{
				Operand *cur = new Operand;
				if (cur == NULL)
				{
					error = "memory allocation error";
					return(NULL);
				}
				uint16 offset = m_tokens[m_curToken+2].getImmediateValue();
				if (nextType == '-')
				{
					sint16 signedOff = (sint16)offset;
					signedOff *= -1;
					offset = (uint16)signedOff;
				}
				cur->constructIndexedDirect(m_tokens[m_curToken].getRegisterNum(),offset);
				m_curToken += 3;
				return(cur);
			}
		}
	}
	else if (m_tokens[m_curToken].getTokenType() == TOKEN_LABEL)
	{
		uint16 nextType = m_tokens[m_curToken+1].getTokenType();
		if (nextType == '+')
		{
			if (m_tokens[m_curToken+2].getTokenType() == TOKEN_REGISTER)
			{
				Operand *cur = new Operand;
				if (cur == NULL)
				{
					error = "memory allocation error";
					return(NULL);
				}
				cur->constructIndexedDirect(m_tokens[m_curToken+2].getRegisterNum(),
											m_tokens[m_curToken].getString());
				m_curToken += 3;
				return(cur);
			}
		}
	}

	return(NULL);
}


Operand *Compiler::index(std::string &error)
{
	Operand *cur;

	cur = register_offset(error);
	if (cur != NULL)
		return(cur);

	if (!enoughTokens(1))
		return(NULL);

	if (m_tokens[m_curToken].getTokenType() == TOKEN_REGISTER)
	{
		uint16 regNum = m_tokens[m_curToken].getRegisterNum();
		cur = new Operand;
		if (cur == NULL)
		{
			error = "memory allocation error";
			return(NULL);
		}
		cur->constructIndexedDirect(regNum,0);		// [reg + 0]
		++m_curToken;
		return(cur);
	}
	else if (m_tokens[m_curToken].getTokenType() == TOKEN_LABEL)
	{
		cur = new Operand;
		if (cur == NULL)
		{
			error = "memory allocation error";
			return(NULL);
		}
		cur->constructDirect(m_tokens[m_curToken].getString());
		++m_curToken;
		return(cur);
	}
	else if (m_tokens[m_curToken].getTokenType() == TOKEN_IMMED)
	{
		cur = new Operand;
		if (cur == NULL)
		{
			error = "memory allocation error";
			return(NULL);
		}
		cur->constructDirect(m_tokens[m_curToken].getImmediateValue());
		++m_curToken;
		return(cur);
	}

	return(NULL);
}

Operand *Compiler::memory(std::string &error)
{
	uint32 backupToken = m_curToken;

	if (!enoughTokens(1))
		return(NULL);

	if (m_tokens[m_curToken].getTokenType() != '[')
		return(NULL);

	++m_curToken;

	Operand *cur = index(error);

	if (cur == NULL)
	{
		m_curToken = backupToken;
		return(NULL);
	}

	if (!enoughTokens(1))
		return(NULL);

	if (m_tokens[m_curToken].getTokenType() != ']')
	{
		delete cur;
		m_curToken = backupToken;
		return(NULL);
	}

	++m_curToken;

	return(cur);
}

Operand *Compiler::immediate(std::string &error)
{
	uint16 tokenType = m_tokens[m_curToken].getTokenType();
	Operand *cur;

	if (!enoughTokens(1))
		return(NULL);

	if (tokenType == TOKEN_LABEL)
	{
		cur = new Operand;
		if (cur == NULL)
		{
			error = "memory allocation error";
			return(NULL);
		}
		cur->constructImmediate(m_tokens[m_curToken].getString());
		++m_curToken;
		return(cur);
	}
	else if (tokenType == TOKEN_IMMED)
	{
		cur = new Operand;
		if (cur == NULL)
		{
			error = "memory allocation error";
			return(NULL);
		}
		cur->constructImmediate(m_tokens[m_curToken].getImmediateValue());
		++m_curToken;
		return(cur);
	}

	return(NULL);
}

Operand *Compiler::operand(std::string &error)
{
	uint32 tokenType;
	Operand *cur;

	if (!enoughTokens(1))
		return(NULL);

	tokenType = m_tokens[m_curToken].getTokenType();

	if (tokenType == TOKEN_REGISTER)
	{
		cur = new Operand;
		if (cur == NULL)
			return(NULL);
		cur->constructRegister(m_tokens[m_curToken].getRegisterNum());
		++m_curToken;
		return(cur);
	}

	// now handle a label or an immediate
	cur = immediate(error);
	if (cur != NULL)
		return(cur);

	// now handle the memory indexing case
	cur = memory(error);
	if (cur == NULL)
	{
		char temp[256];
		sprintf(temp,"invalid operand %s found on line %d",
			m_tokens[m_curToken].getString().c_str(),
			m_tokens[m_curToken].getLineNum());
		error = temp;
	}

	return(cur);
}

bool Compiler::label(std::string &error)
{
	if (!enoughTokens(2))
		return(false);

	if (m_tokens[m_curToken].getTokenType() != TOKEN_LABEL)
	{
		char temp[256];
		sprintf(temp,"expecting label definition on line %d",m_tokens[m_curToken].getLineNum());
		error = temp;
		return(false);
	}
	if (m_tokens[m_curToken+1].getTokenType() != ':')
	{
		char temp[256];
		sprintf(temp,"expecting colon after label definition on line %d",m_tokens[m_curToken+1].getLineNum());
		error = temp;
		return(false);
	}

	// check for duplicate labels!
	if (m_labelsSoFar.find(m_tokens[m_curToken].getString()) != m_labelsSoFar.end())
	{
		char temp[256];
		sprintf(temp,"found duplicate label %s on line %d",
				m_tokens[m_curToken].getString().c_str(),
				m_tokens[m_curToken+1].getLineNum());
		error = temp;
		return(false);
	}

	// add our new label to our list of used labels
	m_labelsSoFar.insert(m_tokens[m_curToken].getString());

	Instr i;
	i.setOpcode(OPCODE_LABEL,m_tokens[m_curToken].getString());
	m_instrs.push_back(i);

	m_curToken += 2;		// skip over label

	return(true);
}

bool Compiler::two_operands(std::string & error)
{
	uint32 backupToken = m_curToken;

	if (!enoughTokens(1))
		return(false);

	uint32 lineNum = m_tokens[m_curToken].getLineNum();

	if (m_tokens[m_curToken].getTokenType() != TOKEN_OPCODE)
		return(false);

	Instr	i;
	i.setOpcode(m_tokens[m_curToken].getOpcode(),m_tokens[m_curToken].getString());
	if (i.getNumOperands() != 2)
	{
		char temp[256];
		sprintf(temp,"the %s instruction on line %d has the wrong number of operands",
			m_tokens[m_curToken].getString().c_str(),m_tokens[m_curToken].getLineNum());
		error = temp;
		return(false);	
	}

	++m_curToken;

	Operand *o1, *o2;

	o1 = operand(error);
	if (o1 == NULL)
	{
		m_curToken = backupToken;
		return(false);
	}

	if (processComma(error) == false)
	{
		delete o1;
		m_curToken = backupToken;
		return(false);
	}

	o2 = operand(error);
	if (o2 == NULL)
	{
		m_curToken = backupToken;
		delete o1;
		return(false);
	}

	// found all 2!

	bool result = true;

	i.setLineNum(lineNum);
	result = result && i.addOperand(*o1,error);
	result = result && i.addOperand(*o2,error);

	m_instrs.push_back(i);

	delete o1;
	delete o2;

	return(result);
}

bool Compiler::one_operands(std::string & error)
{
	uint32 backupToken = m_curToken;

	if (!enoughTokens(1))
		return(false);

	uint32 lineNum = m_tokens[m_curToken].getLineNum();

	if (m_tokens[m_curToken].getTokenType() != TOKEN_OPCODE)
		return(false);

	Instr	i;
	i.setOpcode(m_tokens[m_curToken].getOpcode(),m_tokens[m_curToken].getString());
	if (i.getNumOperands() != 1)
	{
		char temp[256];
		sprintf(temp,"the %s instruction on line %d has the wrong number of operands",
			m_tokens[m_curToken].getString().c_str(),m_tokens[m_curToken].getLineNum());
		error = temp;
		return(false);	
	}

	++m_curToken;

	Operand *o1;

	o1 = operand(error);
	if (o1 == NULL)
	{
		m_curToken = backupToken;
		return(false);
	}

	// found all 1!
	i.setLineNum(lineNum);
	bool result = i.addOperand(*o1,error);

	m_instrs.push_back(i);

	delete o1;

	return(result);
}

bool Compiler::no_operands(std::string & error)
{
	if (!enoughTokens(1))
		return(false);

	uint32 lineNum = m_tokens[m_curToken].getLineNum();

	if (m_tokens[m_curToken].getTokenType() != TOKEN_OPCODE)
		return(false);

	Instr	i;
	i.setOpcode(m_tokens[m_curToken].getOpcode(),m_tokens[m_curToken].getString());
	if (i.getNumOperands() != 0)
	{
		char temp[256];
		sprintf(temp,"the %s instruction on line %d has the wrong number of operands",
			m_tokens[m_curToken].getString().c_str(),m_tokens[m_curToken].getLineNum());
		error = temp;
		return(false);	
	}

	++m_curToken;
	i.setLineNum(lineNum);

	m_instrs.push_back(i);

	return(true);
}


bool Compiler::instruction(std::string &error)
{
	if (two_operands(error) == false)
	{
		if (one_operands(error) == false)
		{
			if (no_operands(error) == false)
			{
				if (error == "")
				{
					char temp[256];
					sprintf(temp,"invalid instruction found on line %d",m_tokens[m_curToken].getLineNum());
					error = temp;
				}
				return(false);
			}
			else
				error = "";
		}
		else
			error = "";
	}
	else
		error = "";

	return(true);
}

bool Compiler::data(std::string &error)
{
	if (!enoughTokens(4))	// DATA { }
		return(false);

	if (m_tokens[m_curToken].getTokenType() != TOKEN_OPCODE || 
		m_tokens[m_curToken].getString() != "DATA")
		return(false);

	if (m_tokens[m_curToken+1].getTokenType() != '{')
		return(false);

	m_curToken += 2;

	Instr d;

	d.setOpcode(OPCODE_DATA,(string)"DATA");
	d.setLineNum(m_tokens[m_curToken-2].getLineNum());

	for (;;)
	{
		if (!enoughTokens(1))	// need at least one more!
			return(false);

		uint16 type = m_tokens[m_curToken].getTokenType();

		if (type == '}')
		{
			++m_curToken;
			break;
		}

		Operand *op = operand(error);
		if (op == NULL)
			return(false);

		d.addOperand(*op,error);
		delete op;
		//++m_curToken;		// operand member advances curtoken
	}

	m_instrs.push_back(d);

	return(true);
}

bool Compiler::statement(std::string &error)
{
	if (data(error) == false)
	{
		if (label(error) == false)
		{
			if (instruction(error) == false)
			{
				return(false);
			}
		}
	}

	return(true);
}

bool Compiler::eof(std::string &error)
{
	error = "";

	if (!enoughTokens(1))
		return(false);

	return(m_tokens[m_curToken].getTokenType() == TOKEN_EOF);
}

bool Compiler::statement_list(std::string &error)
{
	for (;;)
	{
		if (eof(error) == true)
			break;

		if (statement(error) == false)
		{
			return(false);
		}
	} 

	return(true);
}

bool Compiler::parse(std::string &error)
{
	m_curToken = 0;

	/*
	Module information now computed before tokenization
	if (moduleInfo() == false)
	{
		error = "invalid or missing module information";
		return(false);
	}
	*/

	return(statement_list(error));	
}

bool Compiler::processComma(std::string &error)
{
	if (!enoughTokens(1))
		return(false);

	if (m_tokens[m_curToken].getTokenType() != ',')
	{
		char temp[256];
		sprintf(temp,"missing comma on line %d",m_tokens[m_curToken].getLineNum());
		error = temp;
		return(false);
	}

	m_curToken++;
	return(true);
}
	
bool Compiler::buildLabelTable(void)
{
	uint16 curOffset = 0;
	OPCODE cur;
	unsigned int i;

	for (i=0;i<m_instrs.size();i++)
	{
		cur = (OPCODE)m_instrs[i].getOpcode();
		if (cur == OPCODE_LABEL)
		{
			unsigned int j;
			for (j=i+1;j<m_instrs.size();j++)
			{
				OPCODE next = (OPCODE)m_instrs[j].getOpcode();
				if (next != OPCODE_LABEL)
				{
					if (!m_instrs[j].requiresAlignment())
						m_labelTable[m_instrs[i].getString()] = curOffset;
					else
					{
						// regular instruction; needs to be on INSTR_SLOTS alignment
						uint16 offset = curOffset;
						if (offset % INSTR_SLOTS != 0)
							offset = ((offset/INSTR_SLOTS)+1)*INSTR_SLOTS;

						m_labelTable[m_instrs[i].getString()] = offset;
					}
					break;
				}
			}
			if (j == m_instrs.size())
			{
				// label was last item in the file
				m_labelTable[m_instrs[i].getString()] = curOffset;
			}
		}
		else if (!m_instrs[i].requiresAlignment())
			curOffset = curOffset + m_instrs[i].getInstrSize();
		else
		{
			// regular instruction
			if (curOffset % INSTR_SLOTS != 0)
				curOffset = ((curOffset/INSTR_SLOTS)+1)*INSTR_SLOTS;
			curOffset = curOffset + m_instrs[i].getInstrSize();
		}
	}

	m_totalProgramSize = curOffset;

	return(true);
}

/*
bool Compiler::moduleInfo(void)
{
	if (!enoughTokens(1))
		return(false);

	uint16 curLineNum = (uint16)m_tokens[m_curToken].getLineNum();

	if (m_tokens[m_curToken].getTokenType() != TOKEN_LABEL || 
		m_tokens[m_curToken].getString() != "INFO")
		return(false);

	if (m_tokens[m_curToken+1].getTokenType() != ':')
		return(false);

	m_curToken += 2;

	m_moduleInfo = "";

	while (enoughTokens(1) && m_tokens[m_curToken].getLineNum() ==  curLineNum)
	{
		m_moduleInfo += m_tokens[m_curToken].getString() + " ";
		++m_curToken;
	}

	if (m_moduleInfo.size())
		m_moduleInfo = m_moduleInfo.substr(0,m_moduleInfo.size()-1);

	return(m_moduleInfo.size() > 0);
}
*/

OrganismBinary *Compiler::getProgram(void)
{
	uint32 curOffset = 0;
	uint16 arr[MAX_DNA] = {0};

	for (unsigned int i=0;i<m_instrs.size();i++)
	{
		if (curOffset + m_instrs[i].getLength() > MAX_DNA)
			return(NULL);										// program was too long!

		if (!m_instrs[i].requiresAlignment())
			m_instrs[i].serialize(arr,curOffset);
		else
		{
			// regular instruction; needs to be on INSTR_SLOTS alignment
			while (curOffset % INSTR_SLOTS != 0)
				arr[curOffset++] = 0;
			m_instrs[i].serialize(arr,curOffset);
		}

		curOffset += m_instrs[i].getInstrSize();
	}

	OrganismBinary *ob = new OrganismBinary(arr,m_totalProgramSize,m_moduleInfo);
	return(ob);
}

