#ifndef HLM_THREAD_H
#define HLM_THREAD_H

#include <string>
#include <thread>
#include <functional>

using namespace std;

class HlmThread {
public:
    HlmThread(const string& name, function<void()> process_func);

    void start();
    void stop();
    bool isRunning() const;

private:
    void run();

private:
    string thread_name_;
    function<void()> process_func_;
    thread thread_;
    bool running_;
};

#endif // HLM_THREAD_H
