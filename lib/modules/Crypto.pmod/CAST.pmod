#pike __REAL_VERSION__

//! CAST-128 is a block cipher, specified in RFC 2144. It uses a 64
//! bit (8 octets) block size, and a variable key size of up to 128
//! bits.

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.CAST128_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.CAST128_State(); }
