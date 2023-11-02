#ifndef __GAMEDATABASE__H__
#define __GAMEDATABASE__H__

#include "DatabaseStructs.h"
#include <optional>

namespace sql {
	class Statement;
	class PreparedStatement;
};

class GameDatabase {
public:
	virtual void Connect() = 0;
	virtual void Destroy(std::string source = "", bool log = true) = 0;
	virtual sql::Statement* CreateStmt() = 0;
	virtual sql::PreparedStatement* CreatePreppedStmt(const std::string& query) = 0;
	virtual void Commit() = 0;
	virtual bool GetAutoCommit() = 0;
	virtual void SetAutoCommit(bool value) = 0;

	// Get the Master servers info
	virtual std::optional<DatabaseStructs::MasterInfo> GetMasterInfo() = 0;

	// Get a list of all approved character names
	virtual std::optional<DatabaseStructs::ApprovedNames> GetApprovedCharacterNames() = 0;

	// Get a list of all friends for a given character ID
	virtual std::optional<DatabaseStructs::FriendsList> GetFriendsList(uint32_t charID) = 0;

	virtual bool DoesCharacterExist(const std::string& name) = 0;
};

#endif  //!__GAMEDATABASE__H__
