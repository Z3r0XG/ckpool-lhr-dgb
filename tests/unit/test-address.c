/*
 * Unit tests for DigiByte address encoding
 * Tests Base58 and Bech32 decoding through address_to_txn()
 *
 * DGB address formats:
 *   P2PKH mainnet:  'D...'   version byte 0x1E (30)
 *   P2SH  mainnet:  'S...'   version byte 0x3F (63)
 *   P2WPKH mainnet: 'dgb1q...'   bech32, HRP "dgb"
 *   P2WPKH testnet: 'dgbt1q...'  bech32, HRP "dgbt"
 *   P2WPKH regtest: 'dgbrt1q...' bech32, HRP "dgbrt"
 *
 * Note: address_to_txn() does not validate the address via RPC; it only
 * decodes the encoding (Base58 or bech32) and builds the output script.
 * Full address validation happens via digibyted validateaddress RPC in
 * bitcoin.c:validate_address().
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../test_common.h"
#include "libckpool.h"

/*
 * P2PKH output script template (25 bytes):
 *   OP_DUP OP_HASH160 OP_PUSH20 <hash160[20]> OP_EQUALVERIFY OP_CHECKSIG
 *   0x76   0xa9       0x14                     0x88            0xac
 *
 * address_to_txn with script=false,segwit=false calls address_to_pubkeytxn()
 * which decodes the Base58Check address via b58tobin() and constructs this
 * script, placing b58bin[1..20] as the hash160 (skipping the version byte).
 * The version byte (0x1E for DGB mainnet) is NOT written into the script.
 */
static void test_p2pkh_script_opcodes(void)
{
	char txn[100];
	int len;

	/* Use a valid-format Base58Check address. b58tobin is coin-agnostic;
	 * the version byte (0x00 for this BTC address) is stripped and the
	 * remaining 20 bytes become the hash160. The opcode template is the
	 * same for all P2PKH addresses regardless of coin. */
	const char *addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
	len = address_to_txn(txn, addr, false, false);

	assert_true(len == 25);

	/* Verify the P2PKH script template byte-by-byte */
	assert_true((unsigned char)txn[0] == 0x76); /* OP_DUP */
	assert_true((unsigned char)txn[1] == 0xa9); /* OP_HASH160 */
	assert_true((unsigned char)txn[2] == 0x14); /* push 20 bytes */
	assert_true((unsigned char)txn[23] == 0x88); /* OP_EQUALVERIFY */
	assert_true((unsigned char)txn[24] == 0xac); /* OP_CHECKSIG */
}

/*
 * P2SH output script template (23 bytes):
 *   OP_HASH160 OP_PUSH20 <hash160[20]> OP_EQUAL
 *   0xa9       0x14                    0x87
 *
 * DGB P2SH mainnet addresses start with 'S' (version byte 0x3F).
 */
static void test_p2sh_script_opcodes(void)
{
	char txn[100];
	int len;

	const char *addr = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy";
	len = address_to_txn(txn, addr, true, false);

	assert_true(len == 23);

	/* Verify the P2SH script template byte-by-byte */
	assert_true((unsigned char)txn[0] == 0xa9); /* OP_HASH160 */
	assert_true((unsigned char)txn[1] == 0x14); /* push 20 bytes */
	assert_true((unsigned char)txn[22] == 0x87); /* OP_EQUAL */
}

/*
 * DGB bech32 P2WPKH addresses (version 0, 20-byte witness program).
 *
 * Output script (22 bytes):
 *   witness_version  push_len  <witness_program[20]>
 *   0x00             0x14
 *
 * bech32_decode() in libckpool.c is HRP-agnostic: it finds the last '1'
 * separator and never checks the HRP string. So "dgb1q...", "dgbt1q...",
 * and "dgbrt1q..." all decode identically to the same output script given
 * the same witness program bytes.
 *
 * Test addresses are the hardcoded donation addresses from ckpool.c:
 *   mainnet  dgb1q6tf0myda7plmpksdqc8k4tf8q957z0fm0y9a5m
 *   testnet  dgbt1qysts53eu2y6et25a8ap7lr03muyw2czk3sezhx
 *   regtest  dgbrt1q73s4v2mgt9mmcd5kum3d2jzvfuy297are8yv7l
 */
static void test_dgb_bech32_mainnet_p2wpkh(void)
{
	char txn[100];
	int len;

	/* Mainnet donation address (HRP "dgb") */
	const char *addr = "dgb1q6tf0myda7plmpksdqc8k4tf8q957z0fm0y9a5m";
	len = address_to_txn(txn, addr, false, true);

	/* P2WPKH: 2 header bytes + 20 witness bytes = 22 */
	assert_true(len == 22);
	assert_true((unsigned char)txn[0] == 0x00); /* witness version 0 */
	assert_true((unsigned char)txn[1] == 0x14); /* push 20 bytes */
}

static void test_dgb_bech32_testnet_p2wpkh(void)
{
	char txn[100];
	int len;

	/* Testnet donation address (HRP "dgbt") */
	const char *addr = "dgbt1qysts53eu2y6et25a8ap7lr03muyw2czk3sezhx";
	len = address_to_txn(txn, addr, false, true);

	assert_true(len == 22);
	assert_true((unsigned char)txn[0] == 0x00); /* witness version 0 */
	assert_true((unsigned char)txn[1] == 0x14); /* push 20 bytes */
}

static void test_dgb_bech32_regtest_p2wpkh(void)
{
	char txn[100];
	int len;

	/* Regtest donation address (HRP "dgbrt") */
	const char *addr = "dgbrt1q73s4v2mgt9mmcd5kum3d2jzvfuy297are8yv7l";
	len = address_to_txn(txn, addr, false, true);

	assert_true(len == 22);
	assert_true((unsigned char)txn[0] == 0x00); /* witness version 0 */
	assert_true((unsigned char)txn[1] == 0x14); /* push 20 bytes */
}

/*
 * The three DGB bech32 donation addresses (mainnet/testnet/regtest) each
 * encode a different 20-byte witness program, so their output scripts must
 * differ in bytes 2..21 even though the header bytes are the same.
 */
static void test_dgb_bech32_addresses_differ(void)
{
	char txn_main[100], txn_test[100], txn_regt[100];
	int len_main, len_test, len_regt;

	len_main = address_to_txn(txn_main, "dgb1q6tf0myda7plmpksdqc8k4tf8q957z0fm0y9a5m",  false, true);
	len_test = address_to_txn(txn_test, "dgbt1qysts53eu2y6et25a8ap7lr03muyw2czk3sezhx",  false, true);
	len_regt = address_to_txn(txn_regt, "dgbrt1q73s4v2mgt9mmcd5kum3d2jzvfuy297are8yv7l", false, true);

	assert_true(len_main == 22);
	assert_true(len_test == 22);
	assert_true(len_regt == 22);

	/* Witness programs (bytes 2..21) must all be distinct */
	assert_true(memcmp(txn_main + 2, txn_test + 2, 20) != 0);
	assert_true(memcmp(txn_main + 2, txn_regt + 2, 20) != 0);
	assert_true(memcmp(txn_test + 2, txn_regt + 2, 20) != 0);
}

/*
 * address_to_txn() routing: same input, different flags → different outputs.
 */
static void test_address_type_routing(void)
{
	char txn[100];
	int len_p2pkh, len_p2sh;
	const char *addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";

	len_p2pkh = address_to_txn(txn, addr, false, false);
	assert_true(len_p2pkh == 25);

	len_p2sh = address_to_txn(txn, addr, true, false);
	assert_true(len_p2sh == 23);
}

int main(void)
{
	printf("Running DigiByte address encoding tests...\n\n");

	run_test(test_p2pkh_script_opcodes);
	run_test(test_p2sh_script_opcodes);
	run_test(test_dgb_bech32_mainnet_p2wpkh);
	run_test(test_dgb_bech32_testnet_p2wpkh);
	run_test(test_dgb_bech32_regtest_p2wpkh);
	run_test(test_dgb_bech32_addresses_differ);
	run_test(test_address_type_routing);

	printf("\nAll DigiByte address encoding tests passed!\n");
	return 0;
}

