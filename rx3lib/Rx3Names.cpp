#include "Rx3Names.h"

using namespace rx3utils;

vector<pair<uint32_t, string>> ExtractNamesFromChunk(Rx3Chunk const *namesChunk) {
    vector<pair<uint32_t, string>> result;
    Rx3Reader reader(namesChunk);
    reader.Skip(4);
    uint32_t numNames = reader.Read<uint32_t>();
    reader.Skip(8);
    for (size_t i = 0; i < numNames; i++) {
        uint32_t id = reader.Read<uint32_t>();
        uint32_t nameLength = reader.Read<uint32_t>();
        result.emplace_back(id, reader.GetString());
        reader.Skip(nameLength);
    }
    return result;
}

vector<pair<uint32_t, string>> ExtractNamesFromChunk(Rx3Chunk const &namesChunk) {
    return ExtractNamesFromChunk(&namesChunk);
}

vector<pair<uint32_t, string>> ExtractNamesFromRx3(Rx3Container &rx3) {
    Rx3Chunk *namesChunk = rx3.FindFirstChunk(RX3_CHUNK_NAME_TABLE);
    if (namesChunk)
        return ExtractNamesFromChunk(namesChunk);
    return {};
}

vector<string> ExtractNamesFromRx3(Rx3Container &rx3, uint32_t type) {
    auto typeAndName = ExtractNamesFromRx3(rx3);
    vector<string> result;
    for (auto const &[nameType, name] : typeAndName) {
        if (nameType == type)
            result.push_back(name);
    }
    return result;
}

Rx3Chunk &AddNamesChunkToRx3(Rx3Container &rx3, vector<pair<uint32_t, string>> const &names) {
    auto &namesChunk = rx3.AddChunk(RX3_CHUNK_NAME_TABLE);
    Rx3Writer namesWriter(namesChunk);
    namesWriter.Put<uint32_t>(0);
    namesWriter.Put<uint32_t>(names.size());
    namesWriter.Align();
    for (auto const &[type, name] : names) {
        namesWriter.Put<uint32_t>(type);
        namesWriter.Put<uint32_t>(name.size() + 1);
        namesWriter.Put(name.c_str(), name.size() + 1);
    }
    namesWriter.AlignAndUpdateTotalSize();
    return namesChunk;
}

Rx3Chunk &AddNamesChunkToRx3(Rx3Container &rx3, vector<string> const &names, uint32_t type) {
    vector<pair<uint32_t, string>> typeAndName;
    for (size_t i = 0; i < names.size(); i++)
        typeAndName.emplace_back(type, names[i]);
    return AddNamesChunkToRx3(rx3, typeAndName);
}
