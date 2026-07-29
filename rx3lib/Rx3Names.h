#pragma once
#include "Rx3Container.h"

vector<pair<uint32_t, string>> ExtractNamesFromChunk(Rx3Chunk const *namesChunk);
vector<pair<uint32_t, string>> ExtractNamesFromChunk(Rx3Chunk const &namesChunk);
vector<pair<uint32_t, string>> ExtractNamesFromRx3(Rx3Container &rx3);
vector<string> ExtractNamesFromRx3(Rx3Container &rx3, uint32_t type);
Rx3Chunk &AddNamesChunkToRx3(Rx3Container &rx3, vector<pair<uint32_t, string>> const &names);
Rx3Chunk &AddNamesChunkToRx3(Rx3Container &rx3, vector<string> const &names, uint32_t type = 0);
