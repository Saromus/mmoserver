/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/
#include "LogManager.h"

#include <cstdarg>
#include <stdarg.h>
#include <stdio.h>
#include <vector>

#include <exception>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iomanip>

#include "Utils/clock.h"

LogManager* LogManager::mSingleton;

class LOG_ENTRY
{
public:
	LogManager::LOG_PRIORITY	mPriority;
	uint8			mChannels;
	std::string		mMessage;

	bool mContinuation;
};

LogManager::LogManager(LOG_PRIORITY console_priority, LOG_PRIORITY file_priority, std::string filename)
{
	mMinPriorities[0] = console_priority;
	mMinPriorities[1] = file_priority;

	mFileName = filename;
    
    mOutputFile = std::unique_ptr<std::ofstream>(new std::ofstream(mFileName, std::ios_base::out));
	if(!mOutputFile->is_open())
	{
		throw std::exception();
	}

	_printLogo();
    
    mEntriesMutex = std::unique_ptr<boost::mutex>(new boost::mutex());
	mThread = std::unique_ptr<boost::thread>(new boost::thread(std::tr1::bind(&LogManager::_LoggerThread, this)));
}

LogManager::~LogManager()
{
    mThread->interrupt();
    mThread->join();
    mOutputFile->close();
}

void LogManager::_LoggerThread()
{
	std::vector<LOG_ENTRY*> mTempEntries;
    
	char* priority_strings[] = {"EMER", "ALRT", "CRIT", "ERRO", "WARN", "NOTI", "INFO", "DEBG"};

	while(true)
	{
		mEntriesMutex->lock();

		mTempEntries.reserve(mEntries.size());

		while(mEntries.size() > 0)
		{
			mTempEntries.push_back(mEntries.front());
			mEntries.pop();
		}
		mEntriesMutex->unlock();

		std::vector<LOG_ENTRY*>::iterator end = mTempEntries.end();

		struct tm t;
		time_t te = time(NULL);
		localtime_r(&te, &t);

		for(std::vector<LOG_ENTRY*>::iterator it=mTempEntries.begin(); it != end; it++)
		{
			if((*it)->mChannels & LOG_CHANNEL_CONSOLE && ((*it)->mPriority <= mMinPriorities[0]))
			{
				if(!(*it)->mContinuation)
					printf("[%02d:%02d:%02d] [%s] ",t.tm_hour,t.tm_min,t.tm_sec, priority_strings[(int)(*it)->mPriority - 1]);
				else
					printf("                  ");

				printf("%s\n", (*it)->mMessage.c_str());
			}
			
			if((*it)->mChannels & LOG_CHANNEL_FILE && ((*it)->mPriority <= mMinPriorities[1]))
			{
				if(mOutputFile->is_open())
				{
					if(!(*it)->mContinuation)
					{
						*mOutputFile << "[" << std::setw(2) << t.tm_hour;
						*mOutputFile << ":" << std::setw(2) << t.tm_min;
						*mOutputFile << ":" << std::setw(2) << t.tm_sec;
						*mOutputFile << "] [" << priority_strings[(int)(*it)->mPriority - 1] << "] ";

						//fprintf(mOutputFile, "[%02d:%02d:%02d] [%s] ",t.tm_hour,t.tm_min,t.tm_sec, priority_strings[(int)(*it)->mPriority - 1]);
					}
					else
					{
						*mOutputFile << "                  ";
						//fprintf(mOutputFile, "                  ");
					}
					
					*mOutputFile << (*it)->mMessage.c_str() << std::endl;
					mOutputFile->flush();
					//fprintf(mOutputFile, "%s\n", (*it)->mMessage.c_str());
					//fflush(mOutputFile);
				}
			}
            
			delete (*it);
		}

		mTempEntries.clear();

		boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	}
}

void	LogManager::_printLogo()
{
printf("                    ______        ______    _    _   _ _   _ \n");
printf("                   / ___\\ \\      / / ___|  / \\  | \\ | | | | |\n");
printf("                   \\___ \\\\ \\ /\\ / / |  _  / _ \\ |  \\| | |_| |\n");
printf("                    ___) |\\ V  V /| |_| |/ ___ \\| |\\  |  _  |\n");
printf("                   |____/  \\_/\\_/  \\____/_/   \\_\\_| \\_|_| |_|\n");
printf("                                               There is Another...\n\n");
}
	
void LogManager::log(LOG_PRIORITY priority, std::string format, ...)
{
	//Read the VA List Info
	va_list args;
	va_start(args, format);

  // Determine the size of the buffer needed and create an appropriately
  // sized container.
  const int size = vsnprintf(NULL, 0, format.c_str(), args);
  std::vector<char> buffer(size+1); // Account for the \n terminator.

	vsprintf(&buffer[0], format.c_str(), args);
	va_end(args);

	LOG_ENTRY* entry = new LOG_ENTRY();

	entry->mPriority = priority;
	entry->mChannels = LOG_CHANNEL_ALL;
  entry->mMessage.assign(buffer.begin(), buffer.end());
	entry->mContinuation = false;

	mEntriesMutex->lock();
	mEntries.push(entry);
	mEntriesMutex->unlock();
}

void LogManager::logCont(LOG_PRIORITY priority, std::string format, ...)
{

	//Read the VA List Info
	va_list args;
	va_start(args, format);  
  
  // Determine the size of the buffer needed and create an appropriately
  // sized container.
  const int size = vsnprintf(NULL, 0, format.c_str(), args);
  std::vector<char> buffer(size+1); // Account for the \n terminator.

	vsprintf(&buffer[0], format.c_str(), args);
	va_end(args);

	LOG_ENTRY* entry = new LOG_ENTRY();

	entry->mPriority = priority;
	entry->mChannels = LOG_CHANNEL_ALL;
  entry->mMessage.assign(buffer.begin(), buffer.end());
	entry->mContinuation = true;

	mEntriesMutex->lock();
	mEntries.push(entry);
	mEntriesMutex->unlock();
}

void LogManager::logS(LOG_PRIORITY priority, uint8_t channels, std::string format, ...)
{
	//Read the VA List Info
	va_list args;
	va_start(args, format);
  
  // Determine the size of the buffer needed and create an appropriately
  // sized container.
  const int size = vsnprintf(NULL, 0, format.c_str(), args);
  std::vector<char> buffer(size+1); // Account for the \n terminator.

	vsprintf(&buffer[0], format.c_str(), args);
	va_end(args);

	LOG_ENTRY* entry = new LOG_ENTRY();

	entry->mPriority = priority;
	entry->mChannels = channels;
  entry->mMessage.assign(buffer.begin(), buffer.end());
	entry->mContinuation = false;

	mEntriesMutex->lock();
	mEntries.push(entry);
	mEntriesMutex->unlock();
}

void LogManager::logContS(LOG_PRIORITY priority, uint8_t channels, std::string format, ...)
{
	//Read the VA List Info
	va_list args;
	va_start(args, format);
  
  // Determine the size of the buffer needed and create an appropriately
  // sized container.
  const int size = vsnprintf(NULL, 0, format.c_str(), args);
  std::vector<char> buffer(size+1); // Account for the \n terminator.

	vsprintf(&buffer[0], format.c_str(), args);
	va_end(args);

	LOG_ENTRY* entry = new LOG_ENTRY();

	entry->mPriority = priority;
	entry->mChannels = channels;
  entry->mMessage.assign(buffer.begin(), buffer.end());
	entry->mContinuation = true;

	mEntriesMutex->lock();
	mEntries.push(entry);
	mEntriesMutex->unlock();
}
