#include "ValidationParser.h"

// Global variables
bool hasConfig=true;
bool hasValidReference = true;
TTree *tree;
TTree *reftree;
map<string,string> configParams;
string plotdir;
ofstream textOut;

/**
 *  main function
 * Arguments are <root file> <config file (optional)>
 */
int main(int argc, char **argv)
{
  gStyle->SetOptStat(0);
  gStyle->SetPalette(PALETTE);
  gErrorIgnoreLevel = kWarning;
  if (argc < 2)
  {
    cout<<"Usage: "<<argv[0]<<" -i <data ROOT file> -r <reference ROOT file (optional)> -c <config file (optional)> -o <output directory (optional)> -t <temp directory (optional)>"<<endl;
    return -1;
  }
  // This bit is kept for compatibility with old version that would take just a root file name and a config file name
  string dataFileInput="";
  string referenceFileInput="";
  string configFileInput="";
  string tempDirInput="";
  string plotDirInput="";
  if (argc == 2 && argv[1][0]!= '-')
  {
    dataFileInput = argv[1];
  }
  else if (argc == 3 && argv[1][0]!= '-')
  {
    dataFileInput = argv[1];
    configFileInput = (argv[2]);
  }
  else
  {
    int flag=0;
    while ((flag = getopt (argc, argv, "i:r:c:t:o:")) != -1)
    {
      switch (flag)
      {
        case 'i':
          dataFileInput = optarg;
          break;
        case 'r':
          referenceFileInput = optarg;
          break;
        case 'c':
          configFileInput = optarg;
          break;
        case 'o':
          plotDirInput = optarg;
          break;
        case 't':
          tempDirInput = optarg;
          break;
        case '?':
          if (optopt == 'i' || optopt == 'r' || optopt == 'c' || optopt == 't' || optopt == 'o' )
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
            fprintf (stderr,
                     "Unknown option character `\\x%x'.\n",
                     optopt);
          cout<<"Usage: "<<argv[0]<<" -i <data ROOT file> -r <reference ROOT file (optional)> -c <config file (optional)> -o <output directory (optional)> -t <temp directory (optional)>"<<endl;
          return 1;
        default:
          abort ();
      }
    }
  }

  if (dataFileInput.length()<=0)
  {
    cout<<"ERROR: Data file name is needed."<<endl;
    cout<<"Usage: "<<argv[0]<<" -i <data ROOT file> -r <reference ROOT file (optional)> -c <config file (optional)>"<<endl;
    return -1;
  }
  ParseRootFile(dataFileInput,configFileInput,referenceFileInput,tempDirInput,plotDirInput);
  return 0;
}


/**
 *  Main work function - parses a ROOT file and plots the variables in the branches
 *  rootFileName: path to the ROOT file with SuperNEMO validation data
 *  configFileName: optional to specify how to plot certain variables
 */
void ParseRootFile(string rootFileName, string configFileName, string refFileName, string tempDirName, string plotDirName)
{

  // Check the input root file can be opened and contains a tree with the right name
  cout<<"Processing "<<rootFileName<<endl;
  TFile *rootFile;

  rootFile = new TFile(rootFileName.c_str());
  if (rootFile->IsZombie())
  {
    cout<<"Error: file "<<rootFileName<<" not found"<<endl;
    return ;
  }
  tree = (TTree*) rootFile->Get(treeName.c_str()); // Name is in the .h file for now
  // Check if it found the tree
  if (tree==0)
    {
      cout<<"Error: no data in a tree named "<<treeName<<endl;
      return;
    }

  // Check if we have a config file
  ifstream configFile (configFileName.c_str());
  if (configFileName.length()==0 ) // No config file given
  {
    cout<<"No config file provided - using default settings"<<endl;
    hasConfig=false;
  }
  else if  (!configFile) // file name given but file not found
  {
    cout<<"WARNING: Config file "<<configFileName<<" not found - using default settings"<<endl;
    hasConfig=false;
  }
  else
  {
    cout<<"Using config file "<<configFileName<<endl;
    configParams=LoadConfig(configFile);
  }
  
  // Check for a reference file
  
  TFile *refFile;
  if (refFileName.length() > 0)
  {
    
    refFile = new TFile(refFileName.c_str());
    if (refFile->IsZombie())
    {
      cout<<"WARNING: No valid reference ROOT file given. To generate comparison plots, provide a valid reference ROOT file.";
      if (refFileName.length()>0) cout<<" Bad ROOT file: "<<refFileName;
      cout <<endl;
      hasValidReference = false;
    }
  }
  else
  {
    cout<<"WARNING: No reference ROOT file given. To generate comparison plots, provide a valid reference ROOT file."<<endl;
    hasValidReference = false;
  }

  if (hasValidReference)
  {
    reftree = (TTree*) refFile->Get(treeName.c_str()); // Name is in the .h file for now
    // Check if it found the tree
    if (reftree==0)
    {
      cout<<"WARNING: no reference data in a tree named "<<treeName<<" found in "<<refFileName<<". To generate comparison plots, provide a valid reference ROOT file."<<endl;
      hasValidReference = false;
    }
  }

  // Make a directory to put the plots in
  if (plotDirName.length() > 0)
  {
    plotdir=plotDirName;
  }
  else
  {
    string rootFileNameNoPath=rootFileName;
    try{
      rootFileNameNoPath=rootFileName.substr(rootFileName.find_last_of("/")+1);
    }
    catch (exception e)
    {
      rootFileNameNoPath=rootFileName;
    }
    plotdir = "plots_"+rootFileNameNoPath.substr(0,rootFileNameNoPath.length()-5);
  }

  boost::filesystem::path dir(plotdir.c_str());
  if(boost::filesystem::create_directories(dir))
  {
    cout<< "Directory Created: "<<plotdir<<std::endl;
  }
  
  // If there is a temp directory specified, make that too
  if (tempDirName.length() > 0)
  {
    boost::filesystem::path dir(tempDirName.c_str());
    if(boost::filesystem::create_directories(dir))
    {
      cout<< "Directory Created: "<<tempDirName<<std::endl;
    }
  }
  else tempDirName=plotdir; // If no temp directory is specified, we will use the output directory for temp files
  
  // In the plots directory, make an output ROOT file for the histograms
  TFile *outputFile=new TFile((tempDirName+"/TempHistograms.root").c_str(),"RECREATE");
  outputFile->cd();
  
  if (hasValidReference)
  {
    // Open the output text file
    textOut.open((plotdir+"/ValidationResults.txt").c_str());
    textOut<<"Sample: "<<rootFileName<<" ("<<tree->GetEntries() <<" entries)"<<endl;
    textOut<<"SHA-256 hash: "<<FirstWordOf(exec(("shasum -a 256 "+rootFileName).c_str()))<<endl;
    textOut<<"Compared with "<<refFileName<<" ("<<reftree->GetEntries() <<" entries)"<<endl;
    textOut<<"SHA-256 hash: "<<FirstWordOf(exec(("shasum -a 256 "+refFileName).c_str()))<<endl;
    textOut<<endl;
    
  }

  // Get a list of all the branches in the main tree
  TObjArray* branches = tree->GetListOfBranches();
  TIter next(branches);
  TBranch *branch;
  
  // Loop the branches and decide how to treat them based on the first character of the name
  while( (branch=(TBranch *)next() )){
    string branchName=branch->GetName();
    PlotVariable(branchName);
  }
  
  if (configFile.is_open()) configFile.close();
  outputFile->Close();
  if (textOut.is_open())  textOut.close();
  
  // Fix ROOT problem by copying all histograms from the output file to another file
  // If your input ntuple files are too big, ROOT will write them to the output file
  // We don't want that, but we can't avoid it so instead, we will copy the good
  // stuff to a new file and then delete the old stuff. Sigh.
  MoveHistograms(tempDirName+"/TempHistograms.root" , plotdir+"/ValidationHistograms.root");
  return;
}

// Hack to move all the histograms from one ROOT file to another, while not moving
// any TTrees. Then delete the old file. It fixes a "feature" of ROOT which means that it will
// write the input tuples to the output file if they are too big to hold in memory.
// Not ideal.
void MoveHistograms(string fromFile, string toFile)
{
  TFile *fIn = new TFile(fromFile.c_str());
  TFile *fOut = new TFile(toFile.c_str(),"RECREATE");
  // Loop through the input file and if it's a histogram, write it to the output file
  TList* list = fIn->GetListOfKeys() ;
  TIter next(list) ;
  TKey* key ;
  TObject* obj ;
  
  while (( key = (TKey*)next() )) {
    obj = key->ReadObj() ;
    if ( obj->InheritsFrom("TH2") || obj->InheritsFrom("TH1"))
    {
      obj->Write("",TObject::kOverwrite);
    }
  }
  fOut->cd();
  fOut->Close();
  fIn->Close();
  remove(fromFile.c_str());
}

/**
 *  Decides what plot to make for a branch
 *  depending on the prefix
 */
bool PlotVariable(string branchName)
{
  string config="";
  bool configFound=false;
  // If we have a config file, check if this var is in it
  if (hasConfig)
  {
    config = configParams[branchName];
    if (config.length() > 0)
    {
      configFound=true;
    }
  }
  cout<<"Plotting "<<branchName<<":"<<endl;
  switch (branchName[0])
  {
    case 'h':
    {
      Plot1DHistogram(branchName);
      break;
    }
    case 't':
    {
      PlotTrackerMap(branchName);
      break;
    }
    case 'c':
    {
      PlotCaloMap(branchName);
      break;
    }
    default:
    {
      cout<< "Unknown variable type "<<branchName<<": ignoring this branch"<<endl;
      break;
    }
  }

  return true;
}


/**
 *  Loads the config file (which should be short) into a memory
 *  map where the key is the branch name (first item in the CSV line)
 *  value is the rest of the line
 */
map<string,string> LoadConfig(ifstream& configFile)
{
  map<string,string> configLookup;
  string thisLine;
  
  while (configFile)
  {
    getline(configFile, thisLine);
    string key=GetBitBeforeComma(thisLine);
    configLookup[key]=thisLine; // the remainder of the line
  }
  return configLookup;
}

/**
 *  Make a basic histogram of a variable. The number of bins etc will come from
 *  the config file if there is one, if not we will guess
 */
void Plot1DHistogram(string branchName)
{
  int notSetVal=-9999;
  string config="";
  config=configParams[branchName]; // get the config loaded from the file if there is one
  int nbins=100;
  double lowLimit=0;
  double highLimit=notSetVal;
  string title="";
  bool hasReferenceBranch=hasValidReference;
  
  // Check whether the reference file contains this branch
  if (hasValidReference)
  {
    hasReferenceBranch=reftree->GetBranchStatus(branchName.c_str());
    if (!hasReferenceBranch) cout<<"WARNING: branch "<<branchName<<" not found in reference file. No comparison plots will be made for this branch"<<endl;
  }
  
  // Read the config information
  if (config.length()>0)
  {
    // title, nbins, low limit, high limit separated by commas
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage

    // Number of bins
    try
    {
      string nbinString=GetBitBeforeComma(config);
      std::string::size_type sz;   // alias of size_t
      nbins = std::stoi (nbinString,&sz); // hopefully the next chunk is turnable into an integer
    }
    catch (exception &e)
    {
      nbins=0;
    }
    
    // Low bin limit
    try
    {
      string lowString=GetBitBeforeComma(config);
      lowLimit = std::stod (lowString); // hopefully the next chunk is turnable into an double
    }
    catch (exception &e)
    {
      lowLimit=0;
    }
    
    // High bin limit
    try
    {
      string highString=GetBitBeforeComma(config);
      highLimit = std::stod (highString); // hopefully the next chunk is turnable into an double
    }
    catch (exception &e)
    {
      highLimit=notSetVal;
    }
  }

  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }
  TCanvas *c = new TCanvas (("plot_"+branchName).c_str(),("plot_"+branchName).c_str(),900,600);
  TH1D *h;
  tree->Draw(branchName.c_str());
  
  if (highLimit == notSetVal)
  {
    // Use the default limits
    TH1D *tmp = (TH1D*) gPad->GetPrimitive("htemp");
    highLimit=tmp->GetXaxis()->GetXmax();
    delete tmp;
    EDataType datatype;
    TClass *ctmp;
    tree->FindBranch(branchName.c_str())->GetExpectedType(ctmp,datatype);
    delete ctmp;
    if (datatype==kBool_t)
    {
      nbins=2;
      highLimit=2;
      lowLimit=0;
    }
    else if (datatype== kInt_t || datatype== kUInt_t)
    {
      
      highLimit +=1;
      if (highLimit <=100) nbins = (int) highLimit;
      else nbins = 100;
    }
    else
    {
      highLimit += highLimit /10.;
      nbins = 100;
    }
  }
  h = new TH1D(("plt_"+branchName).c_str(),title.c_str(),nbins,lowLimit,highLimit);
  if( h->GetSumw2N() == 0 )h->Sumw2();
  h->GetYaxis()->SetTitle("Events");
  h->GetXaxis()->SetTitle(title.c_str());
  h->SetFillColor(kPink-6);
  h->SetFillStyle(1001);
  tree->Draw((branchName + ">> plt_"+branchName).c_str());
  h->Write("",TObject::kOverwrite);
  h->Draw("HIST");
  
  h->Draw("E SAME");
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  if (hasReferenceBranch)
  {
    TCanvas  *comp_canv= new TCanvas(("compare_"+branchName).c_str(),("compare_"+branchName).c_str(),900,900);
    TPad *p_comp = new TPad("p_comp",
                            "",0.0,0.4,1,1,0);
    
    TPad *p_ratio = new TPad("p_ratio",
                            "",0.0,0.0,1,0.4,0);
    p_comp->Draw();
    p_ratio->Draw();
    
    p_comp->cd();
    // Make the reference plot with the same binning
    TH1D *href = new TH1D(("ref_"+branchName).c_str(),title.c_str(),nbins,lowLimit,highLimit);
    if( href->GetSumw2N() == 0 )href->Sumw2();
    reftree->Draw((branchName + ">> ref_"+branchName).c_str());
    
    // Normalise reference number of events to data
    Double_t scale = (double)tree->GetEntries()/(double)reftree->GetEntries();
    href->Scale(scale);
    
    // Save a plot with both on the same axes
    double maxy = h->GetMaximum()>href->GetMaximum()?h->GetMaximum()*1.1:href->GetMaximum()*1.1;
    h->GetYaxis()->SetRangeUser(0,maxy);

    // Draw the sample to get the right axis
    h->SetLineColor(kBlack);
    h->SetMarkerStyle(20);
    h->SetMarkerSize(.5);
    h->SetMarkerColor(kBlack);
    h->SetLineWidth(1);
    h->SetLineStyle(1);
    h->Draw("E");
    
    // Draw the error bars on the reference
    href->SetFillColor(REF_FILL_COLOR);
    href->SetFillStyle(REF_FILL_STYLE);
    href->SetLineColor(REF_LINE_COLOR);
    href->SetMarkerStyle(0);
    href->DrawCopy("E2 SAME");
    // Then the reference central value
    href->SetFillColor(0);
    href->DrawCopy("HIST SAME");
    // Finally redraw the sample so it is on top
    h->DrawCopy("E1 X0 SAME");


    // Add a legend
    TLegend* legend = new TLegend(0.75,0.8,0.9,0.9);
    href->SetFillColor(REF_FILL_COLOR); // change it back so it is included in the legend
    legend->AddEntry(h, "Sample", "lep");
    legend->AddEntry(href,"Reference", "fl");
    legend->Draw();
    
    // Calculate some stats
    // Kolmogorov-Smirnov goodness of fit
    Double_t ks = h->KolmogorovTest(href);
    Double_t chisq;
    Int_t ndf;
    Double_t p_value = ChiSquared(h, href, chisq, ndf, false);
    cout<<"Kolmogorov: "<<ks<<endl;
    cout<<"P-value: "<<p_value<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;
    
    // Write to output file
    textOut<<branchName<<":"<<endl;
    textOut<<"KS score: "<<ks<<endl;
    textOut<<"P-value: "<<p_value<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;


    // Now make a ratio plot

    p_ratio->cd();
    TH1D *ratio_hist = (TH1D*)h->Clone(("ratio_"+branchName).c_str());
    ratio_hist->SetTitle("");
    ratio_hist->GetXaxis()->SetTitle("");
    ratio_hist->Divide(href);
    // Set a more sensible y axis
    ratio_hist->GetYaxis()->SetRangeUser(ratio_hist->GetBinContent(ratio_hist->GetMinimumBin())*0.9,ratio_hist->GetBinContent(ratio_hist->GetMaximumBin())*1.1);
    ratio_hist->SetLineColor(kBlack);
    ratio_hist->GetYaxis()->SetTitle("Ratio to reference");
    ratio_hist->GetYaxis()->SetLabelSize(ratio_hist->GetYaxis()->GetLabelSize() * 1.5);
    ratio_hist->GetYaxis()->SetTitleSize(ratio_hist->GetYaxis()->GetTitleSize() * 1.5);
    ratio_hist->Draw();

    WriteLabel(0.6,0.8, Form("K-S score (binned): %.2f",ks),0.04);
    WriteLabel(0.6,0.72, Form("#chi^{2}/NDF: %.1f/%d = %.1f",chisq,ndf,chisq/(double)ndf),0.04);
    WriteLabel(0.6,0.64, Form("(p-value %.2f)",p_value),0.04);
    
    TLine *line=new TLine(c->GetUxmin(),1.0,c->GetUxmax(),1.0);
    line->SetLineColor(kRed);
    line->Draw();

    comp_canv->SaveAs((plotdir+"/compare_"+branchName+".png").c_str());
    
    textOut<<endl;
    delete href;
    delete ratio_hist;
   // delete c_ratio;
    delete comp_canv;
  }
  delete h;
  delete c;
}


/**
 *  Plot a map of the calorimeter walls
 *  We have 6 walls in total : 2 main walls (Italy, France)
 *  2 x walls (tunnel, mountain) and 2 gamma vetos (top, bottom)
 *  This makes two kinds of map: for c_... variables, it just adds 1 for every listed calo
 *  for cm... variables it needs to use two branches - the cm_ one and its corresponding c_ one
 *  and then it uses the calorimeter locations in the c_ variable to calculate the mean for
 *  each calorimeter, based on the paired numbers in the cm_ variable
 */
void PlotCaloMap(string branchName)
{
  
  string config="";
  
  string fullBranchName=branchName;//This is a combo of name and parent for average branches
  
  // is it an average?
  string mapBranch=branchName;
  
  bool isAverage=false;
  if (branchName[1]=='m')
  {
    // In this case, the branch name should be split in two with a . character
    int pos=branchName.find(".");
    if (pos<=1)
    {
      cout<<"Error - could not find map branch for "<<branchName<<": remember to provide a map branch name with a dot"<<endl;
      return;
    }
    mapBranch=branchName.substr(pos+1);
    branchName=branchName.substr(0,pos);
    isAverage=true;
  }

  config=configParams[branchName]; // get the config loaded from the file if there is one
  
  string title="";
  // Load the title from the config file
  if (config.length()>0)
  {
    // title is the only thing for this one
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage
  }
  // Set the title to a default if there isn't anything in the config file
  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }
  
  vector<TH2D*> hists = MakeCaloPlotSet(fullBranchName, branchName, title, false, isAverage, mapBranch);
  PrintCaloPlots(branchName,title,hists);
  
  // Can we do a comparison to the reference for this plot?
  if (!hasValidReference) return;
  if (!reftree->GetBranchStatus(fullBranchName.c_str()))
  {
    cout<<"WARNING: branch "<<fullBranchName<<" not found in reference file. No comparison plots will be made for this branch"<<endl;
    return;
  }
  if (isAverage && !reftree->GetBranchStatus(mapBranch.c_str()))
  {
    cout<<"WARNING: map branch "<<mapBranch<<" not found in reference file. No comparison plots can be made for the branch "<<branchName<<endl;
    return;
  }
  
  // Compare to reference now that we have checked that we have one.
  vector<TH2D*> refHists = MakeCaloPlotSet(fullBranchName, branchName, title, true, isAverage, mapBranch);

  PrintCaloPlots("ref_"+branchName,title,refHists);
  

  // Calculate some stats
  // Don't know how to make a Kolmogorov calculation for this set of 6, but we can do a chi-square and look at the pull...

  Double_t chisq=0;
  Int_t ndf=0;
  for (int i=0;i<hists.size();i++)
  {
    // Should be able to calculate the individual chi-squares and then sum them, as long as we remember to sum degrees of freedom too
    Double_t thisChisq=0;
    Int_t thisNdf=0;
    ChiSquared(hists.at(i), refHists.at(i), thisChisq, thisNdf, isAverage);
    chisq += thisChisq;
    ndf += thisNdf;
  }
  
  Double_t prob = TMath::Prob(chisq, ndf); // Get it from the combined chi square
  cout<<"P-value: "<<prob<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;
  
  // Write to output file
  textOut<<branchName<<":"<<endl;
  textOut<<"P-value: "<<prob<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;
  
  // Pull plots
  vector<TH2D*> pullHists = MakeCaloPullPlots(hists,refHists);
  gStyle->SetPalette(PULL_PALETTE);
  
  PrintCaloPlots("pull_"+branchName,"Pull: "+title,pullHists);
  CheckCaloPulls(pullHists,title);
  gStyle->SetPalette(PALETTE);
  
  textOut<<endl;
  cout<<endl;
}

// Go through a set of calorimeter pull histograms and report overall pull and
// any problems
double CheckCaloPulls(vector<TH2D*> hPulls, string title)
{
  bool foundPull=false;
  for (int i=0;i<hPulls.size();i++)
  {
    // Check whether the distributions are identical (all pulls 0)

    if ( (hPulls.at(i)->GetBinContent(hPulls.at(i)->GetMaximumBin())) != 0 || (hPulls.at(i)->GetBinContent(hPulls.at(i)->GetMinimumBin())) != 0)
    {
      foundPull=true;
    }
  }
  if (!foundPull)
  { // If the plots are the same there is no need to make a plot of all pulls
    cout<<"Pull is zero - plots are identical"<<endl;
    textOut<<"Pull is zero - plots are identical"<<endl;
    return 0;
  }
  string firstName=hPulls.at(0)->GetName();
  int pos=firstName.find_last_of('_');
  string hPullName="allpulls"+firstName.substr(8,pos-8);
  
  TH1D *h1Pulls = new TH1D(hPullName.c_str(),(title+" pulls").c_str(),100,-10,10);
  
  double totalPull=0;
  int pullCells=0;
  for (int i=0;i<hPulls.size();i++)
  {
    TH2D *hPull = hPulls.at(i);
    for (int x=1;x<=hPull->GetNbinsX();x++)
    {
      for (int y=1;y<=hPull->GetNbinsY();y++)
      {
        // Pull is sample - ref / total uncertainty
        double pull=hPull->GetBinContent(x,y) ;
        if (!std::isnan(pull) && !std::isinf(pull))
        {
          totalPull+=pull;
          pullCells++;
          h1Pulls->Fill(pull);
        }
        
        // Report any cells where sample and reference are too different
        if (TMath::Abs(pull) > REPORT_PULLS_OVER ||  std::isnan(pull) )
        {
          string reportString;
          // Unfortunately the numbering scheme maps differently to the bin numbers for each wall
          string intExt="external"; // Translate x coordinate to position for X walls and vetoes
          string side="French";
          switch (i)
          {
            case 0: // Italy
              reportString=Form("Italian main wall: module (%d,%d)",20-x,y-1);
              break;
            case 1: // France
              reportString=Form("French main wall: module (%d,%d)",x-1,y-1);
              break;
            case 2: // Tunnel
              if (x>2) side = "Italian"; // Translate x bin to location
              if (x==2 || x ==3) intExt="internal";
              reportString=Form("Tunnel X-wall: module %d (",y-1)+side+" side "+intExt+")";
              break;
            case 3: // Mountain
              if (x<3) side = "Italian"; // Translate x bin to location
              if (x==2 || x ==3) intExt="internal";
              reportString=Form("Mountain X-wall: module %d (",y-1)+side+" side "+intExt+")";
              break;
            case 4: // Top
              if (y==2) side = "Italian"; // Translate y bin to location
              reportString=Form("Top veto wall: module %d (",x-1)+side+" side)";
              break;
            case 5: // Bottom
              if (y==1) side = "Italian"; // Translate y bin to location
              reportString=Form("Bottom veto wall: module %d (",x-1)+side+" side)";
              break;
            default:
              break;
              reportString=Form("ERROR: pull found for unknown calorimeter wall %d: this is a bug!",i);
          }
          if (std::isnan(pull)) reportString += ": not enough data to calculate pull";
          else if (std::isinf(pull)) reportString += ": not enough data to calculate pull";
          else reportString += Form(": pull = %.2f",pull);
          cout<<reportString<<endl;
          textOut<<reportString<<endl;
        }
        
      }
    }
  }
  PrintPlotOfPulls(h1Pulls,pullCells,title);
  return totalPull;
}

double  PrintPlotOfPulls(TH1D *h1Pulls, int pullCells, string title)
{
  // Save the plot of pulls
  h1Pulls->GetXaxis()->SetTitle("Pull");
  h1Pulls->GetYaxis()->SetTitle("Frequency");
  
  
  h1Pulls->Fit("gaus","LQ");
  TF1 *fit = (TF1*)h1Pulls->GetFunction("gaus");
  double mean=fit->GetParameter(1);
  double rms=fit->GetParameter(2);
  double meanerr=fit->GetParError(1);
  double rmserr=fit->GetParError(2);
  
  // Report fitted mean pulls
  textOut<<"Mean pull:"<<mean<<" +/- "<<meanerr<<" for "<<pullCells<<" modules with data. ";
  cout<<"Mean pull:"<<mean<<" +/- "<<meanerr<<" for "<<pullCells<<" modules with data."<<endl;
  if (mean < 0)  textOut<<"Note: negative pull indicates sample deficit."<<endl;
  else textOut<<"Note: positive pull indicates sample excess."<<endl;
  cout<<"RMS of pulls "<<rms<<" +/- "<<rmserr<<endl;
  textOut<<"RMS of pulls "<<rms<<" +/- "<<rmserr<<endl;
  
  
  h1Pulls->Write("",TObject::kOverwrite);
  TCanvas *cPull = new TCanvas("cPull","cPull",900,600);
  h1Pulls->Draw("HIST");
  fit->SetLineColor(kRed);
  fit->SetLineWidth(2);
  fit->Draw("SAME");
  
  WriteLabel(.6,.75,Form ("Mean pull %.2f #pm %.2f",mean,meanerr),0.03);
  WriteLabel(.6,.7,Form ("RMS  %.2f #pm %.2f",rms,rmserr),0.03);
  WriteLabel(.15,.84,title+" pulls",0.04);
  cPull->SaveAs((plotdir+Form("/%s.png",h1Pulls->GetName())).c_str());
  delete cPull;

  return mean;
}

vector<TH2D*>MakeCaloPullPlots(vector<TH2D*> vSample, vector<TH2D*> vRef)
{
  vector <TH2D*> vPull;
  if (vSample.size() != vRef.size())
  { // This should never happen if it only gets called from here
    cout<<"ERROR: number of sample and reference histograms does not match for "<<vSample.at(0)->GetName()<<endl;
    return vPull;
  }
  for (int i=0;i<vSample.size();i++)
  {
    vPull.push_back(PullPlot2D(vSample.at(i), vRef.at(i)));
  }
  return vPull;
}

vector<TH2D*> MakeCaloPlotSet(string fullBranchName, string branchName, string title, bool isRef, bool isAverage, string mapBranch)
{
  
  vector<TH2D*> hists;
  vector<TH2D*> ave_hists;
  vector<TH2D*> var_hists;
  
  string prefix = (isRef)?"ref_":"plt_";
  
  for (int i=0; i<6; i++)
  {
    // Make a histogram to hold the count for each calo location
    string prefix = (isRef)?"ref_":"plt_";
    // The binnings etc are all in the header file
    TH2D *h = new TH2D((prefix+branchName+"_"+CALO_WALL[i]).c_str(),(CALO_WALL[i]).c_str(),CALO_XBINS[i],CALO_XLO[i],CALO_XHI[i],CALO_YBINS[i],0,CALO_YBINS[i]);
    if( h->GetSumw2N() == 0 ) h->Sumw2();
    hists.push_back(h);
    
    // Another for the value to be averaged (if an average plot)
    prefix = (isRef)?"refave_":"ave_";
    TH2D *m = new TH2D((prefix+branchName+"_"+CALO_WALL[i]).c_str(),(CALO_WALL[i]).c_str(),CALO_XBINS[i],CALO_XLO[i],CALO_XHI[i],CALO_YBINS[i],0,CALO_YBINS[i]);
    if( m->GetSumw2N() == 0 ) m->Sumw2();
    ave_hists.push_back(m);
    
    // And another to histogram the quantity squared, to be used to calculate the error on the mean
    prefix = (isRef)?"refvar_":"var_";
    TH2D *v = new TH2D((prefix+branchName+"_"+CALO_WALL[i]).c_str(),(CALO_WALL[i]).c_str(),CALO_XBINS[i],CALO_XLO[i],CALO_XHI[i],CALO_YBINS[i],0,CALO_YBINS[i]);
    if( v->GetSumw2N() == 0 ) v->Sumw2();
    var_hists.push_back(v);
  }
  

  // Loop the event tree and decode the position
  
  // Set up a vector of strings to receive the list of calorimeter IDs
  // There is one entry in the vector for each calorimeter hit in the event
  // And it will have a format something like [1302:0.1.0.10.*]
  std::vector<string> *caloHits = 0;
  
  TTree *thisTree = (isRef)?reftree->CopyTree(""):tree->CopyTree("");
  
  // Count the number of entries in the tree
  int nEntries = thisTree -> GetEntries();
  // Map the branches
  thisTree->SetBranchAddress(mapBranch.c_str(), &caloHits);
  
  std::vector<double> *toAverage = 0;
  if (isAverage)
  {
    thisTree->SetBranchAddress(fullBranchName.c_str(), &toAverage);
  }
  
  // Loop through the tree
  for( int iEntry = 0; iEntry < nEntries; iEntry++ )
  {
    thisTree->GetEntry(iEntry);
    // Populate these with which histogram we will fill and what cell
    int xValue=0;
    int yValue=0;
   // TH2D *whichHistogram=0;
   // TH2D *whichAverage=0; // this is a tad messy
    
    int whichWall=-1;
    
    // This should always work, but there is next to no catching of badly formatted
    // geom ID strings. Are they a possibility?
    if (caloHits->size()>0)
    {
      for (int i=0;i<caloHits->size();i++)
      {
        string thisHit=caloHits->at(i);
        
        if (thisHit.length()>=9)
        {
          bool isFrance=(thisHit.substr(8,1)=="1");
          //Now to decode it
          string wallType = thisHit.substr(1,4);
          
          if (wallType=="1302") // Main walls
          {
            
            //if (isFrance) whichHistogram = hFrance; else whichHistogram = hItaly;
            if (isFrance) whichWall = FRANCE; else whichWall = ITALY;
            string useThisToParse = thisHit;
            
            // Hacky way to get the bit between the 2nd and 3rd "." characters for x
            int pos=useThisToParse.find('.');
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find('.');
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find('.');
            std::string::size_type sz;   // alias of size_t
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            
            // and the bit before the next . characters for y
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find_first_of('.');
            yValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            
            // The numbering is from mountain to tunnel
            // But we draw the Italian side as we see it, with the mountain on the left
            // So let's flip it around
            if (!isFrance)xValue = -1 * (xValue + 1);
            //cout<<iEntry<<" : " <<thisHit<<" : " <<(isFrance?"Fr.":"It")<<" - "<<xValue<<":"<<yValue<<endl;
          }
          else if (wallType == "1232") //x walls
          {
            bool isTunnel=(thisHit.substr(10,1)=="1");
//            if (isTunnel) whichHistogram = hTunnel; else whichHistogram = hMountain;
            if (isTunnel) whichWall=TUNNEL; else whichWall = MOUNTAIN;
            // Hacky way to get the bit between the 3rd and 4th "." characters for x
            string useThisToParse = thisHit;
            int pos=0;
            for (int j=0;j<3;j++)
            {
              int pos=useThisToParse.find('.');
              useThisToParse=useThisToParse.substr(pos+1);
            }
            pos=useThisToParse.find('.');
            std::string::size_type sz;   // alias of size_t
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            
            // and the bit before the next . characters for y
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find_first_of('.');
            yValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            if (!isFrance)xValue = -1 * (xValue + 1); // Italy is on the left so reverse these to draw them
            
            if (isTunnel) // Switch it so France is on the left for the tunnel side
            {
              xValue = -1 * (xValue + 1);
            }
            
            //cout<<iEntry<<" : " <<thisHit<<" : " <<(isFrance?"Fr.":"It")<<" - "<<(isTunnel?"tunnel":"mountain")<<" - "<<xValue<<":"<<yValue<<endl;
            
          }
          else if (wallType == "1252") // veto walls
          {
            bool isTop=(thisHit.substr(10,1)=="1");
//            if (isTop) whichHistogram = hTop; else whichHistogram = hBottom;
            if (isTop) whichWall = TOP; else whichWall = BOTTOM;
            string useThisToParse = thisHit;
            int pos=useThisToParse.find('.');
            for (int j=0;j<4;j++)
            {
              useThisToParse=useThisToParse.substr(pos+1);
              pos=useThisToParse.find('.');
            }
            
            std::string::size_type sz;   // alias of size_t
            yValue=((isFrance^isTop)?1:0); // We flip this so that French side is inwards on the print
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            //cout<<iEntry<<" : " <<(isFrance?"Fr.":"It")<<" "<<(isTop?"top ":"bottom ")<<xValue<<endl;
          }
          else
          {
            cout<<"WARNING -- Calo hit found with unknown wall type "<<wallType<<endl;
            continue; // We can't plot it if we don't know where to plot it
          }
          
          // Now we know which histogram and the coordinates so write it
          if (whichWall>=0)
          {
            hists.at(whichWall)->Fill(xValue,yValue);
            if (isAverage)
            {
              ave_hists.at(whichWall)->Fill(xValue,yValue,toAverage->at(i)); // Sum it for now and we will divide out by number of hits
              var_hists.at(whichWall)->Fill(xValue,yValue, pow(toAverage->at(i),2)  ); // Sum the squares for variance calculation
            }
          }
          else
          {
            cout<<"WARNING -- Calo hit found with unknown wall type "<<endl;
            continue; // We can't plot it if we don't know where to plot it
          }
        }// end parsable string
      } // end for each hit
    } // End if there are calo hits
  }
  if (isAverage)
  {
    for (int i=0;i<hists.size();i++)
    {
      ave_hists.at(i)->Divide(hists.at(i)); // Go from totals to averages
      
      var_hists.at(i)->Divide(hists.at(i)); // This should correctly give us the mean of the squares
      
      
      // Then variance of the sample is n/(n-1) times  mean of (x^2) - (mean of x)^2
      // Variance on the MEAN is then variance of sample / number of hits
      // Take the square root of that to get the error on the mean, which is what we need here
      // Thank you Glen Cowan, "Statistical data analysis"
      for (int x = 1; x<=hists.at(i)->GetNbinsX(); x++)
      {
        for (int y = 1; y<=hists.at(i)->GetNbinsY(); y++)
        {
          double nHits = hists.at(i)->GetBinContent(x,y);
          if (nHits>1)
          {
            double meanSquared= pow(ave_hists.at(i)->GetBinContent(x,y),2);
            double meanOfSquares = var_hists.at(i)->GetBinContent(x,y);
            double variance =  (meanOfSquares - meanSquared)  * nHits / (nHits - 1);
            ave_hists.at(i)->SetBinError(x,y, TMath::Sqrt(variance / nHits) );
          }
          else
          { // What's the uncertainty on a single measurement?
            ave_hists.at(i)->SetBinError(x,y,0);
          }
        }
      }
      ave_hists.at(i)->Write("",TObject::kOverwrite); // Write the average histograms
    }
    return ave_hists;
  }
  else
  {
    // Write the histograms to a file
    double scale=(double)tree->GetEntries()/thisTree->GetEntries(); // Scale to the main tree, if it is a reference tree - otherwise scale is just 1
    for (int i=0;i<hists.size();i++)
    {
      // If count is 0, set uncertainty to 1
      for (int x = 1; x<=hists.at(i)->GetNbinsX(); x++)
      {
        for (int y = 1; y<=hists.at(i)->GetNbinsY(); y++)
        {
          double nHits = hists.at(i)->GetBinContent(x,y);
          if (nHits==0) hists.at(i)->SetBinError(x,y, 1);
        }
      }
      if (isRef) hists.at(i)->Scale(scale);
      hists.at(i)->Write("",TObject::kOverwrite);
    }
    return hists;
  }
  
  return hists;
}



/**
 *  Plot a map of the tracker cells
 */
void PlotTrackerMap(string branchName)
{

  string config="";
  config=configParams[branchName]; // get the config loaded from the file if there is one
  
  // Can we do a comparison to the reference for this plot?
  bool hasReferenceBranch=hasValidReference;
  
  // Check whether the reference file contains this branch
  if (hasValidReference)
  {
    hasReferenceBranch=reftree->GetBranchStatus(branchName.c_str());
    if (!hasReferenceBranch) cout<<"WARNING: branch "<<branchName<<" not found in reference file. No comparison plots will be made for this branch"<<endl;
  }
  
  string title="";
  // Load the title from the config file
  if (config.length()>0)
  {
    // title is the only thing for this one
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage
  }
  
  string fullBranchName=branchName;//This is a combo of name and parent for average branches
  // is it an average?
  string mapBranch=branchName;
  bool isAverage=false;
  if (branchName[1]=='m')
  {
    // In this case, the branch name should be split in two with a . character
    int pos=branchName.find(".");
    if (pos<=1)
    {
      cout<<"Error - could not find map branch for "<<branchName<<": remember to provide a map branch name with a dot"<<endl;
      return;
    }
    mapBranch=branchName.substr(pos+1);
    
    // Check whether the sample file and the reference file contain the map branch
    if (!tree->GetBranchStatus(mapBranch.c_str()))
    {
      cout<<"WARNING: map branch "<<mapBranch<<" not found in sample file. No plots can be made for the branch "<<branchName<<endl;
      return; // We can't do the plot at all
    }
    if (hasReferenceBranch) // so far it is alright... but does the reference have the map branch?
    {
      hasReferenceBranch=reftree->GetBranchStatus(mapBranch.c_str());
      if (!hasReferenceBranch) cout<<"WARNING: map branch "<<mapBranch<<" not found in reference file. No comparison plots can be made for the branch "<<branchName<<endl;
    }
    branchName=branchName.substr(0,pos);
    isAverage=true;
  }
  
  // Set the title to a default if there isn't anything in the config file
  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }

  // Make the plot
  TCanvas *c = new TCanvas (("plot_"+branchName).c_str(),("plot_"+branchName).c_str(),600,1200);
  TH2D *h=TrackerMapHistogram(fullBranchName,branchName, title, false, isAverage, mapBranch);
  if( h->GetSumw2N() == 0 )h->Sumw2();
  h->Draw("COLZ0");
  c->SetRightMargin(0.15);
  OverlayWhiteForNaN(h);
  AnnotateTrackerMap();
  
  // Save to a ROOT file and to a PNG
  h->Write("",TObject::kOverwrite);
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  
  // If there is a reference plot, make a pull plot
  if (hasReferenceBranch)
  {
    TH2D *href=TrackerMapHistogram(fullBranchName,branchName, title, true, isAverage, mapBranch);
    if( href->GetSumw2N() == 0 )href->Sumw2();
    
    double scale=(double)tree->GetEntries()/(double)reftree->GetEntries();
    if (!isAverage) href->Scale(scale); // Normalise it if it is a plot of counts. Don't normalise it if it is an average plot; the number of entries shouldn't matter

    Double_t ks = h->KolmogorovTest(href);
    Double_t chisq;
    Int_t ndf;
    Double_t p_value=ChiSquared(h, href, chisq, ndf, isAverage);
    
    cout<<"Kolmogorov: "<<ks<<endl;
    cout<<"P-value: "<<p_value<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;
    
    // Write to output file
    textOut<<branchName<<":"<<endl;
    textOut<<"KS score: "<<ks<<endl;
    textOut<<"P-value: "<<p_value<<" Chi-square: "<<chisq<<" / "<<ndf<<" DoF = "<<chisq/(double)ndf<<endl;
    
    TH2D *hPull = PullPlot2D(h,href);
    CheckTrackerPull(hPull,title);
    gStyle->SetPalette(PULL_PALETTE);
    hPull->GetZaxis()->SetRangeUser(-4,4);
    hPull->Draw("COLZ0");
    OverlayWhiteForNaN(hPull);
    AnnotateTrackerMap();
    hPull->Write("",TObject::kOverwrite);
    c->SaveAs((plotdir+"/pull_"+branchName+".png").c_str());
    
    gStyle->SetPalette(PALETTE);
    delete hPull;
    textOut<<endl;
  }
  
  delete h;
  delete c;

}

// Draw the foil and label the French/Italian sides and Tunnel/Mountain ends
void AnnotateTrackerMap()
{
    // Annotate to make it clear what the detector layout is
    TLine *foil=new TLine(0,0,0,113);
    foil->SetLineColor(kGray);
    foil->SetLineWidth(5);
    foil->Draw("SAME");
  
    // Decorate the print
    WriteLabel(.16,.5,"Italy");
    WriteLabel(.65,.5,"France");
    WriteLabel(0.39,.8,"Tunnel");
    WriteLabel(.38,.15,"Mountain");

}
// Calculate the pull between two 2d histograms
TH2D *PullPlot2D(TH2D *hSample, TH2D *hRef )
{
  
  if( hSample->GetSumw2N() == 0 )  hSample->Sumw2();
  if( hRef->GetSumw2N() == 0 )hRef->Sumw2();
  TH2D *hPull = (TH2D*)hSample->Clone();
  hPull->SetName(Form("pull_%s",hPull->GetName()));
  hPull->SetTitle(Form("Pull: %s",hPull->GetTitle()));
  hPull->ClearUnderflowAndOverflow (); // There shouldn't be anything in them anyway but let's be sure
  
  for (int x=0;x<=hSample->GetNbinsX();x++)
  {
      for (int y=0;y<=hSample->GetNbinsY();y++)
      {
        // Initialize it just in case
        hPull->SetBinContent(x,y,0);
        hPull->SetBinError(x,y,0); // This is being lazy, I could probably calculate the error on the pull if I were a better statistician. But do we need it?

        // Pull is sample - ref / total uncertainty
        double pull=( hSample->GetBinContent(x,y) - hRef->GetBinContent(x,y)) /
          TMath::Sqrt( pow(hSample->GetBinError(x,y),2) + pow(hRef->GetBinError(x,y),2) );
        // On an average plot, this doesn't make sense if the number of hits in either sample
        // is zero - then we just don't know the value of the thing we are averaging.
        // Same if we only get one hit - we don't know the uncertainty of it so the pull will
        // be artificially high. We can tell these cases because the error will be 0
        if (hSample->GetBinError(x,y) == 0 || hRef->GetBinContent(x,y) == 0 ) pull = NAN;
        hPull->SetBinContent(x,y,pull);
      }
  }
  hPull->GetZaxis()->SetRangeUser(-4.,4.);
  hPull->Write("",TObject::kOverwrite);
  return hPull;
}

// Go through a tracker pull histogram and report overall pull and
// any problems
double CheckTrackerPull(TH2D *hPull, string title)
{
  bool problemPulls=false;
  double totalPull=0;
  int pullCells=0;
  
  string firstName=hPull->GetName();
  string hPullName="allpulls"+firstName.substr(4);
  
  TH1D *h1Pulls = new TH1D(hPullName.c_str(),(title+" pulls").c_str(),100,-10,10);
  
  for (int x=1;x<=hPull->GetNbinsX();x++)
  {
    for (int y=1;y<=hPull->GetNbinsY();y++)
    {
      // Pull is sample - ref / total uncertainty
      double pull=hPull->GetBinContent(x,y) ;
      if (!std::isnan(pull))
      {
        totalPull+=pull;
        pullCells++;
        h1Pulls->Fill(pull);
      }
      else
      {
        if (x > MAX_TRACKER_LAYERS)
          textOut<<"Layer "<<x - MAX_TRACKER_LAYERS<<" (France), row "<<y<<": not enough data to calculate pull"<<endl;
        else textOut<<"Layer "<< MAX_TRACKER_LAYERS + 1 - x<<" (Italy), row "<<y<<": not enough data to calculate pull"<<endl;      }
      // Report any cells where sample and reference are too different
      if (TMath::Abs(pull) > REPORT_PULLS_OVER)
      {
        if (x > MAX_TRACKER_LAYERS)
          textOut<<"Layer "<<x - MAX_TRACKER_LAYERS<<" (France), row "<<y<<": pull = "<<pull<<endl;
        else textOut<<"Layer "<< MAX_TRACKER_LAYERS + 1 - x<<" (Italy), row "<<y<<": pull = "<<pull<<endl;
        problemPulls=true;
      }
    }
  }
  if (problemPulls)
  {
    textOut<<"Layers are numbered 1 to 9, with 1 nearest the foil. Rows count from mountain (1) to tunnel ("<<MAX_TRACKER_ROWS<<")."<<endl;
  }
  
  // Check whether the distributions are identical (all pulls 0)

  if ( (hPull->GetBinContent(hPull->GetMaximumBin())) == 0 && (hPull->GetBinContent(hPull->GetMinimumBin())) == 0)
  {
    cout<<"Pull is zero - plots are identical"<<endl;
    textOut<<"Pull is zero - plots are identical"<<endl;
  }
  else
  {
    // If not, plot all the pulls and fit to a Gaussian
    PrintPlotOfPulls(h1Pulls,pullCells,title);
  }
  return totalPull;
}

// Make the tracker map histogram (either counts or averages, depending on whether there is a map branch)
// The formatting and decision-making about what goes into the histogram is done separately,
// this just loops the tree and fills the histogram
TH2D *TrackerMapHistogram(string fullBranchName, string branchName, string title, bool isRef, bool isAverage, string mapBranch)
{
  TTree *inputTree = (isRef?reftree:tree);

  string tmpName="plt_"+branchName;
  if (isRef) tmpName = "ref_"+tmpName;
    TH2D *h = new TH2D(tmpName.c_str(),title.c_str(),MAX_TRACKER_LAYERS*2,MAX_TRACKER_LAYERS*-1,MAX_TRACKER_LAYERS,MAX_TRACKER_ROWS,0,MAX_TRACKER_ROWS); // Map of the tracker
  if( h->GetSumw2N() == 0 )h->Sumw2(); // Important to get errors right
  
    tmpName="ave_"+branchName;
    if (isRef) tmpName = "ref_"+tmpName;
    TH2D *hAve = new TH2D(tmpName.c_str(),title.c_str(),MAX_TRACKER_LAYERS*2,MAX_TRACKER_LAYERS*-1,MAX_TRACKER_LAYERS,MAX_TRACKER_ROWS,0,MAX_TRACKER_ROWS); // Map of the tracker
  
    if( hAve->GetSumw2N() == 0 )hAve->Sumw2(); // Important to get errors right
  
  
    tmpName ="sq_"+tmpName;
    TH2D *hQuantitySquared = new TH2D(tmpName.c_str(),title.c_str(),MAX_TRACKER_LAYERS*2,MAX_TRACKER_LAYERS*-1,MAX_TRACKER_LAYERS,MAX_TRACKER_ROWS,0,MAX_TRACKER_ROWS); // Use this to get the standard deviation of the measurements
    if( hQuantitySquared->GetSumw2N() == 0 )hQuantitySquared->Sumw2(); // Important to get errors right
  
    // This decodes the encoded tracker map to extract the x and y positions
  
    // Unfortunately it is not so easy to make the averages plot so we need to loop the tuple
    std::vector<int> *trackerHits = 0;
    std::vector<double> *toAverageTrk = 0;
    TTree *thisTree=inputTree->CopyTree("");
    thisTree->SetBranchAddress(mapBranch.c_str(), &trackerHits);

    if (isAverage)
    {
      thisTree->SetBranchAddress(fullBranchName.c_str(), &toAverageTrk);
    }

    // Now we can fill the two plots
    // Loop through the tree
  
    int nEntries = thisTree -> GetEntries();
    for( int iEntry = 0; iEntry < nEntries; iEntry++ )
    {
      thisTree->GetEntry(iEntry);
      // Populate these with which histogram we will fill and what cell
      int xValue=0;
      int yValue=0;
      if (trackerHits->size()>0)
      {
        for (int i=0;i<trackerHits->size();i++)
        {
          yValue=TMath::Abs(trackerHits->at(i)/100);
          xValue=trackerHits->at(i)%100;
          if (isAverage && !std::isnan(toAverageTrk->at(i)))
          {
            hAve->Fill(xValue,yValue,toAverageTrk->at(i)); // Ignore the uncertainties
            hQuantitySquared->Fill(xValue,yValue,pow(toAverageTrk->at(i),2)); // We will use this to calculate uncertainty
            h->Fill(xValue,yValue); // Only fill this if there is something to average over! We don't want to divide by a denominator that includes hits with no useful info. Obviously the best thing would be to not put that stuff in the tuple in the first place, but this works as a protection in case you do
          }
          if (!isAverage)
          {
            h->Fill(xValue,yValue); // We will take the lot!
          }
        }
      }
    } // Loop all entries

    if (isAverage)
    {
      hAve->Divide(h); // This should correctly give us the mean
      hQuantitySquared->Divide(h); // This should correctly give us the mean of the squares
      
      
      // Then variance of the sample is n/(n-1) times  mean of (x^2) - (mean of x)^2
      // Variance on the MEAN is then variance of sample / number of hits
      // Take the square root of that to get the error on the mean, which is what we need here
      // Thank you Glen Cowan, "Statistical data analysis"
      for (int x = 1; x<=h->GetNbinsX(); x++)
      {
        for (int y = 1; y<=h->GetNbinsY(); y++)
        {
          double nHits = h->GetBinContent(x,y);
          if (nHits>1)
          {
            double meanSquared= pow(hAve->GetBinContent(x,y),2);
            double meanOfSquares = hQuantitySquared->GetBinContent(x,y);
            double variance =  (meanOfSquares - meanSquared)  * nHits / (nHits - 1);
            hAve->SetBinError(x,y, TMath::Sqrt(variance / nHits) );
          }
          else
          { // Don't know the variance on a single measurement...
            hAve->SetBinError(x,y,0);
          }
        }
      }
      h=hAve; // overwrite the temp plot with the one we actually want to save
    }
    else
    { // If count is 0, set uncertainty to 1
      for (int x = 1; x<=h->GetNbinsX(); x++)
      {
        for (int y = 1; y<=h->GetNbinsY(); y++)
        {
          double nHits = h->GetBinContent(x,y);
          if (nHits==0) h->SetBinError(x,y, 1);
        }
      }
    }
  h->GetYaxis()->SetTitle("Row");
  h->GetXaxis()->SetTitle("Layer");
  return h;
}

// Just a quick routine to write text at a (x,y) coordinate
void WriteLabel(double x, double y, string text, double size)
{
  TLatex *txt=new TLatex();
  txt->SetTextSize(size);
  txt->SetNDC();
  txt->DrawLatex(x,y,text.c_str());
}

/**
 *  Return the part of the string that is before the first comma (trimmed of white space)
 *  Modify the input string to be whatever is AFTER the first comma
 *  If there is no comma, return the whole (trimmed) string and modify the input to zero-length string
 */
string GetBitBeforeComma(string& input)
{
  string output;
  int pos=input.find_first_of(',');
  if (pos <=0)
  {
    boost::trim(input);
    output=input;
    input="";
  }
  else
  {
    output=input.substr(0,pos);
    boost::trim(output);
    input=input.substr(pos+1);
  }
  return output;
}

/**
 *  Return the part of the string that is before the first space
 */
string FirstWordOf(string input)
{
  string output;
  int pos=input.find_first_of(' ');
  if (pos <=0)
  {
    output=input;
  }
  else
  {
    output=input.substr(0,pos);
  }
  return output;
}

string BranchNameToEnglish(string branchname)
{
  int pos = branchname.find_first_of("_");
  int initpos=pos;
  while (pos >=0)
  {
    branchname.replace(pos,1," ");
    pos = branchname.find_first_of("_");
  }
  string output = branchname.substr(initpos+1,branchname.length());
  output[0]=toupper(output[0]);
  return output;
}

// Draw white squares over anything that is marked as infinite or not a number
void OverlayWhiteForNaN(TH2D *hist)
{
  for (int x=1;x<=hist->GetNbinsX();x++)
  {
    for (int y=1;y<=hist->GetNbinsY();y++)
    {
      if (std::isnan(hist->GetBinContent(x,y)))
      {
        Double_t x1 = hist->GetXaxis()->GetBinLowEdge(x);
        Double_t x2 = hist->GetXaxis()->GetBinUpEdge(x);
        Double_t y1 = hist->GetYaxis()->GetBinLowEdge(y);
        Double_t y2 = hist->GetYaxis()->GetBinUpEdge(y);
        
        TBox *box2 = new TBox(x1, y1, x2, y2);
        box2->SetFillStyle(1001);
        box2->SetFillColor(kWhite);
        box2->SetLineColor(kWhite);
        box2->SetLineWidth(0);
        box2->Draw();
      }
    }
  }
}

// Arrange all the bits of calorimeter on a canvas
void PrintCaloPlots(string branchName, string title, vector <TH2D*> histos)
{
  if (histos.size() !=6)
  {
    cout<<"Unable to print calorimeter map for "<<branchName<<" as we do not have 6 input histograms"<<endl;
    return;
  }
  TCanvas *c = new TCanvas ("caloplots","caloplots",2000,1000);
  // Easier if we name them!
  TH2D *hItaly=histos.at(0);
  TH2D *hFrance=histos.at(1);
  TH2D *hTunnel=histos.at(2);
  TH2D *hMountain=histos.at(3);
  TH2D *hTop=histos.at(4);
  TH2D *hBottom=histos.at(5);
  
  //First, 20x13
  TPad *pItaly = new TPad("p_italy",
                          "",0.6,0.2,1,0.8,0);
  // Third 20x13
  TPad *pFrance = new TPad("p_france",
                           "",0.1,0.2,0.5,0.8,0);
  // Second, 4x16
  TPad *pMountain = new TPad("p_mountain",
                             "",0.02,0.2,0.12,0.8,0);
  // Fourth 4x16
  TPad *pTunnel = new TPad("p_tunnel",
                           "",0.5,0.2,.6,0.8,0);
  // 16x2
  TPad *pTop = new TPad("p_top",
                        "",0.1,0.8,0.5,.98,0);
  //16 x2
  TPad *pBottom = new TPad("p_bottom",
                           "",0.1,0.02,0.5,0.2,0);
  TPad *pTitle = new TPad("p_title",
                          "",0.6,0.8,0.95,1,0);
  pTitle->Draw();
  
  std::vector <TPad*> pads;
  pads.push_back(pItaly);
  pads.push_back(pTunnel);
  pads.push_back(pFrance);
  pads.push_back(pMountain);
  pads.push_back(pTop);
  pads.push_back(pBottom);
  
  
  hBottom->GetYaxis()->SetBinLabel(2,"France");
  hBottom->GetYaxis()->SetBinLabel(1,"Italy");
  hTop->GetYaxis()->SetBinLabel(1,"France");
  hTop->GetYaxis()->SetBinLabel(2,"Italy");
  
  gStyle->SetOptTitle(0);
  
  // Set them all to the same scale; first work out what it should be
  double max=-9999;
  double min=0;
  for (int i=0;i<histos.size();i++)
  {
    max=TMath::Max(max,(double)histos.at(i)->GetMaximum());
    min=TMath::Min(min,(double)histos.at(i)->GetMinimum());
    // While we are here, let's draw the pads
    pads.at(i)->Draw();pads.at(i)->SetGrid();
  }
  if (min>0)min=0;
  gStyle->SetGridStyle(3);
  gStyle->SetGridColor(kGray);
  for (int i=0;i<histos.size();i++)
  {
    histos.at(i)->GetZaxis()->SetRangeUser(min,max);
    histos.at(i)->GetYaxis()->SetNdivisions(histos.at(i)->GetNbinsY());
    histos.at(i)->GetXaxis()->SetNdivisions(histos.at(i)->GetNbinsX());
    histos.at(i)->GetXaxis()->CenterLabels();
    histos.at(i)->GetYaxis()->CenterLabels();
    
  }
  
  // Italian Main wall
  pItaly->cd();
  for (int i=1;i<=hItaly->GetNbinsX();i++)
  {
    hItaly->GetXaxis()->SetBinLabel(i,(to_string(MAINWALL_WIDTH-i)).c_str());
  }
  
  hItaly->GetXaxis()->SetLabelSize(0.06);
  hItaly->Draw("COLZ0"); // Only have the scale over on the right
  
  OverlayWhiteForNaN(hItaly);
  WriteLabel(.45,.95,"Italy");
  
  // French main wall
  pFrance->cd();
  hFrance->Draw("COL0");
  OverlayWhiteForNaN(hFrance);
  WriteLabel(.4,.95,"France");
  
  // Mountain x wall
  pMountain->cd();
  hMountain->GetYaxis()->SetLabelSize(0.1);
  hMountain->GetYaxis()->SetLabelOffset(0.01);
  hMountain->GetXaxis()->SetBinLabel(1,"It.");
  hMountain->GetXaxis()->SetBinLabel(2,"");
  hMountain->GetXaxis()->SetBinLabel(3,"");
  hMountain->GetXaxis()->SetBinLabel(4,"Fr.");
  hMountain->GetXaxis()->SetLabelSize(0.2);
  
  hMountain->Draw("COL0");
  OverlayWhiteForNaN(hMountain);
  WriteLabel(.2,.95,"Mountain",0.15);
  // Draw on the source foil
  TLine *foil=new TLine(0,0,0,16);
  foil->SetLineColor(kGray+3);
  foil->SetLineWidth(5);
  foil->Draw("SAME");
  
  // Tunnel x wall
  pTunnel->cd();
  hTunnel->GetYaxis()->SetLabelSize(0.1);
  hTunnel->GetYaxis()->SetLabelOffset(0.01);
  hTunnel->GetXaxis()->SetBinLabel(1,"Fr.");
  hTunnel->GetXaxis()->SetBinLabel(2,"");
  hTunnel->GetXaxis()->SetBinLabel(3,"");
  hTunnel->GetXaxis()->SetBinLabel(4,"It.");
  hTunnel->GetXaxis()->SetLabelSize(0.2);
  hTunnel->Draw("COL0");
  OverlayWhiteForNaN(hTunnel);
  WriteLabel(.25,.95,"Tunnel",0.15);
  foil->Draw("SAME");
  
  // Top veto wall
  pTop->cd();
  hTop->Draw("COL0");
  OverlayWhiteForNaN(hTop);

  hTop->GetXaxis()->SetLabelSize(0.1);
  hTop->GetYaxis()->SetLabelSize(0.15);
  TLine *foilveto=new TLine(0,1,16,1);
  foilveto->SetLineColor(kGray+3);
  foilveto->SetLineWidth(5);
  foilveto->Draw("SAME");
  WriteLabel(.42,.2,"Top",0.2);
  
  
  // Bottom veto wall
  pBottom->cd();
  hBottom->GetXaxis()->SetLabelSize(0.1);
  hBottom->GetYaxis()->SetLabelSize(0.15);
  hBottom->Draw("COL0");
  OverlayWhiteForNaN(hBottom);
  foilveto->Draw("SAME");
  WriteLabel(.42,.6,"Bottom",0.2);
  
  pTitle->cd();
  WriteLabel(.1,.5,title,0.2);
  
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  
  delete c;
  return;
}

double ChiSquared(TH1 *h1, TH1 *h2, double &chisq, int &ndf, bool isAverage)
{
  // Calculate a chi squared per degree of freedom
  chisq=0;
  ndf=0;
  
  for (int x=1; x<=h1->GetNbinsX();x++)
  {
    for (int y=1; y<=h1->GetNbinsY();y++)
    {
      double val1 = h1->GetBinContent(x,y);
      double  val2 = h2->GetBinContent(x,y);
      double err1 = h1->GetBinError(x,y);
      double err2 = h2->GetBinError(x,y);
      
      // We won't count the bin (or increase the degrees of freedom) if we don't have enough info
      // This is the case if either uncertainty is zero or  if a value or uncertainty is not a number
      if ((std::isnan(val1) || std::isnan(val2) || std::isnan(err1) || std::isnan(err2)))
      {
       // cout<<"Insufficient information for bin  ("<<x<<","<<y<<"): do not include in chi square calculation"<<endl;
        continue;
      }
      
      if (err1 == 0 || err2 == 0) // Should never be the case!
      {
        //cout<<"Insufficient information for bin  ("<<x<<","<<y<<"): do not include in chi square calculation"<<endl;
        continue;
      }
      
      ndf++;
      double numerator = pow((val1 - val2), 2);
      double denominator = pow(err1,2) + pow(err2,2);
      chisq += numerator/denominator;
    }
  }
  return TMath::Prob(chisq, ndf);
}


std::string exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}
