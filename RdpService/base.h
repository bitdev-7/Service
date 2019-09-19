#pragma once
// C functions
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>

#define NOTIFY_MESSAGE(msg)		do { time_t now = time(NULL); struct tm tstruct = *localtime(&now);		 \
									 char buf[80]; strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);  \
									 std::ofstream logfile; logfile.open("rdpservice.log", std::ios_base::app); \
									 logfile << "[" << buf << "] " << msg << std::endl; logfile.close(); \
								} while(0)
