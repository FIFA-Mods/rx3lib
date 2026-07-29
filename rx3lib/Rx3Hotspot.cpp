#include "Rx3Hotspot.h"
#include <fstream>
#include <unordered_map>
#include "nlohmann/json.hpp"

using namespace rx3utils;
using json = nlohmann::ordered_json;

void ExtractHotspotFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options) {
    auto hotspotChunk = container.FindFirstChunk(RX3_CHUNK_HOTSPOT);
    if (hotspotChunk) {
        Rx3Reader hotspotReader(hotspotChunk);
        hotspotReader.Skip(4);
        uint8_t version = hotspotReader.Read<uint8_t>();
        if (version == 1) {
            uint8_t numGroups = hotspotReader.Read<uint8_t>();
            hotspotReader.Skip(10);
            struct HotspotEntry {
                string name, group;
                float bounds[4];
            };
            vector<HotspotEntry> hotspots;
            for (size_t groupIndex = 0; groupIndex < numGroups; groupIndex++) {
                string groupName = hotspotReader.ReadString();
                uint8_t numHotspots = hotspotReader.Read<uint8_t>();
                for (size_t hotspotIndex = 0; hotspotIndex < numHotspots; hotspotIndex++) {
                    auto &hotspot = hotspots.emplace_back();
                    hotspot.group = groupName;
                    hotspot.name = hotspotReader.ReadString();
                    for (size_t bound = 0; bound < 4; bound++)
                        hotspot.bounds[bound] = hotspotReader.Read<float>();
                }
            }
            json j;
            j["AssetName"] = container.mName;
            j["HotspotFormatVersion"] = (int)version;
            if (!hotspots.empty()) {
                json arr = json::array();
                for (auto const &h : hotspots) {
                    json entry;
                    entry["Name"] = h.name;
                    entry["Group"] = h.group;
                    entry["Bounds"] = {
                        {"X", h.bounds[0]},
                        {"Y", h.bounds[1]},
                        {"Z", h.bounds[2]},
                        {"W", h.bounds[3]}
                    };
                    arr.push_back(move(entry));
                }
                j["Hotspots"] = move(arr);
            }
            ofstream outHotspot(outputDir / (container.mName + ".hotspot"));
            outHotspot << j.dump(2);
        }
    }
}

void ImportHotspotToRX3(Rx3Container &container, path const &hotspotFile, Rx3Options const &rx3options) {
    if (hotspotFile.empty() || !exists(hotspotFile))
        return;
    ifstream inHotspot(hotspotFile);
    if (!inHotspot.is_open())
        return;
    json root;
    try {
        inHotspot >> root;
    }
    catch (json::parse_error const &) {
        return;
    }
    struct HotspotEntryData {
        string name;
        float bounds[4] = { 0.f, 0.f, 0.f, 0.f };
    };
    struct HotspotGroupData {
        string name;
        vector<HotspotEntryData> hotspots;
    };
    vector<HotspotGroupData> groups;
    unordered_map<string, size_t> groupLookup;
    if (root.contains("Hotspots") && root["Hotspots"].is_array()) {
        for (auto const &hotspotJson : root["Hotspots"]) {
            HotspotEntryData entry;
            entry.name = hotspotJson.value("Name", string());
            string groupName = hotspotJson.value("Group", string());
            if (hotspotJson.contains("Bounds")) {
                auto const &bounds = hotspotJson["Bounds"];
                entry.bounds[0] = bounds.value("X", 0.f);
                entry.bounds[1] = bounds.value("Y", 0.f);
                entry.bounds[2] = bounds.value("Z", 0.f);
                entry.bounds[3] = bounds.value("W", 0.f);
            }
            auto [it, inserted] = groupLookup.try_emplace(groupName, groups.size());
            if (inserted)
                groups.push_back(HotspotGroupData{ groupName, {} });
            groups[it->second].hotspots.push_back(move(entry));
        }
    }
    if (groups.size() > 255) {
        //throw runtime_error("Too many hotspot groups (max 255): " + hotspotFile.string());
        groups.resize(255);
    }
    for (auto &g : groups) {
        if (g.hotspots.size() > 255) {
            //throw runtime_error("Too many hotspots in group \"" + g.name + "\" (max 255): " + hotspotFile.string());
            g.hotspots.resize(255);
        }
    }
    Rx3Chunk &hotspotChunk = container.AddChunk(RX3_CHUNK_HOTSPOT);
    Rx3Writer hotspotWriter(hotspotChunk);
    hotspotWriter.Put<uint32_t>(0);
    hotspotWriter.Put<uint8_t>(1);
    hotspotWriter.Put<uint8_t>((uint8_t)groups.size());
    for (int i = 0; i < 10; i++)
        hotspotWriter.Put<uint8_t>(0);
    for (auto const &group : groups) {
        hotspotWriter.Put(group.name);
        hotspotWriter.Put<uint8_t>((uint8_t)group.hotspots.size());
        for (auto const &hotspot : group.hotspots) {
            hotspotWriter.Put(hotspot.name);
            for (float b : hotspot.bounds)
                hotspotWriter.Put<float>(b);
        }
    }
    hotspotWriter.AlignAndUpdateTotalSize();
}
