/**
 * dnsq.c - a DNS query client
 *
 * zach sheppard (zsheppa)
 * cpsc 360
 * Dr. Remy
 * Due: 2-29-2016
 */

/*
 * dnsq [-t <time>] [-r <retries>] [-p <port>] @<svr> <name>
 * timeout (Optional): Indicates in seconds, how long to wait
 * before regenerating an unanswered query. Default value: 5.
 *
 * retries (Optional): The number of times the client will
 * re-generate the query, before it quits with an error, if no
 * response is received from the server. Default value: 3.
 *
 * port (Optional): The DNS server’s UDP port. Default value: 53.
 *
 * server (Required): The IP address of the DNS server (e.g.,
 * 192.168.0.12).
 *
 * name (Required): The name we’re looking for.
 */

#include "dnsq.h"

int DEBUG_OFFSET = 0;

int main(int argc, char** argv) {

    // server address structure
    struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
    
    // random ID number
    int id = 0;
    
    // name we're searching
    char* name = NULL;
    
    // DNS server's UDP port
    unsigned int port = 53;
    
    // number of times the client will re-generate the query
    unsigned int retries = 3;
    
    // IP of DNS server
    char* server = NULL;
    
    // socket descriptor
    int sock = 0;
    
    // timeout in seconds before regenerating a query
    unsigned int timeout = 5;
    
    // seed random number generator
    srand(time(NULL));
    
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-t <time>] [-r <retries>] [-p <port>] @<svr> <name>\n", argv[0]);
        exit(1);
    } else {
        for (int i = 1; i < argc; i++) {
            // check optional arguments
            if (strcmp(argv[i], "-t") == 0) {
                timeout = atoi(argv[++i]);
                continue;
            } else if (strcmp(argv[i], "-r") == 0) {
                retries = atoi(argv[++i]);
                continue;
            } else if (strcmp(argv[i], "-p") == 0) {
                port = atoi(argv[++i]);
                continue;
            }
            
            // check required arguments
            if (server == NULL) {
                server = argv[i];
                
                // check if @ was provided
                if (server[0] == '@') {
                    // remove
                    strcpy(server, server+1);
                }
            } else if (name == NULL) {
                name = argv[i];
            }
        }
    }
    
    // check to make sure our required arguments were passed
    if (server == NULL || name == NULL) {
        fprintf(stderr, "A server and a lookup name must be passed\n");
        fprintf(stderr, "Usage: %s [-t <time>] [-r <retries>] [-p <port>] @<svr> <name>\n", argv[0]);
        exit(1);
    }
    
    // generate a new ID number for this request
    id = rand();
    
    // generate a dns header
    dns_h* header = malloc(sizeof(dns_h));
    
    // assign the ID number for this request
    // assign other defaults
    header->id = htons(id);
    
    // set the 16 bit flag to 0x100 which effectively enables recursion
    // 00000001 00000000
    header->flags = htons(0x100);
    
    header->qdcount = htons(1);
    header->ancount = htons(0);
    header->nscount = htons(0);
    header->arcount = htons(0);
    
    // generate a dns question
    dns_q* question = malloc(sizeof(dns_q));

    // defaults for question
    question->qtype = htons(RECORD_A);
    question->qclass = htons(1);
    
    // generate a dns answer to store the reply from the server
    dns_r* record = malloc(sizeof(dns_r));
    
    // create a connection to the DNS server
    sock = init_connection(addr, server, port, timeout);
    
    // send a question
    // publish the encoded domain name globally so other functions can
    // use it
    char* enc_name = encode_domain_name(name);
    send_question(sock, addr, header, enc_name, question);
    
    // receive the answer
    recv_answer(sock, addr, retries, enc_name, record);
    
    return 0;
}

// return a length encoded name
char* encode_domain_name(char* name) {
    // count of domains chars
    unsigned int count = 0;
    
    // a list of chars that are splitted subdomains
    // e.g. www.clemson.edu would be 3: www, clemson, edu
    char* domains[DOMAIN_DEPTH];
    
    // tmp to store subdomains
    char* tmp = NULL;
    
    for (int i = 0; i < strlen(name); i++) {
        // if we reach a period we need to reallocate
        // (or beginning of string)
        if (name[i] == '.' || i == 0) {
            // if it's a period we need to store the old domain sub
            if (name[i] == '.') {
                domains[count++] = tmp;
            }
            
            // reallocate
            tmp = malloc(strlen(name) * sizeof(char));
            memset(tmp, '\0', sizeof(*tmp));
            
            // if it's a period we can skip the assignment
            if (name[i] == '.') continue;
        }
        tmp[strlen(tmp)] = name[i];
    }
    
    // add the last part (tld)
    domains[count++] = tmp;
    
    // cycle through the domains and create an encoded string
    
    // first get the total length of the string
    unsigned int total = 0;
    for (int i = 0; i < count; i++) {
        // add an extra bit for the length portion
        total += strlen(domains[i]) + 1;
    }
    
    // setup the encoded string
    unsigned int enccount = 0;
    char* enc = malloc(total * sizeof(char));
    
    // fill it
    for (int i = 0; i < count; i++) {
        char c = strlen(domains[i]);
        enc[enccount++] = c;
        for (int z = 0; z < c; z++) {
            enc[enccount++] = domains[i][z];
        }
    }
    
    // publish
    return enc;
}

// initializes the connection with the DNS server
int init_connection(struct sockaddr_in* addr, char* server, unsigned int port, unsigned int seconds) {
    // timeout structure
    struct timeval timeout;
    
    // create the socket
    int descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (descriptor < 0) {
        fprintf(stderr, "There was a problem creating a socket to %s:%d\n", server, port);
        exit(1);
    }

    // if no error, assign values to the addr struct
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(server);
    addr->sin_port = htons(port);
    
    // assign timeout values if provided
    if (seconds > 0) {
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    }
    
    return descriptor;
}

// receive an answer from a question to the server
void recv_answer(int sock, struct sockaddr_in* addr, unsigned int retries, char* encoded_name, dns_r* record) {
    char buffer[BUFFER_SIZE];
    unsigned int count = 0; // retry count
    
    socklen_t addr_l = sizeof(*addr); // this is weird, but required
    
    // bytes received from the socket
    ssize_t bytes_recv = -1;
    
    // if there was en error, try (retries) times
    while (bytes_recv < 0) {
        bytes_recv = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)addr, &addr_l);
        
        // if no bytes received, check the error
        if (bytes_recv < 0) {
            if (errno == 11) {
                if (++count > retries) {
                    fprintf(stderr, "ERROR\treached maximum number of retries (%d)\n", retries);
                    exit(1);
                }
            } else {
                // bad error!
                perror("ERROR\t");
                exit(errno);
            }
        }
    }
    
    // start the header at the beginning of the buffer
    dns_h* header = (dns_h*)&buffer;
    
    // for some reason I had to redeclare this
    // print_answers did not like making a copy
    uint16_t answer_count = ntohs(header->ancount);
    
    // verify the header is correct and exit if there are any errors
    verify_header(header);
    
    // check to see if there are any answers to print
    if (answer_count == 0) {
        printf("NOTFOUND\n");
        return;
    }
    
    // extract the authoritative flag
    unsigned int aa = 0;
    
    // print all answers
    // get the offset where the buffer should start
    //      this skips past the header, the domain
    //      name that is queried, (+1 for \0)
    //      the question, and the name of the
    //      domain the reply references
    int offset = sizeof(dns_h) + strlen(encoded_name) + 1 + sizeof(dns_q) + 2;
    for (int i = 0; i < answer_count; i++) {
        offset += print_answer(buffer + offset, bytes_recv, aa);
    }
}

// outputs an answer from a server response
int print_answer(char* buffer, ssize_t size, unsigned int aa) {
    // compute the authoritative string
    char auth[7];
    memset(auth, '\0', sizeof(auth));
    
    if (aa == 0) {
        strcpy(auth, "nonauth");
    } else if (aa == 1) {
        strcpy(auth, "auth");
    } else {
        fprintf(stderr, "ERROR\tthe provided aa bit is not valid (%d)\n", aa);
        exit(1);
    }
    
    // patch most of the data into a record structure
    dns_r* record = (dns_r*)buffer;
    
    // socket structure to convert IP address
    struct sockaddr_in answersock;
    
    // ip address
    char* inet_addr = NULL;
    
    // move all bits from host order
    record->type = ntohs(record->type);
    record->class = ntohs(record->class);
    record->ttl = ntohl(record->ttl);
    record->rdlength = ntohs(record->rdlength);
    
    /*printf("type: %d (0x%02x)\n", record->type, record->type);
    printf("class: %d (0x%02x)\n", record->class, record->class);
    printf("ttl: %d (0x%02x)\n", record->ttl, record->ttl);
    printf("record length: %d (0x%02x)\n", record->rdlength, record->rdlength);*/
    
    // declare space to store the IP address or CNAME
    char rdata[record->rdlength + 1];
    memset(rdata, '\0', sizeof(rdata));
    
    // fill it
    for (int i = 0; i < record->rdlength; i++) {
        // grabs the data that occurs after rdlength
        //      the minus 2 at the end is because I had to skip
        //      the name in the original offset since we assume
        //      that the name given matches what we requested
        rdata[i] = (char)*(buffer + sizeof(dns_r) + i - 2);
    }
    
    // if the record is an IP, output that
    if (record->type == RECORD_A) {
        // to get the actual string IP address we have to use
        // inet_ntoa which requires a long
        long* aptr = (long*)&rdata;
        
        // store it in the answersock
        answersock.sin_addr.s_addr = (*aptr);
        
        // convert
        inet_addr = inet_ntoa(answersock.sin_addr);
        
        // output
        printf("IP\t%s\t%d\t%s\n", inet_addr, record->ttl, auth);
    // if the record is a CNAME, output that
    } else if (record->type == RECORD_CNAME) {
        // rdata is the CNAME
        printf("CNAME\t%s\t%d\t%s\n", rdata, record->ttl, auth);
    } else {
        fprintf(stderr, "ERROR\tthe returned record type of %d is not supported\n", record->type);
        exit(1);
    }
    
    return (sizeof(dns_r) + record->rdlength);
}

// send a question to the server
void send_question(int sock, struct sockaddr_in* addr, dns_h* header, char* domain, dns_q* question) {
    // initialize buffer and set it to the length of our two sub-buffers
    // plus the encoded domain name
    size_t BUF_SIZE = sizeof(*header) + sizeof(*question) + strlen(domain) + 1;
    char buffer[BUF_SIZE];
    memset(buffer, '\0', BUF_SIZE);
    
    // copy the structs directly into the buffer
    int offset = 0;
    memcpy(buffer, header, sizeof(*header));
    offset += sizeof(*header);
    
    // after header, send the domain name directly
    for (int i = 0; i <= strlen(domain); i++) {
        buffer[offset++] = domain[i];
    }
    
    // then copy the question
    memcpy(buffer + offset, question, sizeof(*question));
    
    ssize_t sent = sendto(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)addr, sizeof(*addr));
    
    // check if what we sent was correct
    if (sent < 0) {
        perror("There was a problem querying the server");
        exit(1);
    } else if (sent != BUF_SIZE) {
        fprintf(stderr, "The query sent does not match the intended size\n");
        exit(1);
    }
}

void verify_header(dns_h* header) {
    // check return code first
    
    // get the return code from the server by clearing the bits to the right, then the left
    uint16_t rcode = (header->flags >> 8);
    rcode = rcode << 12;
    rcode = rcode >> 12;
    
    // if the return code is 1, 2, 4, or 5 there is a server error
    if (rcode == 1) {
        fprintf(stderr, "ERROR\tformat error; the server was unable to understand our request\n");
        exit(1);
    } else if (rcode == 2) {
        fprintf(stderr, "ERROR\tserver failure; the server was unable to process this request\n");
        exit(2);
    // 3 is when a domain does not exist
    } else if (rcode == 3) {
        fprintf(stderr, "NOTFOUND\n");
        exit(3);
    } else if (rcode == 4) {
        fprintf(stderr, "ERROR\tnot implemented; the server does not support this query\n");
        exit(4);
    } else if (rcode == 5) {
        fprintf(stderr, "ERROR\trefused; the server refused to talk to us... did you make it mad?\n");
        exit(5);
    } else if (rcode != 0) {
        fprintf(stderr, "ERROR\tunknown\n");
        exit(6);
    }
    
    // if our return code is fine, check the flags
    /*if (header->tc != 0) {
        fprintf(stderr, "ERROR\tmessage was truncated\n");
        exit(7);
    } else if (header->ra == 0) {
        fprintf(stderr, "ERROR\tserver does not support recursion\n");
        exit(8);
    }*/
    
    // looks good
}
