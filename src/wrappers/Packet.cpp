#include "Packet.h"

Packet::Packet(): PacketBase(nullptr, [](AVPacket*){}), fmtCtx(nullptr){};

Packet::Packet(AVFormatContext *fmtCtx) : PacketBase(av_packet_alloc(), [](AVPacket* pkt)
{
    av_packet_free(&pkt);
}), fmtCtx(fmtCtx){
    readFrame(fmtCtx, get()); //get() returns the stored pointer of the unique_ptr
};
/*
 * Prefix increment
 * @return
 */
Packet& Packet::operator++(){
    if (!(*this)) {return *this;}

    if (0 >= get()->size) {
        // delete managed object
        reset();
    } else {
        readFrame(fmtCtx, get());
    }

    return *this;
}
