/* ./client/main.c */

#include <stdio.h>
#include <enet/enet.h>
#include <string>
#include <sstream>
#include <cereal/archives/portable_binary.hpp>

enum ENET_TYPES {
    ENET_UINT16,
    ENET_FLOAT
};


void SendPacket(ENetPeer* peer, std::stringstream* data)
{
    ENetPacket* packet;
    int dataLength = data->tellp();
    if (dataLength > 0)
    {
        packet = enet_packet_create(data->str().c_str(), dataLength, ENET_PACKET_FLAG_RELIABLE);
    }
    // enet_packet_create takes a const void * for data to be sent.
    
    // Second arg is channel:
    enet_peer_send(peer, 0, packet);
}

int main(int argc, char ** argv)
{

    if(enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet!\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost* client;
    client = enet_host_create(NULL, 1, 1, 0, 0);

    if(client == NULL)
    {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    peer = enet_host_connect(client, &address, 1, 0);
    if(peer == NULL)
    {
        fprintf(stderr, "No available peers for initiating an ENet connection!\n");
        return EXIT_FAILURE;
    }

    if(enet_host_service(client, &event, 5000) > 0
        && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("Connection to 127.0.0.1:7777 succeeded.");
    }
    else
    {
        enet_peer_reset(peer);
        puts("Connection to 127.0.0.1:7777 failed.");
        return EXIT_SUCCESS;
    }

    char header = ENET_FLOAT;
    float floatdata = 0.1f;
    std::stringstream ss; // Use std::ios::binary if filestream
    cereal::PortableBinaryOutputArchive outArchive(ss);

    outArchive(header, floatdata); // Serialize
    SendPacket(peer, &ss);

    // [...Game Loop...] start 

    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    while(enet_host_service(client, &event, 1000) > 0) // ENet hosts are polled for events, enet_host_service should be called fairly regularly for adequate performance
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
                printf ("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
                event.packet -> dataLength,
                event.packet -> data,
                event.peer -> address.host,
                event.peer -> address.port,
                event.channelID);
                break;
        }
    }
    

    char msg [80];
    bool running = true;
    printf("%s", "Running!\n");
    while(running)
    {
        scanf("%s", msg);
        if (msg == "exit")
        {
            running = false;
        }
        else
        {
            printf("Got: %s\n", msg);
        }
    }


    // Game loop end

    enet_peer_disconnect(peer, 0);

    while(enet_host_service(client, &event, 3000) > 0)
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                puts("Disconnection succeeded.");
                break;
        }
    }

    /* We've arrived here, so the disconnect attempt didn't */
    /* succeed yet.  Force the connection down.             */
    enet_peer_reset (peer);


    return EXIT_SUCCESS;
}