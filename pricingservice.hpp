/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include "soa.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price() = default;
  Price(const T &_product, double _mid, double _bidOfferSpread);

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

  vector<string> ToStrings() const;

private:
  T product;
  double mid;
  double bidOfferSpread;

};

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */

template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}

template<typename T>
vector<string> Price<T>::ToStrings() const {
    string _product = product.GetProductId();
    string _mid = ConvertPrice(mid);
    string _bidOfferSpread = ConvertPrice(bidOfferSpread);

    vector<string> _strings;
    _strings.push_back(_product);
    _strings.push_back(_mid);
    _strings.push_back(_bidOfferSpread);
    return _strings; // a vector of strings of product, mid, bidOfferSpread
}

template<typename T>
class PricingConnector;


template<typename T>
class PricingService : public Service<string, Price<T>>
{
private:
    map<string, Price<T>> prices;
    vector<ServiceListener<Price<T>>*> listeners;
    PricingConnector<T>* connector;

public:
    PricingService();
    ~PricingService();

    // First, get the data
    Price<T>& GetData(string _key);

    // callback function of a Connector that should invoke for any new data
    void OnMessage(Price<T>& _data);

    // Add a listener to the Service for callbacks
    void AddListener(ServiceListener<Price<T>>* _listener);

    // Get all listeners on the Service
    const vector<ServiceListener<Price<Bond>> *>& GetListeners() const;

    // Get the connector of the service
    PricingConnector<T>* GetConnector();
};

template<typename T>
PricingService<T>::PricingService()
{
    prices = map<string, Price<T>>();
    listeners =vector<ServiceListener<Price<T>>*>();
    connector = new PricingConnector<T>(this);
}

template<typename T>
PricingService<T>::~PricingService() {}

template<typename T>
Price<T>& PricingService<T>::GetData(string _key)
{
    return prices[_key];
}

template<typename T>
void PricingService<T>::OnMessage(Price<T> &_data)
{
    prices[_data.GetProduct().GetProductId()] = _data;

    for (auto& l: listeners)
    {
        l->ProcessAdd(_data);
    }
}

template<typename T>
void PricingService<T>::AddListener(ServiceListener<Price<T>>* _listener)
{
    listeners.push_back(_listener);
}
template<typename T>
const vector<ServiceListener<Price<Bond>> *>& PricingService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
PricingConnector<T> *PricingService<T>::GetConnector()
{
    return connector;
}



template<typename T>
class PricingConnector : public Connector<Price<T>>
{
private:
    PricingService<T>* service;
public:
    // Constructor & Destructor
    PricingConnector(PricingService<T>* _service)
    {
        service = _service;
    };
    ~PricingConnector(){};

    // Publish data to the Connector
    void Publish(Price<T>& _data){};

    // Subscribe a connector
    void Subscribe(ifstream& _data);

};

template<typename T>
void PricingConnector<T>::Subscribe(ifstream& _data)
{
    string _line;
    while (getline(_data,_line))
    {
        stringstream _lineStream(_line);
        string _cell;
        vector<string> _tmps;
        while (getline(_lineStream, _cell, ','))
        {
            _tmps.push_back(_cell);
        }

        string _productId = _tmps[0];
        double _bid = stod(_tmps[1]);
        double _offer = stod(_tmps[2]);
        double _mid = (_bid + _offer) / 2.0;
        double _spread = _offer - _bid;
        T _product = GetBond(_productId);
        Price<T> _price(_product, _mid, _spread);
        service->OnMessage(_price);
    }
}

#endif
