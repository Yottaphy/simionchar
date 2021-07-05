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

void txt2Root(string filename){
	//Read the non commented lines of the file and create a tree with the relevant variables.
	TFile* f = new TFile((filename+".root").c_str(),"RECREATE");
	TTree* tree = new TTree("T", "");
	tree->ReadFile(filename.c_str(), "n/i:tof/D:x:y:z:vx:vy:vz:ke");
	tree->Write();
	f->Close();
}

vector<float> rootPlotGaus(string filename, string var){	
	//Read tree and plot a new histogram for the variable var
	TTree* tree = new TTree("T", "");
	txt2Root(filename);
	TFile* f = new TFile((filename+".root").c_str(),"UPDATE");
	tree = f->Get<TTree>("T");
	TCanvas *c = new TCanvas("canvas","canvas", 200, 200);
	tree->Draw((var+">>"+var+"_new").c_str(),"","",2000,0);
	
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
	TTree* tree = new TTree("T", "");
	txt2Root(filename);
	TFile* f = new TFile((filename+".root").c_str(),"UPDATE");
	tree = f->Get<TTree>("T");
	TCanvas *c = new TCanvas("canvas","canvas", 200, 200);
	tree->Draw((var2+":"+var1+">>"+var1+"_"+var2).c_str(),"","",2000,0);
	
	//Histogram is taken from tree and plotted in a 2D colour plot, then saved.
	TH2F* histo = new TH2F("histo","histo",2000,0,2000,2000,0,2000);
	histo = f->Get<TH2F>((var1+"_"+var2).c_str());
	histo->SetStats(0);
	histo->Draw("colz");
	c->SaveAs((filename+"_"+var1+"_vs_"+var2+".pdf").c_str());
	histo->Write();
	f->Close();
}

map<string, vector<float>> fitAllVariables(string filename, vector<string> inlist){
	map<string, vector<float>> dic;
	string key;
	for (int i=0; i<inlist.size(); i++){
		key = inlist[i];
		dic.emplace(key , rootPlotGaus(filename, key));
	}
	return dic;
}

void outDic(string filename, map<string, vector<float>> dic, vector<string> inlist){
	//Exports text file with the variables and parameters.
	TFile* f = new TFile((filename.replace(0,4,"results")).c_str(),"a");
	f->Write("\n\nVariable\tPeak maximum\t\tCentroid\t\tSigma\n-------------------------------------------------------------------------");
	for (auto key : dic){		
		f->Write(("\n"+key.first+"\t").c_str());
		for (auto i : key.second){
			f->Write((to_string(key.second[i])+"\t").c_str());
		}
	}
	
	f->Write("\n-------------------------------------------------------------------------\n\n");
	f->Close();
}

int simionchar(){
	//Function takes two arguments, name and variable.
	string filename, dirname;
	ifstream f;

	bool flag = false;
	while (flag == false){
		cout << "Insert filename: ";
		cin >> filename;
		f.open(filename.c_str());
		flag = f.good();
	}

	//Create directory with the same name as the txt file and move the working directory to it
	
	filename = "read_"+filename;

	//List of the variables is fed to the fitAllVariables function, which outputs a dictionary in the form {char(variable): [array of fit parameters]}
	vector<string> vars = {"tof", "x", "y", "z", "vx", "vy", "vz", "ke"};

	map<string, vector<float>>
	dic = fitAllVariables(filename,vars);

	rootPlotCorr(filename, "x", "vx");

	outDic(filename, dic, vars);
	
	return 0;
}
