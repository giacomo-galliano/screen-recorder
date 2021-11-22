#ifndef SCREEN_RECORDER_PACKET_H
#define SCREEN_RECORDER_PACKET_H

#include "wrappers.h"

/*
 * AVPacket is extended to enable ranged for loop.
 * Since C++ 11, anything that has a begin() and an end() function that returns something that can be incremented,
 * you can use it in a for loop. This is why we overload the operator++().
 */

using PacketBase = std::unique_ptr<AVPacket, void (*)(AVPacket*)>;
class Packet : public PacketBase {
private:
    AVFormatContext* fmtCtx = nullptr;
public:
    Packet();
    Packet(AVFormatContext *fmtCtx);
    Packet& operator++();
};


#endif //SCREEN_RECORDER_PACKET_H
