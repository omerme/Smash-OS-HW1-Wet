#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

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

SmallShell::~SmallShell() {

}

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
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;

}

void SmallShell::executeCommand(const char *cmd_line) {
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

Command::Command(const char *cmd_line){
    ///dynamyc cast isBuiltIn
    argc = _parseCommandLine(cmd_line, argv);
}

BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line){

}


ChangePoromptCommand::ChangePoromptCommand(const char *cmd_line): BuiltInCommand(cmd_line){
    if (argc >= 2)
        prompt = string(argv[1]);
    else
        prompt = "smash";
}

void ChangePoromptCommand::execute() {
    SmallShell::getInstance().setPrompt(prompt);
}


ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
    cout << "pid is " << getpid();
}


GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
    cout << SmallShell::getInstance().getCurWD();
}


ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), prev(plastPwd){}

void ChangeDirCommand::execute() {
    if (argc > 2){
        cerr << "smash error: cd: too many arguments";
        return;
    }
    int res;
    string prev_dir = SmallShell::getInstance().getPrevWD(); //save the prev dir
    if (string(argv[1]) == "-"){             ///cd with "-" arg
        if (prev_dir.empty()){
            cerr << "smash error: cd: OLDPWD not set";
            return;
        }
        res = chdir(prev_dir.c_str());
    }
    else{                                   ///cd with path arg
        res = chdir(argv[1]);
    }
    if (res < 0){                            ///check chdir ret val
        perror("smash error: chdir failed");
        return;
    }
    SmallShell::getInstance().setPrevWD(SmallShell::getInstance().getCurWD()); //update prev dir to old curr dir
    SmallShell::getInstance().setCurWD(getcwd(nullptr,0)); //update curr dir
}

