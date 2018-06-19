
// Standard Library
#include <iostream>
#include <fstream>
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"

// ROOT
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TText.h"
#include "TROOT.h"
#include "TStyle.h"
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
void PlotTrackerMap(string branchName);
void PlotCaloMap(string branchName);
string BranchNameToEnglish(string branchname);
void WriteLabel(double x, double y, string text, double size=0.05);
void PrintCaloPlots(string branchName, string title, TH2* hItaly,TH2* hFrance,TH2* hTunnel,TH2* hMountain,TH2* hTop,TH2* hBottom);
