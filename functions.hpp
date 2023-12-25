//
// Created by 范易扬 on 2023/12/23.
//

#ifndef TRADINGSYSTEM_FUNCTIONS_HPP
#define TRADINGSYSTEM_FUNCTIONS_HPP

#include <iostream>
#include <string>
#include <chrono>
#include "products.hpp"

using namespace std;
using namespace chrono;

// Generate uniformly distributed random variables between 0 to 1.
vector<double> GenerateUniform(long N, long seed = 0)
{
    long m = 2147483647;
    long a = 39373;
    long q = m / a;
    long r = m % a;

    if (seed == 0) seed = time(0);
    seed = seed % m;
    vector<double> result;
    for (long i = 0; i < N; i++)
    {
        long k = seed / q;
        seed = a * (seed - k * q) - k * r;
        if (seed < 0) seed = seed + m;
        result.push_back(seed / (double)m);
    }
    return result;
}

// Get Bond object for US Treasury 2Y, 3Y, 5Y, 7Y, 10Y, and 30Y.
Bond GetBond(string _cusip)
{
    Bond _bond;
    if (_cusip == "9128283H1") _bond = Bond("9128283H1", CUSIP, "US2Y", 0.01750, from_string("2019/11/30"));
    if (_cusip == "9128283L2") _bond = Bond("9128283L2", CUSIP, "US3Y", 0.01875, from_string("2020/12/15"));
    if (_cusip == "912828M80") _bond = Bond("912828M80", CUSIP, "US5Y", 0.02000, from_string("2022/11/30"));
    if (_cusip == "9128283J7") _bond = Bond("9128283J7", CUSIP, "US7Y", 0.02125, from_string("2024/11/30"));
    if (_cusip == "9128283F5") _bond = Bond("9128283F5", CUSIP, "US10Y", 0.02250, from_string("2027/12/15"));
    if (_cusip == "912810RZ3") _bond = Bond("912810RZ3", CUSIP, "US30Y", 0.02750, from_string("2047/12/15"));
    return _bond;
}

// Get PV01 value for US Treasury 2Y, 3Y, 5Y, 7Y, 10Y, and 30Y.
double GetPV01Value(string _cusip)
{
    double _pv01 = 0;
    if (_cusip == "9128283H1") _pv01 = 0.01948992;
    if (_cusip == "9128283L2") _pv01 = 0.02865304;
    if (_cusip == "912828M80") _pv01 = 0.04581119;
    if (_cusip == "9128283J7") _pv01 = 0.06127718;
    if (_cusip == "9128283F5") _pv01 = 0.08161449;
    if (_cusip == "912810RZ3") _pv01 = 0.15013155;
    return _pv01;
}

// Convert fraction price to decimal price
double ConvertPrice(string _stringPrice)
{
    // empty string and push letter in it
    string _stringPrice100 = "";
    string _stringPrice32  = "";
    string _stringPrice8   = "";

    int count= 0;

    // logic to split the string
    for (int i = 0; i < _stringPrice.size(); i++)
    {
        if (_stringPrice[i] == '-')
        {
            count++;
            continue;
        }
        if (count == 0)
        {
            _stringPrice100.push_back(_stringPrice[i]);
        }
        else if (count == 1 || count == 2)
        {
            _stringPrice32.push_back(_stringPrice[i]);
            count++;
        }
        else if (count == 3)
        {
            _stringPrice8.push_back(_stringPrice[i] == '+' ? '4' : _stringPrice[i]);
        }
    }

    // string to decimal
    double _doublePrice100 = stod(_stringPrice100);
    double _doublePrice32 = stod(_stringPrice32);
    double _doublePrice8 = stod(_stringPrice8);
    //
    double _doublePrice = _doublePrice100 + _doublePrice32 * 1.0 / 32.0 + _doublePrice8 * 1.0 / 256.0;
    return _doublePrice;
}

// Convert decimal price to fraction price
string ConvertPrice(double _doublePrice) {
    int _doublePrice100 = floor(_doublePrice);
    int _doublePrice256 = floor((_doublePrice - _doublePrice100) * 256.0);
    int _doublePrice32 = floor(_doublePrice256 / 8.0);
    int _doublePrice8 = _doublePrice256 % 8;

    string _stringPrice100 = to_string(_doublePrice100);
    string _stringPrice32 = to_string(_doublePrice32);
    string _stringPrice8 = to_string(_doublePrice8);

    // We then apply the special rule of Bond price on it
    if (_doublePrice32 < 10) _stringPrice32 = "0" + _stringPrice32;
    if (_doublePrice8 == 4) _stringPrice8 = "+";

    string _stringPrice = _stringPrice100 + "-" + _stringPrice32 + _stringPrice8;
    return _stringPrice;
}

// Convert string to date
string TimeStamp()
{
    auto _timePoint = system_clock::now();
    auto _sec = chrono::time_point_cast<chrono::seconds>(_timePoint);
    auto _millisec = chrono::duration_cast<chrono::milliseconds>(_timePoint - _sec);
    auto _millisecCount = _millisec.count();
    auto _milliString = to_string(_millisecCount);

    if (_millisecCount < 10) _milliString = "00" + _milliString;
    else if (_millisecCount < 100) _milliString = "0" + _milliString;

    auto _timeT = system_clock::to_time_t(_timePoint);
    char _timeChar[24];
    strftime(_timeChar, 24, "%F %T", localtime(&_timeT));
    string _timeString = string(_timeChar) + "." + _milliString + " ";

    return _timeString;
}

//long GerMillesecond()
//{
//    auto _timePoint = system_clock::now();
//    auto _sec = chrono::time_point_cast<chrono::seconds>(_timePoint);
//    auto _millisec = chrono::duration_cast<chrono::milliseconds>(_timePoint - _sec);
//    long _millisecCount = _millisec.count();
//
//    return _millisecCount;
//}

// Get the millisecond count of current time.
long GetMilliseconds()
{
    auto duration = chrono::system_clock::now().time_since_epoch();
    return chrono::duration_cast<chrono::milliseconds>(duration).count() % 1000;
}

// Generate Random IDs for data
string GenerateId()
{
    string _base = "0123456789QWERTYUIOPASDFGHJKLZXCVBNM";
    vector<double> _randoms = GenerateUniform(12, GetMilliseconds());
    string _id = "";
    for (auto& r : _randoms)
    {
        int i = r*36;
        _id.push_back(_base[i]);
    }
    return _id;
}

#endif //TRADINGSYSTEM_FUNCTIONS_HPP
