// ------------------------------------ //
#include "SFMLPackets.h"

#include "Common/DataStoring/NamedVars.h"
#include "Common/Plane.h"
// ------------------------------------ //
namespace Leviathan {

// ------------------------------------ //
// Float2
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const Float2& data)
{
    return packet << data.X << data.Y;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, Float2& data)
{
    return packet << data.X << data.Y;
}
// ------------------ Float3 ------------------ //
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const Float3& data)
{
    return packet << data.X << data.Y << data.Z;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, Float3& data)
{
    return packet >> data.X >> data.Y >> data.Z;
}

// ------------------ Float4 ------------------ //
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const Float4& data)
{
    return packet << data.X << data.Y << data.Z << data.W;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, Float4& data)
{
    return packet >> data.X >> data.Y >> data.Z >> data.W;
}

// ------------------ NamedVariableList ------------------ //
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const NamedVariableList& data)
{
    DEBUG_BREAK;
    return packet;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, const NamedVariableList& data)
{
    DEBUG_BREAK;
    return packet;
}

// ------------------ SFML Packet into a packet ------------------ //
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const sf::Packet& packetinner)
{
    LEVIATHAN_ASSERT(&packet != &packetinner, "Trying to insert packet into itself");

    const uint32_t size = static_cast<uint32_t>(packetinner.getDataSize());

    packet << size;

    if(size > 0)
        packet.append(packetinner.getData(), size);
    return packet;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, sf::Packet& packetinner)
{
    LEVIATHAN_ASSERT(&packet != &packetinner, "Trying to extract packet from itself");
    // uint32_t size = 0;

    // packet >> size;

    // if(!packet)
    //     throw InvalidArgument("Invalid packet format for loading sf::Packet, no size");

    // TODO: make this more efficient
    // if(size > 0){

    std::string tmpstr;
    packet >> tmpstr;

    if(!packet)
        throw InvalidArgument("Invalid packet format for loading Packet, no size");

    packetinner.append(tmpstr.c_str(), tmpstr.size());
    //}

    return packet;
}

// ------------------------------------ //
// Plane
DLLEXPORT sf::Packet& operator<<(sf::Packet& packet, const Plane& data)
{
    return packet << data.Normal << data.Distance;
}

DLLEXPORT sf::Packet& operator>>(sf::Packet& packet, Plane& data)
{
    return packet >> data.Normal >> data.Distance;
}

} // namespace Leviathan
