#ifndef PROFILINGDATA_H
#define PROFILINGDATA_H

#include "utils.h"
#include <cassert>
#include <deque>
#include <ostream>
#include <unordered_map>

namespace emulator {

    class ThreadProfilingData {
    public:
        struct Event {
            enum class Type {
                CALL,
                RET,
            } type;
            u64 tick;
            u64 address;
        };

        ThreadProfilingData(int pid, int tid) : pid_(pid), tid_(tid) { }

        void addEvent(Event::Type type, u64 tick, u64 address) {
            events_.push_back(Event{type, tick, address});
        }

        int pid() const { return pid_; }
        int tid() const { return tid_; }

        size_t nbEvents() const { return events_.size(); }

        const Event& event(size_t i) const {
            assert(i < nbEvents());
            return events_[i];
        }

    private:
        int pid_;
        int tid_;
        std::deque<Event> events_;
    };

    class ProfilingSymbolTable {
    public:
        void add(u64 address, std::string symbol) {
            symbols_.emplace(address, std::move(symbol));
        }

        size_t size() const {
            return symbols_.size();
        }

        std::optional<std::string> findSymbol(u64 address) const {
            auto it = symbols_.find(address);
            if(it != symbols_.end()) {
                return it->second;
            } else {
                return {};
            }
        }

    private:
        std::unordered_map<u64, std::string> symbols_;
    };

    class ProfilingData {
    public:
        ThreadProfilingData& addThread(int pid, int tid) {
            threadProfilingData_.emplace_back(pid, tid);
            return threadProfilingData_.back();
        }

        void addSymbol(u64 address, std::string symbol) {
            symbolTable_.add(address, std::move(symbol));   
        }

        size_t nbThreads() const {
            return threadProfilingData_.size();
        }

        const ThreadProfilingData& threadData(size_t i) const {
            assert(i < nbThreads());
            return threadProfilingData_[i];
        }

        const ProfilingSymbolTable& symbolTable() const {
            return symbolTable_;
        }

        void toJson(std::ostream& os) const;


    private:
        std::deque<ThreadProfilingData> threadProfilingData_;
        ProfilingSymbolTable symbolTable_;
    };

}

#endif