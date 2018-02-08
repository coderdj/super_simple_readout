#include <iostream>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <stdint.h>

// CAEN
#include <CAENVMElib.h>
#include <CAENVMEtypes.h>

#include <TApplication.h>
#include <TH1F.h>
#include <TCanvas.h>

// I think it doesn't matter what we put here
// if using optical link. Otherwise you need
// the board VME base address.
#define gBOARD_VME    0x80000000

using namespace std;

int WriteRegister(u_int32_t reg, u_int32_t val, int handle){
  // This function writes a user option to a register
  
  int ret = CAENVME_WriteCycle(handle,gBOARD_VME+reg,
			       &val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    cout<<"Failed to write register "<<hex<<reg<<" with value "<<val<<
      ", returned value: "<<dec<<ret<<endl;
  return ret;
  
}

int ReadRegister(u_int32_t reg, u_int32_t &val, int handle){
  // This function reads an options from a register
  
  int ret = CAENVME_ReadCycle(handle,gBOARD_VME+reg,
			      &val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    cout<<"Failed to read register "<<hex<<reg<<" with return value: "<<
      dec<<ret<<endl;
  return ret;
  
}


int main(int argc, char *argv[], char *envp[]){
  // This is the main function run when you run the program.

  // If you want to plot things then need an TApplication
  TApplication *theApp = new TApplication("App", &argc, argv);
  TCanvas *win = new TCanvas("win", "waveform display", 1);

  // Command line arguments
  int crate=0;
  int link=0;

  // Initialize a digitizer
  CVBoardTypes BType = cvV2718;
  int handle = -1; // you address the board with this later
  int cerror;
  cout<<crate<<link<<endl;
  if((cerror=CAENVME_Init(BType,link,crate,
			  &handle))!=cvSuccess){
    cerr<<"Couldn't initialize board. Check cables and drivers. ERR: "<<
      cerror<<endl;
    exit(-1);
  }

  // Write some options to the digitizer
  // RTFM
  int r=0;
  r+= WriteRegister(0xEF24, 0x1, handle);
  r+= WriteRegister(0xEF1C, 0x1, handle);
  r+= WriteRegister(0xEF00, 0x10, handle);
  r+= WriteRegister(0x8120, 0xFF, handle);
  r+= WriteRegister(0x8000, 0x310, handle);
  r+= WriteRegister(0x8080, 0x310000, handle);
  r+= WriteRegister(0x800C, 0xA, handle);
  r+= WriteRegister(0x8098, 0x1000, handle);
  r+= WriteRegister(0x8020, 0x32, handle);
  r+= WriteRegister(0x811C, 0x110, handle);
  r+= WriteRegister(0x8034, 0x0, handle);
  r+= WriteRegister(0x8060, 0x32, handle);
  r+= WriteRegister(0x8078, 0x19, handle);
  r+= WriteRegister(0x8100, 0x0, handle);
  //r+= WriteRegister(0x810C, 0x80000000, handle);
  if (r!=0){
    cerr<<"Quitting since registers not written."<<endl;
    exit(-1);
  }

  // Start the board and wait
  WriteRegister(0x8100, 0x4, handle);
  usleep(1000);

  char a='r';

  while(a!='q'){
    
    // Read some data from the digitizer and print it
    unsigned int blt_bytes=0, buff_size=10000, blt_size=524288;
    int nb=0,ret=-5;
    u_int32_t *buff = new u_int32_t[buff_size]; //too large is OK
    do{
      // read manual
      ret = CAENVME_FIFOBLTReadCycle(handle,gBOARD_VME,
				     ((unsigned char*)buff)+blt_bytes,
				     blt_size,cvA32_U_BLT,cvD32,&nb);
      if((ret!=cvSuccess) && (ret!=cvBusError)){
	cout<<"Board read error: "<<ret<<" for board "<<handle<<endl;
	delete[] buff;
	return -1;
      }
      blt_bytes+=nb;
      if(blt_bytes>buff_size)   {
	cout<<"Buffer size too small for read!"<<endl;
	delete[] buff;
	return -1;
      }
    }while(ret!=cvBusError);
    
    // Plot
    // Assume channel 0 only one with data
    unsigned int idx = 0;
    vector<u_int16_t> data;
    u_int32_t channelSize=0;
    //u_int32_t channelTime=0;
    while(idx<((blt_bytes/sizeof(u_int32_t))) &&
	  (buff[idx]!=0xFFFFFFFF))   {
      if((buff[idx]>>20) == 0xA00) { //found a header    
	idx+=4;
	channelSize = ((buff)[idx]);
	idx++;
	//channelTime = ((buff)[idx])&0x7FFFFFFF;
	idx++;
	
	for(unsigned int i=0; i<channelSize-2; i++){
	  data.push_back((buff)[i+6]&0x3FFF);
	  data.push_back(((buff)[i+6]>>16)&0x3FFF);
	}
	break;
      }
    }
    TH1F *hist = new TH1F("hist", "hist", data.size(), 0, data.size());
    for(unsigned int i=0; i<data.size(); i++)
      hist->SetBinContent(i, data[i]);
    win->cd();
    hist->Draw();
    win->Update();
    cout<<"Any input to continue, q to quit: "<<endl;
    cin>>a;
  }

  // Stop acquisition and close link
  WriteRegister(0x8100, 0x0, handle);
  CAENVME_End(handle);

  exit(0);


}
