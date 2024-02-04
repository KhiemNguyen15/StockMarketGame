#include "../include/databaseHandler.hpp"
#include <iostream>

DatabaseHandler::DatabaseHandler(const std::string &dbPath) {
  int rc = sqlite3_open(dbPath.c_str(), &db);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to open database file." << std::endl;
  }
}

DatabaseHandler::~DatabaseHandler() { sqlite3_close(db); }

bool DatabaseHandler::insertUser(const std::string &userId) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  std::string insertUserQuery =
      "INSERT INTO users (user_id, balance) VALUES (?, ?)";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, insertUserQuery.c_str(), -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, 1000.00);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_DONE) {
      return true;
    } else {
      std::cerr << "Failed to insert user data." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for inserting user data."
              << std::endl;
  }

  return false;
}

bool DatabaseHandler::insertUserStock(const std::string &userId,
                                      const std::string &stockName) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  std::string query = "INSERT INTO user_stocks (user_id, stock_name, quantity) "
                      "VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, stockName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 0);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_DONE) {
      return true;
    } else {
      std::cerr << "Failed to add stock to user." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for adding stock to user."
              << std::endl;
  }

  return false;
}

bool DatabaseHandler::userExists(const std::string &userId) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  std::string query = "SELECT 1 FROM users WHERE user_id = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_ROW) {
      return true;
    } else if (result == SQLITE_DONE) {
      return false;
    } else {
      std::cerr << "Failed to check user existence." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for checking user existence."
              << std::endl;
  }

  return false;
}

bool DatabaseHandler::userHasStock(const std::string &userId,
                                   const std::string &stockName) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  std::string query =
      "SELECT 1 FROM user_stocks WHERE user_id = ? AND stock_name = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, stockName.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_ROW) {
      return true;
    } else if (result == SQLITE_DONE) {
      return false;
    } else {
      std::cerr << "Failed to check user stock possession." << std::endl;
    }
  } else {
    std::cerr
        << "Failed to prepare statement for checking user stock possession."
        << std::endl;
  }

  return false;
}

bool DatabaseHandler::createTables() {
  std::lock_guard<std::mutex> lock(connectionMutex);

  const std::string createUsersTableQuery = "CREATE TABLE IF NOT EXISTS users ("
                                            "user_id TEXT PRIMARY KEY,"
                                            "balance REAL"
                                            ");";

  const std::string createUserStocksTableQuery =
      "CREATE TABLE IF NOT EXISTS user_stocks ("
      "user_id TEXT,"
      "stock_name TEXT,"
      "quantity INTEGER,"
      "PRIMARY KEY (user_id, stock_name),"
      "FOREIGN KEY (user_id) REFERENCES users(user_id)"
      ");";

  int rc1 = sqlite3_exec(db, createUsersTableQuery.c_str(), nullptr, nullptr,
                         nullptr);
  int rc2 = sqlite3_exec(db, createUserStocksTableQuery.c_str(), nullptr,
                         nullptr, nullptr);

  if (rc1 != SQLITE_OK || rc2 != SQLITE_OK) {
    std::cerr << "Failed to create tables." << std::endl;
    return false;
  }

  return true;
}

bool DatabaseHandler::updateUserBalance(const std::string &userId,
                                        double balanceChange) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  if (!userExists(userId)) {
    if (!insertUser(userId)) {
      return false;
    }
  }

  double newBalance = getUserBalance(userId) + balanceChange;

  if (newBalance < 0.0) {
    return false;
  }

  const std::string query = "UPDATE users SET balance = ? WHERE user_id = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_double(stmt, 1, newBalance);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_DONE) {
      return true;
    } else {
      std::cerr << "Failed to update user balance." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for updating user balance."
              << std::endl;
  }

  return false;
}

bool DatabaseHandler::updateUserStock(const std::string &userId,
                                      const std::string &stockName,
                                      int quantityChange) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  if (!userHasStock(userId, stockName)) {
    if (!insertUserStock(userId, stockName)) {
      return false;
    }
  }

  int newQuantity = getUserStockQuantity(userId, stockName) + quantityChange;

  if (newQuantity < 0) {
    return false;
  }

  std::string query = "UPDATE user_stocks SET quantity = ? WHERE user_id = ? "
                      "AND stock_name = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, newQuantity);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, stockName.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result == SQLITE_DONE) {
      return true;
    } else {
      std::cerr << "Failed to update user's stock." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for updating user's stock."
              << std::endl;
  }

  return false;
}

std::vector<std::pair<std::string, int>>
DatabaseHandler::getUserStocks(const std::string &userId) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  std::vector<std::pair<std::string, int>> userStocks;

  std::string query =
      "SELECT stock_name, quantity FROM user_stocks WHERE user_id = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *stockName =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      int quantity = sqlite3_column_int(stmt, 1);

      userStocks.emplace_back(stockName ? stockName : "", quantity);
    }

    sqlite3_finalize(stmt);
  } else {
    std::cerr << "Failed to prepare statement for getting user stocks."
              << std::endl;
  }

  return userStocks;
}

double DatabaseHandler::getUserBalance(const std::string &userId) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  double balance = 0.0;
  std::string query = "SELECT balance FROM users WHERE user_id = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      balance = sqlite3_column_double(stmt, 0);
    } else {
      std::cerr << "Failed to find user." << std::endl;
    }
  } else {
    std::cerr << "Failed to prepare statement for getting user balance."
              << std::endl;
  }

  return balance;
}

int DatabaseHandler::getUserStockQuantity(const std::string &userId,
                                          const std::string &stockName) {
  std::lock_guard<std::mutex> lock(connectionMutex);

  int stockQuantity = 0;

  std::string query =
      "SELECT quantity FROM user_stocks WHERE user_id = ? AND stock_name = ?";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, stockName.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      stockQuantity = sqlite3_column_int(stmt, 0);
    } else {
      std::cerr << "Failed to retrieve user's stock quantity." << std::endl;
    }

    sqlite3_finalize(stmt);
  } else {
    std::cerr
        << "Failed to prepare statement for getting user's stock quantity."
        << std::endl;
  }

  return stockQuantity;
}
