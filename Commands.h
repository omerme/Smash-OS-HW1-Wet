#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
//#define MAX_PARAMS 20

/** job TODO:
 * Job:
 *      * c'tor
 *      * d'tor
 * JobsList:
 *      * create vector size of 101 (0->null ptr, min_idx=1, max_idx=100)
 *          - inn  the beginning - all nullptr, max_id = 0
 *          - max_id = 0 <==> JobsList isEmpty
 *          - job_not_exist <==> vec[idx] = nullptr;
 *          - when deleting a job - *) go from end backwards
 *                                  1) first free command and job
 *                                  2) then change vec[idx] to nullptr
 *                                  3) if job_id==max_id: update max_id
 *                                  4) keep going backwards
 *      * all methods
 * **/

class Command {
protected:
    char** argv;
    int argc;
 public:
    explicit Command(const char* cmd_line);
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
    explicit BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
protected:
    bool isBg; ///may need to update to job bg or fg
public:
    explicit ExternalCommand(const char* cmd_line, bool isBg);
    virtual ~ExternalCommand() {}
    void execute() override; /// keep it?
    virtual void execParams() = 0;
    // do we need child list?
};

class SimpleExternalCommand : public ExternalCommand {
public:
    explicit SimpleExternalCommand(const char* cmd_line, bool isBg);
    virtual ~SimpleExternalCommand() {}
    void execParams() override;

    // do we need child list?
};

class ComplexExternalCommand : public ExternalCommand {
public:
    explicit ComplexExternalCommand(const char* cmd_line, bool isBg);
    virtual ~ComplexExternalCommand() {}
    void execParams() override;
    // do we need child list?
};

class PipeCommand : public Command {
    // TODO: Add your data members
 public:
    explicit PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
    explicit RedirectionCommand(const char* cmd_line); /// here there was an wxplicit;
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
    char** prev;
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class ChangePoromptCommand : public BuiltInCommand {
    std::string prompt;
public:
    ChangePoromptCommand(const char* cmd_line);
    virtual ~ChangePoromptCommand();
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    /// omer 29/04 - debug - added for now:

// TODO: Add your data members
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class Job {
private:
    Command* command;
    int job_id;
    std::string orig_cmd_line;
    bool is_stopped;
    //bool is_finished;
    int time_added;
    pid_t process_pid;
public:
    Job(const char* cmd_line, int job_id);
    ~Job();
};


class JobsList {
    int max_id;
    // TODO: Add your data members
 public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    Job * getJobById(int jobId);
    void removeJobById(int jobId);
    Job * getLastJob(int* lastJobId);
    Job *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class ChmodCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    ChmodCommand(const char* cmd_line);
    virtual ~ChmodCommand() {}
    void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
    GetFileTypeCommand(const char* cmd_line);
    virtual ~GetFileTypeCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    SetcoreCommand(const char* cmd_line);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
 public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class SmallShell {
 private:
    // TODO: Add your data members
    std::string prompt; //for chprompt
    std::string curWD;
    std::string prevWD;
    /// add job list
    /// add fg job

    SmallShell();
 public:
    Command *CreateCommand(char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    void changePrompt(std::string new_prompt); //for chprompt

    ///added methods:
    std::string getCurWD() const;
    void setCurWD(std::string newCurWD);
    std::string getPrevWD() const;
    void setPrevWD(std::string newPrevWD);
    std::string getPrompt() const; //for chprompt
    void setPrompt(std::string newPrompt); //for chprompt

};

#endif //SMASH_COMMAND_H_
