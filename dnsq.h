/**
 * dnsq.h - a DNS query client
 *
 * zach sheppard (zsheppa)
 * cpsc 360
 * Dr. Remy
 * Due: 2-29-2016
 */

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#define RECORD_A 1
#define RECORD_CNAME 5

#define BUFFER_SIZE 2048
#define DOMAIN_DEPTH 20

typedef struct DNS_HEADER {

    // id to identify the request
    uint16_t id;
    
    // 16-bit flag representation
    // qr (query):                  1 bit
    // opcode:                      4 bits
    // aa (authoritative answer):   1 bit
    // tc (truncated):              1 bit
    // rd (recursion desired):      1 bit
    // ra (recursion avail.):       1 bit
    // z (reserved):                3 bits
    // rcode (return code):         4 bits
    uint16_t flags;
    
    // entries in the question section; default is 1
    uint16_t qdcount;
    
    // resource records
    uint16_t ancount;
    
    // server resource records (in the authority section)
    uint16_t nscount;
    
    // resource records (in addl. section)
    uint16_t arcount;
    
} dns_h;

typedef struct DNS_QUESTION {
    
    // question type (default is 1)
    uint16_t qtype;
    
    // question class (default is 1)
    uint16_t qclass;
    
} dns_q;

typedef struct DNS_RECORD {
    
    // type of response (A, CNAME, etc)
    uint16_t type;
    
    // class response (default is 1)
    uint16_t class;
    
    // cache length
    uint32_t ttl;
    
    // rdata length
    uint16_t rdlength;

} dns_r;

// encode a domain name... e.g. www.google.com = 3www6google3com
char* encode_domain_name(char*);

// initializes the connection with the DNS server
int init_connection(struct sockaddr_in*, char*, unsigned int, unsigned int);

// receive an answer from a question to the server
void recv_answer(int, struct sockaddr_in*, unsigned int, char*, dns_r*);

// output answers (records)
int print_answer(char*, ssize_t, unsigned int);

// send a question to a DNS server
void send_question(int, struct sockaddr_in*, dns_h*, char*, dns_q*);

// verify a response header is correct; exit if not
void verify_header(dns_h*);

