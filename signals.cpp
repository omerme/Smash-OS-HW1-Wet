#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


///how to print "smash: got ctrl-Z" ?
/**
 *  I think if smash is running we're OK with using cout from handler (because no other process has access to cout)..
 *  and if external if fg we use the flags
 *  changed it in ctrlZHandler
 *  **/
void ctrlZHandler(int sig_num) { // sig_num == SIGTSTP
    ExternalCommand* cmd = SmallShell::getInstance().getCurrCommand();
    if (cmd == nullptr) {
        cout << "smash: got ctrl-Z" << endl;
        return;
    }
    else { // external in fg
        SmallShell::getInstance().sigZ = true; /// for printing flag after return?
        /** all code used to be here.. **/
        kill(cmd->getPid(), SIGSTOP); ///when do we do this? does it matter?
        //cout<< "It's me! Hi!" << endl;
        return;
        /// how to print: "smash: process <foreground-PID> was stopped"? (only if a job was stopped)
    }
}


void ctrlCHandler(int sig_num) {  // sig_num == SIGINT
    if(SmallShell::getInstance().getCurrCommand() != nullptr){
        SmallShell::getInstance().sigC= true;
        kill(SmallShell::getInstance().getCurrCommand()->getPid(), sig_num);
    }
    else
        cout << "smash: got ctrl-Z" <<endl;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

/** the code:
 * if(cmd->getJobId()==-1) { // if new in JobList
            SmallShell::getInstance().jobs.addJob(new Job(cmd));
            // addJob sets command->job_id.
            // set command->job_id manually: cmd->setJobId(SmallShell::getInstance().jobs.getMaxId());
        }
        cmd->setBg(true);
        // below - set job in list to "stopped"
        SmallShell::getInstance().jobs.getJobById(cmd->getJobId())->setIsStopped(true);
 * **/