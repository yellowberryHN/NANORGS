//----------------------------------------------------------------------------
//
// constants.h
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

#ifndef _CONSTANTS_H_

#define _CONSTANTS_H_

#define GRID_WIDTH				70
#define GRID_HEIGHT				40
#define MAX_DNA					3600
#define MAX_REGS				16
#define MAX_USER_REGS			14		// does not include SP and flags
#define FLAGS_REG				14
#define SP_REG					15
#define MAX_ORGANISM_ENERGY		65535
#define START_ENERGY			10000
#define FOOD_ENERGY				2000
#define TRAVEL_ENERGY			9		// 9 + 1 is 10 total energy for instruction
#define	COMPUTE_ENERGY			1
#define INSTR_SLOTS				3
#define OPCODE_MASK				0xFF
#define OFFSET_MASK				0x0FFF
#define OFFSET_TOP_BIT_MASK		0x1000
#define MAX_FOOD_ID				32		// at most 32 different food IDs
#define MIN_FOOD_ID				5		// at least 5 different food IDs
#define COLLECTION_POINT_ID		0xFFFF
#define MAX_COLLECTION_POINTS	10
#define DEFAULT_MAX_ITERATIONS	1000000
#define DEFAULT_MAX_ORGANISMS	50
#define DEFAULT_MAX_DRONES		20
#define DEFAULT_FOOD_DENSITY	10
#define PERCENT_POISONED_FOOD	20
#define INVALID_COORD			65535
#define INVALID_IP				65535	
#define GO_INDEFINITELY_IP		65532 // multiple of INSTR_SLOTS
#define INVALID_ID				0xFFFF
#define POKE_REG				0
#define MAX_INSTR_STRING_WIDTH	36

#define UNASSEMBLE_LINES		40
#define DATA_LINES				40

#define MAX_DATA_ITEMS			256

#define DRONE_STRING			"drone"

#define EOF_TOKEN_STRING		"<EOF>"

#define MAX_WORD_VALUE			0xFFFF

#define ADDR_MODE_DNA_DIRECT			0
#define ADDR_MODE_REG					1
#define ADDR_MODE_IMMED					2
#define ADDR_MODE_DNA_INDEXED_DIRECT	3

// flags

#define FLAG_LESS		1
#define FLAG_EQUAL		2
#define FLAG_GREATER	4
#define FLAG_SUCCESS	8

// tokens

#define TOKEN_REGISTER	256
#define TOKEN_LABEL		257
#define TOKEN_OPCODE	258
#define TOKEN_IMMED		259
#define	TOKEN_INVALID	1000
#define TOKEN_EOF		1001

// lvalue masks

#define OP1_LVALUE		1
#define OP2_LVALUE		2
#define OP3_LVALUE		4

// opcodes

enum OPCODE { 
	OPCODE_NOP = 0,
	OPCODE_MOV,	
	OPCODE_PUSH,
	OPCODE_POP,
	OPCODE_CALL,
	OPCODE_RET,
	OPCODE_JMP,
	OPCODE_JL,
	OPCODE_JLE,
	OPCODE_JG,
	OPCODE_JGE,
	OPCODE_JE,
	OPCODE_JNE,
	OPCODE_JS,
	OPCODE_JNS,
	OPCODE_ADD,
	OPCODE_SUB,
	OPCODE_MULT,
	OPCODE_DIV,
	OPCODE_MOD,
	OPCODE_AND,
	OPCODE_OR,
	OPCODE_XOR,
	OPCODE_CMP,
	OPCODE_TEST,
	OPCODE_GETXY,
	OPCODE_ENERGY,
	OPCODE_TRAVEL,
	OPCODE_SHL,
	OPCODE_SHR,
	OPCODE_SENSE,
	OPCODE_EAT,
	OPCODE_RAND,
	OPCODE_RELEASE,
	OPCODE_CHARGE,
	OPCODE_POKE,
	OPCODE_PEEK,
	OPCODE_CKSUM,

	OPCODE_DATA,
	OPCODE_LABEL
};

#define START_IP		0

#define FAIL_VALUE		0
#define SUCCESS_VALUE	1

#define DIR_NORTH		0
#define DIR_SOUTH		1
#define DIR_EAST		2
#define DIR_WEST		3

#define MAX_TOKEN_LEN	256

#endif // #ifndef _CONSTANTS_H_
