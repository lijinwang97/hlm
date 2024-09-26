#ifndef HLM_THREAD_H
#define HLM_THREAD_H

#include <string>
#include <thread>
#include <functional>

class HlmThread {
public:
    HlmThread(const std::string& name, std::function<void()> process_func);

    void start();
    void stop();
    bool isRunning() const;

private:
    void run();

private:
    std::string thread_name_;
    std::function<void()> process_func_;
    std::thread thread_;
    bool running_;
};

#endif // HLM_THREAD_H
