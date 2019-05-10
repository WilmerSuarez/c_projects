#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "csapp.h"
#include "protocol.h"

/*
 * Send a packet, which consists of a fixed-size header followed by an
 * optional associated data payload.
 */
int proto_send_packet(int fd, MZW_PACKET *pkt, void *data) {
    int bytes = 0; // Number of bytes returned by write()
    uint16_t psize = pkt->size; // Save original payload size

    /* Convert the multi-byte fields to Network Byte Order */
    pkt->size = htons(pkt->size);
    pkt->timestamp_sec = htonl(pkt->timestamp_sec);
    pkt->timestamp_nsec = htonl(pkt->timestamp_nsec);

    /* Send converted Header */
    if((bytes = write(fd, pkt, sizeof(MZW_PACKET))) == -1 ||
        bytes != sizeof(MZW_PACKET)) { // Make sure corrent number of bytes have been sent
        fprintf(stderr, "Send Header Error\n");
        return -1;
    }

    /* Send Payload, if any */
    if(data) {
        if((bytes = write(fd, data, psize)) == -1 || 
            bytes != psize) { 
            fprintf(stderr, "Send Payload Error\n");
            return -1;
        }
    }

    return 0;
}

/*
 * Receive a packet, blocking until one is available.
 */
int proto_recv_packet(int fd, MZW_PACKET *pkt, void **datap) {
    int bytes = 0; // Nuber of bytes returned by read()

    /* Read Packet Header */
    if((bytes = read(fd, pkt, sizeof(MZW_PACKET))) <= 0 ||
        bytes != sizeof(MZW_PACKET)) { // Make sure corrent number of bytes have been sent
        return -1;
    }

    /* Convert fields to Host Byte Order */
    pkt->size = ntohs(pkt->size);
    pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
    pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

    /* Read Payload, if any */
    if(datap && pkt->size) {
        /* Allocate memory for Payload */
        *datap = Calloc(1, pkt->size);
        if((bytes = read(fd, *datap, pkt->size)) <= 0 ||
            bytes != pkt->size) {
            fprintf(stderr, "Read Payload Error\n");
            return -1;
        }
    }
    
    return 0;
}
