#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <dpp/appcommand.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/message.h>
#include <dpp/once.h>
#include <dpp/presence.h>
#include <dpp/restresults.h>
#include <dpp/snowflake.h>
#include <dpp/user.h>
#include <fstream>
#include <iomanip>
#include <ios>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <optional>
#include <sstream>
#include <string>

#include "../include/databaseHandler.hpp"
#include "../include/stockRetriever.h"

const std::string configPath = "../data/config.json";
const std::string dbPath = "../data/gameData.db";

std::string getBotToken() {
  std::ifstream file(configPath, std::ifstream::in);

  if (!file.is_open()) {
    std::cerr << "Error opening file: " << configPath << std::endl;
    return "";
  }

  Json::Value root;
  Json::CharReaderBuilder reader;
  Json::parseFromStream(reader, file, &root, nullptr);

  std::string token = root["discord_bot_token"].asString();

  return token;
}

std::string getCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();

  std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d");

  return ss.str();
}

int main(int argc, char *argv[]) {
  DatabaseHandler dbHandler(dbPath);
  dbHandler.createTables();

  dpp::cluster bot(getBotToken());

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&bot, &dbHandler](const dpp::slashcommand_t &event) {
    dpp::user user = event.command.get_issuing_user();

    if (event.command.get_command_name() == "stockinfo") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));
      std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

      double price = getStockPrice(symbol);

      if (price == -1.0) {
        event.reply("Invalid ticker.");
        return;
      }

      double change = getChange(symbol);
      double percentChange = getPercentChange(symbol);

      std::ostringstream oss;
      oss.imbue(std::locale(""));

      if (price < 10.0) {
        oss << std::fixed << std::setprecision(4) << price;
      } else {
        oss << std::fixed << std::setprecision(2) << price;
      }
      std::string priceString = oss.str();

      oss.str("");

      if (price < 10.0) {
        oss << std::fixed << std::setprecision(4) << change;
      } else {
        oss << std::fixed << std::setprecision(2) << change;
      }
      std::string changeString = oss.str();

      oss.str("");

      oss << std::fixed << std::setprecision(2) << std::abs(percentChange);
      std::string percentChangeString = oss.str();

      std::ostringstream replyStream;

      replyStream << "## Stock Data for " << symbol << "\n> **$" << priceString
                  << " USD**"
                  << "\n> " << (change > 0.0 ? "+" : "") << changeString << " ("
                  << percentChangeString << "%) "
                  << (percentChange < 0.0 ? "↓" : "↑");

      std::string reply = replyStream.str();

      event.reply(reply);
    }

    if (event.command.get_command_name() == "stocks") {
      std::vector<std::pair<std::string, int>> stocks =
          dbHandler.getUserStocks(user.id.str());

      std::ostringstream replyStream;
      replyStream << "## <@" << user.id.str() << ">'s Stocks:";

      int stockCount = 0;
      for (const auto &stock : stocks) {
        if (stock.second == 0) {
          continue;
        }

        std::string symbol = stock.first;
        int quantity = stock.second;

        std::ostringstream oss;
        oss.imbue(std::locale(""));

        oss << quantity;

        replyStream << "\n> **Stock: " << symbol
                    << "**\n> - Quantity: " << oss.str();

        stockCount++;
      }

      if (stockCount == 0) {
        event.reply("No stocks to display.");
        return;
      }

      std::string reply = replyStream.str();

      event.reply(reply);
    }

    if (event.command.get_command_name() == "balance") {
      double balance = dbHandler.getUserBalance(user.id.str());

      std::ostringstream oss;
      oss.imbue(std::locale(""));
      oss << std::fixed << std::setprecision(2) << balance;
      std::string balanceString = oss.str();

      std::ostringstream replyStream;
      replyStream << "## <@" << user.id.str() << ">'s Balance:\n> $"
                  << balanceString;

      event.reply(replyStream.str());
    }

    if (event.command.get_command_name() == "buy") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));
      std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

      std::optional<int64_t> quantityOptional =
          std::get<std::int64_t>(event.get_parameter("quantity"));

      int quantity = 1;

      if (quantityOptional.has_value()) {
        if (quantityOptional.value() <= 0) {
          event.reply("Invalid quantity.");
          return;
        }

        quantity = static_cast<int>(quantityOptional.value());
      }

      double price = getStockPrice(symbol);

      if (price == -1.0) {
        event.reply("Invalid ticker.");
        return;
      }

      double balance = dbHandler.getUserBalance(user.id.str());

      if (price * quantity > balance) {
        event.reply("Insufficient funds.");
        return;
      }

      std::ostringstream oss;
      oss.imbue(std::locale(""));
      oss << quantity;

      std::string quantityString = oss.str();

      oss.str("");

      oss << std::fixed << std::setprecision(2) << (price * quantity);

      std::string priceString = oss.str();

      std::ostringstream replyStream;
      replyStream << "Successfully purchased **" << quantityString << " "
                  << symbol << "** stock" << (quantity > 1 ? "s" : "")
                  << " for $**" << priceString << "**.";

      if (dbHandler.updateUserBalance(user.id.str(), price * quantity * -1.0)) {
        if (dbHandler.updateUserStock(user.id.str(), symbol, quantity)) {
          event.reply(replyStream.str());
        }

        dbHandler.updateTransactionsHistory(user.id.str(), symbol, quantity,
                                            price * quantity * -1.0,
                                            getCurrentTimestamp());
      }
    }

    if (event.command.get_command_name() == "sell") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));
      std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

      std::optional<int64_t> quantityOptional =
          std::get<int64_t>(event.get_parameter("quantity"));

      int userQuantity = dbHandler.getUserStockQuantity(user.id.str(), symbol);
      int quantity = 1;

      if (quantityOptional.has_value()) {
        if (quantityOptional.value() <= 0 || quantity > userQuantity) {
          event.reply("Invalid quantity.");
          return;
        }

        quantity = static_cast<int>(quantityOptional.value());
      }

      double price = getStockPrice(symbol);

      if (price == -1.0) {
        event.reply("Invalid ticker.");
      }

      double balance = dbHandler.getUserBalance(user.id.str());

      std::ostringstream oss;
      oss.imbue(std::locale(""));
      oss << quantity;

      std::string quantityString = oss.str();

      oss.str("");

      oss << std::fixed << std::setprecision(2) << (price * quantity);

      std::string priceString = oss.str();

      std::ostringstream replyStream;
      replyStream << "Successfully sold **" << quantityString << " " << symbol
                  << "** stock" << (quantity > 1 ? "s" : "") << " for $**"
                  << priceString << "**.";

      if (dbHandler.updateUserStock(user.id.str(), symbol, quantity * -1)) {
        if (dbHandler.updateUserBalance(user.id.str(), price * quantity)) {
          event.reply(replyStream.str());
        }

        dbHandler.updateTransactionsHistory(user.id.str(), symbol, quantity,
                                            price * quantity,
                                            getCurrentTimestamp());
      }
    }

    if (event.command.get_command_name() == "history") {
      std::vector<std::vector<std::string>> history =
          dbHandler.getUserHistory(user.id.str());

      if (history.size() == 0) {
        event.reply("No history to display.");
        return;
      }

      std::ostringstream replyStream;
      replyStream << "## <@" << user.id.str() << ">'s Transactions:";

      int count = 1;
      for (auto &values : history) {
        std::ostringstream oss;
        oss.imbue(std::locale(""));

        oss << std::stoi(values[1]);

        std::string quantityString = oss.str();

        oss.str("");

        oss << std::fixed << std::setprecision(2)
            << std::abs(std::stod(values[2]));

        std::string valueString = oss.str();

        replyStream << "\n> **" << count++ << ".** **"
                    << (std::stod(values[2]) < 0.0 ? "Bought " : "Sold ")
                    << values[0] << "**"
                    << "\n>     Quantity: " << quantityString
                    << "\n>     Total Value: $" << valueString << " USD"
                    << "\n>     Date: " << values[3];
      }

      std::string reply = replyStream.str();

      event.reply(reply);
    }

    if (event.command.get_command_name() == "help") {
      std::string reply = "## Commands"
                          "\n> `/stockinfo [ticker]` - Retrieve data for a "
                          "stock given the ticker"
                          "\n> `/balance` - Display your current balance"
                          "\n> `/buy [ticker] [quantity]` - Purchase stocks of "
                          "the given ticker and quantity"
                          "\n> `/sell [ticker] [quantity]` - Sell stocks of "
                          "the given ticker and quantity"
                          "\n> `/stocks` - Display your current stocks"
                          "\n> `/history` - Display your past transactions"
                          "\n> `/help` - Display this help message";

      event.reply(reply);
    }
  });

  bot.on_ready([&bot](const dpp::ready_t &event) {
    bot.set_presence(
        dpp::presence(dpp::ps_online, dpp::at_game, "with stonks"));

    if (dpp::run_once<struct clear_bot_commands>()) {
      bot.global_bulk_command_delete();
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand stockinfocommand(
          "stockinfo", "Retrieves data for a stock.", bot.me.id);
      stockinfocommand.add_option(dpp::command_option(
          dpp::co_string, "ticker", "The ticker for the stock", true));

      dpp::slashcommand balancecommand("balance", "Displays your balance.",
                                       bot.me.id);

      dpp::slashcommand buycommand("buy", "Purchase stocks.", bot.me.id);
      buycommand
          .add_option(dpp::command_option(dpp::co_string, "ticker",
                                          "The ticker for the stock", true))
          .add_option(dpp::command_option(dpp::co_integer, "quantity",
                                          "The amount of stocks to buy", true));

      dpp::slashcommand sellcommand("sell", "Sell your stocks.", bot.me.id);
      sellcommand
          .add_option(dpp::command_option(dpp::co_string, "ticker",
                                          "The ticker for the stock", true))
          .add_option(dpp::command_option(dpp::co_integer, "quantity",
                                          "The amount of stocks to sell",
                                          true));

      dpp::slashcommand getstockscommand("stocks", "Displays your stocks.",
                                         bot.me.id);

      dpp::slashcommand historycommand(
          "history", "Displays your past transactions.", bot.me.id);

      dpp::slashcommand helpcommand("help", "Displays a list of commands.",
                                    bot.me.id);

      bot.global_command_create(stockinfocommand);
      bot.global_command_create(balancecommand);
      bot.global_command_create(buycommand);
      bot.global_command_create(sellcommand);
      bot.global_command_create(getstockscommand);
      bot.global_command_create(historycommand);
      bot.global_command_create(helpcommand);
    }
  });

  bot.start(dpp::st_wait);

  return 0;
}
