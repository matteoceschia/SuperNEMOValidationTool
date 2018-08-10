
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

// Colour palettes for 1-D histogram comparisons
int REF_FILL_COLOR=kRed-10;
int REF_LINE_COLOR=kRed;
int REF_FILL_STYLE=1001;

// Tracker geometry
int MAX_TRACKER_LAYERS=9;
int MAX_TRACKER_ROWS=113;

// Palettes for general plots and for pull plots
// (where we want different colours for positive and negative values)
int PALETTE = kBird;
int PULL_PALETTE=kRedBlue;

// For 2-D comparisons - report if the magnitude of the pull
// (difference between sample and reference) for a cell
// is more than this many sigma
double REPORT_PULLS_OVER=3.;

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
void PrintCaloPlots(string branchName, string title, vector <TH2D*> histos);

string exec(const char* cmd);
string FirstWordOf(string input);
TH2D *TrackerMapHistogram(string fullBranchName, string branchName, string title, bool isRef, bool isAverage, string mapBranch = "");
TH2D *PullPlot2D(TH2D *hSample, TH2D *hRef);
void AnnotateTrackerMap();
double CheckTrackerPull(TH2D *hPull);
vector<TH2D*> MakeCaloPlotSet(string fullBranchName, string branchName, string title, bool isRef, bool isAverage, string mapBranch = "");
