/*
 * beaconEmitter.cpp
 */

#include "beaconEmitter.h"

#include <introspectionRegistry.h>

#include <errlog.h>
#include <algorithm>

#include <serverContext.h>

using namespace std;

namespace epics { namespace pvAccess {

const float BeaconEmitter::EPICS_CA_MIN_BEACON_PERIOD = 1.0;
const float BeaconEmitter::EPICS_CA_MIN_BEACON_COUNT_LIMIT = 3.0;

//BeaconEmitter::BeaconEmitter(Transport::shared_pointer& transport, ServerContextImpl::shared_pointer& context) :
BeaconEmitter::BeaconEmitter(Transport::shared_pointer& transport, std::tr1::shared_ptr<ServerContextImpl>& context) :
    _transport(transport),
    _beaconSequenceID(0),
    _startupTime(),
    _fastBeaconPeriod(std::max(context->getBeaconPeriod(), EPICS_CA_MIN_BEACON_PERIOD)),
    _slowBeaconPeriod(std::max(180.0, _fastBeaconPeriod)), // TODO configurable
    _beaconCountLimit((int16)std::max(10.0f, EPICS_CA_MIN_BEACON_COUNT_LIMIT)), // TODO configurable
    _serverAddress(*(context->getServerInetAddress())),
    _serverPort(context->getServerPort()),
    _serverStatusProvider(context->getBeaconServerStatusProvider()),
    _timer(context->getTimer()),
    _timerNode(*this)
{
	_startupTime.getCurrent();
}

BeaconEmitter::BeaconEmitter(Transport::shared_pointer& transport, const osiSockAddr& serverAddress) :
    _transport(transport),
    _beaconSequenceID(0),
    _startupTime(),
    _fastBeaconPeriod(EPICS_CA_MIN_BEACON_PERIOD),
    _slowBeaconPeriod(180.0),
    _beaconCountLimit(10),
    _serverAddress(serverAddress),
    _serverPort(serverAddress.ia.sin_port),
    _serverStatusProvider(),
    _timer(new Timer("pvAccess-server timer", lowPriority)),
    _timerNode(*this)
{
 	_startupTime.getCurrent();
}

BeaconEmitter::~BeaconEmitter()
{
    destroy();
}

void BeaconEmitter::lock()
{
	//noop
}

void BeaconEmitter::unlock()
{
	//noop
}

void BeaconEmitter::send(ByteBuffer* buffer, TransportSendControl* control)
{
	// get server status
	PVField::shared_pointer serverStatus;
	if(_serverStatusProvider.get())
	{
		try
		{
			serverStatus = _serverStatusProvider->getServerStatusData();
		}
		catch (...) {
			// we have to proctect internal code from external implementation...
			errlogSevPrintf(errlogMinor, "BeaconServerStatusProvider implementation thrown an exception.");
		}
	}

	// send beacon
	control->startMessage((int8)0, (sizeof(int16)+2*sizeof(int32)+128+sizeof(int16))/sizeof(int8));

	buffer->putShort(_beaconSequenceID);
	buffer->putLong((int64)_startupTime.getSecondsPastEpoch());
	buffer->putInt((int32)_startupTime.getNanoSeconds());

	// NOTE: is it possible (very likely) that address is any local address ::ffff:0.0.0.0
	encodeAsIPv6Address(buffer, &_serverAddress);
	buffer->putShort((int16)_serverPort);

	if (serverStatus)
	{
		// introspection interface + data
		IntrospectionRegistry::serializeFull(serverStatus->getField(), buffer, control);
		serverStatus->serialize(buffer, control);
	}
	else
	{
        IntrospectionRegistry::serializeFull(FieldConstPtr(), buffer, control);
	}
	control->flush(true);

	// increment beacon sequence ID
	_beaconSequenceID++;

	reschedule();
}

void BeaconEmitter::timerStopped()
{
	//noop
}

void BeaconEmitter::destroy()
{
	_timerNode.cancel();
}

void BeaconEmitter::start()
{
	_timer->scheduleAfterDelay(_timerNode, 0.0);
}

void BeaconEmitter::reschedule()
{
	const double period = (_beaconSequenceID >= _beaconCountLimit) ? _slowBeaconPeriod : _fastBeaconPeriod;
	if (period > 0)
	{
		_timer->scheduleAfterDelay(_timerNode, period);
	}
}

void BeaconEmitter::callback()
{
    // requires this instance has already a valid pointer to this
    TransportSender::shared_pointer thisSender = shared_from_this();
	_transport->enqueueSendRequest(thisSender);
}

}}
