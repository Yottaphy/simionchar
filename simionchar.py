#Short program to characterise the output of a SIMION simulation. Position and velocity in particular. 
#
#
# Jorge Romero 2021 joromero@jyu.fi

from datetime import datetime as dt
import numpy as np
import sys
import ROOT as root
import os

def txt2Root(filename):
	#Read the non commented lines of the file and create a tree with the relevant variables.
	f = root.TFile(filename+".root","RECREATE")
	tree = root.TTree("T", "")
	tree.ReadFile(filename, "n/i:tof/D:x:y:z:vx:vy:vz:ke")
	tree.Write()
	f.Close()

def rootPlotGaus(filename, var):	
	#Read tree and plot a new histogram for the variable var
	txt2Root(filename)	
	f = root.TFile.Open(filename+".root","UPDATE")
	tree = f.Get("T")
	canvas = root.TCanvas("canvas","canvas", 200, 200)
	tree.Draw(var+">>"+var+"_new","","",2000,0)
	
	#Histogram is taken and fitted with a gaussian, outputting an array with its parameters
	histo = root.gDirectory.Get(var+"_new")	
	fitgaus = root.TF1("GaussianFit", "gaus(0)")
	histo.Fit(fitgaus, "LSQ")
	canvas.SaveAs(filename+"_"+var+"_fit_gaus.pdf")
	histo.Write()
	params = []
	for i in range(0,3):
		params.append(fitgaus.GetParameter(i)) #Gaussian parameters in order: const (peak counts), mean, sigma
	f.Close()
	return params

def rootPlotCorr(filename, var1, var2):	
	#Read tree and plot a 2D histogram for var1 against var2
	txt2Root(filename)	
	f = root.TFile.Open(filename+".root","UPDATE")
	tree = f.Get("T")
	canvas = root.TCanvas("canvas","canvas", 200, 200)
	tree.Draw(var2+":"+var1+">>"+var1+"_"+var2,"","",2000,0)
	
	#Histogram is taken from tree and plotted in a 2D colour plot, then saved.
	histo = root.gDirectory.Get(var1+"_"+var2)
	histo.SetStats(0)
	histo.Draw("colz")
	canvas.SaveAs(filename+"_"+var1+"_vs_"+var2+".pdf")
	histo.Write()
	f.Close()

def fitAllVariables(filename,inlist):
	#Loop rootPlot function for all variables, taken from an input list, and output a dictionary in the form {char(variable): [array of fit parameters]}
	dic = {}
	for key in inlist:
		dic[key] = rootPlotGaus(filename, key)
	return dic

def outDic(filename, dic, inlist):
	#Exports text file with the variables and parameters.
	f = open(filename.replace("read","results"),"a")
	f.write(str(dt.now())+"\n\nVariable\tPeak maximum\t\tCentroid\t\tSigma\n-------------------------------------------------------------------------")
	for key in inlist:
		f.write("\n"+str(key)+"\t")
		for i in range(0, len(dic[key])):
			f.write("\t"+str(dic[key][i])+"\t")
	f.write("\n-------------------------------------------------------------------------\n\n")
	f.close()

def main():
	#Function takes two arguments, name and variable.
	filename = sys.argv[1]
	if (os.path.isfile(filename) == False):
		return

	#Create directory with the same name as the txt file and move the working directory to it
	dirname = filename.replace('.txt', '')
	os.system('mkdir '+dirname)
	os.chdir(dirname)

	#Open text file and comment out the first 56 lines (header) of the simion output text file
	os.system('sed -e \'1, 56 s/^/#/\' '+'../'+filename+' > read_'+filename)
	filename = "read_"+filename

	#List of the variables is fed to the fitAllVariables function, which outputs a dictionary in the form {char(variable): [array of fit parameters]}
	listOfVariables = ["tof", "x", "y", "z", "vx", "vy", "vz", "ke"]
	dic = fitAllVariables(filename,listOfVariables)
	rootPlotCorr(filename, "x", "vx")
	outDic(filename, dic, listOfVariables)

if __name__ == "__main__":    
	main()
