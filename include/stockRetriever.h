#ifndef STOCK_RETRIEVER_H
#define STOCK_RETRIEVER_H

#include <cstddef>
#include <string>

double getStockPrice(const std::string &symbol);
double getPercentChange(const std::string &symbol);

#endif // STOCK_RETRIEVER_H
