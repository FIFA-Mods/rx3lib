#include "Rx3Container.h"

using namespace rx3utils;

void Rx3SwapEndian(uint32_t &value) {
    value = _byteswap_ulong(value);
}

void Rx3SwapEndian(uint16_t &value) {
    value = _byteswap_ushort(value);
}

void Rx3SwapEndian(int32_t &value) {
    value = _byteswap_ulong(value);
}

void Rx3SwapEndian(int16_t &value) {
    value = _byteswap_ushort(value);
}

void Rx3SwapEndian(float &value) {
    SetAt(&value, 0, _byteswap_ulong(GetAt<unsigned long>(&value, 0)));
}

Rx3Reader::Rx3Reader(void const *data, bool bigEndian) {
    mBegin = (uint8_t const *)data;
    mCurrent = mBegin;
    mBigEndian = bigEndian;
}

Rx3Reader::Rx3Reader(Rx3Chunk *rx3chunk) : Rx3Reader(rx3chunk->mData.data(), rx3chunk->mBigEndian) {}
Rx3Reader::Rx3Reader(Rx3Chunk const *rx3chunk) : Rx3Reader(rx3chunk->mData.data(), rx3chunk->mBigEndian) {}
Rx3Reader::Rx3Reader(Rx3Chunk &rx3chunk) : Rx3Reader(rx3chunk.mData.data(), rx3chunk.mBigEndian) {}
Rx3Reader::Rx3Reader(Rx3Chunk const &rx3chunk) : Rx3Reader(rx3chunk.mData.data(), rx3chunk.mBigEndian) {}

size_t Rx3Reader::Position() const {
    return mCurrent - mBegin;
}

void Rx3Reader::MoveTo(size_t position) {
    mCurrent = mBegin + position;
}

void Rx3Reader::Skip(size_t bytes) {
    mCurrent += bytes;
}

char const *Rx3Reader::GetString() {
    return (char const *)mCurrent;
}

char const *Rx3Reader::ReadString() {
    char const *result = GetString();
    mCurrent += strlen(result) + 1;
    return result;
}

void const *Rx3Reader::GetCurrentPtr() {
    return mCurrent;
}

void Rx3Reader::SetBigEndian(bool set) {
    mBigEndian = set;
}

void Rx3Writer::PutData(void const *data, size_t size) {
    if (size > 0) {
        size_t current = mData.size();
        mData.resize(current + size);
        memcpy(&mData[current], data, size);
    }
}

Rx3Writer::Rx3Writer(vector<uint8_t> &data, bool bigEndian) : mData(data), mBigEndian(bigEndian) {}
Rx3Writer::Rx3Writer(Rx3Chunk *rx3chunk) : mData(rx3chunk->mData), mBigEndian(rx3chunk->mBigEndian) {}
Rx3Writer::Rx3Writer(Rx3Chunk const *rx3chunk) : mData(const_cast<vector<uint8_t> &>(rx3chunk->mData)), mBigEndian(rx3chunk->mBigEndian) {}
Rx3Writer::Rx3Writer(Rx3Chunk &rx3chunk) : mData(rx3chunk.mData), mBigEndian(rx3chunk.mBigEndian) {}
Rx3Writer::Rx3Writer(Rx3Chunk const &rx3chunk) : mData(const_cast<vector<uint8_t> &>(rx3chunk.mData)), mBigEndian(rx3chunk.mBigEndian) {}

void Rx3Writer::Put(char const *str) {
    PutData(str, strlen(str));
    Put<char>('\0');
}

void Rx3Writer::Put(wchar_t const *str) {
    PutData(str, wcslen(str) * 2);
    Put<wchar_t>('\0');
}

void Rx3Writer::Put(string const &str) {
    PutData((void *)str.c_str(), str.size());
    Put<char>('\0');
}

void Rx3Writer::Put(wstring const &str) {
    PutData((void *)str.c_str(), str.size() * 2);
    Put<wchar_t>(L'\0');
}

void Rx3Writer::Put(void const *data, size_t size) {
    PutData(data, size);
}

void Rx3Writer::Align(size_t alignment) {
    auto numBytes = GetNumBytesToAlign(mData.size(), alignment);
    for (size_t i = 0; i < numBytes; i++)
        Put<uint8_t>(0);
}

void Rx3Writer::SetBigEndian(bool set) {
    mBigEndian = set;
}

void Rx3Writer::Reserve(size_t size) {
    if (size > mData.capacity())
        mData.reserve(size);
}

void Rx3Writer::UpdateTotalSize() {
    if (mData.size() >= 4) {
        size_t totalSize = mData.size();
        if (mBigEndian)
            Rx3SwapEndian(totalSize);
        SetAt(mData.data(), 0, totalSize);
    }
}

void Rx3Writer::AlignAndUpdateTotalSize() {
    Align();
    UpdateTotalSize();
}

Rx3Container::Rx3Container(path const &rx3path) {
    mBigEndian = false;
    Load(rx3path);
}

bool Rx3Container::Load(path const &rx3path) {
    bool result = false;
    FILE *file = nullptr;
    _wfopen_s(&file, rx3path.c_str(), L"rb");
    if (file) {
        mName = rx3path.stem().string();
        fseek(file, 0, SEEK_END);
        auto fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (fileSize >= 16) {
            uint8_t *fileData = new uint8_t[fileSize];
            fread(fileData, 1, fileSize, file);
            Rx3Reader reader(fileData);
            uint32_t magic = reader.Read<uint32_t>();
            bool littleEndian = magic == 'l3XR';
            bool bigEndian = magic == 'b3XR';
            if (littleEndian || bigEndian) {
                mBigEndian = bigEndian;
                reader.SetBigEndian(bigEndian);
                reader.Skip(8);
                uint32_t numChunks = reader.Read<uint32_t>();
                if (numChunks > 0) {
                    mChunks.resize(numChunks);
                    for (uint32_t s = 0; s < numChunks; s++) {
                        uint32_t id = reader.Read<uint32_t>();
                        uint32_t offset = reader.Read<uint32_t>();
                        uint32_t size = reader.Read<uint32_t>();
                        reader.Skip(4);
                        mChunks[s].mId = id;
                        mChunks[s].mBigEndian = mBigEndian;
                        if (size > 0) {
                            mChunks[s].mData.resize(size);
                            memcpy(mChunks[s].mData.data(), &fileData[offset], size);
                        }
                    }
                }
                result = true;
            }
            delete[] fileData;
        }
        fclose(file);
    }
    return result;
}

bool Rx3Container::Save(path const &rx3path) {
    bool result = false;
    vector<uint8_t> data;
    Rx3Writer writer(data);
    if (mBigEndian)
        writer.Put<uint32_t>('b3XR');
    else
        writer.Put<uint32_t>('l3XR');
    writer.SetBigEndian(mBigEndian);
    writer.Put<uint32_t>(4);
    uint32_t fileHeaderSize = 16 + 16 * mChunks.size();
    uint32_t totalSize = fileHeaderSize;
    for (auto const &chunk : mChunks)
        totalSize += chunk.mData.size();
    writer.Put<uint32_t>(totalSize);
    uint32_t numChunks = mChunks.size();
    writer.Put<uint32_t>(numChunks);
    uint32_t currentOffset = fileHeaderSize;
    for (auto const &chunk : mChunks) {
        writer.Put<uint32_t>(chunk.mId);
        writer.Put<uint32_t>(currentOffset);
        writer.Put<uint32_t>(chunk.mData.size());
        writer.Put<uint32_t>(0);
        currentOffset += chunk.mData.size();
    }
    for (auto const &chunk : mChunks)
        writer.Put(chunk.mData.data(), chunk.mData.size());
    FILE *file = nullptr;
    _wfopen_s(&file, rx3path.c_str(), L"wb");
    if (file) {
        fwrite(data.data(), data.size(), 1, file);
        fclose(file);
        result = true;
    }
    return result;
}

Rx3Container::Rx3Container(bool bigEndian) {
    mBigEndian = bigEndian;
}

Rx3Chunk *Rx3Container::FindFirstChunk(uint32_t chunkId) {
    for (auto &s : mChunks) {
        if (s.mId == chunkId)
            return &s;
    }
    return nullptr;
}

vector<Rx3Chunk *> Rx3Container::FindAllChunks(uint32_t chunkId) {
    vector<Rx3Chunk *> result;
    for (auto &s : mChunks) {
        if (s.mId == chunkId)
            result.push_back(&s);
    }
    return result;
}

void Rx3Container::RemoveAllChunks(uint32_t chunkId) {
    mChunks.erase(remove_if(mChunks.begin(), mChunks.end(),
        [chunkId](const Rx3Chunk &chunk) { return chunk.mId == chunkId; }),
        mChunks.end());
}

Rx3Chunk &Rx3Container::AddChunk(uint32_t chunkId) {
    Rx3Chunk chunk;
    chunk.mId = chunkId;
    chunk.mBigEndian = mBigEndian;
    return mChunks.emplace_back(chunk);
}

bool Rx3Container::IsEmpty() {
    return mChunks.empty();
}
