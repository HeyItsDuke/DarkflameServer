#ifndef __MYSQLDATABASE__H__
#define __MYSQLDATABASE__H__

#include <conncpp.hpp>
#include <memory>

#include "GameDatabase.h"

class MySQLDatabase : public GameDatabase {
public:
	void Connect() override;
	void Destroy(std::string source = "", bool log = true) override;

	sql::Statement* CreateStmt() override;
	sql::PreparedStatement* CreatePreppedStmt(const std::string& query) override;
	void Commit() override;
	bool GetAutoCommit() override;
	void SetAutoCommit(bool value) override;

	// Overloaded queries
	std::optional<DatabaseStructs::MasterInfo> GetMasterInfo() override;

	std::optional<DatabaseStructs::ApprovedNames> GetApprovedCharacterNames() override;

	std::optional<DatabaseStructs::FriendsList> GetFriendsList(uint32_t charID) override;
private:
	std::unique_ptr<sql::PreparedStatement> CreatePreppedStmtUnique(const std::string& query);

	std::unique_ptr<sql::ResultSet> ExecuteQueryUnique(const std::string& query);
	std::unique_ptr<sql::ResultSet> ExecuteQueryUnique(const std::unique_ptr<sql::PreparedStatement>& query);
	bool DoesCharacterExist(const std::string& name) override;
};

#endif  //!__MYSQLDATABASE__H__
