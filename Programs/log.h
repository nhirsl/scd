/* based on http://www.drdobbs.com/cpp/logging-in-c/201804215?pgno=1 .*/

#ifndef __LOG_H__
#define __LOG_H__

#include <sstream>
#include <string>
#include <stdio.h>

enum LogLevel {ERROR, DEBUG};

class Log
{
public:
    Log() {}
    virtual ~Log()
    {
        os << std::endl;
        fprintf(stderr, "%s", os.str().c_str());
        fflush(stderr);
    }
    std::ostringstream& Get(LogLevel level = DEBUG)
    {
        os << time(0);
        os << " " << ToString(level) << ": ";
        return os;
    }
public:
    static LogLevel& LLevel()
    {
        static LogLevel llevel = DEBUG;
        return llevel;
    }
    static std::string ToString(LogLevel level)
    {
        static std::string const buffer[] = {"ERROR", "DEBUG"};
        return buffer[level];
    }
    static LogLevel FromString(const std::string& level);
protected:
    std::ostringstream os;
private:
    Log(const Log&);
    Log& operator =(const Log&);
};

#define LOGDEBUG() \
    if (DEBUG > Log::LLevel()) ; \
    else Log().Get(DEBUG) << "[" << __FILE__ << " (" << __LINE__ << ")] "

#define LOGERR() \
    if (ERROR > Log::LLevel()) ; \
    else Log().Get(ERROR) << "[" << __FILE__ << " (" << __LINE__ << ")] "

#endif
