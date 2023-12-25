/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry() = default;
  Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  // Get the current state on the inquiry
  InquiryState GetState() const;

  // Set the current state on the inquiry
  void SetState(InquiryState _state);

  // Change attributes to strings
  vector<string> ToStrings() const;


private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};



template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state) :
  product(_product)
{
  inquiryId = _inquiryId;
  side = _side;
  quantity = _quantity;
  price = _price;
  state = _state;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
  return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
  return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
  return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
  return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
  return state;
}

template<typename T>
void Inquiry<T>::SetState(InquiryState _state)
{
    state = _state;
}

template<typename T>
vector<string> Inquiry<T>::ToStrings() const
{
    string _inquiryId = inquiryId;
    string _product = product.GetProductId();
    string _side;
    if (side == BUY) {
        _side = "BUY";
    }
    else {
        _side = "SELL";
    }

    string _quantity = to_string(quantity);
    string _price = ConvertPrice(price);
    string _state;
    switch(state)
    {
    case RECEIVED:
        _state = "RECEIVED";
        break;
    case QUOTED:
        _state = "QUOTED";
        break;
    case DONE:
        _state = "DONE";
        break;
    case REJECTED:
        _state = "REJECTED";
        break;
    case CUSTOMER_REJECTED:
        _state = "CUSTOMER_REJECTED";
        break;
    }

    vector<string> _strings = {_inquiryId, _product, _side, _quantity, _price, _state};
    return _strings;
}

template<typename T>
class InquiryConnector;

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{
private:
    map<string, Inquiry<T>> inquiries;
    vector<ServiceListener<Inquiry<T>>*> listeners;
    InquiryConnector<T>* connector;
public:
    InquiryService();
    // Send a quote back to the client
    void SendQuote(const string &inquiryId, double price);

    // Reject an inquiry from the client
    void RejectInquiry(const string &inquiryId);

    // Get data by our service with a key
    Inquiry<T>& GetData(string _key);

    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(Inquiry<T>& _data);

    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<Inquiry<T>>* _listener)
    {
        listeners.push_back(_listener);
    };
    const vector<ServiceListener<Inquiry<T>>*>& GetListeners() const
    {
        return listeners;
    };

    InquiryConnector<T>* GetConnector()
    {
        return connector;
    };

};

template<typename T>
InquiryService<T>::InquiryService()
{
    inquiries = map<string, Inquiry<T>>();
    listeners = vector<ServiceListener<Inquiry<T>>*>();
    connector = new InquiryConnector<T>(this);
}

template<typename T>
Inquiry<T>& InquiryService<T>::GetData(string _key)
{
    return inquiries[_key];
}

template<typename T>
void InquiryService<T>::OnMessage(Inquiry<T>& _data)
{
    InquiryState _state = _data.GetState();
    switch (_state)
    {
        case RECEIVED: // if the inquiry is received, then we need to send the inquiry to the market
            inquiries[_data.GetInquiryId()] = _data;
            connector -> Publish(_data);
            break;
        case QUOTED: // if the inquiry is quoted, then we need to send the quote back to the client
            _data.SetState(DONE);
            inquiries[_data.GetInquiryId()] = _data;
            for (auto& l : listeners)
            {
                l->ProcessAdd(_data);
            }
            break;
        case DONE:
            // handle DONE
            break;
        case REJECTED:
            // handle REJECTED
            break;
        case CUSTOMER_REJECTED:
            // handle CUSTOMER_REJECTED
            break;
        default:
            // handle other cases
            break;
    }
}

template<typename T>
void InquiryService<T>::SendQuote(const string& _inquiryId, double _price)
{
    Inquiry<T>& _inquiry = inquiries[_inquiryId];
    InquiryState _state = _inquiry.GetState();
    _inquiry.SetPrice(_price);
    for (auto& l : listeners)
    {
        l->ProcessAdd(_inquiry);
    }
}

template<typename T>
void InquiryService<T>::RejectInquiry(const string& _inquiryId)
{
    Inquiry<T>& _inquiry = inquiries[_inquiryId];
    _inquiry.SetState(REJECTED);
}

/****************************************************************************************/

template<typename T>
class InquiryConnector : public Connector<Inquiry<T>>
{
private:
    InquiryService<T>* service;
public:
    InquiryConnector(InquiryService<T>* _service) {
        service = _service;
    };

    // Publish data to the Connector
    void Publish(Inquiry<T>& _data);

    // Subscribe data from the Connector
    void Subscribe(ifstream& _data);

    // Re-subscribe data from the Connector
    void Subscribe(Inquiry<T>& _data);

};

template<typename T>
void InquiryConnector<T>::Publish(Inquiry<T>& _data)
{
    InquiryState _state = _data.GetState();
    if (_state == RECEIVED)
    {
        _data.SetState(QUOTED);
        this->Subscribe(_data);
    }
}

template<typename T>
void InquiryConnector<T>::Subscribe(ifstream& _data)
{
    string _line;
    while (getline(_data, _line))
    {
        stringstream _lineStream(_line);
        string _cell;
        vector<string> _cells;
        while (getline(_lineStream, _cell, ','))
        {
            _cells.push_back(_cell);
        }

        string _inquiryId = _cells[0];
        string _productId = _cells[1];
        Side _side;
        if (_cells[2] == "BUY") _side = BUY;
        else if (_cells[2] == "SELL") _side = SELL;
        long _quantity = stol(_cells[3]);
        double _price = ConvertPrice(_cells[4]);
        InquiryState _state;
        if (_cells[5] == "RECEIVED") _state = RECEIVED;
        else if (_cells[5] == "QUOTED") _state = QUOTED;
        else if (_cells[5] == "DONE") _state = DONE;
        else if (_cells[5] == "REJECTED") _state = REJECTED;
        else if (_cells[5] == "CUSTOMER_REJECTED") _state = CUSTOMER_REJECTED;
        T _product = GetBond(_productId);
        Inquiry<T> _inquiry(_inquiryId, _product, _side, _quantity, _price, _state);
        service->OnMessage(_inquiry);
    }
}

template<typename T>
void InquiryConnector<T>::Subscribe(Inquiry<T>& _data)
{
    service->OnMessage(_data);
}


#endif
