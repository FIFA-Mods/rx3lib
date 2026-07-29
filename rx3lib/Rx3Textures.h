#pragma once
#include "Rx3Container.h"
#include "Rx3Options.h"

struct PackedTextureInfo {
    string name;
    path filePath;
    int format = -1;
    char levels = 0;
    unsigned short width = 0;
    unsigned short height = 0;

    PackedTextureInfo();
    PackedTextureInfo(path const &_filePath);
    PackedTextureInfo(string const &_name, path const &_filePath);
    PackedTextureInfo(string const &_name, path const &_filePath, int _format);
    PackedTextureInfo(string const &_name, path const &_filePath, int _format, char _levels);
    PackedTextureInfo(string const &_name, path const &_filePath, int _format, char _levels, unsigned short _width, unsigned short _height);
};

void ExtractTexturesFromRX3(Rx3Container &container, path const &outputDir, Rx3Options const &rx3options);
bool ImportTexturesToRX3(Rx3Container &rx3, vector<PackedTextureInfo> const &inTextures, Rx3Options const &rx3options,
    bool withoutNames = false);
bool ImportTexturesToRX3(Rx3Container &rx3, vector<path> const &inTextures, path const &localMetadataFile,
    Rx3Options const &rx3options, bool withoutNames = false);
