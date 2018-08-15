
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
int PULL_PALETTE=kThermometer;

// For 2-D comparisons - report if the magnitude of the pull
// (difference between sample and reference) for a cell
// is more than this many sigma
double REPORT_PULLS_OVER=3.;

// Calorimeter dimensions

int MAINWALL_WIDTH = 20;
int MAINWALL_HEIGHT = 13;
int XWALL_DEPTH = 4;
int XWALL_HEIGHT = 16;
int VETO_DEPTH = 2;
int VETO_WIDTH = 16;

// 6 walls for the calorimeters, the order matters
enum WALL  {ITALY, FRANCE, TUNNEL, MOUNTAIN, TOP, BOTTOM};
string CALO_WALL[6] = {"Italy","France","Tunnel","Mountain","Top","Bottom"};
int CALO_XBINS[6] = {MAINWALL_WIDTH,MAINWALL_WIDTH,XWALL_DEPTH,XWALL_DEPTH,VETO_WIDTH,VETO_WIDTH};
int CALO_XLO[6] = {-1*MAINWALL_WIDTH,0,-1 * XWALL_DEPTH/2,-1 * XWALL_DEPTH/2,0,0};
int CALO_XHI[6] = {0,MAINWALL_WIDTH,XWALL_DEPTH/2,XWALL_DEPTH/2,VETO_WIDTH,VETO_WIDTH};
int CALO_YBINS[6] = {MAINWALL_HEIGHT,MAINWALL_HEIGHT,XWALL_HEIGHT,XWALL_HEIGHT,VETO_DEPTH,VETO_DEPTH}; // They are all zero to nbins in the y direction

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
vector<TH2D*>MakeCaloPullPlots(vector<TH2D*> vSample, vector<TH2D*> vRef);
double CheckCaloPulls(vector<TH2D*> hPulls);
void OverlayWhiteForNaN(TH2D *hist);
