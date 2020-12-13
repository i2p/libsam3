//#include <string>
//#include <iostream>
#include "../libsam3/libsam3.h"
#include <stdio.h>

int main() {
  // The session is only usef for transporting the data
  Sam3Session ss;

  if (0 > sam3GenerateKeys(&ss, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, 4)) {
    printf("got error");
    return -1;
  }
  printf("\tpubkey: %s \n \tprivkey: %s", ss.pubkey, ss.privkey);
  /*auto pub = std::string(ss.pubkey);
  auto priv = std::string(ss.privkey);

  std::cout << "pub  " << pub << std::endl << "priv " << priv << std::endl;*/
  return 0;
}
