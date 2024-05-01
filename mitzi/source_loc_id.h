#pragma once

#include <source_location>
#include <cstdint>
#include "utils.h"

namespace mitzi {

	struct source_loc_id {
		uint32_t line;
		uint32_t column;
		uint32_t file_id;

		constexpr source_loc_id(std::source_location sl = std::source_location::current()) :
			line(sl.line()),
			column(sl.column()),
			file_id(hash_str(sl.file_name()))
		{
		}
	};

}