/*
 * sonataInfoDisplay
 *
 * Project: OpenSonATA
 * Version: 1.0
 * Author:  Jon Richards (current maintainer)
 *          The OpenSonATA code is the result of many programmers over many
 *          years.
 *
 * Displays information in a curses screen while SonATA is running.
 *
 * Copyright 2010 The SETI Institute
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by
 * applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * Attribution must be: “Licensed through SETI” in all published
 * uses of the software including analytics based on the software,
 * combined and merged software, papers, articles, books, reports,
 * web pages, etc.
 */

/**
 * @file main.cpp
 *
 * Displays information in a curses screen while SonATA is running.
 *
 * Project: OpenSonATA
 * <BR>
 * Version: 1.0
 * <BR>
 * Authors:
 * - Jon Richards (current maintainer)
 * - The OpenSonATA code is the result of many programmers over many
 * years.
 */

#include "details.h"
#include "utils.h"
#include "screen.h"
#include "components.h"

/**
 * Main entry point of the program.
 *
 * This application requires 3 arguments:
 *  - The system status file that is created in real time when SonATA
 *    is running. This is usually "sse-system-status.txt".
 *  - The system log file that is created in real time when SonATA
 *    is running. This is usually "systemlog-YYYY-MM-DD.txt".
 *  - The error log  file that is created in real time when SonATA
 *    is running. This is usually "errorlog-YYYY-MM-DD.txt".
 *
 * Note: This version does not actually process "systemlog-YYYY-MM-DD.txt" and
 * "errorlog-YYYY-MM-DD.txt". Future development will involve implementing 
 * the reading and displaying of the information in these files.
 */
int main(int argc, char **argv)
{
    string systemStatusFileName = "";
    string systemLogFileName = "";
    string systemErrorFileName = "";
    string commandSystemStatus = "";
    string commandLog = "";
    string commandError = "";
    Screen screen;
    fd_set rfds;
    struct timeval tv;
    int retVal = -1;
    char line[2048];

    Components componentDetails;

    // Print help if not enough arguments on the command line
    if (argc < 3)
    {
        fprintf(stderr, "\nsonataInfoDisplay - curses display for SonATA information\n\n");
        fprintf(stderr, "  USAGE: sonataInfoDisplay <sse-system-status.txt> \\\n");
        fprintf(stderr, "         <systemlog-YYYY-MM-DD.txt> <errorlog-YYYY-MM-DD.txt>\n");
        fprintf(stderr, "  NOTE:  The arguments need to be the file prefixed with the path.\n\n");
        return(1);
    }

    // Read in the arguments.
    systemStatusFileName = argv[1]; 
    systemLogFileName    = argv[2]; 
    systemErrorFileName  = argv[3]; 

    //Touch the files to make sure they exists
    string touchCommand = "touch " + systemStatusFileName;
    system(touchCommand.c_str());
    touchCommand = "touch " + systemLogFileName;
    system(touchCommand.c_str());
    touchCommand = "touch " + systemErrorFileName;
    system(touchCommand.c_str());

    //Create the commands to read in the files
    FILE *statusFp = NULL;
    FILE *logFp = NULL;
    FILE *errorFp = NULL;

    commandSystemStatus = systemStatusFileName;
    commandLog          = systemLogFileName;
    commandError        = systemErrorFileName;

    statusFp = fopen(commandSystemStatus.c_str(), "r");
    if(statusFp == NULL)
    {
        fprintf(stdout," Could not open %s, EXITING.\n",
                commandSystemStatus.c_str());
    }

    logFp = fopen(commandLog.c_str(), "r");
    if(logFp == NULL)
    {
        fclose(logFp);
        fprintf(stdout," Could not open %s, EXITING.\n", commandLog.c_str());
    }

    errorFp = fopen(commandLog.c_str(), "r");
    if(errorFp == NULL)
    {
        fclose(errorFp);
        fprintf(stdout," Could not open %s, EXITING.\n", commandError.c_str());
    }


    //Get the file descriptors from file pointers.
    int statusFd = fileno(statusFp);
    int logFd = fileno(logFp);
    int errorFd = fileno(errorFp);

    //Figure out the max file descriptor to use in the select() call.
    int maxFd = statusFd;
    if(logFd > maxFd) maxFd = logFd;
    if(errorFd > maxFd) maxFd = errorFd;

    //Initialize the curses screen.
    screen.init();
    screen.screenResize(0);

    time_t lastStatusTime = time(NULL);;
    int linesSinceLastStatus = 0;

    //Loop foever
    while(1)
    {

        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(statusFd, &rfds);
        FD_SET(logFd, &rfds);
        FD_SET(errorFd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 200000; //1/5 second

        retVal = select(maxFd+1, &rfds, NULL, NULL, &tv);

        //Process any data read from the status file.
        if(FD_ISSET(statusFd, &rfds))
        {
            fgets(line, sizeof(line)-1, statusFp);
            if(line[0] != 0 && componentDetails.addWithFilter(line))
                screen.paint(&componentDetails);
            linesSinceLastStatus++;
            memset(line, 0, sizeof(line));
        }

        //The trigger for the end of a status screen paint is "===..." but sometimes
        //this does not arrive, so we have to force it.
        if(linesSinceLastStatus > 0 && (int)(time(NULL) - lastStatusTime) > 1)
        {
            linesSinceLastStatus = 0;
            lastStatusTime = time(NULL);
            componentDetails.addWithFilter("====================================");
            screen.paint(&componentDetails);
        }

        //Process the log file
        if(FD_ISSET(logFd, &rfds))
        {
        }

        //Process the error file
        if(FD_ISSET(errorFd, &rfds))
        {
        }

        screen.processKey(&componentDetails);
    }


    return 0;

}
