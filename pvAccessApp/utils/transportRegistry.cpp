/*
 * transportRegistry.cpp
 */

#include "transportRegistry.h"

using namespace epics::pvData;
using namespace std;

namespace epics { namespace pvAccess {

TransportRegistry::TransportRegistry(): _transports(), _transportCount(0), _mutex()
{

}

TransportRegistry::~TransportRegistry()
{
}

void TransportRegistry::put(Transport::shared_pointer& transport)
{
	Lock guard(_mutex);
	//const String type = transport.getType();
	const int16 priority = transport->getPriority();
	const osiSockAddr* address = transport->getRemoteAddress();

	transportsMap_t::iterator transportsIter = _transports.find(address);
	prioritiesMapSharedPtr_t priorities;
	if(transportsIter == _transports.end())
	{
		priorities.reset(new prioritiesMap_t());
		_transports[address] = priorities;
		_transportCount++;
	}
	else
	{
		priorities = transportsIter->second;
		prioritiesMap_t::iterator prioritiesIter = priorities->find(priority);
		if(prioritiesIter == priorities->end()) //only increase transportCount if not replacing
		{
			_transportCount++;
		}
	}
	(*priorities)[priority] = transport;
}

Transport::shared_pointer TransportRegistry::get(String type, const osiSockAddr* address, const int16 priority)
{
	Lock guard(_mutex);
	transportsMap_t::iterator transportsIter = _transports.find(address);
	if(transportsIter != _transports.end())
	{
		prioritiesMapSharedPtr_t priorities = transportsIter->second;
		prioritiesMap_t::iterator prioritiesIter = priorities->find(priority);
		if(prioritiesIter != priorities->end())
		{
			return prioritiesIter->second;
		}
	}
	return Transport::shared_pointer();
}

auto_ptr<TransportRegistry::transportVector_t> TransportRegistry::get(String type, const osiSockAddr* address)
{
	Lock guard(_mutex);
	transportsMap_t::iterator transportsIter = _transports.find(address);
	if(transportsIter != _transports.end())
	{
		prioritiesMapSharedPtr_t priorities = transportsIter->second;
        auto_ptr<transportVector_t> transportArray(new transportVector_t(priorities->size()));
        int32 i = 0;
		for(prioritiesMap_t::iterator prioritiesIter = priorities->begin();
            prioritiesIter != priorities->end();
            prioritiesIter++, i++)
		{
			transportArray->at(i) = prioritiesIter->second;
		}
		return transportArray;
	}
	return auto_ptr<transportVector_t>();
}

Transport::shared_pointer TransportRegistry::remove(Transport::shared_pointer& transport)
{
	Lock guard(_mutex);
	const int16 priority = transport->getPriority();
	const osiSockAddr* address = transport->getRemoteAddress();
    Transport::shared_pointer retTransport;
	transportsMap_t::iterator transportsIter = _transports.find(address);
	if(transportsIter != _transports.end())
	{
		prioritiesMapSharedPtr_t priorities = transportsIter->second;
		prioritiesMap_t::iterator prioritiesIter = priorities->find(priority);
		if(prioritiesIter != priorities->end())
		{
            retTransport = prioritiesIter->second;
			priorities->erase(prioritiesIter);
            _transportCount--;
			if(priorities->size() == 0)
			{
				_transports.erase(transportsIter);
			}
		}
	}
	return retTransport;
}

void TransportRegistry::clear()
{
	Lock guard(_mutex);
	_transports.clear();
	_transportCount = 0;
}

int TransportRegistry::numberOfActiveTransports()
{
	Lock guard(_mutex);
	return _transportCount;
}

auto_ptr<TransportRegistry::transportVector_t> TransportRegistry::toArray(String type)
{
	// TODO support type
    return toArray();
}

auto_ptr<TransportRegistry::transportVector_t> TransportRegistry::toArray()
{
	Lock guard(_mutex);
    if (_transportCount == 0)
        return auto_ptr<transportVector_t>(0);
    
    auto_ptr<transportVector_t> transportArray(new transportVector_t(_transportCount));

    int32 i = 0;
	for (transportsMap_t::iterator transportsIter = _transports.begin();
         transportsIter != _transports.end();
         transportsIter++)
    {
		prioritiesMapSharedPtr_t priorities = transportsIter->second;
		for (prioritiesMap_t::iterator prioritiesIter = priorities->begin();
             prioritiesIter != priorities->end();
             prioritiesIter++, i++)
        {
            transportArray->at(i) = prioritiesIter->second;
        }
    }
    
	return transportArray;
}

}}

