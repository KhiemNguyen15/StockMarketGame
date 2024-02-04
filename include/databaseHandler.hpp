#ifndef DATABASE_HANDLER_HPP
#define DATABASE_HANDLER_HPP

#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>

class DatabaseHandler {
private:
  sqlite3 *db;
  std::mutex connectionMutex;

  bool insertUser(const std::string &userId);
  bool insertUserStock(const std::string &userId, const std::string &stockName);

  bool userExists(const std::string &userId);
  bool userHasStock(const std::string &userId, const std::string &stockName);

public:
  DatabaseHandler(const std::string &dbPath);
  ~DatabaseHandler();

  bool createTables();

  bool updateUserBalance(const std::string &userId, double balanceChange);
  bool updateUserStock(const std::string &userId, const std::string &stockName,
                       int quantityChange);

  std::vector<std::pair<std::string, int>>
  getUserStocks(const std::string &userId);
  double getUserBalance(const std::string &userId);
  int getUserStockQuantity(const std::string &userId,
                           const std::string &stockName);
};

#endif // DATABASE_HANDLER_HPP
