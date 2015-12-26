#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <curl/curl.h>

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

static size_t
read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

class TreeReceiver
{
public:
    boost::property_tree::ptree receivedTree;
    char* urlAPI;
    char* postback ;
    TreeReceiver(const std::string&,const std::string&);
    void setPostback(const std::string&);
    boost::property_tree::ptree getTree();
private:
    CURL* curlHandler;
    CURLcode res;
    struct MemoryStruct chunkReceive;
    struct MemoryStruct chunkPost;
};

class bitfinexSpot {
public:
    double getSpot();
    char* quotingType;
    bitfinexSpot(char* _quotingType);
private:
    const std::string bitfinexUrl;
    TreeReceiver spotReceiver;
    boost::property_tree::ptree priceTree;
};

class coinutOrderbook {
public:
    coinutOrderbook
    (char* _derivativeType, char* _optionType,
     int _expiryDate, double _strike, std::string _quoteType);
    double getOptionPrice();
    char* derivativeType;
    char* optionType;
    int expiryDate;
    double strike;
    std::string quoteType;
private:
    const std::string coinutUrl;
    std::string tempOrderbookPost;
    TreeReceiver orderbookReceiver;
    boost::property_tree::ptree orderbookTree;
};

class bitfinexLendbook {
public:
    double getRate();
    std::string rateCurrency;
    std::string quoteType;
    bitfinexLendbook(std::string _rateCurrency,
                     std::string _quoteType);
private:
    const std::string bitfinexUrl;
    TreeReceiver lendbookReceiver;
    boost::property_tree::ptree lendbookTree;
};
