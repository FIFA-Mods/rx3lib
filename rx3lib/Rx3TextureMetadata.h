#pragma once
#include "Rx3Options.h"

struct TexNamePattern {
    string key;
    vector<string> literals;
};

int TexFormatNameToID(string const &name);
void ReadTexFormatFile(path const &filePath, map<string, TexFormatTarget> &out,
    vector<string> &outOrder);
bool TryParseTexNamePattern(string const &key, TexNamePattern &out);
string const *FindBestPatternMatch(vector<TexNamePattern> const &patterns, string const &nameLowered);
