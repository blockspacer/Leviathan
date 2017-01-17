// ------------------------------------ //
#include "NetworkMasterServerInterface.h"

#include "NetworkRequest.h"
#include "Connection.h"
#include "NetworkInterface.h"
#include "NetworkHandler.h"
#include "Exceptions.h"
using namespace Leviathan;

DLLEXPORT Leviathan::NetworkMasterServerInterface::NetworkMasterServerInterface() : 
    NetworkInterface(NETWORKED_TYPE::Master)
{

}

DLLEXPORT Leviathan::NetworkMasterServerInterface::~NetworkMasterServerInterface() {

}
// ------------------------------------ //
DLLEXPORT void Leviathan::NetworkMasterServerInterface::HandleRequestPacket(
    std::shared_ptr<NetworkRequest> request, Connection &connection) 
{
    LEVIATHAN_ASSERT(request, "request is null");

    if (_HandleDefaultRequest(request, connection))
        return;

    // switch (request->GetType()) {
    // }

    if (_CustomHandleRequestPacket(request, connection))
        return;

    LOG_ERROR("NetworkMasterServerInterface: failed to handle request of type: " +
        Convert::ToString(static_cast<int>(request->GetType())));
}

DLLEXPORT void Leviathan::NetworkMasterServerInterface::HandleResponseOnlyPacket(
    std::shared_ptr<NetworkResponse> message, Connection &connection, 
    bool &dontmarkasreceived) 
{
    LEVIATHAN_ASSERT(message, "message is null");

    if (_HandleDefaultResponseOnly(message, connection, dontmarkasreceived))
        return;

    // switch (message->GetType()) {
    // }

    if (_CustomHandleResponseOnlyPacket(message, connection, dontmarkasreceived))
        return;

    LOG_ERROR("NetworkMasterServerInterface: failed to handle response of type: " +
        Convert::ToString(static_cast<int>(message->GetType())));
}
// ------------------------------------ //
DLLEXPORT void Leviathan::NetworkMasterServerInterface::TickIt() {

}

DLLEXPORT void Leviathan::NetworkMasterServerInterface::CloseDown() {

}

DLLEXPORT std::vector<std::shared_ptr<Leviathan::Connection>>& 
Leviathan::NetworkMasterServerInterface::GetClientConnections() 
{
    DEBUG_BREAK;
    LEVIATHAN_ASSERT(false, "GetClientConnections called on a master server");
    throw Exception("GetClientConnections called on a master server");
}

// ------------------------------------ //
