#include <sstream>
#include "Environment.hpp"



std::unordered_map<std::wstring, std::wstring>
get_environment_map() {
	std::unordered_map<std::wstring, std::wstring> map;

	size_t eq, len, s_len;
	wchar_t* env = GetEnvironmentStrings();
	if (env != nullptr) {
		wchar_t* env_init = env;
		while (env[0] != L'\x00') {
			for (eq = 0; env[eq] != L'\x00' && env[eq] != L'='; ++eq);
			for (len = eq; env[len] != L'\x00'; ++len);

			if (eq > 0) {
				s_len = (eq < len ? len - eq - 1 : 0);
				map.emplace(decltype(map)::value_type(std::wstring(env, eq), std::wstring(env + eq + 1, s_len)));
			}

			env += len + 1;
		}

		FreeEnvironmentStrings(env_init);
	}

	return map;
}

std::wstring
environment_map_to_string(const std::unordered_map<std::wstring, std::wstring>& map) {
	std::wstringstream s;

	for (auto it = map.begin(); it != map.end(); ++it) {
		s << it->first << L'=' << it->second << L'\x00';
	}

	s << L'\x00';

	return s.str();
}


