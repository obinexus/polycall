/**
 * @file dhcp-c-polycall-qa.c
 * @brief Zero-Trust DHCP-C QA Integration for Polycall v2
 * @author OBINexus Engineering
 * @date 2025-07-06
 * 
 * This DHCP-C script implements zero-trust validation for network-based
 * command distribution and QA testing across distributed Polycall nodes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <time.h>
#include "polycall/core/polycall_core.h"
#include "polycall/core/polycall_error.h"
#include "polycall/core/polycall_zerotrust.h"

/* DHCP-C Protocol Constants */
#define DHCPC_MAGIC_COOKIE      0x63825363
#define DHCPC_SERVER_PORT       67
#define DHCPC_CLIENT_PORT       68
#define DHCPC_MAX_PACKET_SIZE   1024
#define DHCPC_QA_OPTION         250  /* Custom option for QA commands */

/* Zero-Trust QA Constants */
#define ZT_QA_CHALLENGE_SIZE    32
#define ZT_QA_RESPONSE_SIZE     64
#define ZT_QA_SESSION_TIMEOUT   300  /* 5 minutes */

/* QA Test Categories */
typedef enum {
    QA_CAT_TRUE_POSITIVE  = 0x01,  /* Expected success */
    QA_CAT_TRUE_NEGATIVE  = 0x02,  /* Expected failure */
    QA_CAT_FALSE_POSITIVE = 0x04,  /* Unexpected success */
    QA_CAT_FALSE_NEGATIVE = 0x08,  /* Unexpected failure */
} qa_test_category_t;

/* DHCP-C QA Message Structure */
typedef struct {
    uint8_t  op;                    /* Message opcode */
    uint8_t  htype;                 /* Hardware type */
    uint8_t  hlen;                  /* Hardware address length */
    uint8_t  hops;                  /* Gateway hops */
    uint32_t xid;                   /* Transaction ID */
    uint16_t secs;                  /* Seconds elapsed */
    uint16_t flags;                 /* Bootp flags */
    uint32_t ciaddr;                /* Client IP address */
    uint32_t yiaddr;                /* Your (client) IP address */
    uint32_t siaddr;                /* Next server IP address */
    uint32_t giaddr;                /* Relay agent IP address */
    uint8_t  chaddr[16];            /* Client hardware address */
    uint8_t  sname[64];             /* Server host name */
    uint8_t  file[128];             /* Boot file name */
    uint32_t magic;                 /* Magic cookie */
    uint8_t  options[308];          /* DHCP options */
} dhcpc_qa_message_t;

/* Zero-Trust QA Context */
typedef struct {
    uint8_t  challenge[ZT_QA_CHALLENGE_SIZE];
    uint8_t  response[ZT_QA_RESPONSE_SIZE];
    uint64_t timestamp;
    uint32_t session_id;
    qa_test_category_t test_category;
    char     command[256];
    char     config_path[256];
    bool     repl_mode;
} zerotrust_qa_context_t;

/* QA Result Tracking */
typedef struct {
    uint32_t total_tests;
    uint32_t tp_count;  /* True Positive */
    uint32_t tn_count;  /* True Negative */
    uint32_t fp_count;  /* False Positive */
    uint32_t fn_count;  /* False Negative */
    double   accuracy;
    double   precision;
    double   recall;
    double   f1_score;
} qa_metrics_t;

/**
 * @brief Generate zero-trust challenge for QA session
 */
static int generate_qa_challenge(zerotrust_qa_context_t* qa_ctx) {
    if (RAND_bytes(qa_ctx->challenge, ZT_QA_CHALLENGE_SIZE) != 1) {
        fprintf(stderr, "Failed to generate cryptographic challenge\n");
        return -1;
    }
    
    qa_ctx->timestamp = (uint64_t)time(NULL) * 1000000;  /* microseconds */
    qa_ctx->session_id = (uint32_t)rand();
    
    return 0;
}

/**
 * @brief Verify zero-trust response
 */
static int verify_qa_response(zerotrust_qa_context_t* qa_ctx,
                              const uint8_t* provided_response) {
    /* Verify timestamp freshness */
    uint64_t current_time = (uint64_t)time(NULL) * 1000000;
    if ((current_time - qa_ctx->timestamp) > (ZT_QA_SESSION_TIMEOUT * 1000000)) {
        fprintf(stderr, "QA session expired\n");
        return -1;
    }
    
    /* Compute expected response */
    SHA256_CTX sha_ctx;
    uint8_t expected[ZT_QA_RESPONSE_SIZE];
    uint8_t combined[ZT_QA_CHALLENGE_SIZE + sizeof(uint64_t) + sizeof(uint32_t)];
    
    memcpy(combined, qa_ctx->challenge, ZT_QA_CHALLENGE_SIZE);
    memcpy(combined + ZT_QA_CHALLENGE_SIZE, &qa_ctx->timestamp, sizeof(uint64_t));
    memcpy(combined + ZT_QA_CHALLENGE_SIZE + sizeof(uint64_t), 
           &qa_ctx->session_id, sizeof(uint32_t));
    
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, combined, sizeof(combined));
    SHA256_Final(expected, &sha_ctx);
    
    /* Compare responses */
    if (memcmp(provided_response, expected, ZT_QA_RESPONSE_SIZE) != 0) {
        fprintf(stderr, "Invalid zero-trust response\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Execute Polycall command with QA validation
 */
static int execute_qa_command(zerotrust_qa_context_t* qa_ctx,
                              qa_metrics_t* metrics) {
    int result = -1;
    int expected_result = (qa_ctx->test_category & (QA_CAT_TRUE_POSITIVE | QA_CAT_FALSE_NEGATIVE)) ? 0 : -1;
    
    /* Build command line */
    char cmd_buffer[512];
    if (qa_ctx->repl_mode) {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "polycall repl -c %s --qa-mode --session-id %u",
                 qa_ctx->config_path, qa_ctx->session_id);
    } else {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "polycall %s --qa-validate --session-id %u",
                 qa_ctx->command, qa_ctx->session_id);
    }
    
    /* Execute command and capture result */
    FILE* fp = popen(cmd_buffer, "r");
    if (fp) {
        char output[256];
        if (fgets(output, sizeof(output), fp) != NULL) {
            result = (strstr(output, "SUCCESS") != NULL) ? 0 : -1;
        }
        pclose(fp);
    }
    
    /* Update metrics based on actual vs expected */
    metrics->total_tests++;
    
    if (result == 0 && expected_result == 0) {
        metrics->tp_count++;  /* True Positive */
    } else if (result != 0 && expected_result != 0) {
        metrics->tn_count++;  /* True Negative */
    } else if (result == 0 && expected_result != 0) {
        metrics->fp_count++;  /* False Positive */
        fprintf(stderr, "FALSE POSITIVE detected: %s\n", qa_ctx->command);
    } else {
        metrics->fn_count++;  /* False Negative */
        fprintf(stderr, "FALSE NEGATIVE detected: %s\n", qa_ctx->command);
    }
    
    /* Calculate metrics */
    double total_positive = metrics->tp_count + metrics->fp_count;
    double total_actual_positive = metrics->tp_count + metrics->fn_count;
    
    metrics->accuracy = (double)(metrics->tp_count + metrics->tn_count) / metrics->total_tests;
    metrics->precision = (total_positive > 0) ? (double)metrics->tp_count / total_positive : 0.0;
    metrics->recall = (total_actual_positive > 0) ? (double)metrics->tp_count / total_actual_positive : 0.0;
    
    if (metrics->precision + metrics->recall > 0) {
        metrics->f1_score = 2.0 * (metrics->precision * metrics->recall) / 
                            (metrics->precision + metrics->recall);
    }
    
    return (result == expected_result) ? 0 : -1;
}

/**
 * @brief DHCP-C QA Server
 */
static int run_dhcpc_qa_server(uint16_t port) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    dhcpc_qa_message_t msg;
    zerotrust_qa_context_t qa_ctx;
    qa_metrics_t metrics = {0};
    
    /* Create UDP socket */
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }
    
    /* Bind to port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    
    printf("DHCP-C QA Server listening on port %d\n", port);
    
    /* Main server loop */
    while (1) {
        ssize_t recv_len = recvfrom(server_fd, &msg, sizeof(msg), 0,
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("recvfrom");
            continue;
        }
        
        /* Verify magic cookie */
        if (ntohl(msg.magic) != DHCPC_MAGIC_COOKIE) {
            fprintf(stderr, "Invalid DHCP magic cookie\n");
            continue;
        }
        
        /* Process QA options */
        uint8_t* opt_ptr = msg.options;
        while (*opt_ptr != 0xFF && opt_ptr < msg.options + sizeof(msg.options)) {
            uint8_t opt_type = *opt_ptr++;
            uint8_t opt_len = *opt_ptr++;
            
            if (opt_type == DHCPC_QA_OPTION && opt_len >= sizeof(zerotrust_qa_context_t)) {
                memcpy(&qa_ctx, opt_ptr, sizeof(zerotrust_qa_context_t));
                
                /* Verify zero-trust response */
                if (verify_qa_response(&qa_ctx, qa_ctx.response) == 0) {
                    /* Execute QA command */
                    execute_qa_command(&qa_ctx, &metrics);
                    
                    /* Send response with metrics */
                    dhcpc_qa_message_t response = msg;
                    response.op = 2;  /* BOOTREPLY */
                    
                    /* Add metrics to response */
                    uint8_t* resp_opt = response.options;
                    *resp_opt++ = DHCPC_QA_OPTION;
                    *resp_opt++ = sizeof(qa_metrics_t);
                    memcpy(resp_opt, &metrics, sizeof(qa_metrics_t));
                    resp_opt += sizeof(qa_metrics_t);
                    *resp_opt = 0xFF;  /* End option */
                    
                    sendto(server_fd, &response, sizeof(response), 0,
                           (struct sockaddr*)&client_addr, client_len);
                    
                    /* Log metrics */
                    printf("QA Metrics - Tests: %u, Accuracy: %.2f%%, Precision: %.2f%%, "
                           "Recall: %.2f%%, F1: %.2f%%\n",
                           metrics.total_tests, metrics.accuracy * 100,
                           metrics.precision * 100, metrics.recall * 100,
                           metrics.f1_score * 100);
                }
            }
            
            opt_ptr += opt_len;
        }
    }
    
    close(server_fd);
    return 0;
}

/**
 * @brief DHCP-C QA Client
 */
static int run_dhcpc_qa_client(const char* server_ip, uint16_t port,
                               const char* command, qa_test_category_t category,
                               const char* config_path, bool repl_mode) {
    int client_fd;
    struct sockaddr_in server_addr;
    dhcpc_qa_message_t msg = {0};
    zerotrust_qa_context_t qa_ctx = {0};
    
    /* Create UDP socket */
    if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }
    
    /* Set server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    /* Prepare QA context */
    qa_ctx.test_category = category;
    strncpy(qa_ctx.command, command, sizeof(qa_ctx.command) - 1);
    if (config_path) {
        strncpy(qa_ctx.config_path, config_path, sizeof(qa_ctx.config_path) - 1);
    }
    qa_ctx.repl_mode = repl_mode;
    
    /* Generate challenge and compute response */
    if (generate_qa_challenge(&qa_ctx) < 0) {
        close(client_fd);
        return -1;
    }
    
    /* Compute response (simplified for demo) */
    SHA256_CTX sha_ctx;
    uint8_t combined[ZT_QA_CHALLENGE_SIZE + sizeof(uint64_t) + sizeof(uint32_t)];
    
    memcpy(combined, qa_ctx.challenge, ZT_QA_CHALLENGE_SIZE);
    memcpy(combined + ZT_QA_CHALLENGE_SIZE, &qa_ctx.timestamp, sizeof(uint64_t));
    memcpy(combined + ZT_QA_CHALLENGE_SIZE + sizeof(uint64_t), 
           &qa_ctx.session_id, sizeof(uint32_t));
    
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, combined, sizeof(combined));
    SHA256_Final(qa_ctx.response, &sha_ctx);
    
    /* Build DHCP-C message */
    msg.op = 1;  /* BOOTREQUEST */
    msg.htype = 1;  /* Ethernet */
    msg.hlen = 6;
    msg.xid = htonl(qa_ctx.session_id);
    msg.magic = htonl(DHCPC_MAGIC_COOKIE);
    
    /* Add QA option */
    uint8_t* opt_ptr = msg.options;
    *opt_ptr++ = DHCPC_QA_OPTION;
    *opt_ptr++ = sizeof(zerotrust_qa_context_t);
    memcpy(opt_ptr, &qa_ctx, sizeof(zerotrust_qa_context_t));
    opt_ptr += sizeof(zerotrust_qa_context_t);
    *opt_ptr = 0xFF;  /* End option */
    
    /* Send request */
    if (sendto(client_fd, &msg, sizeof(msg), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(client_fd);
        return -1;
    }
    
    /* Wait for response */
    struct timeval tv = {5, 0};  /* 5 second timeout */
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    dhcpc_qa_message_t response;
    ssize_t recv_len = recvfrom(client_fd, &response, sizeof(response), 0, NULL, NULL);
    
    if (recv_len > 0) {
        /* Extract metrics from response */
        opt_ptr = response.options;
        while (*opt_ptr != 0xFF && opt_ptr < response.options + sizeof(response.options)) {
            uint8_t opt_type = *opt_ptr++;
            uint8_t opt_len = *opt_ptr++;
            
            if (opt_type == DHCPC_QA_OPTION && opt_len == sizeof(qa_metrics_t)) {
                qa_metrics_t* metrics = (qa_metrics_t*)opt_ptr;
                printf("QA Test Complete - Category: %s, Result: %s\n",
                       (category == QA_CAT_TRUE_POSITIVE) ? "TP" :
                       (category == QA_CAT_TRUE_NEGATIVE) ? "TN" :
                       (category == QA_CAT_FALSE_POSITIVE) ? "FP" : "FN",
                       (metrics->total_tests > 0) ? "RECORDED" : "FAILED");
                break;
            }
            opt_ptr += opt_len;
        }
    }
    
    close(client_fd);
    return 0;
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Server: %s server [port]\n", argv[0]);
        fprintf(stderr, "  Client: %s client <server_ip> <command> <category> [options]\n", argv[0]);
        fprintf(stderr, "\nCategories: TP, TN, FP, FN\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  --config <path>  : Config file for REPL mode\n");
        fprintf(stderr, "  --repl           : Use REPL mode with config\n");
        return 1;
    }
    
    if (strcmp(argv[1], "server") == 0) {
        uint16_t port = (argc > 2) ? atoi(argv[2]) : DHCPC_SERVER_PORT;
        return run_dhcpc_qa_server(port);
    }
    else if (strcmp(argv[1], "client") == 0 && argc >= 5) {
        const char* server_ip = argv[2];
        const char* command = argv[3];
        const char* category_str = argv[4];
        
        qa_test_category_t category;
        if (strcmp(category_str, "TP") == 0) category = QA_CAT_TRUE_POSITIVE;
        else if (strcmp(category_str, "TN") == 0) category = QA_CAT_TRUE_NEGATIVE;
        else if (strcmp(category_str, "FP") == 0) category = QA_CAT_FALSE_POSITIVE;
        else if (strcmp(category_str, "FN") == 0) category = QA_CAT_FALSE_NEGATIVE;
        else {
            fprintf(stderr, "Invalid category: %s\n", category_str);
            return 1;
        }
        
        const char* config_path = NULL;
        bool repl_mode = false;
        
        /* Parse additional options */
        for (int i = 5; i < argc; i++) {
            if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
                config_path = argv[++i];
            } else if (strcmp(argv[i], "--repl") == 0) {
                repl_mode = true;
            }
        }
        
        return run_dhcpc_qa_client(server_ip, DHCPC_SERVER_PORT,
                                   command, category, config_path, repl_mode);
    }
    
    fprintf(stderr, "Invalid command: %s\n", argv[1]);
    return 1;
}
