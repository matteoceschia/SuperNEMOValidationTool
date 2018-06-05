
// Standard Library
#include <iostream>
#include <fstream>
#include "boost/algorithm/string.hpp"

// ROOT
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TVector3.h"
#include "TDictionary.h"
#include "TBranch.h"


using namespace std;

string treeName="Validation";


int main(int argc, char **argv);
void ParseRootFile(string rootFileName, string configFileName="");
bool PlotVariable(string branchName);
map<string,string> LoadConfig(ifstream& configFile);
string GetBitBeforeComma(string& input);
void Plot1DHistogram(string branchName);
string BranchNameToEnglish(string branchname);