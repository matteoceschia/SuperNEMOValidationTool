#include "ValidationParser.h"


int main(int argc, char **argv)
{
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

void ParseRootFile(string rootFileName, string configFileName)
{
  
  // Check the root file can be opened and contains a tree with the right name
  cout<<"Processing "<<rootFileName<<endl;
  TFile *rootFile;
  TTree *tree;

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
  bool hasConfig=true;
  if (configFileName.length()==0)
  {
    cout<<"No config file provided - using default settings"<<endl;
    hasConfig=false;
  }
  else
  {
    cout<<"Using config file "<<configFileName<<endl;
  }
  
  // Get a list of all the branches in the tree
  TObjArray* branches = tree->GetListOfBranches	(		);
  TIter next(branches);
  TBranch *branch;
  
  // Loop the branches and decide how to treat them based on the first character of the name
  while( (branch=(TBranch *)next() )){
    string branchName=branch->GetName();
    switch (branchName[0])
    {
    case 'h':
      {
        cout<< "Histogram variable "<<branchName<<endl;
        break;
      }
    default:
      {
        cout<< "Unknown variable "<<branchName<<": treat as histogram"<<endl;
      }
    }
  }
  return;
}
