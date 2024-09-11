#include "emulator/profilingdata.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <vector>

namespace emulator {

    void ProfilingData::toJson(std::ostream& os) const {
        using json = nlohmann::json;

        auto eventTypeToString = [](ThreadProfilingData::Event::Type type) -> std::string {
            switch(type) {
                case ThreadProfilingData::Event::Type::CALL: return "call";
                case ThreadProfilingData::Event::Type::RET:  return "ret";
            }
            return "";
        };

        json profileObject = json::array();
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            json threadObject {
                { "pid", tpd.pid() },
                { "tid", tpd.tid() }
            };
            json events = json::array();
            for(size_t i = 0; i < tpd.nbEvents(); ++i) {
                const ThreadProfilingData::Event& e = tpd.event(i);
                json event = json::array({ e.tick, eventTypeToString(e.type), e.address });
                events.emplace_back(std::move(event));
            }
            threadObject["events"] = std::move(events);
            profileObject.emplace_back(std::move(threadObject));
        }
        
        std::vector<u64> addresses;
        addresses.reserve(std::accumulate(threadProfilingData_.begin(), threadProfilingData_.end(), 0,
            [](size_t size, const ThreadProfilingData& tpd) {
                return size + tpd.nbEvents();
            }));
        for(const ThreadProfilingData& tpd : threadProfilingData_) {
            for(size_t i = 0; i < tpd.nbEvents(); ++i) {
                addresses.push_back(tpd.event(i).address);
            }
        }
        std::sort(addresses.begin(), addresses.end());
        addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());

        json symbolsObject = json::array();
        for(u64 address : addresses) {
            std::string symbolOrEmpty = symbolTable_.findSymbol(address).value_or("???");
            json symbol = json::array();
            symbol.push_back(address);
            symbol.push_back(std::move(symbolOrEmpty));
            symbolsObject.push_back(std::move(symbol));
        }

        json globalObject {
            { "eventItems", json::array({ "tick", "type", "address"}) },
            { "events", std::move(profileObject) },
            { "symbols", std::move(symbolsObject) }
        };

        os << globalObject;
    }

}