
// Standard Library
# include <iostream>

// ROOT
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TVector3.h"


using namespace std;

string treeName="Validation";


int main(int argc, char **argv);
void ParseRootFile(string rootFileName, string configFileName="");
