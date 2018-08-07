
// Standard Library
#include <iostream>
#include <fstream>
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

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
#include "TLegend.h"
#include "TPaveText.h"
#include "TLatex.h"


using namespace std;

string treeName="Validation";
int REF_FILL_COLOR=kRed-10;
int REF_LINE_COLOR=kRed;
int REF_FILL_STYLE=1001;


int main(int argc, char **argv);
void ParseRootFile(string rootFileName, string configFileName="", string refFileName="");
bool PlotVariable(string branchName);
map<string,string> LoadConfig(ifstream& configFile);
string GetBitBeforeComma(string& input);
void Plot1DHistogram(string branchName);
void PlotTrackerMap(string branchName);
void PlotCaloMap(string branchName);
string BranchNameToEnglish(string branchname);
void WriteLabel(double x, double y, string text, double size=0.05);
void PrintCaloPlots(string branchName, string title, TH2* hItaly,TH2* hFrance,TH2* hTunnel,TH2* hMountain,TH2* hTop,TH2* hBottom);
string exec(const char* cmd);
string FirstWordOf(string input);
