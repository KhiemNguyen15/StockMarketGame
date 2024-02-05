#include <cstdint>
#include <dpp/appcommand.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/once.h>
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

int main(int argc, char *argv[]) {
  DatabaseHandler dbHandler(dbPath);
  dbHandler.createTables();

  dpp::cluster bot(getBotToken());

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&bot, &dbHandler](const dpp::slashcommand_t &event) {
    dpp::user user = event.command.get_issuing_user();

    if (event.command.get_command_name() == "stockinfo") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));

      double price = getStockPrice(symbol);
      double percentChange = getPercentChange(symbol);

      if (price == -1.0) {
        event.reply("Invalid ticker.");
        return;
      }

      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << price;
      std::string priceString = oss.str();

      oss.str("");

      oss << std::fixed << std::setprecision(4) << percentChange;
      std::string percentChangeString = oss.str();

      std::string reply = "**Stock data for " + symbol + ":**\n" + "Price: $" +
                          priceString +
                          "\nPercent Change: " + percentChangeString + "%";

      event.reply(reply);
    }

    if (event.command.get_command_name() == "mystocks") {
      std::vector<std::pair<std::string, int>> stocks =
          dbHandler.getUserStocks(user.id.str());

      std::ostringstream replyStream;
      replyStream << "**Your Stocks:**";

      for (const auto &stock : stocks) {
        std::string symbol = stock.first;
        int quantity = stock.second;

        replyStream << "\nStock: " << symbol << "\n  Quantity: " << quantity;
      }

      std::string reply = replyStream.str();

      event.reply(reply);
    }

    if (event.command.get_command_name() == "balance") {
      double balance = dbHandler.getUserBalance(user.id.str());

      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << balance;
      std::string balanceString = oss.str();

      std::string reply = "**Your Balance:**\n$" + balanceString;

      event.reply(reply);
    }

    if (event.command.get_command_name() == "buy") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));
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

      if (dbHandler.updateUserBalance(user.id.str(), price * quantity * -1.0)) {
        if (dbHandler.updateUserStock(user.id.str(), symbol, quantity)) {
          event.reply("Successfully purchased stocks!");
        }
      }
    }

    if (event.command.get_command_name() == "sell") {
      std::string symbol = std::get<std::string>(event.get_parameter("ticker"));
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

      if (dbHandler.updateUserStock(user.id.str(), symbol, quantity * -1)) {
        if (dbHandler.updateUserBalance(user.id.str(), price * quantity)) {
          event.reply("Successfully sold stocks!");
        }
      }
    }
  });

  bot.on_ready([&bot](const dpp::ready_t &event) {
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
      buycommand.add_option(dpp::command_option(
          dpp::co_string, "ticker", "The ticker for the stock", true));
      buycommand.add_option(dpp::command_option(
          dpp::co_integer, "quantity", "The amount of stocks to buy", true));

      dpp::slashcommand sellcommand("sell", "Sell your stocks.", bot.me.id);
      sellcommand.add_option(dpp::command_option(
          dpp::co_string, "ticker", "The ticker for the stock", true));
      sellcommand.add_option(dpp::command_option(
          dpp::co_integer, "quantity", "The amount of stocks to sell", true));

      dpp::slashcommand getstockscommand("mystocks", "Displays your stocks.",
                                         bot.me.id);

      bot.global_command_create(stockinfocommand);
      bot.global_command_create(balancecommand);
      bot.global_command_create(buycommand);
      bot.global_command_create(sellcommand);
      bot.global_command_create(getstockscommand);
    }
  });

  bot.start(dpp::st_wait);

  return 0;
}
