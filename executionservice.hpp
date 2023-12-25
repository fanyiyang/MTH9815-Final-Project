/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "marketdataservice.hpp"
#include "functions.hpp"

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder {
public:
    // Constructor
    ExecutionOrder() = default;
    ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder)
            : product(_product), side(_side), orderId(_orderId), orderType(_orderType), price(_price), visibleQuantity(_visibleQuantity), hiddenQuantity(_hiddenQuantity), parentOrderId(_parentOrderId), isChildOrder(_isChildOrder) {}

    // Get the product
    const T& GetProduct() const {
        return product;
    }

    PricingSide GetPricingSide() const
    {
        return side;
    };

    // Get the order ID
    const string& GetOrderId() const {
        return orderId;
    }

    // Get the order type
    OrderType GetOrderType() const {
        return orderType;
    }

    // Get the price
    double GetPrice() const {
        return price;
    }

    // Get the visible quantity
    long GetVisibleQuantity() const {
        return visibleQuantity;
    }

    // Get the hidden quantity
    long GetHiddenQuantity() const {
        return hiddenQuantity;
    }

    // Get the parent order ID
    const string& GetParentOrderId() const {
        return parentOrderId;
    }

    // Check if it is a child order
    bool IsChildOrder() const {
        return isChildOrder;
    }
    // Change attributes to strings
    vector<string> ToStrings() const;
private:
    T product;
    PricingSide side;
    string orderId;
    OrderType orderType;
    double price;
    double visibleQuantity;
    double hiddenQuantity;
    string parentOrderId;
    bool isChildOrder;
};

template<typename T>
class ExecutionToAlgoExecutionListener;

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class ExecutionService : public Service<string, ExecutionOrder<T>>
{

private:

    map<string, ExecutionOrder<T>> executionOrders;
    vector<ServiceListener<ExecutionOrder<T>>*> listeners;
    ExecutionToAlgoExecutionListener<T>* listener;

public:

    // Constructor and destructor
    ExecutionService();
    ~ExecutionService();

    // Get data on our service given a key
    ExecutionOrder<T>& GetData(string _key);

    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(ExecutionOrder<T>& _data);

    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<ExecutionOrder<T>>* _listener);

    // Get all listeners on the Service
    const vector<ServiceListener<ExecutionOrder<T>>*>& GetListeners() const;

    // Get the listener of the service
    ExecutionToAlgoExecutionListener<T>* GetListener();

    // Execute an order on a market
    void ExecuteOrder(ExecutionOrder<T>& _executionOrder);

};

template<typename T>
ExecutionService<T>::ExecutionService()
{
    executionOrders = map<string, ExecutionOrder<T>>();
    listeners = vector<ServiceListener<ExecutionOrder<T>>*>();
    listener = new ExecutionToAlgoExecutionListener<T>(this);
}

template<typename T>
ExecutionService<T>::~ExecutionService() {}

template<typename T>
ExecutionOrder<T>& ExecutionService<T>::GetData(string _key)
{
    return executionOrders[_key];
}

template<typename T>
void ExecutionService<T>::OnMessage(ExecutionOrder<T>& _data)
{
    executionOrders[_data.GetProduct().GetProductId()] = _data;
}

template<typename T>
void ExecutionService<T>::AddListener(ServiceListener<ExecutionOrder<T>>* _listener)
{
    listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<ExecutionOrder<T>>*>& ExecutionService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
ExecutionToAlgoExecutionListener<T>* ExecutionService<T>::GetListener()
{
    return listener;
}

template<typename T>
void ExecutionService<T>::ExecuteOrder(ExecutionOrder<T>& _executionOrder)
{
    string _productId = _executionOrder.GetProduct().GetProductId();
    executionOrders[_productId] = _executionOrder;

    for (auto& l : listeners)
    {
        l->ProcessAdd(_executionOrder);
    }
}






template<typename T>
vector<string> ExecutionOrder<T>::ToStrings() const
{
    string _product = product.GetProductId();

    map<PricingSide, string> sideMap = {{BID, "BID"}, {OFFER, "OFFER"}};
    map<OrderType, string> orderTypeMap = {{FOK, "FOK"}, {IOC, "IOC"}, {MARKET, "MARKET"}, {LIMIT, "LIMIT"}, {STOP, "STOP"}};
    map<bool, string> isChildOrderMap = {{true, "YES"}, {false, "NO"}};

    string _side = sideMap[side];
    string _orderId = orderId;
    string _orderType = orderTypeMap[orderType];
    string _price = ConvertPrice(price);
    string _visibleQuantity = to_string(visibleQuantity);
    string _hiddenQuantity = to_string(hiddenQuantity);
    string _parentOrderId = parentOrderId;
    string _isChildOrder = isChildOrderMap[isChildOrder];

    vector<string> _strings = {_product, _side, _orderId, _orderType, _price, _visibleQuantity, _hiddenQuantity, _parentOrderId, _isChildOrder};
    return _strings;
};

template<typename T>
class AlgoExecution
{
public:
    // ctor for an order
    AlgoExecution() = default;
    AlgoExecution(const T& _product, PricingSide _side, string _orderId, OrderType _orderType, double _price, long _visibleQuantity, long _hiddenQuantity, string _parentOrderId, bool _isChildOrder)
    {
        executionOrder = new ExecutionOrder<T>(_product, _side, _orderId, _orderType, _price, _visibleQuantity, _hiddenQuantity, _parentOrderId, _isChildOrder);
    }
    // Get the order
    ExecutionOrder<T>* GetExecutionOrder() const
    {
        return executionOrder;
    };
private:
    ExecutionOrder<T>* executionOrder;
};


template<typename T>
class AlgoExecutionToMarketDataListener;

template<typename T>
class AlgoExecutionService : public Service<string, AlgoExecution<T>>
{

private:

    map<string, AlgoExecution<T>> algoExecutions;
    vector<ServiceListener<AlgoExecution<T>>*> listeners;
    AlgoExecutionToMarketDataListener<T>* listener;
    double spread;
    long count;

public:

    // Constructor and destructor
    AlgoExecutionService();
    ~AlgoExecutionService(){};

    // Get data on our service given a key
    AlgoExecution<T>& GetData(string _key)
    {
        return algoExecutions[_key];
    };

    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(AlgoExecution<T>& _data)
    {
        algoExecutions[_data.GetExecutionOrder()->GetProduct().GetProductId()] = _data;
    };

    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<AlgoExecution<T>>* _listener)
    {
        listeners.push_back(_listener);
    };

    // Get all listeners on the Service
    const vector<ServiceListener<AlgoExecution<T>>*>& GetListeners() const
    {
        return listeners;
    };

    // Get the listener of the service
    AlgoExecutionToMarketDataListener<T>* GetListener()
    {
        return listener;
    };

    // Execute an order on a market
    void AlgoExecuteOrder(OrderBook<T>& _orderBook);

};

template<typename T>
AlgoExecutionService<T>::AlgoExecutionService()
{
    algoExecutions = map<string, AlgoExecution<T>>();
    listeners = vector<ServiceListener<AlgoExecution<T>>*>();
    listener = new AlgoExecutionToMarketDataListener<T>(this);
    spread = 1.0 / 128.0;
    count = 0;
}

template<typename T>
void AlgoExecutionService<T>::AlgoExecuteOrder(OrderBook<T>& _orderBook)
{
    T _product = _orderBook.GetProduct();
    string _productId = _product.GetProductId();
    PricingSide _side;
    string _orderId = GenerateId();
    double _price;
    long _quantity;

    BidOffer _bidOffer = _orderBook.GetBidOffer();

    Order _bidOrder = _bidOffer.GetBidOrder();
    double _bidPrice = _bidOrder.GetPrice();
    long _bidQuantity = _bidOrder.GetQuantity();
    Order _offerOrder = _bidOffer.GetOfferOrder();
    double _offerPrice = _offerOrder.GetPrice();
    long _offerQuantity = _offerOrder.GetQuantity();

    if (_offerPrice - _bidPrice <=spread)
    {
        switch (count % 2)
        {
            case 0:
                _price = _bidPrice;
                _quantity = _bidQuantity;
                _side = BID;
                break;
            case 1:
                _price = _offerPrice;
                _quantity = _offerQuantity;
                _side = OFFER;
                break;
        }
        count++;
        AlgoExecution<T> _algoExecution(_product, _side, _orderId, MARKET, _price, _quantity, 0, "", false);
        algoExecutions[_productId] = _algoExecution;

        for (auto& l : listeners)
        {
            l->ProcessAdd(_algoExecution);
        }
    }
}

template<typename T>
class AlgoExecutionToMarketDataListener : public ServiceListener<OrderBook<T>>
{

private:

    AlgoExecutionService<T>* service;

public:

    // Connector and Destructor
    AlgoExecutionToMarketDataListener(AlgoExecutionService<T>* _service)
    {
        service = _service;
    };
    ~AlgoExecutionToMarketDataListener(){};

    // Listener callback to process an add event to the Service
    void ProcessAdd(OrderBook<T>& _data)
    {
        service->AlgoExecuteOrder(_data);
    };

    // Listener callback to process a remove event to the Service
    void ProcessRemove(OrderBook<T>& _data){};

    // Listener callback to process an update event to the Service
    void ProcessUpdate(OrderBook<T>& _data){};

};


/****************************************************************************************/
/**
* Execution Service Listener subscribing data from Algo Execution Service to Execution Service.
* Type T is the product type.
*/
template<typename T>
class ExecutionToAlgoExecutionListener : public ServiceListener<AlgoExecution<T>>
{

private:

    ExecutionService<T>* service;

public:

    // Connector and Destructor
    ExecutionToAlgoExecutionListener(ExecutionService<T>* _service);
    ~ExecutionToAlgoExecutionListener();

    // Listener callback to process an add event to the Service
    void ProcessAdd(AlgoExecution<T>& _data);

    // Listener callback to process a remove event to the Service
    void ProcessRemove(AlgoExecution<T>& _data);

    // Listener callback to process an update event to the Service
    void ProcessUpdate(AlgoExecution<T>& _data);

};

template<typename T>
ExecutionToAlgoExecutionListener<T>::ExecutionToAlgoExecutionListener(ExecutionService<T>* _service)
{
    service = _service;
}

template<typename T>
ExecutionToAlgoExecutionListener<T>::~ExecutionToAlgoExecutionListener() {}

template<typename T>
void ExecutionToAlgoExecutionListener<T>::ProcessAdd(AlgoExecution<T>& _data)
{
    // Get the execution order from the algo execution
    ExecutionOrder<T>* _executionOrder = _data.GetExecutionOrder();
    service->OnMessage(*_executionOrder);
    service->ExecuteOrder(*_executionOrder);
}

template<typename T>
void ExecutionToAlgoExecutionListener<T>::ProcessRemove(AlgoExecution<T>& _data) {}

template<typename T>
void ExecutionToAlgoExecutionListener<T>::ProcessUpdate(AlgoExecution<T>& _data) {}



#endif
