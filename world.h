//----------------------------------------------------------------------------
//
// world.h
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

#ifndef _WORLD_H_
#define _WORLD_H_

#include <vector>
#include <string>

#include "types.h"
#include "constants.h"
#include "settings.h"
#include "mycon.h"
#include "compiler.h"

class Organism;

struct Coord
{
	Coord(uint16 x, uint16 y)
	{
		this->x = x;
		this->y = y;
	}
	uint16 x, y;
};

class World
{
public:

	World(Settings *settings,CConsole *console);
	bool populateWorld(OrganismBinary *player,OrganismBinary *drone);
	~World();
	Organism *occupied(uint16 x, uint16 y);
	uint16 getFoodID(uint16 x, uint16 y);
	bool eatFood(Organism *me, uint16 x, uint16 y);	// returns true if food was eaten
	bool addNewOrganism(uint16 newX,uint16 newY,Organism *newOrg);
	bool generatePower(Organism *me,uint16 energyToRelease);
	void run(void);
	void getNumAlive(uint16 *orgs, uint16 *drones);
	void terminate(void);
	void redrawAll(void)
	{
		m_redrawAll = true;
	}

	double getScore(void)
	{
		return(m_score);
	}
	uint32 getTickNum(void)
	{
		return(m_curIteration);
	}

	void setQuiet(bool quiet)
	{
		m_quiet = quiet;
	}
	void showDisplay(void);

private:
	bool tick(void);

private:
	std::vector<Organism *>	m_orgs;
	uint16					m_foodGrid[GRID_HEIGHT][GRID_WIDTH];
	uint16					m_maxFoodID;
	double					m_score;
	uint32					m_maxIterations;
	bool					m_poisoned[MAX_FOOD_ID+1];
	bool					m_terminate;
	Settings				*m_settings;
	CConsole				*m_console;
	uint32					m_curIteration;
	std::vector<Coord>		m_foodCoords;
	bool					m_redrawAll;
	bool					m_quiet;
	FILE					*m_debugStream;
};


#endif // #ifndef _WORLD_H_
