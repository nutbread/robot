#ifndef ___H_ENVIRONMENT
#define ___H_ENVIRONMENT

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>
#include <unordered_map>



std::unordered_map<std::wstring, std::wstring>
get_environment_map();

std::wstring
environment_map_to_string(const std::unordered_map<std::wstring, std::wstring>& map);



#endif // ___H_ENVIRONMENT


