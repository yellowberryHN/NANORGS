//----------------------------------------------------------------------------
//
// settings.h
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

#ifndef _SETTINGS_H_

#define _SETTINGS_H_

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <cctype>

#include "types.h"
#include "constants.h"

class Settings
{
public:
	Settings()
	{
		m_maxIterations = DEFAULT_MAX_ITERATIONS;
		m_maxOrganisms = DEFAULT_MAX_ORGANISMS;
		m_maxDrones = DEFAULT_MAX_DRONES;
		m_foodDensity = DEFAULT_FOOD_DENSITY;
		m_singleStepID = INVALID_ID;
		m_singleStep = false;
		m_seed = time(NULL);
		m_quiet = false;
	}
	
	bool LoadSettings(int argc, char *argv[], std::string &error)
	{
		FILE *stream;

		if (argc == 1)
		{
			printf("\nSymantec 2006 University Programming Competition\n"); 
			printf("(C)Copyright 2006, Symantec Corporation\n");
//			printf("by Carey Nachenberg\n");
			printf("\nusage: contest06 -option1:value1 -option2:value2 ...\n\n");
//			printf(" -d:drone.asm  *Specify the drone's DNA\n");
//			printf(" -f:##         Specify food density percentage (default=%d%%)\n",DEFAULT_FOOD_DENSITY);
			printf(" -g:X          Single-step debug the organism specified by X (a letter)\n");
			printf(" -i:####       Specify # of iterations (default=%d)\n",DEFAULT_MAX_ITERATIONS);
			printf(" -l:log.txt    Log organism program trace to log.txt\n");
//			printf(" -n:####       Specify # of drones (default=%d)\n",DEFAULT_MAX_DRONES);
//			printf(" -o:####       Specify # of clones of the entrant's organism (default=%d)\n",DEFAULT_MAX_ORGANISMS);
			printf(" -p:org.asm    *Specify the player's organism source file\n");
			printf(" -q            Run in quiet mode (no display)\n");
			printf(" -s:####       Specify the randomization seed\n");
			printf(" -z:org.asm    Show the disassembly and bytecode for this organism\n");
			printf("\n   * means required field\n\n");
		}

		for (int i=1;i<argc;i++)
		{
			if (argv[i][0] == '-')
			{
 				if ((argv[i][2] == ':' && strlen(argv[i]) >= 3) || strlen(argv[i]) == 2)
				{
					switch(tolower(argv[i][1]))
					{
						case 'i':
							m_maxIterations = atol(argv[i]+3);	// -i:10
							break;
/*
						case 'o':
							m_maxOrganisms = (uint16)atol(argv[i]+3);
							if (m_maxOrganisms > DEFAULT_MAX_ORGANISMS)
							{
								error = "invalid number of organisms specified";
								return(false);
							}
							break;

						case 'n':
							// # of drones
							m_maxDrones = (uint16)atol(argv[i]+3);
							if (m_maxDrones > DEFAULT_MAX_DRONES)
							{
								error = "invalid number of drones specified";
								return(false);
							}
							break;
*/
						case 's':
							m_seed = atol(argv[i]+3);
							break;
						case 'l':
							m_debugFile = argv[i]+3;
							break;
						case 'z':
							m_printFile = argv[i]+3;
							break;
						case 't':		// tournament: undocumented
							m_tournamentFile = argv[i]+3;
							break;
						case 'g':
							if (m_quiet == true)
							{
								error = "cannot single-step debug while in quiet mode";
								return(false);
							}
							m_singleStep = true;
							// ID is 0 to 25
							m_singleStepID = INVALID_ID;
							if (argv[i][3] >= 'A' && argv[i][3] <= 'Z')
								m_singleStepID = argv[i][3]-'A';
							else if (argv[i][3] >= 'a' && argv[i][3] <= 'z')
								m_singleStepID = argv[i][3]-'a' + 26;
							
							if (m_singleStepID == INVALID_ID ||
								m_singleStepID >= m_maxOrganisms)
							{
								error = "invalid debug ID specified";
								return(false);
							}
							
							break;
							/*
						case 'f':
							m_foodDensity = (uint16)atol(argv[i]+3);
							break;
							*/
						case 'p':
							m_playerFile = argv[i]+3;
							stream = fopen(m_playerFile.c_str(),"rt");
							if (stream == NULL)
							{
								error = "invalid player file";
								return(false);
							}
							fclose(stream);
							break;
/*
						case 'd':
							m_droneFile = argv[i]+3;
							stream = fopen(m_droneFile.c_str(),"rt");
							if (stream == NULL)
							{
								error = "invalid drone file";
								return(false);
							}
							fclose(stream);
							break;
*/
						case 'q':
							if (m_singleStep == true)
							{
								error = "cannot single-step debug while in quiet mode";
								return(false);
							}
							m_quiet = true;
							break;
						default:
							error = (std::string)"invalid parameter (" + argv[i] +(std::string)")";
							return(false);
					}
				}
				else
				{
					error = (std::string)"invalid parameter syntax (" + argv[i] +(std::string)")";
					return(false);
				}
			}
			else
			{
				error = (std::string)"invalid option: " + argv[i];
				return(false);
			}
		}

		return(true);
	}

	uint32 getMaxIterations(void) const
	{
		return(m_maxIterations);
	}

	uint16 getMaxOrganisms(void) const
	{
		return(m_maxOrganisms);
	}

	uint16 getMaxDrones(void) const
	{
		return(m_maxDrones);
	}

	uint32 getFoodDensity(void) const
	{
		return(m_foodDensity);
	}

	uint32 getSeed(void) const
	{
		return(m_seed);
	}

	sint32 getSingleStepID(void) const
	{
		return(m_singleStepID);
	}

	std::string getDebug(void) const
	{
		return(m_debugFile);
	}

	std::string getPlayerFile(void) const
	{
		return(m_playerFile);
	}

	std::string getDroneFile(void) const
	{
		return(m_droneFile);
	}

	std::string getPrintFile(void) const
	{
		return(m_printFile);
	}

	std::string getTournamentFile(void) const
	{
		return(m_tournamentFile);
	}

	bool getSingleStep(void) const
	{
		return(m_singleStep);
	}

	bool getQuiet(void) const
	{
		return(m_quiet);
	}

	void setSeed(uint32 seed)
	{
		m_seed = seed;
	}

	void setQuiet(bool quiet)
	{
		m_quiet = quiet;
	}

private:
	uint32			m_maxIterations;
	uint16			m_maxOrganisms;
	uint16			m_maxDrones;
	uint16			m_foodDensity;
	uint32			m_seed;
	std::string		m_debugFile;
	std::string		m_printFile;
	std::string		m_playerFile;
	std::string		m_droneFile;
	std::string		m_tournamentFile;
	bool			m_singleStep;
	bool			m_quiet;
	uint32			m_singleStepID;
};

struct NANORG_RESULT
{
	NANORG_RESULT(double score, std::string moduleInfo, std::string result)
	{
		this->totalScore = score;
		this->moduleInfo = moduleInfo;
		this->result = result;
	}

	double			totalScore;
	std::string		moduleInfo;
	std::string		result;
};

inline bool operator==(const NANORG_RESULT &a,const NANORG_RESULT &b)
{
	return(a.totalScore == b.totalScore);
}

inline bool operator<(const NANORG_RESULT &a,const NANORG_RESULT &b)
{
	return(a.totalScore < b.totalScore);
}

#endif // #ifndef _SETTINGS_H_
