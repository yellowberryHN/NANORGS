//----------------------------------------------------------------------------
//
// mycon.h
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

#ifndef _MYCON_H_

#define _MYCON_H_

#include <string>
#include <cstring>
#include <stdio.h>


#ifdef WIN32
#include <conio.h>
#include <windows.h>
#else
#include <curses.h>
#endif


#define START_Y                 0
#define START_X                 0

#define STATUS_Y                44
#define STATUS_X                0
#define PROMPT_Y                48
#define PROMPT_X                0

#define SCORE_X                 0
#define SCORE_Y                 42

#define SCREEN_HEIGHT   50

#define MAX_LINE_WIDTH  79


#ifdef WIN32

class CConsole
{
public:
        void gotoXY(int nX, int nY)
        {
                if (m_noDisplay)
                        return;

                COORD pos = {(short)nX, (short)nY};
                //m_stLastCoord = pos;
                SetConsoleCursorPosition ( m_hConsole, pos );
        }

		void refresh(void)
		{
			// nothing to do
		}

        void printChar(char ch)
        {
                if (m_noDisplay)
                        return;

                DWORD dwNum;
                WriteConsole(m_hConsole,&ch,1,&dwNum,NULL);
        }

        void printString(const std::string &s)
        {
                if (m_noDisplay)
                        return;

                for (unsigned int i=0;i<s.length();i++)
                        printChar(s[i]);
        }

        void printStringOverwrite(const std::string &s)
        {
                if (m_noDisplay)
                        return;

                std::string t = s;

                while (t.length() < MAX_LINE_WIDTH)
                        t += " ";
                printString(t);
        }


        std::string getString(void)
        {
                char temp[256];
                fgets(temp, 256, stdin);

                char* ptr = strchr(temp, '\n');
                if (ptr)
                    *ptr = 0;
                ptr = strchr(temp, '\r');
                if (ptr)
                    *ptr = 0;
                
                return(temp);
        }

        void clearScreen(void)
        {
                if (m_noDisplay)
                        return;

                char temp[256] = {0};
                int i;

                for (i=0;i<80;i++)
                        temp[i] = ' ';

                for (i=0;i<SCREEN_HEIGHT;i++)
                {
                        gotoXY(0,i);
                        printStringOverwrite(temp);
                }
        }

        CConsole(bool noDisplay)
        {
                m_noDisplay = noDisplay;

                if (m_noDisplay == false)
                    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                else
                    m_hConsole = INVALID_HANDLE_VALUE;
        }

        ~CConsole()
        {
                // N/A
        }

private:

        bool    m_noDisplay;
        HANDLE  m_hConsole;
};

#elif CURSES

class CConsole
{
public:
        void gotoXY(int nX, int nY)
        {
                if (m_noDisplay)
                        return;

                wmove( m_hConsole, nY, nX );
        }

        void printChar(char ch)
        {
                if (m_noDisplay)
                        return;

                waddch(m_hConsole, (chtype)ch);
        }

        void printString(const std::string &s)
        {
                if (m_noDisplay)
                        return;

                waddstr(m_hConsole, s.c_str());
                wrefresh(m_hConsole);
        }

		void refresh(void)
		{
			wrefresh(m_hConsole);
		}

        void printStringOverwrite(const std::string &s)
        {
                if (m_noDisplay)
                        return;

                printString(s);
                wclrtoeol(m_hConsole);
                wrefresh(m_hConsole);
        }


        std::string getString(void)
        {
                char temp[256];

                wgetnstr(m_hConsole, temp, 255);
                temp[255] = '\0';

                return(temp);
        }

        void clearScreen(void)
        {
                if (m_noDisplay)
                        return;

                wclear(m_hConsole);
                wrefresh(m_hConsole);
        }

        CConsole(bool noDisplay)
        {
                m_noDisplay = noDisplay;

                if (m_noDisplay == false) {
                        m_hConsole = initscr();
                        if (m_hConsole == NULL)
                                printf("Error in initscr\n");
                }
        }

        ~CConsole()
        {
                endwin();
        }

private:

        bool    m_noDisplay;
        WINDOW  *m_hConsole;
};

#else
#error Need either Win32 or curses
#endif


#endif // #ifndef _MYCON_H_
