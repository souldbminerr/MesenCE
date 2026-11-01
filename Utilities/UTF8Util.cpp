#include "pch.h"
#include "UTF8Util.h"
#ifdef __APPLE__
	#include <codecvt>
	#include <locale>
#else
	#include <cuchar>
#endif

#ifdef _MSC_VER
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#undef WIN32_LEAN_AND_MEAN
#endif

namespace utf8
{
	std::wstring utf8::decode(const std::string& str)
	{
#ifdef _MSC_VER
		std::wstring ret;
		int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
		if(len > 0) {
			ret.resize(len);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &ret[0], len);
		}
		return ret;
#else
		throw std::runtime_error("utf8::decode is not implemented for this platform");
#endif
	}

	string utf8::encode(const std::wstring& wstr)
	{
#ifdef _WIN32
		return encode(std::u16string((char16_t*)wstr.c_str()));
#else
		throw std::runtime_error("utf8::encode(wstring) is not implemented for this platform");
#endif
	}

	string utf8::encode(const std::u16string& wstr)
	{
#ifdef __APPLE__
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
		return conv.to_bytes(wstr);
#elif defined(__SWITCH__)
		//newlib does not provide c16rtomb
		string result;
		result.reserve(wstr.size());
		for(size_t i = 0; i < wstr.size(); i++) {
			uint32_t cp = wstr[i];
			if(cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wstr.size()) {
				char16_t lo = wstr[i + 1];
				if(lo >= 0xDC00 && lo <= 0xDFFF) {
					cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
					i++;
				} else {
					continue;
				}
			} else if(cp >= 0xDC00 && cp <= 0xDFFF) {
				continue;
			}
			if(cp < 0x80) {
				result.push_back((char)cp);
			} else if(cp < 0x800) {
				result.push_back((char)(0xC0 | (cp >> 6)));
				result.push_back((char)(0x80 | (cp & 0x3F)));
			} else if(cp < 0x10000) {
				result.push_back((char)(0xE0 | (cp >> 12)));
				result.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
				result.push_back((char)(0x80 | (cp & 0x3F)));
			} else if(cp <= 0x10FFFF) {
				result.push_back((char)(0xF0 | (cp >> 18)));
				result.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
				result.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
				result.push_back((char)(0x80 | (cp & 0x3F)));
			}
		}
		return result;
#else
		std::mbstate_t state {};
		string result;
		result.reserve(wstr.size());

		char buffer[10];

		for(char16_t c16 : wstr) {
			size_t size = std::c16rtomb(buffer, c16, &state);
			if(size == (size_t)-1) {
				//Ignore invalid characters
				continue;
			} else if(size > 0) {
				result.append(buffer, size);
			}
		}

		return result;
#endif
	}
}