#ifndef XDOS_XAPP_IPC_H
#define XDOS_XAPP_IPC_H

#include <stdint.h>
#include <string.h>

// The V2 runtime must serialize calls on core 0.
class XappIpc {
public:
    enum : unsigned { Slots = 4, Depth = 8, Payload = 64 };
    enum Result { Ok, InvalidEndpoint, Full, Empty, InvalidPayload };

    struct Endpoint {
        unsigned slot;
        uint32_t generation;
    };

    struct Message {
        Endpoint source;
        unsigned size;
        uint8_t data[Payload];
    };

    Endpoint Activate(unsigned slot)
    {
        if (slot >= Slots) return {Slots, 0};

        Inbox &inbox = inboxes_[slot];
        if (++inbox.generation == 0) ++inbox.generation;
        inbox.active = true;
        inbox.head = inbox.count = 0;
        return {slot, inbox.generation};
    }

    bool Deactivate(Endpoint endpoint)
    {
        if (!Active(endpoint)) return false;

        Inbox &inbox = inboxes_[endpoint.slot];
        inbox.active = false;
        inbox.head = inbox.count = 0;
        return true;
    }

    Result Send(Endpoint source, Endpoint target, const void *data, unsigned size)
    {
        if (!Active(source) || !Active(target)) return InvalidEndpoint;
        if (size > Payload || (size && !data)) return InvalidPayload;

        Inbox &inbox = inboxes_[target.slot];
        if (inbox.count == Depth) return Full;

        Message &message = inbox.messages[(inbox.head + inbox.count) % Depth];
        message = Message{};
        message.source = source;
        message.size = size;
        if (size) memcpy(message.data, data, size);
        ++inbox.count;
        return Ok;
    }

    Result Receive(Endpoint endpoint, Message &message)
    {
        if (!Active(endpoint)) return InvalidEndpoint;

        Inbox &inbox = inboxes_[endpoint.slot];
        if (!inbox.count) return Empty;

        message = inbox.messages[inbox.head];
        inbox.head = (inbox.head + 1) % Depth;
        --inbox.count;
        return Ok;
    }

private:
    struct Inbox {
        uint32_t generation;
        bool active;
        unsigned head;
        unsigned count;
        Message messages[Depth];
    };

    bool Active(Endpoint endpoint) const
    {
        return endpoint.slot < Slots && inboxes_[endpoint.slot].active
            && inboxes_[endpoint.slot].generation == endpoint.generation;
    }

    Inbox inboxes_[Slots]{};
};

#endif
