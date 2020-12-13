#include <string>
#include <iostream>
#include <stdio.h>
#include "../libsam3/libsam3.h"

int main() {
	// The session is only usef for transporting the data
	Sam3Session ss;

	if (0 > sam3GenerateKeys(&ss, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, Sam3SigType::EdDSA_SHA512_Ed25519)) {
		printf("got error");
		return -1;
	}
	auto pub = std::string(ss.pubkey);
	auto priv = std::string(ss.privkey);

	std::cout << "pub  " << pub << std::endl << "priv " << priv << std::endl;
	return 0;
} 
