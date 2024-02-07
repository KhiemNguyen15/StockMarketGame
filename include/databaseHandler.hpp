#ifndef DATABASE_HANDLER_HPP
#define DATABASE_HANDLER_HPP

#include <sqlite3.h>
#include <string>
#include <vector>

#define STARTING_MONEY 5000.00

class DatabaseHandler {
private:
  sqlite3 *db;
  // std::mutex connectionMutex;

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
  bool updateTransactionsHistory(const std::string &userId,
                                 const std::string &stockName, int quantity,
                                 double price, const std::string &timestamp);

  std::vector<std::pair<std::string, int>>
  getUserStocks(const std::string &userId);
  double getUserBalance(const std::string &userId);
  int getUserStockQuantity(const std::string &userId,
                           const std::string &stockName);
  std::vector<std::vector<std::string>>
  getUserHistory(const std::string &userId);
};

#endif // DATABASE_HANDLER_HPP
