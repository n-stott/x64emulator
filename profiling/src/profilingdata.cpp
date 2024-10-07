#include "profilingdata.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <vector>

namespace profiling {

    size_t ThreadProfilingData::largestCallTickDifference() const {
        size_t maxDiff = 0;
        for(size_t i = 1; i < callEvents_.size(); ++i) {
            size_t diff = callEvents_[i].tick - callEvents_[i-1].tick;
            maxDiff = std::max(maxDiff, diff);
        }
        return maxDiff;
    }

    void ThreadProfilingData::analyzeCallTickDifference() const {
        size_t differenceFitsIn8bits = 0;
        size_t differenceFitsIn16bits = 0;
        size_t differenceFitsIn32bits = 0;
        size_t differenceFitsIn64bits = 0;
        for(size_t i = 1; i < callEvents_.size(); ++i) {
            size_t diff = callEvents_[i].tick - callEvents_[i-1].tick;
            differenceFitsIn8bits += (diff < (size_t)std::numeric_limits<u8>::max());
            differenceFitsIn16bits += (diff < (size_t)std::numeric_limits<u16>::max());
            differenceFitsIn32bits += (diff < (size_t)std::numeric_limits<u32>::max());
            differenceFitsIn64bits += 1;
        }
        fmt::print(">= 64 : {} ({})\n", differenceFitsIn64bits, 10000 * differenceFitsIn64bits / differenceFitsIn64bits);
        fmt::print(">= 32 : {} ({})\n", differenceFitsIn32bits, 10000 * differenceFitsIn32bits / differenceFitsIn64bits);
        fmt::print(">= 16 : {} ({})\n", differenceFitsIn16bits, 10000 * differenceFitsIn16bits / differenceFitsIn64bits);
        fmt::print(">= 8  : {} ({})\n", differenceFitsIn8bits, 10000 * differenceFitsIn8bits / differenceFitsIn64bits);

        size_t compressedSize = differenceFitsIn8bits
                + 2 * (differenceFitsIn16bits - differenceFitsIn8bits)
                + 4 * (differenceFitsIn32bits - differenceFitsIn16bits)
                + 8 * (differenceFitsIn64bits - differenceFitsIn32bits);

        size_t rawSize = 8 * differenceFitsIn64bits;
        fmt::print("Raw        size : {}B\n", rawSize);
        fmt::print("Compressed size : {}B\n", compressedSize);
    }

    void ProfilingData::toJson(std::ostream& os) const {
        using json = nlohmann::json;

        json threadsObject = json::array();
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            json threadObject {
                { "pid", tpd.pid() },
                { "tid", tpd.tid() }
            };
            json callEvents = json::array();
            tpd.forEachCallEvent([&](const auto& e) {
                json event = json::array({ e.tick, e.address });
                callEvents.emplace_back(std::move(event));
            });
            threadObject["callEvents"] = std::move(callEvents);
            json retEvents = json::array();
            tpd.forEachRetEvent([&](const auto& e) {
                retEvents.emplace_back(e.tick);
            });
            threadObject["retEvents"] = std::move(retEvents);
            json syscallEvents = json::array();
            tpd.forEachSyscallEvent([&](const auto& e) {
                json event = json::array({ e.tick, e.syscallNumber });
                syscallEvents.emplace_back(std::move(event));
            });
            threadObject["syscallEvents"] = std::move(syscallEvents);
            threadsObject.emplace_back(std::move(threadObject));
        }
        
        std::unordered_set<u64> addresses;
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            tpd.forEachCallEvent([&](const auto& e) {
                addresses.insert(e.address);
            });
        }

        json symbolsObject = json::array();
        for(u64 address : addresses) {
            std::string symbolOrEmpty = symbolTable_.findSymbol(address).value_or("???");
            json symbol = json::array();
            symbol.push_back(address);
            symbol.push_back(std::move(symbolOrEmpty));
            symbolsObject.push_back(std::move(symbol));
        }

        json globalObject {
            { "threads", std::move(threadsObject) },
            { "symbols", std::move(symbolsObject) }
        };

        os << globalObject;
    }

    template<typename T>
    void to_binary_stream(std::ostream& os, T value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void ProfilingData::toBin(std::ostream& os) const {

        fmt::print("Retrieved profiling data from {} threads\n", nbThreads());
        for(size_t i = 0; i < nbThreads(); ++i) {
            const ThreadProfilingData& threadProfilingData = threadData(i);
            fmt::print("  [{}:{}] Retrieved {} call events\n", threadProfilingData.pid(), threadProfilingData.tid(), threadProfilingData.nbCallEvents());
            fmt::print("  [{}:{}] Retrieved {} ret  events\n", threadProfilingData.pid(), threadProfilingData.tid(), threadProfilingData.nbRetEvents());

            threadProfilingData.analyzeCallTickDifference();
        }
        fmt::print("Retrieved {} symbols\n", symbolTable().size());

        std::unordered_set<u64> addresses;
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            fmt::print("Max tick diff for thread {}:{} is {}\n", tpd.pid(), tpd.tid(), tpd.largestCallTickDifference());
            tpd.forEachCallEvent([&](const auto& e) {
                addresses.insert(e.address);
            });
        }

        bool compressTo32bits = true;
        auto maxAddress = std::max_element(addresses.begin(), addresses.end());
        if(maxAddress != addresses.end()) {
            fmt::print("Highest address is {}\n", *maxAddress);
            compressTo32bits &= (*maxAddress < (u64)std::numeric_limits<u32>::max());
        }

        fmt::print("compressTo32bits ? : {}\n", compressTo32bits);

        to_binary_stream(os, threadProfilingData_.size());
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            to_binary_stream(os, sizeof(tpd.pid()) << tpd.pid());
            to_binary_stream(os, sizeof(tpd.tid()) << tpd.tid());
            to_binary_stream(os, tpd.nbCallEvents());
            to_binary_stream(os, sizeof(ThreadProfilingData::CallEvent::tick));
            to_binary_stream(os, sizeof(ThreadProfilingData::CallEvent::address));
            tpd.forEachCallEvent([&](const auto& e) {
                if(compressTo32bits) {
                    to_binary_stream(os, (u32)e.tick);
                    to_binary_stream(os, (u32)e.address);
                } else {
                    to_binary_stream(os, e.tick);
                    to_binary_stream(os, e.address);
                }
            });
            to_binary_stream(os, tpd.nbRetEvents());
            to_binary_stream(os, sizeof(ThreadProfilingData::RetEvent::tick));
            tpd.forEachRetEvent([&](const auto& e) {
                if(compressTo32bits) {
                    to_binary_stream(os, (u32)e.tick);
                } else {
                    to_binary_stream(os, e.tick);
                }
            });
        }

        to_binary_stream(os, addresses.size());
        for(u64 address : addresses) {
            to_binary_stream(os, address);
            std::string symbolOrEmpty = symbolTable_.findSymbol(address).value_or("???");
            to_binary_stream(os, symbolOrEmpty.size());
            to_binary_stream(os, symbolOrEmpty);
        }
    }

    void ProfilingData::toChallenge(std::string name) const {
        const ThreadProfilingData* biggestThread = nullptr;
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            if(biggestThread == nullptr) {
                biggestThread = &tpd;
                continue;
            }
            if(tpd.nbCallEvents() > biggestThread->nbCallEvents()) {
                biggestThread = &tpd;
                continue;
            }
        };
        if(!biggestThread) return;
        
        std::unordered_set<u64> uniqueAddresses;
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            tpd.forEachCallEvent([&](const auto& e) {
                uniqueAddresses.insert(e.address);
            });
        }

        std::vector<u64> sortedAddresses(uniqueAddresses.begin(), uniqueAddresses.end());
        std::sort(sortedAddresses.begin(), sortedAddresses.end());

        std::unordered_map<u64, u64> addressToIndex;
        for(size_t i = 0; i < sortedAddresses.size(); ++i) {
            addressToIndex[sortedAddresses[i]] = i;
        }

        if(addressToIndex.size() < 256) return;


        name = fmt::format("{}_{}_{}", biggestThread->nbCallEvents(), addressToIndex.size(), name);
        std::ofstream os(name);

        os << biggestThread->nbCallEvents() << ' ' << addressToIndex.size() << '\n';

        biggestThread->forEachCallEvent([&](const auto& e) {
            os << addressToIndex[e.address] << '\n';
        });
    }


    std::unique_ptr<ProfilingData> ProfilingData::tryCreateFromJson(std::istream& is) {
        using json = nlohmann::json;
        json data = json::parse(is);

        // we need at least this top level structure
        if(!data.is_object()) return {};
        if(!data.contains("threads")) return {};
        if(!data.contains("symbols")) return {};

        ProfilingData pd;

        // read events
        for(const auto& threadData : data["threads"]) {
            assert(threadData.is_object());
            assert(threadData.contains("pid"));
            assert(threadData.contains("tid"));
            assert(threadData.contains("callEvents"));
            assert(threadData.contains("retEvents"));
            int pid = threadData["pid"];
            int tid = threadData["tid"];

            ThreadProfilingData& tpd = pd.addThread(pid, tid);
            for(const auto& event : threadData["callEvents"]) {
                u64 tick = event[0];
                u64 address = event[1];
                tpd.addCallEvent(tick, address);
            }
            for(const auto& event : threadData["retEvents"]) {
                u64 tick = event;
                tpd.addRetEvent(tick);
            }
            for(const auto& event : threadData["syscallEvents"]) {
                u64 tick = event[0];
                u64 syscallNumber = event[1];
                tpd.addSyscallEvent(tick, syscallNumber);
            }
        }

        // read symbols
        const json& symbolsObject = data["symbols"];
        assert(symbolsObject.is_array());
        for(const auto& symbolEntry : symbolsObject) {
            assert(symbolEntry.is_array());
            assert(symbolEntry.size() == 2);
            assert(symbolEntry[0].is_number_unsigned());
            assert(symbolEntry[1].is_string());
            u64 address = symbolEntry[0];
            std::string symbol = symbolEntry[1];

            pd.addSymbol(address, std::move(symbol));
        }
        return std::make_unique<ProfilingData>(std::move(pd));
    }

    
    std::unique_ptr<ProfilingData> ProfilingData::tryCreateFromBin(std::istream& is) {
        (void)is;
        return {};
    }

}