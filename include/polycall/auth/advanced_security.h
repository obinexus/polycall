/*
 * advanced_security.h
 * Advanced security features for PolyCall
 * Part of the auth infrastructure module
 */

#ifndef POLYCALL_ADVANCED_SECURITY_H
#define POLYCALL_ADVANCED_SECURITY_H

#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct polycall_security_context polycall_security_context_t;
typedef struct polycall_security_policy polycall_security_policy_t;

/* Security context management */
polycall_security_context_t* polycall_security_create_context(void);
void polycall_security_destroy_context(polycall_security_context_t* ctx);

/* Policy management */
polycall_security_policy_t* polycall_security_create_policy(const char* name);
void polycall_security_destroy_policy(polycall_security_policy_t* policy);
int polycall_security_apply_policy(polycall_security_context_t* ctx,
                                   polycall_security_policy_t* policy);

/* Security operations */
int polycall_security_validate_token(polycall_security_context_t* ctx,
                                     const char* token);
int polycall_security_encrypt_data(polycall_security_context_t* ctx,
                                   const void* data, size_t data_len,
                                   void** encrypted, size_t* encrypted_len);
int polycall_security_decrypt_data(polycall_security_context_t* ctx,
                                   const void* encrypted, size_t encrypted_len,
                                   void** data, size_t* data_len);

/* Audit and compliance */
void polycall_security_audit_log(polycall_security_context_t* ctx,
                                 const char* action, const char* details);
bool polycall_security_check_compliance(polycall_security_context_t* ctx);

#endif /* POLYCALL_ADVANCED_SECURITY_H */
