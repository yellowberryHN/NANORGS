//----------------------------------------------------------------------------
//
// world.cpp
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

#include <ctime>

using namespace std;

World::World(Settings *settings, CConsole *console)
{
	m_console = console;
	m_settings = settings;
	m_maxFoodID = (uint16)(myrand() % MAX_FOOD_ID);
	if (m_maxFoodID <= MIN_FOOD_ID)
		m_maxFoodID = MIN_FOOD_ID;
	m_score = 0;	
	m_curIteration = 0;
	m_maxIterations = settings->getMaxIterations();
	m_terminate = false;
	m_redrawAll = true;
	m_quiet = settings->getQuiet();

	uint32 i,j , foodDensity = settings->getFoodDensity();

	// zero grid

	for (i=0;i<GRID_HEIGHT;i++)
		for (j=0;j<GRID_WIDTH;j++)
			m_foodGrid[i][j] = 0;		// no food or collection point
		
	// disperse collection points

	for (i=0;i<MAX_COLLECTION_POINTS;i++)
	{
		int x,y;

		do
		{
			x = myrand() % GRID_WIDTH;
			y = myrand() % GRID_HEIGHT;
		}
		while (m_foodGrid[y][x] != 0);

		m_foodGrid[y][x] = COLLECTION_POINT_ID;
	}

	// disperse food

	for (i=0;i<GRID_HEIGHT;i++)
		for (j=0;j<GRID_WIDTH;j++)
		{
			if ((uint32)(myrand() % 100) < foodDensity && m_foodGrid[i][j] == 0)
				m_foodGrid[i][j] = (uint16)((myrand() % m_maxFoodID) + 1);
		}

	// determine which food is poison

	for (i=0;i<MAX_FOOD_ID;i++)
		m_poisoned[i] = false;

	uint32 foodToPoison = (m_maxFoodID * PERCENT_POISONED_FOOD) / 100;

	while (foodToPoison > 0)
	{
		i = myrand() % m_maxFoodID;		
		if (m_poisoned[i] == false)
		{
			m_poisoned[i] = true;
			--foodToPoison;
		}
	}

	// stream

	if (m_settings->getDebug().length() > 0)
		m_debugStream = fopen(m_settings->getDebug().c_str(),"wt");
	else
		m_debugStream = NULL;
}

World::~World()
{
	for (uint32 i=0;i<m_orgs.size();i++)
		delete m_orgs[i];
}

Organism *World::occupied(uint16 x, uint16 y)
{
	for (uint32 i=0;i<m_orgs.size();i++)
		if (m_orgs[i]->getX() == x && m_orgs[i]->getY() == y)
			return(m_orgs[i]);

	return(NULL);
}

uint16 World::getFoodID(uint16 x, uint16 y)
{
	if (x >= GRID_WIDTH || y >= GRID_HEIGHT)
		return(0);
	return(m_foodGrid[y][x]);
}

bool World::eatFood(Organism *me, uint16 x, uint16 y)	// returns true if food was eaten
{
	uint16 id = getFoodID(x,y);
	if (id == 0 || id == COLLECTION_POINT_ID)
		return(false);			// no food to eat

	if (m_poisoned[m_foodGrid[y][x]])
	{
		me->mutate();
	}

	uint16 ateFoodID = m_foodGrid[y][x];

	m_foodGrid[y][x] = 0;		// remove the food

	// place new food of the same ID somewhere else

	for(;;)
	{
		x = (uint16)(myrand() % GRID_WIDTH);
		y = (uint16)(myrand() % GRID_HEIGHT);
		if (m_foodGrid[y][x] == 0)
		{
			m_foodGrid[y][x] = ateFoodID;
			break;
		}
	}

	m_foodCoords.push_back(Coord(x,y));
	
	return(true);				// ate the food
}

bool World::addNewOrganism(uint16 newX,uint16 newY,Organism *newOrg)
{
	if (newOrg == NULL)
		return(false);

	for (uint32 i=0;i<m_orgs.size();i++)
		if (m_orgs[i]->getX() == newX && m_orgs[i]->getY() == newY)
			return(false);		// space is full

	if (newOrg->setXY(newX,newY) == false)
		return(false);

	m_orgs.push_back(newOrg);
	return(true);
}


bool World::tick(void)
{
	int alive = 0;

	for (uint32 i=0;i<m_orgs.size();i++)
	{
		if (m_orgs[i]->alive())
		{
			m_orgs[i]->execInstr();
			++alive;
		}
	}

	return(alive != 0);
}

void World::run(void)
{
	m_console->clearScreen();
	showDisplay();

	for (m_curIteration=0;m_curIteration<m_maxIterations && !m_terminate;m_curIteration++)
	{
		if (tick() == false)
			break;
		showDisplay();
	}
}


bool World::generatePower(Organism *me, uint16 energyToRelease)
{
	if (m_foodGrid[me->getY()][me->getX()] == COLLECTION_POINT_ID)
	{
		m_score += (double)energyToRelease;
		return(true);
	}

	return(false);
}

void World::terminate(void)
{
	m_terminate = true;
}


bool World::populateWorld
(
	 OrganismBinary *player,
	 OrganismBinary *drone
)
{
	uint16 numOrganisms = m_settings->getMaxOrganisms();
	uint16 i,x,y;
	uint16 arr[MAX_DNA];

	player->getProgram(arr);

	for (i=0;i<numOrganisms;i++)
	{
		Organism *org  = new Organism(	this, 
										arr, 
										player->getProgramSize(),
										START_ENERGY, 
										i,
										m_settings->getSingleStep() && 
											i == m_settings->getSingleStepID(),
										player->getModuleInfo(), 
										m_debugStream, 
										false,
										m_console);
		if (org == NULL)
			return(false);

		do
		{
			x = (uint16)(myrand() % GRID_WIDTH);
			y = (uint16)(myrand() % GRID_HEIGHT);
		} while (addNewOrganism(x,y,org) == false);
	}


	uint16 numDrones = m_settings->getMaxDrones();

	drone->getProgram(arr);

	for (i=0;i<numDrones;i++)
	{
		Organism *org  = new Organism(	this, 
										arr, 
										drone->getProgramSize(),
										START_ENERGY, 
										i+numOrganisms,
										false,
										DRONE_STRING, 
										NULL, 
										true,
										m_console);
		if (org == NULL)
			return(false);

		do
		{
			x = (uint16)(myrand() % GRID_WIDTH);
			y = (uint16)(myrand() % GRID_HEIGHT);
		} while (addNewOrganism(x,y,org) == false);
	}

	return(true);
}

void World::showDisplay(void)
{
	if (m_quiet == true)
		return;

	char temp[256];

	if (m_curIteration % 100 == 0 || m_settings->getSingleStep() || m_redrawAll == true)
	{
		sprintf(temp,"Score: %.0lf, Ticks: %d of %d   (Seed=%u)",m_score,m_curIteration,m_maxIterations,m_settings->getSeed());
		m_console->gotoXY(SCORE_X,SCORE_Y);
		m_console->printString(temp);
	}

	if (m_redrawAll)
	{
		m_redrawAll = false;

		for (int i=0;i<GRID_HEIGHT;i++)
		{
			for (int j=0;j<GRID_WIDTH;j++)
			{
				m_console->gotoXY(START_X+j,START_Y+i);
				if (m_foodGrid[i][j] == COLLECTION_POINT_ID)
					m_console->printChar('$');
				else if (m_foodGrid[i][j] > 0)
					m_console->printChar('*');
				else
					m_console->printChar(' ');
			}
		}

		unsigned int size = m_orgs.size();

		for (unsigned int k=0;k<size;k++)
		{
			m_console->gotoXY(START_X + m_orgs[k]->getX(),
							  START_Y + m_orgs[k]->getY());
			m_console->printChar(m_orgs[k]->getDisplayChar());
		}
	}
	else
	{	
		// print all new food out first!

		if (m_foodCoords.size())
		{
			for (int i=0;i<m_foodCoords.size();i++)
			{
				if (m_foodGrid[m_foodCoords[i].y][m_foodCoords[i].x] != 0)
				{
					m_console->gotoXY(START_X+m_foodCoords[i].x,START_Y+m_foodCoords[i].y);
					m_console->printChar('*');
				}
			}

			m_foodCoords.clear();
		}

		unsigned int size = m_orgs.size();
		unsigned int k;
		bool needToRedraw = false;

		for (k=0;k<size;k++)
		{
			uint16 oldX, oldY;
			
			if (m_orgs[k]->getOldXY(&oldX,&oldY) == true)
			{
				m_console->gotoXY(START_X+oldX,START_Y+oldY);
				if (m_foodGrid[oldY][oldX] == COLLECTION_POINT_ID)
					m_console->printChar('$');
				else if (m_foodGrid[oldY][oldX] > 0)
					m_console->printChar('*');
				else
					m_console->printChar(' ');

				needToRedraw = true;
			}
		}

		if (needToRedraw)
		{
			for (k=0;k<size;k++)
			{
				m_console->gotoXY(START_X + m_orgs[k]->getX(),
								  START_Y + m_orgs[k]->getY());
				m_console->printChar(m_orgs[k]->getDisplayChar());
			}
		}
	}

	m_console->refresh();
}


void World::getNumAlive(uint16 *orgs, uint16 *drones)
{
	*orgs = *drones = 0;

	for (uint32 i=0;i<m_orgs.size();i++)
		if (m_orgs[i]->alive())
		{
			if (m_orgs[i]->getModuleName() == "drone")
				++(*drones);
			else 
				++(*orgs);
		}
}
