#pragma once
#include <cstdio>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <any>
#include <filesystem>
#include <iostream>
#include <numeric>
struct IUnknown;
#include <Windows.h>
#undef min
#undef max
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;
using namespace std::filesystem;

namespace rx3utils {

wstring AtoW(string const &str);
string WtoA(wstring const &str);
string ToUpper(string const &str);
string ToLower(string const &str);
wstring ToUpper(wstring const &str);
wstring ToLower(wstring const &str);
void Replace(string &str, const string &from, const string &to);
void Replace(wstring &str, const wstring &from, const wstring &to);
void Trim(string &str);
void Trim(wstring &str);
vector<string> Split(string const &line, char delim, bool trim = true, bool skipEmpty = false, bool quotesHavePriority = true);
vector<wstring> Split(wstring const &line, wchar_t delim, bool trim = true, bool skipEmpty = false, bool quotesHavePriority = true);

class FormattingUtils {
    static const unsigned int BUF_SIZE = 10;
    static unsigned int currentBuf;
    static char buf[BUF_SIZE][4096];
    static unsigned int currentBufW;
    static wchar_t bufW[BUF_SIZE][4096];
public:
    template<typename T> static T const &Arg(T const &arg) { return arg; }
    static char const *Arg(string const &arg) { return arg.c_str(); }
    static wchar_t const *Arg(wstring const &arg) { return arg.c_str(); }
    static char *GetBuf();
    static wchar_t *GetBufW();
    static void WindowsMessageBoxA(char const *msg, char const *title, unsigned int icon);
    static void WindowsMessageBoxW(wchar_t const *msg, wchar_t const *title, unsigned int icon);
};

template<typename ...ArgTypes>
char *FormatStatic(const string &format, ArgTypes... args) {
    char *buf = FormattingUtils::GetBuf();
    snprintf(buf, 4096, format.c_str(), FormattingUtils::Arg(args)...);
    return buf;
}

template<typename ...ArgTypes>
string Format(const string &format, ArgTypes... args) {
    return FormatStatic(format, FormattingUtils::Arg(args)...);
}

template<typename ...ArgTypes>
wchar_t *FormatStatic(const wstring &format, ArgTypes... args) {
    wchar_t *buf = FormattingUtils::GetBufW();
    _snwprintf(buf, 4096, format.c_str(), FormattingUtils::Arg(args)...);
    return buf;
}

template<typename ...ArgTypes>
wstring Format(const wstring &format, ArgTypes... args) {
    return FormatStatic(format, FormattingUtils::Arg(args)...);
}

template <typename... ArgTypes>
bool Message(string const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxA(FormatStatic(format, args...), "Message", 0);
    return false;
}

template <typename... ArgTypes>
bool Error(string const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxA(FormatStatic(format, args...), "Error", 2);
    return false;
}

template <typename... ArgTypes>
bool Warning(string const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxA(FormatStatic(format, args...), "Warning", 1);
    return false;
}

template <typename... ArgTypes>
bool Message(wstring const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxW(FormatStatic(format, args...), L"Message", 0);
    return false;
}

template <typename... ArgTypes>
bool Error(wstring const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxW(FormatStatic(format, args...), L"Error", 2);
    return false;
}

template <typename... ArgTypes>
bool Warning(wstring const &format, ArgTypes... args) {
    FormattingUtils::WindowsMessageBoxW(FormatStatic(format, args...), L"Warning", 1);
    return false;
}

unsigned int Hash(string const &str);

template<typename T>
T SafeConvertInt(wstring const &str, bool isHex = false) {
    T result = 0;
    try {
        result = static_cast<T>(stoull(str, 0, isHex ? 16 : 10));
    }
    catch (...) {}
    return result;
}

float SafeConvertFloat(wstring const &str);
double SafeConvertDouble(wstring const &str);

template<typename T>
T SafeConvertInt(string const &str, bool isHex = false) {
    T result = 0;
    try {
        result = static_cast<T>(stoull(str, 0, isHex ? 16 : 10));
    }
    catch (...) {}
    return result;
}

float SafeConvertFloat(string const &str);
double SafeConvertDouble(string const &str);

wstring GetStringWithoutUnicodeChars(wstring const &src);

vector<wstring> FileToLinesW(path const &filePath, wstring const &commentLineBegin = wstring());
vector<string> FileToLinesA(path const &filePath, string const &commentLineBegin = string());

bool StartsWith(wstring const &str, wstring const &what);
bool StartsWith(string const &str, string const &what);
bool IsNumber(wstring const &str, bool hexadecimal);
bool IsNumber(string const &str, bool hexadecimal);
string ToUTF8(wstring const &wstr);
wstring ToUTF16(string const &str);
float HalfFloatToFloat(uint16_t half);
uint16_t FloatToHalfFloat(float value);

template<typename T>
T *At(void *object, size_t offset) {
    return (T *)((size_t)object + offset);
}

template<typename T>
T const *At(void const *object, size_t offset) {
    return (T const *)((size_t)object + offset);
}

template<typename T>
T GetAt(void const *object, size_t offset) {
    return *At<T>(object, offset);
}

template<typename T>
void SetAt(void *object, size_t offset, T const &value) {
    *At<T>(object, offset) = value;
}

size_t GetNumBytesToAlign(size_t offset, size_t alignment);
size_t GetAligned(size_t offset, size_t alignment);
void Memory_Fill(void *dst, int val, size_t size);
void Memory_Zero(void *dst, size_t size);
void Memory_Copy(void *dst, void const *src, size_t size);

template<typename T>
void Memory_Zero(T &obj) {
    Memory_Zero(&obj, sizeof(T));
}

}
