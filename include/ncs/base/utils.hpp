#pragma once

#include <cxxabi.h>
#include <execinfo.h>
#include <regex>
#include <sstream>
#include <vector>
#include <ncs/types.hpp>

namespace ncs
{
	inline std::string demangle(const std::string& mangled_name)
	{
		int status;
		std::unique_ptr<char, void(*)(void*)> demangled(
			abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status),
			std::free
		);

		if (status == 0 && demangled)
			return demangled.get();
		return mangled_name;
	}

	inline std::string get_stacktrace(int max_frames = 10)
	{
		void* callstack[max_frames];
		int frames = backtrace(callstack, max_frames);
		char** symbols = backtrace_symbols(callstack, frames);

		std::ostringstream stacktrace;
		stacktrace << "NCS stacktrace:\n";

		std::regex symbol_regex(R"(\S+\s+\S+\s+(\S+)\s*\+)");
		/* skip i = 1 because it's our current stack frame */
		for (auto i = 2; i < frames; ++i)
		{
			std::string frame_info(symbols[i]);
			std::smatch match;

			if (std::regex_search(frame_info, match, symbol_regex) && match.size() > 1)
			{
				std::string mangled_name = match[1].str();
				std::string demangled_name = demangle(mangled_name);
				stacktrace << "  " << demangled_name << "\n";
			}
			else
			{
				stacktrace << "  " << frame_info << "\n";
			}
		}

		free(symbols);
		return stacktrace.str();
	}

	uint64_t archash(const std::vector<Component>& components);

	class InvalidEntityError final : public std::runtime_error
	{
	public:
		InvalidEntityError(std::uint64_t entity_id, Generation gen, const char* file, int line)
		: std::runtime_error(
		   "invalid entity access\n"
		   "  entity ID: " + std::to_string(entity_id) + "\n"
		   "  generation: " + std::to_string(gen) + "\n"
		   "  location: " + std::string(file) + ":" + std::to_string(line) + "\n" +
		   get_stacktrace()
		) {}
	};
}
