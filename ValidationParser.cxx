#include "ValidationParser.h"

// Global variables
bool hasConfig=true;
TTree *tree;
map<string,string> configParams;
string plotdir;

/**
 *  main function
 * Arguments are <root file> <config file (optional)>
 */
int main(int argc, char **argv)
{
  gStyle->SetOptStat(0);
  if (argc < 2)
  {
    cout<<"Usage: "<<argv[0]<<" <root file> <config file (optional)>"<<endl;
    return -1;
  }
  string rootFileName (argv[1]);
  if (argc == 2)
  {
    ParseRootFile(rootFileName);
  }
  if (argc >=3)
  {
    string configFileName (argv[2]);
    ParseRootFile(rootFileName,configFileName);
  }
  return 0;
}


/**
 *  Main work function - parses a ROOT file and plots the variables in the branches
 *  rootFileName: path to the ROOT file with SuperNEMO validation data
 *  configFileName: optional to specify how to plot certain variables
 */
void ParseRootFile(string rootFileName, string configFileName)
{
  plotdir = "plots";
  boost::filesystem::path dir(plotdir.c_str());
  if(boost::filesystem::create_directory(dir))
  {
    cout<< "Directory Created: "<<plotdir<<std::endl;
  }
  // Check the root file can be opened and contains a tree with the right name
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
  
  // Get a list of all the branches in the tree
  TObjArray* branches = tree->GetListOfBranches	(		);
  TIter next(branches);
  TBranch *branch;
  
  // Loop the branches and decide how to treat them based on the first character of the name
  while( (branch=(TBranch *)next() )){
    string branchName=branch->GetName();
    PlotVariable(branchName);

  }
  
  if (configFile.is_open()) configFile.close();
  return;
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
    default:
    {
      //cout<< "Unknown variable type "<<branchName<<": treat as histogram"<<endl;
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
  int lowLimit=0;
  int highLimit=notSetVal;
  string title="";
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
  h->GetYaxis()->SetTitle("Events");
  h->GetXaxis()->SetTitle(title.c_str());
  h->SetFillColor(kPink-6);
  h->SetFillStyle(1);
  tree->Draw((branchName + ">> plt_"+branchName).c_str());
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  delete c;
}

/**
 *  Plot a map of the tracker cells
 */
void PlotTrackerMap(string branchName)
{
  int maxlayers=9;
  int maxrows=113;
  
  string config="";
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
  
  // Make the plot
  TCanvas *c = new TCanvas (("plot_"+branchName).c_str(),("plot_"+branchName).c_str(),600,1200);
  TH2I *h = new TH2I(("plt_"+branchName).c_str(),title.c_str(),maxlayers*2,maxlayers*-1,maxlayers,maxrows,0,maxrows); // Map of the tracker
  h->GetYaxis()->SetTitle("Row");
  h->GetXaxis()->SetTitle("Layer");

  tree->Draw(("TMath::Abs("+branchName+"/100) :" + branchName+"%100 >> plt_"+branchName).c_str(),"","COLZ");
  TLine *foil=new TLine(0,0,0,113);
  foil->SetLineColor(kGray);
  foil->SetLineWidth(5);
  foil->Draw("SAME");
  
  
  WriteLabel(.2,.5,"Italy");
  WriteLabel(.7,.5,"France");
  WriteLabel(0.4,.8,"Tunnel");
  WriteLabel(.4,.15,"Mountain");
  
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  delete c;
}

void WriteLabel(double x, double y, string text, bool rotate)
{
  TText *txt = new TText(x,y,text.c_str());
  txt->SetNDC();
  txt->Draw();

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

string BranchNameToEnglish(string branchname)
{
  int pos = branchname.find_first_of("_");
  while (pos >=0)
  {
    branchname.replace(pos,1," ");
    pos = branchname.find_first_of("_");
  }
  string output = branchname.substr(2,branchname.length());
  output[0]=toupper(output[0]);
  return output;
}
