#include "apiCallers.hpp"

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static size_t
read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct MemoryStruct *pooh = (struct MemoryStruct *)userp;
    if(size*nmemb < 1)
        return 0;
    if(pooh->size) {
        *(char *)ptr = pooh->memory[0];
        pooh->memory++;
        pooh->size--;
        return 1;
    }
    return 0;
}

TreeReceiver::TreeReceiver
(const std::string& url, const std::string& pback)
{
    urlAPI = (char*) url.c_str();
    this->setPostback(pback);
    curlHandler = curl_easy_init();
    chunkReceive.memory = (char*) malloc(1);
    chunkReceive.size = 0;
}

void TreeReceiver::setPostback(const std::string& pback) {
    if(pback != "") {
        postback = (char*) pback.c_str();
        chunkPost.memory = (char*) pback.c_str();
        chunkPost.size = pback.length();
    }
    else {
        postback = nullptr;
    }
}

boost::property_tree::ptree
TreeReceiver::getTree()
{
    if(curlHandler) {
        curl_easy_setopt(curlHandler, CURLOPT_URL, (const char*) urlAPI);
        if(postback != nullptr) {
            chunkPost.memory = (char*) postback;
            chunkPost.size = (size_t) strlen(postback);
            curl_easy_setopt(curlHandler, CURLOPT_POST, 1L);
            curl_easy_setopt(curlHandler, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curlHandler, CURLOPT_READDATA, &chunkPost);
#ifdef USE_CHUNKED
            {
                struct curl_slist *chunk = NULL;
                chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
                res = curl_easy_setopt(curlHandler, CURLOPT_HTTPHEADER, chunk);
            }
#else
            curl_easy_setopt
                (curlHandler, CURLOPT_POSTFIELDSIZE, chunkPost.size);
#endif
#ifdef DISABLE_EXPECT
            {
                struct curl_slist *chunk = NULL;
                chunk = curl_slist_append(chunk, "Expect:");
                res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            }
#endif
        }
        chunkReceive.memory = (char*) malloc(1);
        chunkReceive.size = 0;
        curl_easy_setopt
            (curlHandler, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt
            (curlHandler, CURLOPT_WRITEDATA, (void *)&chunkReceive);
        curl_easy_setopt
            (curlHandler, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        res = curl_easy_perform(curlHandler);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        std::stringstream streamedChunk;
        streamedChunk << chunkReceive.memory;
        boost::property_tree::json_parser::
            read_json(streamedChunk, receivedTree);

        return(receivedTree);
    }
}

bitfinexSpot::bitfinexSpot(char* _quotingType)
    : bitfinexUrl("https://api.bitfinex.com/v1/pubticker/BTCUSD"),
      spotReceiver(bitfinexUrl,""),
      quotingType(_quotingType)
{ }

double bitfinexSpot::getSpot() {
    priceTree = spotReceiver.getTree();
    return priceTree.get<double>(quotingType);
}

coinutOrderbook::
coinutOrderbook(char* _derivativeType, char* _optionType, int _expiryDate,
                double _strike, std::string _quoteType)

    : expiryDate(_expiryDate),
      strike(_strike),
      derivativeType(_derivativeType),
      optionType(_optionType),
      quoteType(_quoteType),
      coinutUrl("https://coinut.com/api/orderbook"),
      orderbookReceiver(coinutUrl, "")

{
    std::stringstream orderbookPost;
    orderbookPost << "{ \"deriv_type\": \"" <<  "VANILLA_OPTION"     << "\", "
                  << "\"asset\": \""        <<  "BTCUSD"             << "\", "
                  << "\"expiry_time\": "    <<  expiryDate           << ", "
                  << "\"strike\": \""       <<  (int) strike         << "\", "
                  << "\"put_call\": \""     <<  "CALL" << "\" }";

    tempOrderbookPost = orderbookPost.str();
    orderbookReceiver.setPostback(tempOrderbookPost);
}

double coinutOrderbook::getOptionPrice() {

    double optPrice = 0.0;
    double askPrice = -1.0;
    double bidPrice = -1.0;
    double temp = 0.0;

    orderbookTree = orderbookReceiver.getTree();

    if (quoteType == "ask" || quoteType == "mid") {
        for (const auto& kv : orderbookTree.get_child("ask")) {
            temp = kv.second.get<double>("price");
            if(askPrice < 0 || askPrice > temp)
                {askPrice = temp;}
        }
        optPrice += askPrice;
    }

    if (quoteType == "bid" || quoteType == "mid") {
        for (const auto& kv : orderbookTree.get_child("bid")) {
            temp = kv.second.get<double>("price");
            if(bidPrice < 0 || bidPrice < temp)
                {bidPrice = temp;}
        }
        optPrice += bidPrice;
    }

    if(quoteType == "mid")
        optPrice /= 2.0;

    optPrice *= 100.0;

    return optPrice;
}

bitfinexLendbook::
bitfinexLendbook(std::string _rateCurrency, std::string _quoteType)

    : bitfinexUrl("https://api.bitfinex.com/v1/lendbook/" + _rateCurrency),
      lendbookReceiver(bitfinexUrl, ""),
      rateCurrency(_rateCurrency),
      quoteType(_quoteType)

{ }

double bitfinexLendbook::getRate() {
    double rate = 0.0;
    double temp;
    double bidRate = -1.0;
    double askRate = -1.0;
    lendbookTree = lendbookReceiver.getTree();

    if (quoteType == "ask" || quoteType == "mid") {
        for (const auto& kv : lendbookTree.get_child("asks")) {
            temp = kv.second.get<double>("rate");
            if(askRate < 0 || askRate > temp)
                {askRate = temp;}
        }
        rate += askRate;
    }

    if (quoteType == "bid" || quoteType == "mid") {
        for (const auto& kv : lendbookTree.get_child("bids")) {
            temp = kv.second.get<double>("rate");
            if(bidRate < 0 || bidRate < temp)
                {bidRate = temp;}
        }
        rate += bidRate;
    }

    if(quoteType == "mid")
        rate /= 2.0;

    rate /= 100.0;

    return rate;
}
