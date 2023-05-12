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
    virtual bool getBg() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
    explicit BuiltInCommand(const char* cmd_line);
    bool getBg() override;
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
protected:
    int job_id; /// -1 if new in fg, when inserted to list - change it to idx in joblist.
    bool isBg; ///may need to update to job bg or fg
    std::string orig_cmd;
    pid_t process_pid;
public:
    explicit ExternalCommand(const char* cmd_line, bool isBg, std::string orig_cmd);
    virtual ~ExternalCommand() {}
    void execute() override; /// keep it?
    virtual void execParams() = 0;
    bool getBg() override;
    void setBg(bool Bg) ;
    pid_t getPid();
    void setPid(pid_t pid) ;
    std::string getCmd() const;
    void setJobId(int id);
    int getJobId() const;

    // do we need child list?
};

class SimpleExternalCommand : public ExternalCommand {
public:
    explicit SimpleExternalCommand(const char* cmd_line, bool isBg, std::string orig_cmd);
    virtual ~SimpleExternalCommand() {}
    void execParams() override;

    // do we need child list?
};

class ComplexExternalCommand : public ExternalCommand {
public:
    explicit ComplexExternalCommand(const char* cmd_line, bool isBg, std::string orig_cmd);
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
    friend JobsList;
    ExternalCommand* command;
    int job_id;
    //std::string orig_cmd_line;
    bool is_stopped;
    //bool is_finished;
    time_t time_added;
    //pid_t process_pid;
public:
    Job(ExternalCommand* command, int job_id =0, bool is_stopped = false);
    ~Job() = default;
    int getId();
    void setId(int job_id);
    bool getIsStopped() const;
    void setIsStopped(bool isStopped);
    void printJob();


};


class JobsList {
    int max_id;
    std::vector<Job*> jobs;
    // TODO: Add your data members
 public:
    JobsList();
    ~JobsList() = default;
    void addJob(Job* newJob);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    Job * getJobById(int jobId);
    void removeJobById(int jobId);
    //Job * getLastJob(int* lastJobId);
    int getMaxId() const;
    Job *getLastStoppedJob();
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
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
public:
    JobsList jobs;
private:
    // TODO: Add your data members
    std::string prompt; //for chprompt
    std::string curWD;
    std::string prevWD;
    ExternalCommand* curr_command;
    SmallShell();
public:
    bool sigC;
    bool sigZ;
    bool sigAlarm;
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
    ExternalCommand* getCurrCommand();

};

#endif //SMASH_COMMAND_H_
