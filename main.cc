#include <iostream>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <fstream>
#include <ncurses.h>

// CAEN
#include <CAENVMElib.h>
#include <CAENVMEtypes.h>

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
    std::cout<<"Failed to write register "<<hex<<reg<<" handle "<<
      handle<<std::endl;
  return ret;
  
}

int ReadRegister(u_int32_t reg, u_int32_t &val, int handle){
  // This function reads an options from a register
  
  int ret = CAENVME_ReadCycle(handle,gBOARD_VME+reg,
			      &val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    std::cout<<"Failed to read register "<<hex<<reg<<" handle "<<
      handle<<std::endl;
  return ret;
  
}


int main(int argc, char *argv[], char *envp[]){

  // Some ncurses stuff
  initscr();
  cbreak();
  noecho();
  scrollok(stdscr, TRUE);
  nodelay(stdscr, TRUE);

  // loop
  bool gLOOP=true;
  
  // Command line arguments
  int crate=-1;
  int link=-1;
  std::string fOutputFile = "";
  
  int c;
  while( (c = getopt(argc,argv,"c:l:") ) !=-1)    {
    switch(c){
    case 'l':
      link = atoi(optarg);
      break;
    case 'c':
      crate = atoi(optarg);
    case 'f':
      fOutputFile = optarg;
    default:
      break;
    }
  }

  // Initialize output
  std::ofstream outfile;
  if(fOutputFile != "")
    outfile.open(fOutputFile, std::ios::binary);
  
  // Initialize a digitizer
  CVBoardTypes BType = cvV2718;
  int handle = -1; // you address the board with this later
  int cerror;
  if((cerror=CAENVME_Init(BType,link,crate,
			  &handle))!=cvSuccess){
    std::cout<<"Couldn't initialize board. Check cables and drivers. ERR: "<<cerror<<std::endl;
    endwin();
    exit(-1);
  }

  // Write some options to the digitizer
  // RTFM
  int r=0;
  r+= WriteRegister(0xEF24, 0x1, handle);
  r+= WriteRegister(0xEF1C, 0xFF, handle);
  r+= WriteRegister(0xEF00, 0x10, handle);
  r+= WriteRegister(0x8120, 0xFF, handle);
  r+= WriteRegister(0x8000, 0x310, handle);
  r+= WriteRegister(0x8080, 0x1510000, handle);
  r+= WriteRegister(0x800C, 0xA, handle);
  r+= WriteRegister(0x8020, 0x32, handle);
  r+= WriteRegister(0x811C, 0x110, handle);
  r+= WriteRegister(0x8100, 0x0, handle);
  r+= WriteRegister(0x8098, 0x4000, handle); // tune to get baseline where you want
  //r+= WriteRegister(0x810C, 0x80000000, handle);
  if (r!=0){
    std::cout<<"Quitting since registers not written."<<std::endl;
    endwin();
    exit(-1);
  }
  // DAC sometimes slow
  usleep(2e6);
  
  // Start the board and wait
  WriteRegister(0x8100, 0x4, handle);
  usleep(1000);
  u_int64_t dat = 0;
  u_int32_t counter = 0;
  do{
    
    // Read some data from the digitizer and print it
    unsigned int blt_bytes=0, buff_size=8*8388608, blt_size=8*8388608;
    int nb=0,ret=-5;
    u_int32_t *buff = new u_int32_t[buff_size]; //too large is OK
    do{
      // read manual
      ret = CAENVME_FIFOBLTReadCycle(handle,gBOARD_VME,
				     ((unsigned char*)buff)+blt_bytes,
				     blt_size,cvA32_U_BLT,cvD32,&nb);
      if((ret!=cvSuccess) && (ret!=cvBusError)){
	std::cout<<"Board read error: "<<ret<<" for board "<<handle<<std::endl;
	delete[] buff;
	return -1;
      }
      blt_bytes+=nb;
      if(blt_bytes>buff_size)   {
	std::cout<<"Buffer size too small for read!"<<std::endl;
	delete[] buff;
	return -1;
      }
    }while(ret!=cvBusError);
    
    // Print the data to console. See manual for format.
    /*
    if(!gLOOP){
      printw("Here's your data dump: \n");
      for(unsigned int i=0; i<blt_bytes/4; i+=1)
	cout<<hex<<buff[i]<<endl;
    }
    */
    
    dat += blt_bytes;
    counter++;
    if(counter%1000==0){
      std::cout<<"Read out "<<dat/1e6<<" MB data, (q) to quit"<<std::endl;
      dat = 0;
    }
    
    // Write data to file
    if(fOutputFile != ""){
      outfile.write((const char*)buff, blt_bytes);
    }
    delete[] buff;

    if (getch() == 'q') {
      std::cout<<"Bye!"<<std::endl;
      gLOOP=false;
    }
  }while(gLOOP);


    // Bonus points:
    // 1) Plot data
    // 2) Make auto-updating plot (like oscilloscope) for one channel
  // Stop acquisition and close link
  WriteRegister(0x8100, 0x0, handle);
  CAENVME_End(handle);
  if(fOutputFile != "")
    outfile.close();

  endwin();
  exit(0);


}
