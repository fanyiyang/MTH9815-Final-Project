/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order() = default;
  Order(double _price, long _quantity, PricingSide _side);

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};

Order::Order(double _price, long _quantity, PricingSide _side)
{
    price = _price;
    quantity = _quantity;
    side = _side;
}

double Order::GetPrice() const
{
    return price;
}

long Order::GetQuantity() const
{
    return quantity;
}

PricingSide Order::GetSide() const
{
    return side;
}
/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder):
    bidOrder(_bidOrder), offerOrder(_offerOrder)
    {};

  // Get the bid order
  const Order& GetBidOrder() const
  {
      return bidOrder;
  };

  // Get the offer order
  const Order& GetOfferOrder() const
  {
      return offerOrder;
  };

private:
  Order bidOrder;
  Order offerOrder;

};

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:

  // ctor for the order book
  OrderBook() = default;
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);

  // Get the product
  const T& GetProduct() const
  {
      return product;
  };

  // Get the bid stack
  const vector<Order>& GetBidStack() const
  {
      return bidStack;
  };

  // Get the offer stack
  const vector<Order>& GetOfferStack() const
  {
      return offerStack;
  };

  // Get the best bid/offer order
  const BidOffer GetBidOffer() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};

template<typename T>
OrderBook<T>::OrderBook(const T& _product, const vector<Order>& _bidStack, const vector<Order>& _offerStack) :
        product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const BidOffer OrderBook<T>::GetBidOffer() const
{
    double _bidPrice = INT_MIN;
    Order _bidOrder;
    for (auto& b : bidStack)
    {
        double _price = b.GetPrice();
        if (_price > _bidPrice)
        {
            _bidPrice = _price;
            _bidOrder = b;
        }
    }

    double _offerPrice = INT_MAX;
    Order _offerOrder;
    for (auto& o : offerStack)
    {
        double _price = o.GetPrice();
        if (_price < _offerPrice)
        {
            _offerPrice = _price;
            _offerOrder = o;
        }
    }

    return BidOffer(_bidOrder, _offerOrder);
}

template<typename T>
class MarketDataConnector;

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{
private:
    map<string, OrderBook<T>> orderBooks;
    vector<ServiceListener<OrderBook<T>>*> listeners;
    MarketDataConnector<T>* connector;
    int bookDepth;
public:
    MarketDataService();
    OrderBook<T>& GetData(string _key);

    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(OrderBook<T>& _data);

    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<OrderBook<T>>* _listener)
    {
        listeners.push_back(_listener);
    };

    // Get all listeners on the Service
    const vector<ServiceListener<OrderBook<T>>*>& GetListeners() const
    {
        return listeners;
    };

    // Get the connector of the service
    MarketDataConnector<T>* GetConnector()
    {
        return connector;
    } ;

    // Get the order book depth of the service
    int GetBookDepth() const
    {
        return bookDepth;
    };

    // Get the best bid/offer order
    const BidOffer& GetBestBidOffer(const string &productId)
    {
        return orderBooks[productId].GetBidOffer();
    }

    // Aggregate the order book
    const OrderBook<T>& AggregateDepth(const string &productId);

};

template<typename T>
MarketDataService<T>::MarketDataService()
{
    orderBooks = map<string, OrderBook<T>>();
    listeners = vector<ServiceListener<OrderBook<T>>*>();
    connector = new MarketDataConnector<T>(this);
    bookDepth = 5;
}

template<typename T>
OrderBook<T>& MarketDataService<T>::GetData(string _key)
{
    return orderBooks[_key];
}

template<typename T>
void MarketDataService<T>::OnMessage(OrderBook<T>& _data)
{
    orderBooks[_data.GetProduct().GetProductId()] = _data;
    for (auto& l : listeners)
    {
        l->ProcessAdd(_data);
    }
}

template<typename T>
const OrderBook<T>& MarketDataService<T>::AggregateDepth(const std::string &productId)
{
    T& _product = orderBooks[productId].GetProduct();

    vector<Order>& _bidStackFrom = orderBooks[productId].GetBidStack();
    unordered_map<double,long> _bidHashTable;
    for (auto& b: _bidStackFrom)
    {
        double _price = b.GetPrice();
        long _quantity = b.GetQuantity();
        _bidHashTable[_price] += _quantity;
    }
    vector<Order> _bidStackTo;
    for (auto& b: _bidHashTable)
    {
        _bidStackTo.push_back(Order(b.first, b.second, BID));
    }

    // Same for offer stack
    vector<Order>& _offerStackFrom = orderBooks[productId].GetOfferStack();
    unordered_map<double,long> _offerHashTable;
    for (auto& o: _offerStackFrom)
    {
        double _price = o.GetPrice();
        long _quantity = o.GetQuantity();
        _offerHashTable[_price] += _quantity;
    }
    vector<Order> _offerStackTo;
    for (auto& o: _offerHashTable)
    {
        _offerStackTo.push_back(Order(o.first, o.second, OFFER));
    }

    return OrderBook<T>(_product, _bidStackTo, _offerStackTo);
}

/**
 * Market Data Connector
 **/

template<typename T>
class MarketDataConnector : public Connector<OrderBook<T>>
{
private:
    MarketDataService<T>* service;
public:
    MarketDataConnector(MarketDataService<T>* _service)
    {
        service = _service;
    };
    void Publish(OrderBook<T>& _data){}; // No need for Publish
    void Subscribe(ifstream& _data);
};

template<typename T>
void MarketDataConnector<T>::Subscribe(ifstream& _data)
{
    int _bookDepth = service->GetBookDepth();
    int _thread = _bookDepth*2;
    long _count = 0;
    vector<Order> _bidStack;
    vector<Order> _offerStack;
    string _line;
    while (getline(_data, _line))
    {
        string _productId;
        stringstream _lineStream(_line);

        string _cell;
        vector<string> _cells;

        while (getline(_lineStream, _cell, ','))
        {
            _cells.push_back(_cell);
        }

        _productId = _cells[0];
        double _price = ConvertPrice(_cells[1]);
        long _quantity = stol(_cells[2]);
        PricingSide _side = (_cells[3] == "BID") ? BID : OFFER;

        Order _order(_price, _quantity, _side);
        switch(_side)
        {
            case BID:
                _bidStack.push_back(_order);
                break;
            case OFFER:
                _offerStack.push_back(_order);
                break;
        }
        _count++;
        if (_count == _thread)
        {
            T _product = GetBond(_productId);
            OrderBook<T> _orderBook(_product, _bidStack, _offerStack);
            service->OnMessage(_orderBook);
            _count = 0;
            _bidStack.clear();
            _offerStack.clear();
        }


    }
}




#endif
