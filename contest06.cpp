//----------------------------------------------------------------------------
//
// contest06.cpp
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
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>

#include "settings.h"
#include "world.h"
#include "organism.h"
#include "compiler.h"
#include "mycon.h"
#include "disasm.h"

#include "drone.h"	 

using namespace std;

void myRandomize(const Settings & s)
{
	myrand(s.getSeed());
	//srand(time(NULL));
}

void removeNewline(char *s)
{
	char * ptr = strchr(s,'\n');
	if (ptr)
		*ptr = 0;
	ptr = strchr(s,'\r');
	if (ptr)
		*ptr = 0;
}


void printDisassembly(std::string fileToDisasm)
{
	Compiler c;
	string error;

	if (c.compile(fileToDisasm,error) == false)
	{
		printf("Error compiling source file:\n %s\n",error.c_str());
		return;
	}

	OrganismBinary *ob = c.getProgram();
	if (ob == NULL)
	{
		printf("Error compiling player file:%\n program size exceeds NANORG memory size\n");
		return;
	}

	uint16 arr[MAX_DNA] = {0};

	ob->getProgram(arr);

	DisAsm	d(arr,NULL);

	vector<string>	v;

	printf("Disassembly of %s:\n\n",fileToDisasm.c_str());

	d.getDisassembly(v,0,MAX_DNA);
	for (unsigned int i=0;i<v.size();i++)
		printf("%s\n",v[i].c_str());

	delete ob;
}

bool oneRound
(
	Settings &s, 
	OrganismBinary *player, 
	OrganismBinary *drone, 
	double *finalScore,
	uint16 *finalOrgs,
	uint16 *finalDrones,
	uint32 *finalTickNum
)
{
	myRandomize(s);

	CConsole	*cc = new CConsole(s.getQuiet());
	if (cc == NULL)
		return(false);

	World w(&s,cc);
	if (w.populateWorld(player,drone) == false)
		return(false);

	w.run();

	cc->clearScreen();
	delete cc;

	if (finalScore != NULL)
		*finalScore = w.getScore();
	if (finalOrgs != NULL && finalDrones != NULL)
		w.getNumAlive(finalOrgs,finalDrones);
	if (finalTickNum != NULL)
		*finalTickNum = w.getTickNum();

	return(true);
}

OrganismBinary *getDrone(void)
{
	unsigned short arr[MAX_DNA];

	for (int i=0;i<MAX_DNA;i++)
		arr[i] = droneArray[i] ^ 0xBABE;

	return (new OrganismBinary(arr,MAX_DNA,DRONE_STRING));
}

std::string getCommaDelimitedNumber(double number)
{
	char temp[256];
	std::string result;
	int i,j;

	sprintf(temp,"%.0lf",number);
	for (j=1,i=strlen(temp)-1;i >= 0;--i,++j)
	{
		result = temp[i] + result;
		if (j % 3 == 0 && i > 0)
			result = "," + result;
	}

	return(result);
}

bool runSingle(Settings &s)
{
	string error;

	// if we're running a tournament then we don't want to run a single run...
	if (s.getTournamentFile().length() > 0)
		return(false);
	
	if (s.getPlayerFile() == "")
	{
		printf("No player file specified\n");
		return(false);
	}
	/*
	if (s.getDroneFile() == "")
	{
		printf("No drone file specified\n");
		return(false);
	}
	*/

	myRandomize(s);

	Compiler c;

	if (c.compile(s.getPlayerFile(),error) == false)
	{
		printf("Error compiling player file:\n %s\n",error.c_str());
		return(false);
	}

	OrganismBinary *playerOB = c.getProgram();
	if (playerOB == NULL)
	{
		printf("Error compiling player file:%\n program size exceeds NANORG memory size\n");
		return(false);
	}

	/*
	c.reset();

	if (c.compile(s.getDroneFile(),error) == false)
	{
		printf("Error compiling drone file:\n %s\n",error.c_str());
		return(false);
	}
	*/

	// our drone is encoded internally; don't load from a file
	OrganismBinary *droneOB = getDrone();	// c.getProgram();

	CConsole	*cc = new CConsole(s.getQuiet());
	if (cc == NULL)
	{
		delete playerOB;
		delete droneOB;
		printf("Error initializing text graphics library\n");
		return(false);
	}

	double finalScore = 0;
	uint32 finalTick;
	uint16 orgs, drones;

	oneRound(s,playerOB,droneOB,&finalScore,&orgs,&drones,&finalTick);
	printf("Entrant: %s\n",playerOB->getModuleInfo().c_str());
	printf("Your score: %s\n",getCommaDelimitedNumber(finalScore).c_str());
	printf("Live organisms: %d, Live drones: %d, Final tick #: %d, Seed: %u\n",
		orgs, 
		drones, 
		finalTick,
		s.getSeed());

	delete playerOB;
	delete droneOB;

  	return(true);

}

void runTrials
(
	Settings &s,
	vector<uint32> &seeds,
	OrganismBinary *droneOB,
	OrganismBinary *playerOB,
	vector<NANORG_RESULT> & results
)
{
	s.setQuiet(true);

	double finalScore = 0;
	double totalScore = 0;

	for (int i=0;i<seeds.size();i++)
	{
		printf(" Evaluating %s: %d of %d\r",playerOB->getModuleName().c_str(),i+1,seeds.size());
		s.setSeed(seeds[i]);

		if (oneRound(s, playerOB, droneOB, &finalScore,NULL,NULL,NULL) == true)
		{
			totalScore += finalScore;
		}
		else
		{
			NANORG_RESULT	r(0,playerOB->getModuleInfo(),"Memory allocation error");
			results.push_back(r);
			return;
		}
	}

	printf("\n");

	NANORG_RESULT r(totalScore,playerOB->getModuleInfo(),"");
	results.push_back(r);
}


/* 
format of tournament file:

  results filename
  list of seeds, one per line
  blank line
  drone filename.asm
  list of asm files to test

*/


bool runTournament(Settings &s)
{
	// must have a tournament file
	if (s.getTournamentFile().length() == 0)
		return(false);

	FILE *stream = fopen(s.getTournamentFile().c_str(),"rt");
	if (stream == NULL)
	{
		printf("Unable to open tournament configuration file: %s\n",s.getTournamentFile().c_str());
		return(false);
	}

	FILE	*rstream;
	char	temp[512];
	string	resultFile;

	// get results filename
	if (fgets(temp,511,stream) != NULL)
	{
		removeNewline(temp);
		resultFile = temp;
		rstream = fopen(temp,"wt");
		if (rstream == NULL)
		{
			printf("Unable to create results file: %s\n",temp);
			return(false);
		}
	}
	else
	{
		fclose(stream);
		printf("Improperly formatted tournament configuration file: %s\n",s.getTournamentFile().c_str());
		return(false);
	}

	vector<uint32>		seeds;

	while (!feof(stream))
	{
		if (fgets(temp,511,stream) != NULL)
		{
			uint32 val = (uint32)atol(temp);
			if (val == 0)
				break;
			seeds.push_back(val);
		}
	}

	if (feof(stream))
	{
		fclose(rstream);
		fclose(stream);
		printf("Missing organism filenames in tournament configuration file\n");
		return(false);
	}

	string error;
	OrganismBinary *droneOB, *playerOB;

	// use internal encoding for drone
	droneOB = getDrone();

	/*
	if (fgets(temp,511,stream) != NULL)
	{
		// got the drone filename...
		Compiler c;

		removeNewline(temp);

		if (c.compile(temp,error) == false)
		{
			fclose(rstream);
			printf("Error compiling drone file:\n %s\n",error.c_str());
			return(false);
		}

		droneOB = c.getProgram();
	}
	*/

	vector<NANORG_RESULT>		results;

	printf("Running tournament...\n");

	while (!feof(stream))
	{
		if (fgets(temp,511,stream) != NULL)
		{
			// got the first competitor filename...
			Compiler c;

			removeNewline(temp);

			if (strlen(temp) == 0)
				continue;

			if (c.compile(temp,error) == false)
			{
				NANORG_RESULT	r(0,temp,error);
				results.push_back(r);
				printf(" Evaluating %s: Error (%s)\n",temp,error.c_str());
				continue;
			}

			playerOB = c.getProgram();
			if (playerOB == NULL)
			{
				NANORG_RESULT	r(0,temp,"program size exceeds NANORG memory size");
				results.push_back(r);
				printf(" Evaluating %s: Error (program size exceeds NANORG memory size)\n",temp);
				continue;
			}
 

			runTrials(s,seeds,droneOB,playerOB,results);

			delete playerOB;
			playerOB = NULL;
		}
	}

	sort(results.begin(), results.end());		// sorts ascending

	printf("Writing results to %s...\n",resultFile.c_str());

	for(int i=results.size()-1;i>=0;--i)
	{
		if (results[i].result.length())
			fprintf(rstream,"%.0lf %s (%s)\n",
								results[i].totalScore,
								results[i].moduleInfo.c_str(),
								results[i].result.c_str());
		else
			fprintf(rstream,"%.0lf %s\n",
								results[i].totalScore,
								results[i].moduleInfo.c_str());
	}

	delete droneOB;

	fclose(rstream);
	fclose(stream);

	return(true);
}


bool printDisassembly(const Settings &s)
{
	if (s.getPrintFile().length() > 0)
	{
		printDisassembly(s.getPrintFile());
		return(true);
	}
	return(false);
}

int main(int argc, char *argv[])
{
	Settings s;
	string error;
	
	if (s.LoadSettings(argc,argv,error) == false)
	{
		printf("Error loading settings:\n %s\n",error.c_str());
		return(-1);
	}

	if (printDisassembly(s) == true)
	{
		return(0);
	}

	if (runSingle(s) == true)
	{
		return(0);
	}

	if (runTournament(s) == true)
	{
		return(0);
	}

	// must have been an error
  	return(-1);
}

// when using -q, show final map status at the end
// highlight debug char in special color
