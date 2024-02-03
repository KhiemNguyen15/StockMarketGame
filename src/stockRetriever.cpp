#include <curl/curl.h>
#include <curl/easy.h>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <sstream>
#include <string>

const char *configPath = "../data/config.json";

std::string getApiKey() {
  std::ifstream file(configPath, std::ifstream::in);

  if (!file.is_open()) {
    std::cerr << "Error opening file: " << configPath << std::endl;
    return "";
  }

  Json::Value root;
  Json::CharReaderBuilder reader;
  Json::parseFromStream(reader, file, &root, nullptr);

  std::string apiKey = root["alpha_vantage_api_key"].asString();

  return apiKey;
}

size_t writeCallback(void *contents, size_t size, size_t nmemb,
                     std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

std::string retrieveJsonData(const std::string &symbol) {
  CURL *curl = curl_easy_init();
  std::string url =
      "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=" +
      symbol + "&apikey=" + getApiKey();

  if (curl) {
    CURLcode res;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      std::cerr << "Failed to retrieve data: " << curl_easy_strerror(res)
                << std::endl;
      curl_easy_cleanup(curl);
      return "";
    }

    curl_easy_cleanup(curl);
    return response;
  }

  return "";
}

double parseStockPrice(const std::string &jsonData) {
  Json::CharReaderBuilder reader;
  Json::Value root;
  std::istringstream jsonStream(jsonData);
  Json::parseFromStream(reader, jsonStream, &root, nullptr);

  if (root.isMember("Global Quote")) {
    std::string priceString = root["Global Quote"]["05. price"].asString();
    return std::stod(priceString);
  } else {
    std::cerr << "Failed to extract price from JSON response." << std::endl;
    return -1.0;
  }
}

double parsePercentChange(const std::string &jsonData) {
  Json::CharReaderBuilder reader;
  Json::Value root;
  std::istringstream jsonStream(jsonData);
  Json::parseFromStream(reader, jsonStream, &root, nullptr);

  if (root.isMember("Global Quote")) {
    std::string percentChangeString =
        root["Global Quote"]["10. change percent"].asString();

    percentChangeString.pop_back();

    double percentChange = std::stod(percentChangeString);

    return percentChange;
  } else {
    std::cerr << "Failed to extract percent change from JSON response."
              << std::endl;
    return -1.0;
  }
}

double getStockPrice(const std::string &symbol) {
  std::string jsonData = retrieveJsonData(symbol);

  if (!jsonData.empty()) {
    return parseStockPrice(jsonData);
  } else {
    return -1.0;
  }
}

double getPercentChange(const std::string &symbol) {
  std::string jsonData = retrieveJsonData(symbol);

  if (!jsonData.empty()) {
    return parsePercentChange(jsonData);
  } else {
    return -1.0;
  }
}
