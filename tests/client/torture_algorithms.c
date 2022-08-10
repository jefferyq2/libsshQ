/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2010 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "config.h"

#define LIBSSH_STATIC

#include "torture.h"
#include "libssh/libssh.h"
#include "libssh/priv.h"
#include "libssh/session.h"
#include "libssh/oqs-utils.h"

#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

static int sshd_setup(void **state)
{
    torture_setup_sshd_server(state, false);

    return 0;
}

static int sshd_teardown(void **state) {
    torture_teardown_sshd_server(state);

    return 0;
}

static int sshd_setup_hmac(void **state)
{
    torture_setup_sshd_server(state, false);
    /* Set MAC to be something other than what the client will offer */
    torture_update_sshd_config(state, "MACs hmac-sha2-512");

    return 0;
}


static int session_setup(void **state) {
    struct torture_state *s = *state;
    int verbosity = torture_libssh_verbosity();
    struct passwd *pwd;
    bool false_v = false;
    int rc;

    pwd = getpwnam("bob");
    assert_non_null(pwd);

    rc = setuid(pwd->pw_uid);
    assert_return_code(rc, errno);

    s->ssh.session = ssh_new();
    assert_non_null(s->ssh.session);

    ssh_options_set(s->ssh.session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(s->ssh.session, SSH_OPTIONS_HOST, TORTURE_SSH_SERVER);
    /* Prevent parsing configuration files that can introduce different
     * algorithms then we want to test */
    ssh_options_set(s->ssh.session, SSH_OPTIONS_PROCESS_CONFIG, &false_v);

    return 0;
}

static int session_teardown(void **state)
{
    struct torture_state *s = *state;

    ssh_disconnect(s->ssh.session);
    ssh_free(s->ssh.session);

    return 0;
}

static void test_algorithm(ssh_session session,
                           const char *kex,
                           const char *cipher,
                           const char *hmac) {
    int rc;
    char data[256];
    size_t len_to_test[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 10,
        12, 15, 16, 20,
        31, 32, 33,
        63, 64, 65,
        100, 127, 128
    };
    unsigned int i;

    if (kex != NULL) {
        rc = ssh_options_set(session, SSH_OPTIONS_KEY_EXCHANGE, kex);
        assert_ssh_return_code(session, rc);
    }

    if (cipher != NULL) {
        rc = ssh_options_set(session, SSH_OPTIONS_CIPHERS_C_S, cipher);
        assert_ssh_return_code(session, rc);
        rc = ssh_options_set(session, SSH_OPTIONS_CIPHERS_S_C, cipher);
        assert_ssh_return_code(session, rc);
    }

    if (hmac != NULL) {
        rc = ssh_options_set(session, SSH_OPTIONS_HMAC_C_S, hmac);
        assert_ssh_return_code(session, rc);
        rc = ssh_options_set(session, SSH_OPTIONS_HMAC_S_C, hmac);
        assert_ssh_return_code(session, rc);
    }

    rc = ssh_connect(session);
    assert_ssh_return_code(session, rc);

    /* send ignore packets of all sizes */
    memset(data, 0, sizeof(data));
    for (i = 0; i < (sizeof(len_to_test) / sizeof(size_t)); i++) {
        memset(data, 'A', len_to_test[i]);
        ssh_send_ignore(session, data);
        ssh_handle_packets(session, 50);
    }

    rc = ssh_userauth_none(session, NULL);
    if (rc != SSH_OK) {
        rc = ssh_get_error_code(session);
        assert_int_equal(rc, SSH_REQUEST_DENIED);
    }

    ssh_disconnect(session);
}

static void torture_algorithms_aes128_cbc_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha1");
}

static void torture_algorithms_aes128_cbc_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha2-256");
}

static void torture_algorithms_aes128_cbc_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha2-512");
}

static void torture_algorithms_aes128_cbc_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes128_cbc_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes128_cbc_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-cbc", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes192_cbc_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha1");
}

static void torture_algorithms_aes192_cbc_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha2-256");
}

static void torture_algorithms_aes192_cbc_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha2-512");
}

static void torture_algorithms_aes192_cbc_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes192_cbc_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes192_cbc_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-cbc", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes256_cbc_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha1");
}

static void torture_algorithms_aes256_cbc_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha2-256");
}

static void torture_algorithms_aes256_cbc_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha2-512");
}

static void torture_algorithms_aes256_cbc_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes256_cbc_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes256_cbc_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-cbc", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes128_ctr_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha1");
}

static void torture_algorithms_aes128_ctr_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha2-256");
}

static void torture_algorithms_aes128_ctr_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha2-512");
}

static void torture_algorithms_aes128_ctr_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes128_ctr_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes128_ctr_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-ctr", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes192_ctr_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha1");
}

static void torture_algorithms_aes192_ctr_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha2-256");
}

static void torture_algorithms_aes192_ctr_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha2-512");
}

static void torture_algorithms_aes192_ctr_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes192_ctr_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes192_ctr_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes192-ctr", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes256_ctr_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha1");
}

static void torture_algorithms_aes256_ctr_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha2-256");
}

static void torture_algorithms_aes256_ctr_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha2-512");
}

static void torture_algorithms_aes256_ctr_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_aes256_ctr_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_aes256_ctr_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-ctr", "hmac-sha2-512-etm@openssh.com");
}

static void torture_algorithms_aes128_gcm(void **state)
{
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-gcm@openssh.com", NULL);
}

static void torture_algorithms_aes256_gcm(void **state)
{
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-gcm@openssh.com", NULL);
}

static void torture_algorithms_aes128_gcm_mac(void **state)
{
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes128-gcm@openssh.com", "hmac-sha1");
}

static void torture_algorithms_aes256_gcm_mac(void **state)
{
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, NULL/*kex*/, "aes256-gcm@openssh.com", "hmac-sha1");
}

static void torture_algorithms_3des_cbc_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha1");
}

static void torture_algorithms_3des_cbc_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha2-256");
}

static void torture_algorithms_3des_cbc_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha2-512");
}

static void torture_algorithms_3des_cbc_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_3des_cbc_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_3des_cbc_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "3des-cbc", "hmac-sha2-512-etm@openssh.com");
}

#if defined(WITH_BLOWFISH_CIPHER) && defined(OPENSSH_BLOWFISH_CBC)
static void torture_algorithms_blowfish_cbc_hmac_sha1(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha1");
}

static void torture_algorithms_blowfish_cbc_hmac_sha2_256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha2-256");
}

static void torture_algorithms_blowfish_cbc_hmac_sha2_512(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha2-512");
}

static void torture_algorithms_blowfish_cbc_hmac_sha1_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha1-etm@openssh.com");
}

static void torture_algorithms_blowfish_cbc_hmac_sha2_256_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha2-256-etm@openssh.com");
}

static void torture_algorithms_blowfish_cbc_hmac_sha2_512_etm(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, NULL/*kex*/, "blowfish-cbc", "hmac-sha2-512-etm@openssh.com");
}
#endif /* WITH_BLOWFISH_CIPHER */

#ifdef OPENSSH_CHACHA20_POLY1305_OPENSSH_COM
static void torture_algorithms_chacha20_poly1305(void **state)
{
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session,
                   NULL, /*kex*/
                   "chacha20-poly1305@openssh.com",
                   NULL);
}
static void torture_algorithms_chacha20_poly1305_mac(void **state)
{
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session,
                   NULL, /*kex*/
                   "chacha20-poly1305@openssh.com",
                   "hmac-sha1"); /* different from the server */
}
#endif /* OPENSSH_CHACHA20_POLY1305_OPENSSH_COM */

static void torture_algorithms_zlib(void **state) {
    struct torture_state *s = *state;
    ssh_session session = s->ssh.session;
    int rc;

    rc = ssh_options_set(session, SSH_OPTIONS_COMPRESSION_C_S, "zlib");
#ifdef WITH_ZLIB
    assert_int_equal(rc, SSH_OK);
#else
    assert_int_equal(rc, SSH_ERROR);
#endif

    rc = ssh_options_set(session, SSH_OPTIONS_COMPRESSION_S_C, "zlib");
#ifdef WITH_ZLIB
    assert_int_equal(rc, SSH_OK);
#else
    assert_int_equal(rc, SSH_ERROR);
#endif

    rc = ssh_connect(session);
#ifdef WITH_ZLIB
    if (ssh_get_openssh_version(session)) {
        assert_false(rc == SSH_OK);
        ssh_disconnect(session);
        return;
    }
#endif
    assert_int_equal(rc, SSH_OK);

    rc = ssh_userauth_none(session, NULL);
    if (rc != SSH_OK) {
        rc = ssh_get_error_code(session);
        assert_int_equal(rc, SSH_REQUEST_DENIED);
    }

    ssh_disconnect(session);
}

static void torture_algorithms_zlib_openssh(void **state) {
    struct torture_state *s = *state;
    ssh_session session = s->ssh.session;
    int rc;

    rc = ssh_options_set(session, SSH_OPTIONS_COMPRESSION_C_S, "zlib@openssh.com");
#ifdef WITH_ZLIB
    assert_int_equal(rc, SSH_OK);
#else
    assert_int_equal(rc, SSH_ERROR);
#endif

    rc = ssh_options_set(session, SSH_OPTIONS_COMPRESSION_S_C, "zlib@openssh.com");
#ifdef WITH_ZLIB
    assert_int_equal(rc, SSH_OK);
#else
    assert_int_equal(rc, SSH_ERROR);
#endif

    rc = ssh_connect(session);
#ifdef WITH_ZLIB
    if (ssh_get_openssh_version(session)) {
        assert_true(rc==SSH_OK);
        rc = ssh_userauth_none(session, NULL);
        if (rc != SSH_OK) {
            rc = ssh_get_error_code(session);
            assert_int_equal(rc, SSH_REQUEST_DENIED);
        }
        ssh_disconnect(session);
        return;
    }
    assert_false(rc == SSH_OK);
#else
    assert_int_equal(rc, SSH_OK);
#endif

    ssh_disconnect(session);
}

#if defined(HAVE_ECC)
static void torture_algorithms_ecdh_sha2_nistp256(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, "ecdh-sha2-nistp256", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_ecdh_sha2_nistp384(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, "ecdh-sha2-nistp384", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_ecdh_sha2_nistp521(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, "ecdh-sha2-nistp521", NULL/*cipher*/, NULL/*hmac*/);
}
#endif

#ifdef OPENSSH_CURVE25519_SHA256
static void torture_algorithms_ecdh_curve25519_sha256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, "curve25519-sha256", NULL/*cipher*/, NULL/*hmac*/);
}
#endif /* OPENSSH_CURVE25519_SHA256 */

#ifdef OPENSSH_CURVE25519_SHA256_LIBSSH_ORG
static void torture_algorithms_ecdh_curve25519_sha256_libssh_org(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, "curve25519-sha256@libssh.org", NULL/*cipher*/, NULL/*hmac*/);
}
#endif /* OPENSSH_CURVE25519_SHA256_LIBSSH_ORG */

static void torture_algorithms_dh_group1(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, "diffie-hellman-group1-sha1", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_dh_group14(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, "diffie-hellman-group14-sha1", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_dh_group14_sha256(void **state) {
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, "diffie-hellman-group14-sha256", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_dh_group16(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, "diffie-hellman-group16-sha512", NULL/*cipher*/, NULL/*hmac*/);
}

static void torture_algorithms_dh_group18(void **state) {
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session, "diffie-hellman-group18-sha512", NULL/*cipher*/, NULL/*hmac*/);
}

#ifdef WITH_GEX
static void torture_algorithms_dh_gex_sha1(void **state)
{
    struct torture_state *s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session,
                   "diffie-hellman-group-exchange-sha1",
                   NULL,  /* cipher */
                   NULL); /* hmac */
}

static void torture_algorithms_dh_gex_sha256(void **state)
{
    struct torture_state *s = *state;

    test_algorithm(s->ssh.session,
                   "diffie-hellman-group-exchange-sha256",
                   NULL, /* cipher */
                   NULL); /* hmac */
}
#endif /* WITH_GEX */

#ifdef WITH_POST_QUANTUM_CRYPTO
///// OQS_TEMPLATE_FRAGMENT_OQS_FUNCS_START
static void torture_algorithms_frodokem640aes_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_640_AES_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_frodokem640aes_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_FRODOKEM_640_AES_SHA256, NULL, NULL);
}
static void torture_algorithms_frodokem976aes_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_976_AES_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_frodokem976aes_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_FRODOKEM_976_AES_SHA384, NULL, NULL);
}
static void torture_algorithms_frodokem1344aes_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_1344_AES_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_frodokem1344aes_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_FRODOKEM_1344_AES_SHA512, NULL, NULL);
}
static void torture_algorithms_frodokem640shake_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_640_SHAKE_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_frodokem640shake_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_FRODOKEM_640_SHAKE_SHA256, NULL, NULL);
}
static void torture_algorithms_frodokem976shake_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_976_SHAKE_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_frodokem976shake_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_FRODOKEM_976_SHAKE_SHA384, NULL, NULL);
}
static void torture_algorithms_frodokem1344shake_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_FRODOKEM_1344_SHAKE_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_frodokem1344shake_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_FRODOKEM_1344_SHAKE_SHA512, NULL, NULL);
}
static void torture_algorithms_saberlightsaber_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_SABER_LIGHTSABER_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_saberlightsaber_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_SABER_LIGHTSABER_SHA256, NULL, NULL);
}
static void torture_algorithms_sabersaber_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_SABER_SABER_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_sabersaber_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_SABER_SABER_SHA384, NULL, NULL);
}
static void torture_algorithms_saberfiresaber_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_SABER_FIRESABER_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_saberfiresaber_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_SABER_FIRESABER_SHA512, NULL, NULL);
}
static void torture_algorithms_kyber512_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_512_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_kyber512_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_KYBER_512_SHA256, NULL, NULL);
}
static void torture_algorithms_kyber768_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_768_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_kyber768_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_KYBER_768_SHA384, NULL, NULL);
}
static void torture_algorithms_kyber1024_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_1024_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_kyber1024_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_KYBER_1024_SHA512, NULL, NULL);
}
static void torture_algorithms_kyber51290s_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_512_90S_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_kyber51290s_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_KYBER_512_90S_SHA256, NULL, NULL);
}
static void torture_algorithms_kyber76890s_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_768_90S_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_kyber76890s_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_KYBER_768_90S_SHA384, NULL, NULL);
}
static void torture_algorithms_kyber102490s_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_KYBER_1024_90S_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_kyber102490s_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_KYBER_1024_90S_SHA512, NULL, NULL);
}
static void torture_algorithms_bikel1_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_BIKE_L1_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_bikel1_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_BIKE_L1_SHA512, NULL, NULL);
}
static void torture_algorithms_bikel3_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_BIKE_L3_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_bikel3_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_BIKE_L3_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhps2048509_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HPS2048509_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_ntruhps2048509_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_NTRU_HPS2048509_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhps2048677_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HPS2048677_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruhps2048677_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRU_HPS2048677_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhps4096821_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HPS4096821_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_ntruhps4096821_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_NTRU_HPS4096821_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhps40961229_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HPS40961229_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_ntruhps40961229_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_NTRU_HPS40961229_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhrss701_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HRSS701_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruhrss701_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRU_HRSS701_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruhrss1373_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRU_HRSS1373_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_ntruhrss1373_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_NTRU_HRSS1373_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece348864_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_348864_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_classicmceliece348864_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_CLASSIC_MCELIECE_348864_SHA256, NULL, NULL);
}
static void torture_algorithms_classicmceliece348864f_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_348864F_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_classicmceliece348864f_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_CLASSIC_MCELIECE_348864F_SHA256, NULL, NULL);
}
static void torture_algorithms_classicmceliece460896_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_460896_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_classicmceliece460896_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_CLASSIC_MCELIECE_460896_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece460896f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_460896F_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_classicmceliece460896f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_CLASSIC_MCELIECE_460896F_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece6688128_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_6688128_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece6688128_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_6688128_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece6688128f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_6688128F_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece6688128f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_6688128F_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece6960119_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_6960119_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece6960119_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_6960119_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece6960119f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_6960119F_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece6960119f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_6960119F_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece8192128_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_8192128_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece8192128_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_8192128_SHA512, NULL, NULL);
}
static void torture_algorithms_classicmceliece8192128f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_CLASSIC_MCELIECE_8192128F_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_classicmceliece8192128f_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_CLASSIC_MCELIECE_8192128F_SHA512, NULL, NULL);
}
static void torture_algorithms_hqc128_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_HQC_128_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_hqc128_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_HQC_128_SHA256, NULL, NULL);
}
static void torture_algorithms_hqc192_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_HQC_192_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_hqc192_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_HQC_192_SHA384, NULL, NULL);
}
static void torture_algorithms_hqc256_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_HQC_256_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_hqc256_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_HQC_256_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruprimentrulpr653_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_NTRULPR653_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_ntruprimentrulpr653_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_NTRUPRIME_NTRULPR653_SHA256, NULL, NULL);
}
static void torture_algorithms_ntruprimesntrup653_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_SNTRUP653_SHA256, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp256_ntruprimesntrup653_sha256(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP256_NTRUPRIME_SNTRUP653_SHA256, NULL, NULL);
}
static void torture_algorithms_ntruprimentrulpr761_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_NTRULPR761_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruprimentrulpr761_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRUPRIME_NTRULPR761_SHA384, NULL, NULL);
}
static void torture_algorithms_ntruprimesntrup761_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_SNTRUP761_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruprimesntrup761_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRUPRIME_SNTRUP761_SHA384, NULL, NULL);
}
static void torture_algorithms_ntruprimentrulpr857_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_NTRULPR857_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruprimentrulpr857_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRUPRIME_NTRULPR857_SHA384, NULL, NULL);
}
static void torture_algorithms_ntruprimesntrup857_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_SNTRUP857_SHA384, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp384_ntruprimesntrup857_sha384(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP384_NTRUPRIME_SNTRUP857_SHA384, NULL, NULL);
}
static void torture_algorithms_ntruprimentrulpr1277_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_NTRULPR1277_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_ntruprimentrulpr1277_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_NTRUPRIME_NTRULPR1277_SHA512, NULL, NULL);
}
static void torture_algorithms_ntruprimesntrup1277_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_NTRUPRIME_SNTRUP1277_SHA512, NULL, NULL);
}
static void torture_algorithms_ecdh_nistp521_ntruprimesntrup1277_sha512(void** state)
{
    struct torture_state* s = *state;

    if (ssh_fips_mode()) {
        skip();
    }

    test_algorithm(s->ssh.session, KEX_ECDH_NISTP521_NTRUPRIME_SNTRUP1277_SHA512, NULL, NULL);
}
///// OQS_TEMPLATE_FRAGMENT_OQS_FUNCS_END
#endif /* WITH_POST_QUANTUM_CRYPTO */

int torture_run_tests(void) {
    int rc;
    struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_cbc_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_cbc_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_cbc_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_ctr_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes192_ctr_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_ctr_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_gcm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_gcm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_3des_cbc_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
#if defined(WITH_BLOWFISH_CIPHER) && defined(OPENSSH_BLOWFISH_CBC)
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha2_256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha2_512,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha1_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha2_256_etm,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_blowfish_cbc_hmac_sha2_512_etm,
                                        session_setup,
                                        session_teardown),
#endif /* WITH_BLOWFISH_CIPHER */
#ifdef OPENSSH_CHACHA20_POLY1305_OPENSSH_COM
        cmocka_unit_test_setup_teardown(torture_algorithms_chacha20_poly1305,
                                        session_setup,
                                        session_teardown),
#endif /* OPENSSH_CHACHA20_POLY1305_OPENSSH_COM */
        cmocka_unit_test_setup_teardown(torture_algorithms_zlib,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_zlib_openssh,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_group1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_group14,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_group14_sha256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_group16,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_group18,
                                        session_setup,
                                        session_teardown),
#ifdef WITH_GEX
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_gex_sha1,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_dh_gex_sha256,
                                        session_setup,
                                        session_teardown),
#endif /* WITH_GEX */
#ifdef OPENSSH_CURVE25519_SHA256
        cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_curve25519_sha256,
                                        session_setup,
                                        session_teardown),
#endif /* OPENSSH_CURVE25519_SHA256 */
#ifdef OPENSSH_CURVE25519_SHA256_LIBSSH_ORG
        cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_curve25519_sha256_libssh_org,
                                        session_setup,
                                        session_teardown),
#endif /* OPENSSH_CURVE25519_SHA256_LIBSSH_ORG */
#if defined(HAVE_ECC)
        cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_sha2_nistp256,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_sha2_nistp384,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_sha2_nistp521,
                                        session_setup,
                                        session_teardown),
#endif
#ifdef WITH_POST_QUANTUM_CRYPTO
///// OQS_TEMPLATE_FRAGMENT_OQS_CASES_START
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem640aes_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_frodokem640aes_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem976aes_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_frodokem976aes_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem1344aes_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_frodokem1344aes_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem640shake_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_frodokem640shake_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem976shake_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_frodokem976shake_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_frodokem1344shake_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_frodokem1344shake_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_saberlightsaber_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_saberlightsaber_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_sabersaber_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_sabersaber_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_saberfiresaber_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_saberfiresaber_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber512_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_kyber512_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber768_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_kyber768_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber1024_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_kyber1024_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber51290s_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_kyber51290s_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber76890s_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_kyber76890s_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_kyber102490s_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_kyber102490s_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_bikel1_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_bikel1_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_bikel3_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_bikel3_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhps2048509_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_ntruhps2048509_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhps2048677_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruhps2048677_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhps4096821_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_ntruhps4096821_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhps40961229_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_ntruhps40961229_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhrss701_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruhrss701_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruhrss1373_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_ntruhrss1373_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece348864_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_classicmceliece348864_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece348864f_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_classicmceliece348864f_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece460896_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_classicmceliece460896_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece460896f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_classicmceliece460896f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece6688128_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece6688128_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece6688128f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece6688128f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece6960119_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece6960119_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece6960119f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece6960119f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece8192128_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece8192128_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_classicmceliece8192128f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_classicmceliece8192128f_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_hqc128_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_hqc128_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_hqc192_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_hqc192_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_hqc256_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_hqc256_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimentrulpr653_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_ntruprimentrulpr653_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimesntrup653_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp256_ntruprimesntrup653_sha256,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimentrulpr761_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruprimentrulpr761_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimesntrup761_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruprimesntrup761_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimentrulpr857_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruprimentrulpr857_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimesntrup857_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp384_ntruprimesntrup857_sha384,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimentrulpr1277_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_ntruprimentrulpr1277_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ntruprimesntrup1277_sha512,
                                         session_setup,
                                         session_teardown),
         cmocka_unit_test_setup_teardown(torture_algorithms_ecdh_nistp521_ntruprimesntrup1277_sha512,
                                         session_setup,
                                         session_teardown),
///// OQS_TEMPLATE_FRAGMENT_OQS_CASES_END
#endif
    };

    struct CMUnitTest tests_hmac[] = {
        cmocka_unit_test_setup_teardown(torture_algorithms_aes128_gcm_mac,
                                        session_setup,
                                        session_teardown),
        cmocka_unit_test_setup_teardown(torture_algorithms_aes256_gcm_mac,
                                        session_setup,
                                        session_teardown),
#ifdef OPENSSH_CHACHA20_POLY1305_OPENSSH_COM
        cmocka_unit_test_setup_teardown(torture_algorithms_chacha20_poly1305_mac,
                                        session_setup,
                                        session_teardown),
#endif /* OPENSSH_CHACHA20_POLY1305_OPENSSH_COM */
    };

    ssh_init();

    torture_filter_tests(tests);
    rc = cmocka_run_group_tests(tests, sshd_setup, sshd_teardown);
    if (rc != 0) {
        return rc;
    }

    torture_filter_tests(tests);
    rc = cmocka_run_group_tests(tests_hmac, sshd_setup_hmac, sshd_teardown);

    ssh_finalize();

    return rc;
}
