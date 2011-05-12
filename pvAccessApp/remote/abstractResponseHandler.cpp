/*
 * abstractResponseHandler.cpp
 *
 *  Created on: Jan 10, 2011
 *      Author: Miha Vitorovic
 */

#include "remote.h"
#include <hexDump.h>

#include <byteBuffer.h>

#include <osiSock.h>

#include <sstream>

using std::ostringstream;
using std::hex;

using namespace epics::pvData;

namespace epics {
    namespace pvAccess {

        void AbstractResponseHandler::handleResponse(osiSockAddr* responseFrom,
                Transport::shared_pointer& transport, int8 version, int8 command,
                int payloadSize, ByteBuffer* payloadBuffer) {
            if(_debug) {
                char ipAddrStr[48];
                ipAddrToDottedIP(&responseFrom->ia, ipAddrStr, sizeof(ipAddrStr));

                ostringstream prologue;
                prologue<<"Message [0x"<<hex<<(int)command<<", v0x"<<hex;
                prologue<<(int)version<<"] received from "<<ipAddrStr;

                hexDump(prologue.str(), _description,
                        (const int8*)payloadBuffer->getArray(),
                        payloadBuffer->getPosition(), payloadSize);
            }
        }
    }
}
