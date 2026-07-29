#include "Rx3TextureMetadata.h"
#include "TextFileTable.h"

using namespace rx3utils;

int TexFormatNameToID(string const &name) {
    static map<string, int> texFormatNameToID = {
        { "DXT1", 0 },
        { "BC1", 0 },
        { "DXT3", 1 },
        { "BC2", 1 },
        { "DXT5", 2 },
        { "BC3", 2 },
        { "ARGB8888", 3 },
        { "A8R8G8B8", 3 },
        { "L8", 4 },
        { "AL8", 5 },
        { "A8L8", 5 },
        { "RG8", 6 },
        { "R8G8", 6 },
        { "BC5", 7 },
        { "ATI2", 7 },
        { "RGB565", 8 },
        { "R5G6B5", 8 },
        { "ARGB4444", 9 },
        { "A4R4G4B4", 9 },
        { "BC6", 10 },
        { "BC6H", 10 },
        { "BC6UF", 10 },
        { "BC7", 11 },
        { "BC4", 12 },
        { "ATI1", 12 }
    };
    auto upper = ToUpper(name);
    if (texFormatNameToID.contains(upper))
        return texFormatNameToID[upper];
    return -1;
}

void ReadTexFormatFile(path const &filePath, map<string, TexFormatTarget> &out, vector<string> &outOrder) {
    if (exists(filePath)) {
        TextFileTable texFormatsTable;
        if (texFormatsTable.ReadCSV(filePath)) {
            for (size_t r = 0; r < texFormatsTable.NumRows(); r++) {
                auto const &row = texFormatsTable.Row(r);
                if (row.size() >= 2 && !row[0].empty()) {
                    string name = ToLower(WtoA(row[0]));
                    Trim(name);
                    TexFormatTarget t;
                    for (size_t c = 1; c < min(4u, row.size()); c++) {
                        string value = ToLower(WtoA(row[c]));
                        Trim(value);
                        if (IsNumber(value, false))
                            t.levels = SafeConvertInt<char>(row[2]);
                        else {
                            auto format = TexFormatNameToID(value);
                            if (format != -1)
                                t.format = format;
                            else {
                                auto dim = Split(value, 'x');
                                if (dim.size() == 2 && IsNumber(dim[0], false) && IsNumber(dim[1], false)) {
                                    auto IsProperDim = [](int value) { return value > 0 && value <= 16384; };
                                    int width = SafeConvertInt<unsigned short>(dim[0]);
                                    int height = SafeConvertInt<unsigned short>(dim[1]);
                                    if (IsProperDim(width) && IsProperDim(height)) {
                                        t.width = width;
                                        t.height = height;
                                    }
                                }
                            }
                        }
                    }
                    if (!out.contains(name))
                        outOrder.push_back(name);
                    out[name] = t;
                }
            }
        }
    }
}

bool TryParseTexNamePattern(string const &key, TexNamePattern &out) {
    out.key = key;
    out.literals.clear();
    string currentLiteral;
    bool prevWasX = false;
    bool hasPlaceholder = false;
    for (char c : key) {
        if (c == 'X') {
            if (prevWasX)
                return false;
            out.literals.push_back(ToLower(currentLiteral));
            currentLiteral.clear();
            prevWasX = true;
            hasPlaceholder = true;
        }
        else {
            currentLiteral.push_back(c);
            prevWasX = false;
        }
    }
    out.literals.push_back(ToLower(currentLiteral));
    return hasPlaceholder;
}

bool MatchFrom(vector<string> const &literals, size_t segIdx, string const &name, size_t pos) {
    string const &lit = literals[segIdx];
    if (name.compare(pos, lit.size(), lit) != 0)
        return false;
    pos += lit.size();

    if (segIdx + 1 == literals.size())
        return pos == name.size();

    size_t digitsStart = pos;
    size_t digitsEnd = pos;
    while (digitsEnd < name.size() && isdigit(static_cast<unsigned char>(name[digitsEnd])))
        ++digitsEnd;
    if (digitsEnd == digitsStart)
        return false;

    for (size_t end = digitsEnd; end > digitsStart; --end) {
        if (MatchFrom(literals, segIdx + 1, name, end))
            return true;
    }
    return false;
}

bool MatchesPattern(TexNamePattern const &pattern, string const &nameLowered) {
    return MatchFrom(pattern.literals, 0, nameLowered, 0);
}

string const *FindBestPatternMatch(vector<TexNamePattern> const &patterns, string const &nameLowered) {
    TexNamePattern const *best = nullptr;
    size_t bestLiteralLen = 0;
    for (auto const &pattern : patterns) {
        if (!MatchesPattern(pattern, nameLowered))
            continue;
        size_t literalLen = 0;
        for (auto const &lit : pattern.literals)
            literalLen += lit.size();
        if (!best || literalLen > bestLiteralLen) {
            best = &pattern;
            bestLiteralLen = literalLen;
        }
    }
    return best ? &best->key : nullptr;
}
