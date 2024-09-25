#ifndef PROFILINGDATA_H
#define PROFILINGDATA_H

#include "utils.h"
#include <cassert>
#include <deque>
#include <memory>
#include <optional>
#include <ostream>
#include <unordered_map>

namespace profiling {

    class ThreadProfilingData {
    public:
        struct CallEvent {
            u64 tick;
            u64 address;
        };

        struct RetEvent {
            u64 tick;
        };

        ThreadProfilingData(int pid, int tid) : pid_(pid), tid_(tid) { }

        void addCallEvent(u64 tick, u64 address) {
            callEvents_.push_back(CallEvent{tick, address});
        }

        void addRetEvent(u64 tick) {
            retEvents_.push_back(RetEvent{tick});
        }

        int pid() const { return pid_; }
        int tid() const { return tid_; }

        size_t nbCallEvents() const { return callEvents_.size(); }
        size_t nbRetEvents() const { return retEvents_.size(); }

        template<typename Func>
        void forEachCallEvent(Func&& func) const {
            for(const auto& event : callEvents_) func(event);
        }

        template<typename Func>
        void forEachRetEvent(Func&& func) const {
            for(const auto& event : retEvents_) func(event);
        }

        size_t largestCallTickDifference() const;
        void analyzeCallTickDifference() const;

    private:
        int pid_;
        int tid_;
        std::deque<CallEvent> callEvents_;
        std::deque<RetEvent> retEvents_;
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
        static std::unique_ptr<ProfilingData> tryCreateFromJson(std::istream& is);
        static std::unique_ptr<ProfilingData> tryCreateFromBin(std::istream& is);

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
        void toBin(std::ostream& os) const;
        void toChallenge(std::string) const;

    private:
        std::deque<ThreadProfilingData> threadProfilingData_;
        ProfilingSymbolTable symbolTable_;
    };

}

#endif