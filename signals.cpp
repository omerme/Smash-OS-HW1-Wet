#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    ///how to print "smash: got ctrl-Z" ?
    SmallShell::getInstance().sigZ = true;
    ExternalCommand* cmd = SmallShell::getInstance().getCurrCommand();
    if (cmd == nullptr) {
        return;
    }
    else { // external in fg
        kill(cmd->getPid(), sig_num); ///when do we do this? does it matter?
        if(cmd->getJobId()==-1) { // if new in JobList
            SmallShell::getInstance().jobs.addJob(new Job(cmd));
            // addJob sets command->job_id.
            // set command->job_id manually: cmd->setJobId(SmallShell::getInstance().jobs.getMaxId());
        }
        cmd->setBg(true);
        // below - set job in list to "stopped"
        SmallShell::getInstance().jobs.getJobById(cmd->getJobId())->setIsStopped(true);
    }
}

void ctrlCHandler(int sig_num) { // sig_num == SIGTSTP
    // TODO: Add your implementation
    SmallShell::getInstance().sigC= true;
    if(SmallShell::getInstance().getCurrCommand() != nullptr){
        kill(SmallShell::getInstance().getCurrCommand()->getPid(), sig_num);
    }
}

void alarmHandler(int sig_num) { // sig_num == SIGINT
  // TODO: Add your implementation
}

