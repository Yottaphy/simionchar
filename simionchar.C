//Short program to characterise the output of a SIMION simulation. Position and velocity in particular. 
//
//
// Jorge Romero 2021 joromero@jyu.fi

//C,C++ Libraries
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>

// ROOT libraries
#include <TH1.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TRandom3.h>
using namespace std;

class fitvar{
	public:
		string name;
		float constant;
		float centroid;
		float sigma;
};

void txt2Root(string filename){
	//Read the non commented lines of the file and create a tree with the relevant variables.

	TFile* f = new TFile((filename+".root").c_str(),"RECREATE");
	TTree* tree = new TTree("T", "");
	tree->ReadFile((filename+".txt").c_str(), "n/i:tof/D:x:y:z:vx:vy:vz:ke");
	tree->Write();
	f->Close();
}

vector<float> rootPlotGaus(string filename, string var){	
	//Read tree and plot a new histogram for the variable var
	txt2Root(filename.c_str());
	TFile* f = new TFile((filename+".root").c_str(),"UPDATE");
	TTree* tree = (TTree*) f->Get("T");
	TCanvas *c = new TCanvas("canvas","canvas", 200, 200);
	tree->Draw((var+">>"+var+"_new").c_str(),"","",50000,0);
	
	//Histogram is taken and fitted with a gaussian, outputting an array with its parameters
	TH1F* histo = new TH1F("histo","histo",2000,0,2000);
	histo = f->Get<TH1F>((var+"_new").c_str());

	TF1* fitgaus = new TF1("GaussianFit", "gaus(0)");
	histo->Fit(fitgaus, "LSQ");

	c->SaveAs((filename+"_"+var+"_fit_gaus.pdf").c_str());
	histo->Write();

	vector<float> params;
	for(int i =0; i<3; i++){
		params.push_back(fitgaus->GetParameter(i)); //Gaussian parameters in order: const (peak counts), mean, sigma
	}
	
	f->Close();	
	return params;
}

void rootPlotCorr(string filename, string var1, string var2){	
	//Read tree and plot a 2D histogram for var1 against var2
	txt2Root(filename.c_str());
	TFile* f = new TFile((filename+".root").c_str(),"READ");
	TTree* tree = (TTree*) f->Get("T");
	TCanvas *c = new TCanvas("canvas","canvas", 150, 100);
	tree->Draw((var2+":"+var1+">>"+var1+"_"+var2).c_str(),"","",50000,0);
	
	//Histogram is taken from tree and plotted in a 2D colour plot, then saved.
	TFile* out = new TFile((filename+"_2d.root").c_str(),"UPDATE");
	TH2F* histo = new TH2F("histo","histo",2000,0,2000,2000,0,2000);
	histo = f->Get<TH2F>((var1+"_"+var2).c_str());
	histo->SetStats(kFALSE);
	histo->Draw("colz");
	histo->GetXaxis()->SetTitle(var1.c_str());
	histo->GetYaxis()->SetTitle(var2.c_str());
	c->SaveAs((filename+"_"+var1+"_vs_"+var2+".pdf").c_str());
	histo->Write();
	out->Close();
	f->Close();
}

void emittancePlot(string filename, string direction, string velocity){	
	//Read tree and plot a 2D histogram for var1 against var2
	string anglestring = direction+"'";
	string angle="atan("+velocity+"/vz)";
	txt2Root(filename.c_str());
	TFile* f = new TFile((filename+".root").c_str(),"READ");
	TTree* tree = (TTree*) f->Get("T");
	// TBranch* branch = tree->Branch(anglestring.c_str(), angle, (anglestring+"/F").c_str());
	TCanvas *c = new TCanvas("canvas","canvas", 150, 100);
	
	tree->Draw((angle+":"+direction+">>"+direction+"_"+anglestring).c_str(),"","",50000,0);
	
	//Histogram is taken from tree and plotted in a 2D colour plot, then saved.
	TFile* out = new TFile((filename+"_2d.root").c_str(),"UPDATE");
	TH2F* histo = new TH2F("histo","histo",2000,0,2000,2000,0,2000);
	histo = f->Get<TH2F>((direction+"_"+anglestring).c_str());
	histo->SetStats(kFALSE);
	histo->Draw();
	histo->SetMarkerColor(kAzure-3);
	histo->GetXaxis()->SetTitle((direction+" [mm]").c_str());
	histo->GetYaxis()->SetTitle((anglestring+ " [mrad]").c_str());
	c->SaveAs((filename+"_"+direction+"emittance.pdf").c_str());
	histo->Write();
	out->Close();
	f->Close();
}

vector<fitvar> fitAllVariables(string filename, vector<string> inlist){
	vector<fitvar> vec;
	vector<float> params;
	fitvar aux;
	
	for (int i=0; i<inlist.size(); i++){
		aux.name = inlist[i];
		params = rootPlotGaus(filename, inlist[i]);
		aux.constant = params[0];
		aux.centroid = params[1];
		aux.sigma	= params[2];
		
		vec.push_back(aux);
	}
	return vec;
}

void outDic(string filename, vector<fitvar> vec){
	//Exports text file with the variables and parameters.
	ofstream f((filename+"_results.txt").c_str());

	f << "\n\nVariable\tPeak maximum\t\tCentroid\t\tSigma\n-------------------------------------------------------------------------\n";
	for (int i = 0; i<vec.size(); i++){
		f << vec[i].name << "\t\t" << vec[i].constant << "\t\t" << vec[i].centroid << "\t\t" << vec[i].sigma << "\n";
	}	
	f << "-------------------------------------------------------------------------\n\n";
	f.close();
}

int simionchar(){
	//Function takes two arguments, name and variable.
	string filename, dirname;
	ifstream f;

	bool flag = false;
	while (flag == false){
		cout << "Insert filename: ";
		filename = "Helium50k.txt";
		f.open(filename.c_str());
		flag = f.good();
	}
	filename.replace(filename.size()-4,4,"");
	gSystem->Exec(("mkdir "+filename).c_str());
	gSystem->Exec(("cp "+filename+".txt "+filename).c_str());
	gSystem->cd(filename.c_str());

	gROOT->SetBatch(kTRUE);

	//List of the variables is fed to the fitAllVariables function, which outputs a dictionary in the form {char(variable): [array of fit parameters]}
	vector<string> vars = {"tof", "x", "y", "z", "vx", "vy", "vz", "ke"};
	//vector<fitvar> dic = fitAllVariables(filename,vars);

	for (int i=0; i<vars.size(); i++){
		for (int j=i+1; j<vars.size()-1; j++){
			//rootPlotCorr(filename, vars[i], vars[j]);
		}
	}
	
	emittancePlot(filename, "x", "vx");
	emittancePlot(filename, "y", "vy");

	//outDic(filename, dic);
	
	f.close();
	gSystem->cd("..");
	return 0;
}

//50k_200b_150s_He.txt
//Helium50k.txt