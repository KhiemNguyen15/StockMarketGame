#include <dpp/appcommand.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/once.h>
#include <fstream>
#include <iomanip>
#include <ios>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <sstream>
#include <string>

#include "../include/stockRetriever.h"

const std::string configPath = "../data/config.json";

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
  dpp::cluster bot(getBotToken());

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&bot](const dpp::slashcommand_t &event) {
    if (event.command.get_command_name() == "getstock") {
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

      std::string reply = "Stock data for " + symbol + ":\n" + "Price: $" +
                          priceString +
                          "\nPercent Change: " + percentChangeString + "%";

      event.reply(reply);
    }
  });

  bot.on_ready([&bot](const dpp::ready_t &event) {
    if (dpp::run_once<struct clear_bot_commands>()) {
      bot.global_bulk_command_delete();
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand getstockcommand(
          "getstock", "Retrieves data for a stock.", bot.me.id);
      getstockcommand.add_option(dpp::command_option(
          dpp::co_string, "ticker", "The ticker for the stock", true));

      bot.global_command_create(getstockcommand);
    }
  });

  bot.start(dpp::st_wait);

  return 0;
}
