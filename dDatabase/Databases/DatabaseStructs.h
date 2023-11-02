#ifndef __DATABASESTRUCTS__H__
#define __DATABASESTRUCTS__H__

#include <cstdint>
#include <string>
#include <vector>

#include "dCommonVars.h"

// Contains various structures used as return values from database queries.

namespace DatabaseStructs {
	struct MasterInfo {
		std::string ip;
		uint32_t port;
	};

	struct ApprovedNames {
		std::vector<std::string> names;
	};

	struct FriendsList {
		std::vector<FriendData> friends;
	};
};

#endif  //!__DATABASESTRUCTS__H__
