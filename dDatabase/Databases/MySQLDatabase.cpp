#include "MySQLDatabase.h"

#include "Database.h"
#include "Game.h"
#include "dConfig.h"
#include "Logger.h"

using namespace DatabaseStructs;

namespace {
	sql::Driver* driver;
	sql::Connection* con;
	sql::Properties properties;
	std::string databaseName;
};

void MySQLDatabase::Connect() {
	driver = sql::mariadb::get_driver_instance();

	// The mariadb connector is *supposed* to handle unix:// and pipe:// prefixes to hostName, but there are bugs where
	// 1) it tries to parse a database from the connection string (like in tcp://localhost:3001/darkflame) based on the
	//    presence of a /
	// 2) even avoiding that, the connector still assumes you're connecting with a tcp socket
	// So, what we do in the presence of a unix socket or pipe is to set the hostname to the protocol and localhost,
	// which avoids parsing errors while still ensuring the correct connection type is used, and then setting the appropriate
	// property manually (which the URL parsing fails to do)
	const std::string UNIX_PROTO = "unix://";
	const std::string PIPE_PROTO = "pipe://";
	std::string mysql_host = Game::config->GetValue("mysql_host");
    if (mysql_host.find(UNIX_PROTO) == 0) {
		properties["hostName"] = "unix://localhost";
		properties["localSocket"] = mysql_host.substr(UNIX_PROTO.length()).c_str();
    } else if (mysql_host.find(PIPE_PROTO) == 0) {
		properties["hostName"] = "pipe://localhost";
		properties["pipe"] = mysql_host.substr(PIPE_PROTO.length()).c_str();
    } else {
		properties["hostName"] = mysql_host.c_str();
    }
	properties["user"] = Game::config->GetValue("mysql_username").c_str();
	properties["password"] = Game::config->GetValue("mysql_password").c_str();
	properties["autoReconnect"] = "true";

	databaseName = Game::config->GetValue("mysql_database").c_str();

	// `connect(const Properties& props)` segfaults in windows debug, but
	// `connect(const SQLString& host, const SQLString& user, const SQLString& pwd)` doesn't handle pipes/unix sockets correctly
	if (properties.find("localSocket") != properties.end() || properties.find("pipe") != properties.end()) {
		con = driver->connect(properties);
	} else {
		con = driver->connect(properties["hostName"].c_str(), properties["user"].c_str(), properties["password"].c_str());
	}
	con->setSchema(databaseName.c_str());
}

void MySQLDatabase::Destroy(std::string source, bool log) {
	if (!con) return;

	if (log) {
		if (source != "") LOG("Destroying MySQL connection from %s!", source.c_str());
		else LOG("Destroying MySQL connection!");
	}

	con->close();
	delete con;
}

sql::Statement* MySQLDatabase::CreateStmt() {
	sql::Statement* toReturn = con->createStatement();
	return toReturn;
}

std::unique_ptr<sql::PreparedStatement> MySQLDatabase::CreatePreppedStmtUnique(const std::string& query) {
	auto* stmt = CreatePreppedStmt(query);
	return std::unique_ptr<sql::PreparedStatement>(stmt);
}

sql::PreparedStatement* MySQLDatabase::CreatePreppedStmt(const std::string& query) {
	const char* test = query.c_str();
	size_t size = query.length();
	sql::SQLString str(test, size);

	if (!con) {
		Connect();
		LOG("Trying to reconnect to MySQL");
	}

	if (!con->isValid() || con->isClosed()) {
		delete con;

		con = nullptr;

		Connect();
		LOG("Trying to reconnect to MySQL from invalid or closed connection");
	}

	auto* stmt = con->prepareStatement(str);

	return stmt;
}

void MySQLDatabase::Commit() {
	con->commit();
}

bool MySQLDatabase::GetAutoCommit() {
	// TODO This should not just access a pointer.  A future PR should update this
	// to check for null and throw an error if the connection is not valid.
	return con->getAutoCommit();
}

void MySQLDatabase::SetAutoCommit(bool value) {
	// TODO This should not just access a pointer.  A future PR should update this
	// to check for null and throw an error if the connection is not valid.
	con->setAutoCommit(value);
}

std::unique_ptr<sql::ResultSet> MySQLDatabase::ExecuteQueryUnique(const std::string& query) {
	auto* result = CreatePreppedStmtUnique(query)->executeQuery();
	return std::unique_ptr<sql::ResultSet>(result);
}

std::unique_ptr<sql::ResultSet> MySQLDatabase::ExecuteQueryUnique(const std::unique_ptr<sql::PreparedStatement>& query) {
	auto* result = query->executeQuery();
	return std::unique_ptr<sql::ResultSet>(result);
}

// queries
std::optional<MasterInfo> MySQLDatabase::GetMasterInfo() {

	auto result = ExecuteQueryUnique("SELECT ip, port FROM servers WHERE name='master' LIMIT 1;");

	if (!result->next()) {
		return std::nullopt;
	}

	MasterInfo toReturn;

	toReturn.ip = result->getString("ip").c_str();
	toReturn.port = result->getInt("port");

	return toReturn;
}

std::optional<ApprovedNames> MySQLDatabase::GetApprovedCharacterNames() {
	auto result = ExecuteQueryUnique("SELECT name FROM charinfo;");

	ApprovedNames toReturn;
	if (!result->next()) return std::nullopt;

	do {
		toReturn.names.push_back(result->getString("name").c_str());
	} while (result->next());

	return toReturn;
}

std::optional<FriendsList> MySQLDatabase::GetFriendsList(uint32_t charId) {
	auto stmt = CreatePreppedStmtUnique(
		R"QUERY(
			SELECT fr.requested_player, best_friend, ci.name FROM 
			(
				SELECT CASE 
				WHEN player_id = ? THEN friend_id 
				WHEN friend_id = ? THEN player_id 
				END AS requested_player, best_friend FROM friends
			) AS fr 
			JOIN charinfo AS ci ON ci.id = fr.requested_player 
			WHERE fr.requested_player IS NOT NULL AND fr.requested_player != ?;
		)QUERY");
	stmt->setUInt(1, charId);
	stmt->setUInt(2, charId);
	stmt->setUInt(3, charId);

	FriendsList toReturn;

	auto friendsList = ExecuteQueryUnique(stmt);
	
	if (!friendsList->next()) {
		return std::nullopt;
	}

	toReturn.friends.reserve(friendsList->rowsCount());

	do {
		FriendData fd;
		fd.friendID = friendsList->getUInt(1);
		fd.isBestFriend = friendsList->getInt(2) == 3; // 0 = friends, 1 = left_requested, 2 = right_requested, 3 = both_accepted - are now bffs
		fd.friendName = friendsList->getString(3).c_str();

		toReturn.friends.push_back(fd);
	} while (friendsList->next());
	return toReturn;
}

bool MySQLDatabase::DoesCharacterExist(const std::string& name) {
	auto nameQuery(CreatePreppedStmtUnique("SELECT name from charinfo where name = ?;"));
	nameQuery->setString(1, name);
	auto result(nameQuery->executeQuery());
	return result->next();
}

