#include <iostream>
#include "base64.h"
#include "bsearch.h"
using namespace std;

int main() {
  Base64::runTests();
  BSearch::runTests();
  Global::pauseForKey();
  return 0;
}
