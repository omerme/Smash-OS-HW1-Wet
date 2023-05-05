#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"


using namespace std;

#define DO_SYS( syscall ) do { \
            if( (syscall) == -1) { \
                string syscallStr = (string)#syscall;                \
                string s = "smash error: " + syscallStr.substr(0, syscallStr.find('(')) + " failed";            \
                perror ( s.c_str() ); \
                exit(1);               \
            }                      \
        } while(0)

const std::string WHITESPACE = " \n\r\t\f\v";
/// omer 29/04 - debug
//const std::vector<string> builtInCommands = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "bg", "quit", "kill"};
const int MAX_PARAMS = 20;
const int MAX_LINE_WORDS = MAX_PARAMS + 3; /// plus command-word, &, and nullptr
//0       1   2       20    21  --> len = 22 total
//sleep arg1 arg2 .. arg20  &

/** when adding a command:
 * 1. implement c'tor, d'tor and execute
 * 2. add command name to SmallShell::CreateCommand
 * 3. if built-in - add:  _removeBackgroundSign(cmd_line);
 * 4. add to SmallShell::execute (if needed)
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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : prompt("smash"), prevWD()
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

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(char* cmd_line) {
	/// add more?
	string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    bool isbg = _isBackgroundComamnd(cmd_line);
    string orig_cmd = cmd_line;
    _removeBackgroundSign(cmd_line);
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
        return new QuitCommand(cmd_line, nullptr); /// for jobs: change nullptr to joblist
    }
    else if (firstWord.compare("") == 0) {
        cout << "oops! empty line!";
    }
    else { //external

        if (strchr(cmd_line, '*') || strchr(cmd_line, '?'))
            return new ComplexExternalCommand(cmd_line, isbg);
        return new SimpleExternalCommand(cmd_line, isbg);
    }
    return nullptr;
}

/// need to debug - something caused segfault (we played
void SmallShell::executeCommand(const char *cmd_line_in)
{
    char *cmd_line = new char[strlen(cmd_line_in)+1]();
    strcpy(cmd_line, cmd_line_in);
    ///if built-in:
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    if(true) { /// if built-in
        delete cmd;
    }
    delete[] cmd_line;
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


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}


ExternalCommand::ExternalCommand(const char* cmd_line, bool isBg) : Command(cmd_line), isBg(isBg){
}

SimpleExternalCommand::SimpleExternalCommand(const char* cmd_line, bool isBg) : ExternalCommand(cmd_line, isBg) {}

void ExternalCommand::execute()
{
    pid_t pid = fork(); // fork failed
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) { //child:
        setpgrp();
        //execvp(argv[0], &argv[1]);
        execParams();
        perror("smash error: execvp failed");
    } else { //parent:
        if (isBg) { // background - what?
            DO_SYS(waitpid(pid, nullptr, WNOHANG)); ///is this bg ok?
        }
        else { // foreground
            DO_SYS(waitpid(pid, nullptr, 0)); /// wstatus?
        }
    }
}


ComplexExternalCommand::ComplexExternalCommand(const char* cmd_line, bool isBg) : ExternalCommand(cmd_line, isBg) {
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
    //int res;
    string prev_dir = SmallShell::getInstance().getPrevWD(); //save the prev dir
    if (string(argv[1]) == "-"){             ///cd with "-" arg
        if (prev_dir.empty()){
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        DO_SYS(chdir(prev_dir.c_str()));
        //res = chdir(prev_dir.c_str());
    }
    else{                                   ///cd with path arg
        DO_SYS(chdir(argv[1]));
        //res = chdir(argv[1]);
    }/*
    if (res < 0){                            ///check chdir ret val
        //perror("smash error: chdir failed");
        return;
    }*/
    SmallShell::getInstance().setPrevWD(SmallShell::getInstance().getCurWD()); //update prev dir to old curr dir
    SmallShell::getInstance().setCurWD(getcwd(nullptr,0)); //update curr dir
}


QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {
    jobs = nullptr; /// omer 29/04 - debug - just for now..
}

void QuitCommand::execute() {
    exit(0);
}

Job(const char* cmd_line, int job_id) {

}
~Job();

