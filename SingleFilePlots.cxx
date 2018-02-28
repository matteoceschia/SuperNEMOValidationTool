int main()
{ 
  return 0;
}

void PrintErr(string error)
{
  cout<<"ERROR: "<<error<<endl;
}

bool MakeSingleFilePlots(string filename)
{
  // Check if we have a valid tree. Don't think there is good ROOT error handling so do it manually
  TFile *f=new TFile( filename.c_str());
  TTree *tree = (TTree*)f->Get("Sensitivity");
  int n_entries=tree->GetEntries();
  if (n_entries <=0)
  {
    PrintErr(filename+" does not contain a Sensitivity ROOT tree with entries");
    return false;
  }
  
  // Reconstruction plots requested by P Guzowski
  
  // number of geiger hits per event
  
  // number of tracks per event
  
  // number of negative-curvature tracks per event
  
  // number of positive-curvature tracks per event
  
  // number of hits per track per event
  
  // number of calo hits per event
  
  // number of associated calo hits per event
  
  // number of unassociated calo hits per event
  
  // sum of associated energy per event
  
  // sum of unassociated energy per event
  
  // maximum time difference between calo hits per event
  
  // hit rate per cell
  
  // hit rate per calo
}
