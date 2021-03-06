/* ./server/main.c */

#include <stdio.h>
#include <enet/enet.h>

enum ENET_TYPES {
    ENET_UINT16,
    ENET_FLOAT
};

#include <sstream>
#include <cereal/archives/portable_binary.hpp>

void SendPacket(ENetPeer* peer, const char* data)
{
    ENetPacket* packet = enet_packet_create(data, strlen(data)+1, ENET_PACKET_FLAG_RELIABLE);
    // Second arg is channel:
    enet_peer_send(peer, 0, packet);
}

int main (int argc, char ** argv)
{
    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetEvent event;
    ENetAddress address;
    ENetHost* server;

    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY; // This allows
    /* Bind the server to port 7777. */
    address.port = 7777;

    server = enet_host_create (&address	/* the address to bind the server host to */,
                    32	/* allow up to 32 clients and/or outgoing connections */,
                    1	/* allow up to 1 channel to be used, 0. */,
                    0	/* assume any amount of incoming bandwidth */,
                    0	/* assume any amount of outgoing bandwidth */);

    if (server == NULL)
    {
        printf("An error occurred while trying to create an ENet server host.");
        return 1;
    }

    // gameloop
    while(true)
    {
        ENetEvent event;
        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service (server, &event, 1000) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    event.peer->data = (void*)"FirstConnection";
                    printf ("A new client connected from %x:%u. Giving it name \"%s\"\n",
                        event.peer->address.host,
                        event.peer->address.port,
                        (char*)event.peer->data);
                break;

                case ENET_EVENT_TYPE_RECEIVE:
                    if (event.packet->dataLength > 0)
                    {
                        std::stringstream ss((char*)event.packet->data); // Use std::ios::binary if filestream
                        cereal::PortableBinaryInputArchive inArchive(ss);
                        char header;
                        inArchive(header); // DeSerialize
                        printf("Got header: %u\n", header);
                        
                        switch (header)
                        {
                        case ENET_UINT16:
                            UINT16 result;
                            inArchive(result); // DeSerialize

                            printf ("A packet of length %u containing \"%#X\" was received from %s on channel %u.\n",
                                event.packet->dataLength,
                                result,
                                event.peer->data,
                                event.channelID);
                                
                            break;

                        case ENET_FLOAT:
                            float inputdata;
                            inArchive(inputdata); // DeSerialize
                            printf("Got float: %f\n", inputdata);

                        default:
                            break;
                        }

                        /* Clean up the packet now that we're done using it. */
                        enet_packet_destroy (event.packet);
                    }

                    // SEND A REPLY! :D
                    SendPacket(event.peer, "HELLO!"); // This adds the packet to a queue

                break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf ("%s disconnected.\n", event.peer -> data);
                    /* Reset the peer's client information. */
                    event.peer -> data = NULL;
            }
        }
    }

    enet_host_destroy(server);

    return 0;
}
