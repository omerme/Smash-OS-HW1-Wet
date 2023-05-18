#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>
#include <sched.h>



using namespace std;

#define DO_SYS( syscall ) do { \
            if( (syscall) == -1) { \
                string syscallStr = (string)#syscall;                \
                string s = "smash error: " + syscallStr.substr(0, syscallStr.find('(')) + " failed";            \
                perror ( s.c_str() );  \
            }                      \
        } while(0)

#define STDOUT_FD 1
#define SYSCALL_FAILED (-1)
#define READ 0
#define WRITE 1
#define ERR 2



const std::string WHITESPACE = " \n\r\t\f\v";
/// omer 29/04 - debug
//const std::vector<string> builtInCommands = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "bg", "quit", "kill"};
const int MAX_PARAMS = 20;
const int MAX_LINE_WORDS = MAX_PARAMS + 3; /// plus command-word, &, and nullptr
//0       1   2       20    21  --> len = 22 total
//sleep arg1 arg2 .. arg20  &
const int JOBS_MAX_IDX = 100;
const int JOBS_MIN_IDX = 1;
/** when adding a command:
 * 1. implement c'tor, d'tor and execute
 * 2. add command name to SmallShell::CreateCommand
 * 3. if built-in - add:  _removeBackgroundSign(cmd_line);
 * 4. add to SmallShell::exec
 * ute (if needed)
 * **/

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


/** *****************general functions***************** **/


string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY() /// whats that ?
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT() /// whats that ?
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void postWaitPid( ExternalCommand* exCommand){
    if(SmallShell::getInstance().sigC){
        cout << "smash: got ctrl-C" <<endl;
        cout << "smash: process " << exCommand->getPid() << " was killed" << endl;
        SmallShell::getInstance().sigC = false;
    }
    if(SmallShell::getInstance().sigZ) {  /// "if" or "else if" here?
        cout << "smash: got ctrl-Z" <<endl;
        if(exCommand->getJobId()==-1) { // if new in JobList
            SmallShell::getInstance().jobs.addJob(new Job(exCommand));
            // addJob sets command->job_id.
            // set command->job_id manually: cmd->setJobId(SmallShell::getInstance().jobs.getMaxId());
        }
        exCommand->setBg(true);
        // below - set job in list to "stopped"
        SmallShell::getInstance().jobs.getJobById(exCommand->getJobId())->setIsStopped(true);
        cout << "smash: process " << exCommand->getPid() << " was stopped" << endl;
        SmallShell::getInstance().sigZ = false;
    } //haha

    SmallShell::getInstance().setCurrCommand(nullptr);
}


/** ***************** SmallShell ***************** **/


SmallShell::SmallShell() : jobs(), prompt("smash"), prevWD(), curr_command(nullptr), sigC(false),
sigZ(false), sigAlarm(false)
{
    /// omer 26.4
    curWD = getcwd(nullptr,0);
}

SmallShell::~SmallShell() { }

std::string SmallShell::getCurWD() const {
    return curWD;
}
void SmallShell::setCurWD(std::string newCurWD) {
    curWD = newCurWD;
}
std::string SmallShell::getPrevWD() const {
    return prevWD;
}
void SmallShell::setPrevWD(std::string newPrevWD) {
    prevWD = newPrevWD;
}
void SmallShell::setPrompt(std::string newPrompt) {
    prompt = newPrompt;
}

ExternalCommand* SmallShell::getCurrCommand(){
    return curr_command;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(char* cmd_line) {
	string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    bool isbg = _isBackgroundComamnd(cmd_line);
    string orig_cmd = cmd_line;
    // cout << orig_cmd; /// delete
    orig_cmd = orig_cmd.substr(0, orig_cmd.find('\n'));
    // cout << orig_cmd; /// delete
    if (firstWord.compare("") == 0 || cmd_s.compare("\n") == 0) {
        return nullptr;
    }
    //cout << "check SmallShell::CreateCommand: cmd_s is:" << cmd_s << "and FirstWord  is:" << firstWord << endl;
    if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    else if (cmd_s.find('>') != string::npos) {
        _removeBackgroundSign(cmd_line);
        return new RedirectionCommand(cmd_line);
    }
    //...
    _removeBackgroundSign(cmd_line); //
    if (firstWord.compare("chprompt") == 0) {
        return new ChangePoromptCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, nullptr);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, &jobs); /// for jobs: change nullptr to joblist
    }
    else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("getfiletype") == 0) {
        return new GetFileTypeCommand(cmd_line);
    }
    else if (firstWord.compare("setcore") == 0) {
        return new SetcoreCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("chmod") == 0) {
        return new ChmodCommand(cmd_line);
    }
    else { //external
        if (strchr(cmd_line, '*') || strchr(cmd_line, '?'))
            return new ComplexExternalCommand(cmd_line, isbg, orig_cmd);
        return new SimpleExternalCommand(cmd_line, isbg, orig_cmd);
    }
    //return nullptr;
}


/// need to debug - something caused segfault (we played
void SmallShell::executeCommand(const char *cmd_line_in)
{
    char *cmd_line = new char[strlen(cmd_line_in)+1]();
    strcpy(cmd_line, cmd_line_in);
    //cout << "check in SmallShell::executeCommand:" << cmd_line << endl;
    ///if built-in:
    Command* cmd = CreateCommand(cmd_line);
    if(cmd != nullptr) { // if not empty line:
        //if(!(cmd->getBg()) && (dynamic_cast<const ExternalCommand*>(cmd) != nullptr)) { /// if external in fg
        //   curr_command = dynamic_cast<ExternalCommand*>(cmd);
        //}
        //     bool isWarrior =  dynamic_cast<const Warrior*>(curPlayer.get()) != nullptr;
        jobs.removeFinishedJobs();
        cmd->execute();
        if(!cmd->getBg()) { /// if not in back-ground
            delete cmd;
        }
        //curr_command= nullptr;
        delete[] cmd_line;
    }
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPrompt() const {
    return this->prompt;
}

void SmallShell::changePrompt(string new_prompt) {
    prompt = new_prompt;
}

void SmallShell::setCurrCommand(ExternalCommand* currCom){
    curr_command = currCom;
}



/** ***************** Jobs ***************** **/


//Job::Job(const char* cmd_line, bool isBg, const char* orig_cmd_line, int job_id) :
//command(nullptr), job_id(job_id), orig_cmd_line(orig_cmd_line), is_stopped(false) {
//    command = (strchr(orig_cmd_line, '*') || strchr(orig_cmd_line, '?')) ?
//            new ComplexExternalCommand(cmd_line, isBg) : command = new SimpleExternalCommand(cmd_line, isBg);
//    time_added =
//}

Job::Job(ExternalCommand* command, int job_id, bool is_stopped) : command(command), job_id(job_id), is_stopped(is_stopped)
{
    time_added = time(nullptr);
}

int Job::getId() {
    return job_id;
}

void Job::setId(int j_id) {
    job_id = j_id;
}

bool Job::getIsStopped() const {
    return is_stopped;
}
void Job::setIsStopped(bool isStopped) {
    is_stopped = isStopped;
}

void Job::printJob() {
    cout << "[" << job_id << "] " << command->getCmd() << " : " << command->getPid() << " " << difftime(time(nullptr) ,time_added) << " secs";
    if (is_stopped)
        cout << " (stopped)" <<endl;
    else
        cout << endl;
}


ExternalCommand* Job::getCommand() const {
    return command;
}

JobsList::JobsList() : max_id(0), jobs(101, nullptr){}

void JobsList::addJob(Job* newJob){
    removeFinishedJobs();
    if (max_id >= 100)
        cerr << "too many jobs in list" << endl;
    jobs[max_id + 1] = newJob;
    newJob->setId(max_id + 1);
    newJob->command->setJobId(max_id+1);
    max_id++;
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (int i =1; i<=max_id ; ++i) {
        if (jobs[i] != nullptr) {
            //printf("got here!!");
            (jobs[i])->printJob();
        }
    }
}

void JobsList::killAllJobs(){} /// implement after signal handling

void JobsList::removeFinishedJobs() {
    bool setMaxFlag = false;
    for (int idx = JOBS_MAX_IDX; idx >= JOBS_MIN_IDX; idx--) {
        if(jobs[idx] == nullptr)
            continue;
        else {
            // cerr << "check before removeFinished waitpid: job id = " << idx << endl; /// delete
            // cout << "check before removeFinished waitpid: job id = " << idx << endl; /// delete
            pid_t wait_ret = waitpid(jobs[idx]->command->getPid(), nullptr, WNOHANG); //status?
            if (wait_ret == -1) {
                perror("smash error: waitpid failed");
                // cerr << "check: removeFinished waitpid failed! job id = " << idx << endl; /// delete
                // cout << "check: removeFinished waitpid failed! job id = " << idx << endl; /// delete
            }
            else if (wait_ret==0) { // child not finished
                if(setMaxFlag) {//if max_id needs to be re_set
                    max_id = idx;
                    setMaxFlag = false;
                }
                continue;
            }
            else { // child finished
                if (idx==max_id) {
                    setMaxFlag = true;
                    max_id = 0; /// after the loop - if (max_id==0) - joblist is empty
                }
                delete jobs[idx]->command;
                delete jobs[idx];
                jobs[idx] = nullptr;
            }
        }
    }
}

Job * JobsList::getJobById(int jobId){
    return jobs[jobId];
}

void JobsList::removeJobById(int jobId) {
    if(jobs[jobId] == nullptr) {
        return;
    }
    delete jobs[jobId]->command;
    delete jobs[jobId];
    jobs[jobId] = nullptr;
    if (max_id == jobId) {                      //update max id
        for (int idx = jobId; idx >= JOBS_MIN_IDX ; idx--){
            if (jobs[idx] != nullptr) {
                max_id = idx;
                break;
            }
            max_id =0;                          //will be 0 only if all jobs are null
        }
    }
}

//Job * JobsList::getLastJob(int* lastJobId){
//    return jobs[max_id];
//}

int JobsList::getMaxId() const {
    return max_id;
}


Job * JobsList::getLastStoppedJob()
{
    for (int idx = max_id; idx >= JOBS_MIN_IDX ; idx--) {
        if (jobs[idx] != nullptr && jobs[idx]->is_stopped)
            return jobs[idx];
    }
    return nullptr;
}



/** ***************** Command ***************** **/



Command::Command(const char *cmd_line) {
    ///dynamyc cast isBuiltIn
    /// omer 29/04 - debug:
    argv = (char**)malloc(MAX_LINE_WORDS*sizeof(char*));
    argc = _parseCommandLine(cmd_line, argv);
}

/// omer 29/04 - debug - added command d'tor:
Command::~Command() {
    for(int i=0; i<argc; i++) { ///omer 29/04 - not sure about number if deletions!
        free(argv[i]);
    }
    free(argv);
}



/** ***************** BuildInCommand ***************** **/



BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

bool BuiltInCommand::getBg() {
    return false;
}


ChangePoromptCommand::ChangePoromptCommand(const char *cmd_line): BuiltInCommand(cmd_line){
    if (argc >= 2)
        prompt = string(argv[1]);
    else
        prompt = "smash";
}

/// omer 29/04 - debug - added ChangePoromptCommand d'tor:
ChangePoromptCommand::~ChangePoromptCommand() {}


void ChangePoromptCommand::execute() {
    SmallShell::getInstance().setPrompt(prompt);
}


ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
    cout << "smash pid is " << getpid() << endl;
}


GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
    cout << SmallShell::getInstance().getCurWD() << endl;
}


ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), prev(plastPwd){}

void ChangeDirCommand::execute() {
    if (argc > 2){
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    int res;
    string prev_dir = SmallShell::getInstance().getPrevWD(); //save the prev dir
    if (string(argv[1]) == "-"){             ///cd with "-" arg
        if (prev_dir.empty()){
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        //chdir(prev_dir.c_str());
        res = chdir(prev_dir.c_str());
    }
    else{                                   ///cd with path arg
        //chdir(argv[1]);
        res = chdir(argv[1]);
    }
    if (res < 0){                            ///check chdir ret val
        perror("smash error: chdir failed");
        return;
    }
    SmallShell::getInstance().setPrevWD(SmallShell::getInstance().getCurWD()); //update prev dir to old curr dir
    SmallShell::getInstance().setCurWD(getcwd(nullptr,0)); //update curr dir
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs_ptr) : BuiltInCommand(cmd_line) , jobs(jobs_ptr){}

void QuitCommand::execute() {
    if ((argc > 1) and (string(argv[1]).compare("kill") == 0)) {
        int counter =0;
        for(int i =0 ;  i <= jobs->getMaxId() ; i++){
            if (jobs->getJobById(i) != nullptr){
                counter++;
            }
        }
        cout << "smash: sending SIGKILL signal to " << counter << " jobs:" << endl;
        for(int i =0 ;  i <= jobs->getMaxId() ; i++){
            if (jobs->getJobById(i) != nullptr){
                DO_SYS(kill(jobs->getJobById(i)->getCommand()->getPid(), SIGKILL));
                cout << jobs->getJobById(i)->getCommand()->getPid() << ": " << jobs->getJobById(i)->getCommand()->getCmd() << endl;
            }
        }
    }
    exit(0);
}


KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){}


void KillCommand::execute() {
    if (argc != 3){    //check correct amount of arguments
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig;
    try { sig = -stoi(string(argv[1])); }       //check the signal argument
    catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if((sig <= 0 || sig > 64) and !(sig >31 and sig <34)) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int j_idx;
    try { j_idx = stoi(string(argv[2])); }    //check the job id argument
    catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(j_idx <= 0 || j_idx > 100) {
        cerr << "smash error: kill: job-id " << j_idx << " does not exist" << endl;   ///or should it be job id doesn't exist?
        return;
    }
    Job* job = jobs->getJobById(j_idx);
    if (job == nullptr){           //check job exist
        cerr << "smash error: kill: job-id " << j_idx << " does not exist" << endl;
        return;
    }
    cout << "signal number " << sig << " was sent to pid " <<  job->getCommand()->getPid() << endl;
    DO_SYS(kill(job->getCommand()->getPid(), sig));
}


ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void ForegroundCommand::execute() {
    if(argc>2 ) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    int index = jobs->getMaxId();
    if(argc==2) {
        try { index = stoi(string(argv[1])); }
        catch (...) {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        if(index <= 0 || index > 100) {
            cerr << "smash error: fg: job-id " << argv[1] << " does not exist" << endl;
            return;
        }
    }
    else{
        if(index==0){
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    Job* job = jobs->getJobById(index);
    if (job == nullptr) {
        cerr << "smash error: fg: job-id " << argv[1] << " does not exist" << endl;
        return;
    }
    job->getCommand()->setBg(false);
    job->setIsStopped(false);
    cout << job->getCommand()->getCmd() << " : " << job->getCommand()->getPid() << endl;
    SmallShell::getInstance().setCurrCommand(job->getCommand());
    DO_SYS(kill(job->getCommand()->getPid(), SIGCONT));
    int status;
    DO_SYS(waitpid(job->getCommand()->getPid(), &status, WUNTRACED)); // WIFSTOPPED
    postWaitPid(job->getCommand());
    if(!(WIFSTOPPED(status))) {
        SmallShell::getInstance().jobs.removeJobById(job->getId());
    }
}


BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void BackgroundCommand::execute() {
    Job* cur_job = nullptr;
    int cur_idx = -1;
    if (argc > 2) {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    else if(argc == 2) {
        try { cur_idx = stoi(string(argv[1])); }
        catch (...) {
            cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }
        if(cur_idx <= 0 || cur_idx > 100) {
            cerr << "smash error: bg: job-id " << cur_idx << " does not exist" << endl;
            return;
        }
        else { //proper index in argv
            cur_job = jobs->getJobById(cur_idx);
            if(cur_job == nullptr) {
                cerr << "smash error: bg: job-id " << cur_idx << " does not exist" << endl;
                return;
            }
        }
    }
    else { // no job_id argument
        cur_job = jobs->getLastStoppedJob();
        if(cur_job == nullptr) {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        else {
            cur_idx = cur_job->getId();
        }
    }
    // do for both cases:
    if(cur_job->getIsStopped()) {
        cout << cur_job->getCommand()->getCmd() << " : " << cur_job->getCommand()->getPid() << endl;
        cur_job->setIsStopped(false);
        DO_SYS(kill(cur_job->getCommand()->getPid(), SIGCONT));
    }
    else { //command is not stopped!
        cerr << "smash error: bg: job-id " << cur_idx << " is already running in the background" << endl;
    }
}


JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_ptr(jobs) {}

void JobsCommand::execute() {
    jobs_ptr->printJobsList();
}





/** ***************** ExternalCommand ***************** **/




std::string ExternalCommand::getCmd() const{
    return orig_cmd;
}


ExternalCommand::ExternalCommand(const char* cmd_line, bool isBg, string orig_cmd) : Command(cmd_line), job_id(-1), isBg(isBg), orig_cmd(orig_cmd), process_pid(0) {}

bool ExternalCommand::getBg() {
    return isBg;
}

void ExternalCommand::setBg(bool Bg) {
    isBg = Bg;
}

pid_t ExternalCommand::getPid() {
    return process_pid;
}

void ExternalCommand::setPid(pid_t pid) {
    process_pid = pid;
}

void ExternalCommand::setJobId(int id) {
    job_id = id;
}

int ExternalCommand::getJobId() const {
    return job_id;
}


SimpleExternalCommand::SimpleExternalCommand(const char* cmd_line, bool isBg, std::string orig_cmd) : ExternalCommand(cmd_line, isBg, orig_cmd) {}

void ExternalCommand::execute()
{
    pid_t pid = fork(); // fork failed
    if (pid < 0) {
        perror("smash error: fork failed");
    }
    else if (pid == 0) { //child:
        setpgrp();
        //execvp(argv[0], &argv[1]);
        //cout << "check:" << argv[0]<<endl;
        //cout << "check:" << orig_cmd << endl;
        execParams();
        perror("smash error: execvp failed");
    }
    else { //parent:
        setPid(pid);
        if (isBg) { // background
            DO_SYS(waitpid(pid, nullptr, WNOHANG)); ///is this bg ok?
            SmallShell::getInstance().jobs.addJob(new Job(this));
        }
        else { // foreground
            SmallShell::getInstance().setCurrCommand(this);
            DO_SYS(waitpid(pid, nullptr, WUNTRACED)); /// OPTIONS was 0
            postWaitPid(this);
        }
    }
}


ComplexExternalCommand::ComplexExternalCommand(const char* cmd_line, bool isBg, std::string orig_cmd) : ExternalCommand(cmd_line, isBg, orig_cmd) {
    for(int i =0; i<argc; i++){
        free(argv[i]);
    }
    free(argv);
    argv = (char**)malloc(sizeof(char*)*4);
    argv[0] = (char*) malloc(sizeof(char)*5);
    strcpy(argv[0], "bash");
    argv[1] = (char*)malloc(sizeof(char)*3);
    strcpy(argv[1], "-c");
    argv[2] = (char*)malloc(sizeof(char)* (strlen(cmd_line)+1));
    strcpy(argv[2], cmd_line);
    argv[3] = NULL;
    argc = 3;
}


void ComplexExternalCommand::execParams() {
    DO_SYS(execvp("/bin/bash", argv));
}

void SimpleExternalCommand::execParams() {
    DO_SYS(execvp(argv[0], argv)); ///was &argv[1]
}


/** ***************** SpecialCommand ***************** **/

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
    string allCmdLine = cmd_line;
    calledCMD_str = allCmdLine.substr(0, allCmdLine.find('>'));
    calledCMD_charptr = calledCMD_str.c_str();
    string redirectionPart = _trim(allCmdLine.substr(allCmdLine.find('>')));
    redirectionType = (redirectionPart.find(">>")!=string::npos) ? ">>" : ">";
    fileName = _trim(redirectionPart.substr(redirectionPart.find_first_not_of('>'),
                                            redirectionPart.find('\n')));
    origSTDOUT = dup(STDOUT_FILENO);
}

void RedirectionCommand::execute() {
    int appendOrTruncFlag = (redirectionType == ">>") ? O_APPEND : O_TRUNC;
    int redirectFile = open(fileName.c_str(), /*flags*/ O_WRONLY | O_CREAT | appendOrTruncFlag, //append is on if
                            /*mode*/ /*S_IRWXU | S_IRWXG | S_IRWXO*/ 0655 ); /// -rw-r-xr-x
    if (redirectFile==SYSCALL_FAILED) { ///open check..?
        perror("smash error: waitpid failed");
    }
    DO_SYS(dup2(redirectFile, STDOUT_FD)); //channel stdout to file
    SmallShell::getInstance().executeCommand(calledCMD_charptr);
    DO_SYS(dup2(origSTDOUT, STDOUT_FD)); //channel stdout back to default
    DO_SYS(close(redirectFile));
}

bool RedirectionCommand::getBg() {
    return false;
}


PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line){
    string cmd_s = string(cmd_line);
    orig_rd = dup(READ);
    if (orig_wr == SYSCALL_FAILED)
        perror("smash error: dup failed");
    size_t sep_idx = cmd_s.find("|&");
    if (sep_idx == string::npos){
        sep_idx = cmd_s.find('|');
        is_err = false;
        orig_wr = dup(WRITE);
        if (orig_wr == SYSCALL_FAILED)
            perror("smash error: dup failed");
    }
    else {
        is_err = true;
        orig_wr = dup(ERR);
        if (orig_wr == SYSCALL_FAILED)
            perror("smash error: dup failed");
    }
    string temp_com1 = cmd_s.substr(0,sep_idx);
    //temp_com1 = temp_com1.append("\n");
    int len = 1;
    if (is_err)
        len = 2;
    string temp_com2 = cmd_s.substr(sep_idx+len, string::npos);
    com1 = new char[temp_com1.length()+1]; //&temp_com1[0];
    strcpy(com1, temp_com1.c_str());
    com2 = new char[temp_com2.length()+1];//&temp_com2[0];
    strcpy(com2, temp_com2.c_str());
    //cout << "pipe command com1 is: " << com1 << endl;
    //cout << "pipe command com2 is: " << com2 << endl;
    _removeBackgroundSign(com1);
    _removeBackgroundSign(com2);
    //cout << "pipe command com1 after is: " << com1 << endl;
    //cout << "pipe command com2 after is: " << com2 << endl;
    /** 273.8 Ina'al Abuk **/
}


void PipeCommand::execute()
{
    Command* command2 = SmallShell::getInstance().CreateCommand(com2);
    bool right_in = false;
    if (dynamic_cast<BuiltInCommand*>(command2) != nullptr){
        right_in = true;
    }
    delete command2;
    int FD[2];
    DO_SYS(pipe(FD));
    pid_t pid = fork();
    if (pid == SYSCALL_FAILED)
        perror("smash error: fork failed");
    if (pid == 0){   //son
        if (not right_in){ // external in right - left is father
            DO_SYS(close(FD[WRITE])); // close the write side of the pipe
            DO_SYS(dup2(FD[READ], READ));  //refer the pipe read to stdin
            DO_SYS(close(FD[READ]));
            SmallShell::getInstance().executeCommand(com2);
        }
        else{   //build in right - right is father
            DO_SYS(close(FD[READ])); // close the write side of the pipe
            if (is_err)
                DO_SYS(dup2(FD[WRITE], ERR));  //refer the pipe write to stdout
            else
                DO_SYS(dup2(FD[WRITE], WRITE));  //refer the pipe write to stdout
            DO_SYS(close(FD[WRITE]));
            SmallShell::getInstance().executeCommand(com1);
        }
        exit(0);
    }
    else {   //father
        if (right_in){ // built-in in right - right is father
            DO_SYS(close(FD[WRITE])); // close the write side of the pipe
            DO_SYS(dup2(FD[READ], READ));  //refer the pipe read to stdin
            DO_SYS(close(FD[READ]));
            SmallShell::getInstance().executeCommand(com2);
        }
        else {      //external in right - left is the father
            DO_SYS(close(FD[READ])); // close the write side of the pipe
            if (is_err)
                DO_SYS(dup2(FD[WRITE], ERR));  //refer the pipe write to stderr
            else
                DO_SYS(dup2(FD[WRITE], WRITE));  //refer the pipe write to stdout
            DO_SYS(close(FD[WRITE]));
            SmallShell::getInstance().executeCommand(com1);
        }
        /// gets stuck in waitpid!! but why?
        int status;
        if (is_err)
            DO_SYS(dup2(orig_wr, ERR));
        else
            DO_SYS(dup2(orig_wr, WRITE));
        DO_SYS(dup2(orig_rd, READ));
        DO_SYS(close(orig_wr));
        DO_SYS(close(orig_rd));
        //sleep(5); //check what  happens
        DO_SYS(waitpid(pid, &status, 0));
        delete[] com1;
        delete[] com2;
        //cout << "check!: status: " << status << ", and ret: " << ret <<endl;
    }
}

bool PipeCommand::getBg() {
    return false;
}


/// GetFileTypeCommand

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetFileTypeCommand::execute() {
    if(argc != 2) {
        cerr << "smash error: getfiletype: invalid aruments" << endl;
        return;
    }
    struct stat fileStat;
    if(lstat(argv[1], &fileStat) == -1) {
        perror("smash error: lstat failed");
        return;
    }
    string fileType;
    if (S_ISREG(fileStat.st_mode))
        fileType = "regular file";
    else if (S_ISDIR(fileStat.st_mode))
        fileType = "directory";
    else if (S_ISCHR(fileStat.st_mode))
        fileType = "character device";
    else if (S_ISBLK(fileStat.st_mode))
        fileType = "block device";
    else if (S_ISFIFO(fileStat.st_mode))
        fileType = "FIFO";
    else if (S_ISLNK(fileStat.st_mode))
        fileType = "symbolic link";
    else if (S_ISSOCK(fileStat.st_mode))
        fileType = "socket";
    ///else?
    cout << argv[1] << "'s type is \"" << fileType << "\" and takes up " << fileStat.st_size << " bytes" << endl;
}

///do we need to add setBg? think not..
//bool GetFileTypeCommand::getBg() {
//    return false;
//}

SetcoreCommand::SetcoreCommand(const char *cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void SetcoreCommand::execute() {
    if(argc != 3) {
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }
    int core_num;
    int id_num;
    try {
        core_num = stoi(string(argv[2]));
        id_num = stoi(string(argv[1]));
    }       //check the signal argument
    catch (...) {
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }
    Job* job = jobs->getJobById(id_num);
    if (job == nullptr){
        cerr << "smash error: setcore: job-id "<< id_num <<" does not exist" <<endl;
        return;
    }
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core_num, &set);
    int ret = sched_getaffinity(job->getCommand()->getPid(), sizeof(cpu_set_t), &set);
    if ( ret == -1) {
        std::cerr << "smash error: setcore: invalid core number "<< std::endl;
        return;
    }
}


ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChmodCommand::execute() {
    if(argc != 3) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }
    int fileMod;
    try { fileMod = stoi(string(argv[1]), nullptr, 8); }       //check the signal argument
    catch (...) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }
    if(fileMod < 00 || fileMod > 07777) { // 777 - Max permission num
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }
    if(chmod(argv[2],fileMod) == -1) {
        perror("smash error: chmod failed");
        return;
    }
//    if(fileMod & S_ISUID) {
//        return;
//    }
//
//    }
//    int GIDFlag = (fileMod & S_ISGID);
//    int VTXFlag = (fileMod & S_ISVTX);
}


/*
 * infinite loop command:

#include <iostream>
#include <chrono>
#include <thread>

int main() {
    while (true) {
        // Code or actions to be executed within the infinite loop
        //std::cout << "Running an infinite loop..." << std::endl;
        // Additional code here

        // Add a delay to avoid consuming excessive resources
        //std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}


 //COMPILING:
 //g++ -o infinite_loop infinite_loop.cpp

  * */
