#include <curl/curl.h>
#include <curl/easy.h>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <sstream>
#include <string>

const std::string configPath = "../data/config.json";

std::string getApiKey() {
  std::ifstream file(configPath, std::ifstream::in);

  if (!file.is_open()) {
    std::cerr << "Error opening file: " << configPath << std::endl;
    return "";
  }

  Json::Value root;
  Json::CharReaderBuilder reader;
  Json::parseFromStream(reader, file, &root, nullptr);

  std::string apiKey = root["finnhub_api_key"].asString();

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
  std::string url = "https://finnhub.io/api/v1/quote?symbol=" + symbol +
                    "&token=" + getApiKey();

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

  double price = root["c"].asDouble();

  if (price == 0) {
    std::cerr << "Unable to retrieve price value." << std::endl;
    return -1.0;
  }

  return price;
}

double parseChange(const std::string &jsonData) {
  Json::CharReaderBuilder reader;
  Json::Value root;
  std::istringstream jsonStream(jsonData);
  Json::parseFromStream(reader, jsonStream, &root, nullptr);

  if (root["d"].isNull()) {
    std::cerr << "Change value is null." << std::endl;
    return -1.0;
  }

  return root["d"].asDouble();
}

double parsePercentChange(const std::string &jsonData) {
  Json::CharReaderBuilder reader;
  Json::Value root;
  std::istringstream jsonStream(jsonData);
  Json::parseFromStream(reader, jsonStream, &root, nullptr);

  if (root["dp"].isNull()) {
    std::cerr << "Percent change value is null." << std::endl;
    return -1.0;
  }

  return root["dp"].asDouble();
}

double getStockPrice(const std::string &symbol) {
  std::string jsonData = retrieveJsonData(symbol);

  if (!jsonData.empty()) {
    return parseStockPrice(jsonData);
  } else {
    return -1.0;
  }
}

double getChange(const std::string &symbol) {
  std::string jsonData = retrieveJsonData(symbol);

  if (!jsonData.empty()) {
    return parseChange(jsonData);
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
