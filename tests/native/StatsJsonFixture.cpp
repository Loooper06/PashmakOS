#include "StatsJson.h"
#include <iostream>
#include <string>

class TestJsonString
{
public:
    std::string value;

    TestJsonString &operator+=(char ch)
    {
        value += ch;
        return *this;
    }

    TestJsonString &operator+=(const char *text)
    {
        value += text;
        return *this;
    }
};

int main()
{
    const StatsJson::Pet pets[] = {
        {0, 91, "Pashmak", "happy", "hello \"friend\"\\path\nnext"},
        {1, 64, "Mochi", "sleepy", "quiet\tmode"},
        {2, 12, "Pixel", "curious", "control\x01byte"}};

    const StatsJson::DailyRecord daily[] = {
        {20261023, 4294967000U, 200, 30, 7},
        {20261024, 1234, 567, 89, 2}};

    const StatsJson::MonthlyRecord monthly[] = {
        {202609, 5000000000ULL, 600, 70, 20},
        {202610, 7000000000ULL, 800, 90, 22}};

    const StatsJson::Payload payload{
        1,
        pets,
        3,
        StatsJson::Pet{1, 64, nullptr, "sleepy", "quiet\tmode"},
        StatsJson::Counters{9000000000ULL, 2000000000ULL, 300000000ULL, 1234},
        StatsJson::Recent{7, 2, StatsJson::Counters{5000, 600, 70, 3}},
        daily,
        2,
        monthly,
        2};

    TestJsonString json;
    StatsJson::appendStatisticsJson(json, payload);
    std::cout << json.value << '\n';
    return 0;
}
