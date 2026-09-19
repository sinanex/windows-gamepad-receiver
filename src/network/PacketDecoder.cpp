#include "PacketDecoder.h"

namespace GamepadReceiver {

PacketDecoder::DecodeResult PacketDecoder::decode(
    const uint8_t* data,
    size_t length,
    ControllerState& outState,
    uint32_t& outHandshakeSeq,
    uint32_t& outClientId
) noexcept {
    if (!data) {
        return DecodeResult::InvalidData;
    }

    if (length == CONTROLLER_PACKET_SIZE) {
        const uint16_t magic = readUint16LE(data + 0);
        if (magic != PROTOCOL_MAGIC) {
            return DecodeResult::InvalidMagic;
        }

        const uint8_t version = data[2];
        if (version != PROTOCOL_VERSION) {
            return DecodeResult::InvalidVersion;
        }

        const uint8_t packetType = data[3];
        if (packetType != PACKET_TYPE_STATE) {
            return DecodeResult::InvalidType;
        }

        outState.sequence       = readUint32LE(data + 4);
        outState.timestampNs    = readInt64LE(data + 8);
        outState.leftX          = readInt16LE(data + 16);
        outState.leftY          = readInt16LE(data + 18);
        outState.rightX         = readInt16LE(data + 20);
        outState.rightY         = readInt16LE(data + 22);
        outState.leftTrigger    = readUint16LE(data + 24);
        outState.rightTrigger   = readUint16LE(data + 26);
        outState.buttons        = readUint32LE(data + 28);
        outState.dpad           = data[32];

        return DecodeResult::ValidControllerState;
    }

    if (length == HANDSHAKE_PACKET_SIZE) {
        const uint16_t magic = readUint16LE(data + 0);
        if (magic != PROTOCOL_MAGIC) {
            return DecodeResult::InvalidMagic;
        }

        const uint8_t version = data[2];
        if (version != PROTOCOL_VERSION) {
            return DecodeResult::InvalidVersion;
        }

        const uint8_t packetType = data[3];
        outHandshakeSeq = readUint32LE(data + 4);
        outClientId     = readUint32LE(data + 8);

        switch (packetType) {
            case PACKET_TYPE_HELLO:     return DecodeResult::ValidHandshakeHello;
            case PACKET_TYPE_ACK:       return DecodeResult::ValidHandshakeAck;
            case PACKET_TYPE_HEARTBEAT: return DecodeResult::ValidHeartbeat;
            default:                    return DecodeResult::InvalidType;
        }
    }

    return DecodeResult::InvalidSize;
}

} // namespace GamepadReceiver
