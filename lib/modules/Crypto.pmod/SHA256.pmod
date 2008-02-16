#pike __REAL_VERSION__

//! SHA256 is another hash function specified by NIST, intended as a
//! replacement for @[SHA], generating larger digests. It outputs hash
//! values of 256 bits, or 32 octets.

#if constant(Nettle) && constant(Nettle.SHA256_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA256_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA256_State(); }

string asn1_id() { return "`\206H\1e\3\4\0\1"; }

#else
constant this_program_does_not_exist=1;
#endif
