//
//
// The MIT License (MIT)
//
// Copyright (c) 2016  Michael J. Wouters
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <cmath>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include "Application.h"
#include "Antenna.h"
#include "CGGTTS.h"
#include "Counter.h"
#include "CounterMeasurement.h"
#include "Debug.h"
#include "GPS.h"
#include "MeasurementPair.h"
#include "Receiver.h"
#include "ReceiverMeasurement.h"
#include "Utility.h"

extern Application *app;
extern ostream *debugStream;

#define NTRACKS 89
#define MAXSV   32 // per constellation 

//
//	Public members
//

CGGTTS::CGGTTS(Antenna *a,Counter *c,Receiver *r)
{
	ant=a;
	cntr=c;
	rx=r;
	init();
}

bool CGGTTS::writeObservationFile(string fname,int mjd,int startTime,int stopTime,MeasurementPair **mpairs,bool TICenabled)
{
	FILE *fout;
	if (!(fout = fopen(fname.c_str(),"w"))){
		cerr << "Unable to open " << fname << endl;
		return false;
	}
	
	app->logMessage("generating CGGTTS file for " + boost::lexical_cast<string>(mjd));
	
	double measDelay = rx->ppsOffset + intDly + cabDly - refDly; // the measurement system delay to be subtracted from REFSV and REFSYS
	int useTIC = (TICenabled?1:0);
	DBGMSG(debugStream,1,"Using TIC = " << (TICenabled? "yes":"no"));
	writeHeader(fout);
	
	int lowElevationCnt=0; // a few diagnostics
	int highDSGCnt=0;
	int shortTrackCnt=0;
	int goodTrackCnt=0;
	int ephemerisMisses=0;
	int pseudoRangeFailures=0;
	int badMeasurementCnt=0;
	
	// Constellation/code identifiers as per V2E
	
	string GNSSsys;
	string GNSScode;
	
	switch (code){
		case GNSSSystem::C1:GNSScode="L1C";break;
		case GNSSSystem::P1:GNSScode="L1P";break;
		case GNSSSystem::P2:GNSScode="L2P";break;
		default:break;
	}
			
	switch (constellation){
		case GNSSSystem::BEIDOU:
			GNSSsys="C"; break;
		case GNSSSystem::GALILEO:
			GNSSsys="E"; break;
		case GNSSSystem::GLONASS:
			GNSSsys="R"; break;
		case GNSSSystem::GPS:
			GNSSsys="G";
		default:break;
	}
	
	if (0){
		
		FILE *foutdbg;
		fname += ".dbg";
		if (!(foutdbg = fopen(fname.c_str(),"w"))){
			cerr << "Unable to open " << fname << endl;
			return false;
		}
		writeHeader(foutdbg);
		// Don't be fancy - no ordering
		char sout[155];
		for (int m=startTime;m<=stopTime;m++){	
			if ((mpairs[m]->flags==0x03)){
				ReceiverMeasurement *rm = mpairs[m]->rm;
				int tmeas=rint(rm->tmUTC.tm_sec + rm->tmUTC.tm_min*60+ rm->tmUTC.tm_hour*3600+rm->tmfracs);
				int hh = tmeas / 3600;
				int mm = (tmeas - hh*3600)/60;
				int ss = tmeas - hh*3600 - mm*60;
				if (tmeas % 30 !=0) continue;
				for (unsigned int sv=0;sv<rm->meas.size();sv++){
					SVMeasurement * svm = rm->meas.at(sv);
					if (svm->constellation == constellation && svm->code == code){
						
						GPS::EphemerisData *ed=NULL;
						ed = rx->gps.nearestEphemeris(svm->svn,rm->gpstow,maxURA);
						if (NULL == ed) ephemerisMisses++;
						double refsyscorr,refsvcorr,iono,tropo,az,el,refpps,refsv,refsys;
						int ioe;
						// FIXME MDIO needs to change for L2
						// getPseudorangeCorrections will check for NULL ephemeris
						if (rx->gps.getPseudorangeCorrections(rm->gpstow,svm->meas,ant,ed,code,&refsyscorr,&refsvcorr,&iono,&tropo,&az,&el,&ioe)){
							refpps= useTIC*(rm->cm->rdg + rm->sawtooth)*1.0E9;
							refsv = svm->meas*1.0E9 + refsvcorr  - iono - tropo + refpps;
							refsys =svm->meas*1.0E9 + refsyscorr - iono - tropo + refpps;
							
							el=rint(el*10);
							az=rint(az*10);
							refsv=rint((refsv-measDelay)*10); // apply total measurement system delay
							refsys=rint((refsys-measDelay)*10); // apply total measurement system delay
							iono=rint(iono*10);
							tropo=rint(tropo*10);
							
							if (el >= minElevation*10 ){
								goodTrackCnt++;
								
								snprintf(sout,154,"%s%02i %2s %5i %02i%02i%02i %4i %3i %4i %11i %6i %11i %6i %4i %3i %4i %4i %4i %4i %2i %2i %3s ",
									GNSSsys.c_str(),svm->svn,"FF",mjd,hh,mm,ss,1,(int) el,(int) az,(int) refsv,0, (int) refsys, 0, 0, ioe, (int) tropo, 0, (int) iono, 0,0,0,GNSScode.c_str());
								fprintf(foutdbg,"%s%02X\n",sout,checkSum(sout) % 256);
							}
							else{
								if (el < minElevation*10) lowElevationCnt++;
							}
						
						}
						else{
							pseudoRangeFailures++;
						}
						
					}	
				}
			} 
		}
		
// 		app->logMessage("Ephemeris search misses: " + boost::lexical_cast<string>(ephemerisMisses));
// 		app->logMessage("Pseudorange calculation failures: " + boost::lexical_cast<string>(pseudoRangeFailures-ephemerisMisses) );
// 		app->logMessage("Bad measurements: " + boost::lexical_cast<string>(badMeasurementCnt) );
// 		
// 		app->logMessage(boost::lexical_cast<string>(goodTrackCnt) + " good measurements");
// 		app->logMessage(boost::lexical_cast<string>(lowElevationCnt) + " low elevation measurements");
// 		
		
	}
	
	// Generate the observation schedule as per DefraignePetit2015 pg3

	int schedule[NTRACKS+1];
	int ntracks=NTRACKS;
	// There will be a 28 minute gap between two observations (32-4 mins)
	// which means that you can't just find the first and then add n*16 minutes
	for (int i=0,mins=2; i<NTRACKS; i++,mins+=16){
		schedule[i]=mins-4*(mjd-50722);
		if (schedule[i] < 0){ // always negative in practice anyway 
			int ndays = abs(schedule[i]/1436) + 1;
			schedule[i] += ndays*1436;
		}
	}
	
	// The schedule is not in ascending order so fix this 
	std::sort(schedule,schedule+NTRACKS); // don't include the last element, which may or may not be used
	
	// Fixup - one more track possibly at the end of the day
	// Will need the next day's data to use this properly though
	if ((schedule[NTRACKS-1]%60) < 43){
		schedule[NTRACKS]=schedule[NTRACKS-1]+16;
		ntracks++;
	}

	switch (ver){
		case V1: quadFits=true;break;
		case V2E:quadFits=false;break;
		default:quadFits=false;
	}
	
	// Use a fixed array of vectors so that we can use the index as a hash for the SVN. Memory is cheap
	vector<SVMeasurement *> svtrk[MAXSV+1];
	
	for (int i=0;i<ntracks;i++){
		int trackStart = schedule[i]*60;
		int trackStop =  schedule[i]*60+780-1;
		if (trackStop >= 86400) trackStop=86400-1;
		// Now window it
		if (trackStart < startTime || trackStart > stopTime) continue;
		// Matched measurement pairs can be looked up without a search since the index is TOD
		for (int m=trackStart;m<=trackStop;m++){
			
			if ((mpairs[m]->flags==0x03)){
				ReceiverMeasurement *rm = mpairs[m]->rm;
				for (unsigned int sv=0;sv<rm->meas.size();sv++){
					SVMeasurement * svm = rm->meas.at(sv);
					if (svm->constellation == constellation && svm->code == code)
						svtrk[svm->svn].push_back(svm);
				}
			} 
		}
		
		int hh = schedule[i] / 60;
		int mm = schedule[i] % 60;
		
		 //use arrays which can store the 15s quadratic fits and 30s decimated data
		double refsv[52],refsys[52],mdtr[52],mdio[52],tutc[52],svaz[52],svel[52];
		
		int linFitInterval=30; // length of fitting interval 
		if (quadFits) linFitInterval=15;
		
		for (unsigned int sv=1;sv<=MAXSV;sv++){
			if (svtrk[sv].size() == 0 ) continue;
			
			int npts=0;
			int ioe;
			if (quadFits){
				double qprange[15],qtutc[15],qrefpps[15]; // for the 15s fits
				double uncorrprange[52], refpps[52]; // for the results of the 15s fits
				unsigned int nqfitpts=0,nqfits=0,isv=0,gpsTOW[52];
				int t=trackStart;
				while (t<=trackStop){
					ReceiverMeasurement *rxm = svtrk[sv].at(isv)->rm;
					int tmeas=rint(rxm->tmUTC.tm_sec + rxm->tmUTC.tm_min*60+ rxm->tmUTC.tm_hour*3600+rxm->tmfracs); // tmfracs is set to zero by interpolateMeasurements()
					if (t==tmeas){
						// FIXME MDIO needs to change for L2
						if (nqfitpts > 14){ // shouldn't happen
							cerr << "Error in CGGTTS::writeObservationFile() - nqfits too big" << endl;
							exit(EXIT_FAILURE);
						}
						// smooth the counter measurements - this helps clean up any residual sawtooth error
						qrefpps[nqfitpts]= useTIC*(rxm->cm->rdg + rxm->sawtooth)*1.0E9;
						qprange[nqfitpts]=svtrk[sv].at(isv)->meas;
						qtutc[nqfitpts]=tmeas;
						nqfitpts++;
						t++;
						isv++;
					}
					else if (t<tmeas){
						t++;
						// don't increment isv - have to retest
					}
					else{ // t > tmeas - shouldn't happen
						cerr << "Error in CGGTTS::writeObservationFile() - unexpected tmeas (t=" << t<< ",tmeas=" << tmeas<< endl;
						exit(EXIT_FAILURE);
					}
					
					if (((t-trackStart) % 15 == 0) || (isv == svtrk[sv].size())){ // have got a full set of points for a quadratic fit
						//DBGMSG(debugStream,1,sv << " " << trackStart << " " << nqfitpts << " " << nqfits);
						// Sanity checks
						if (nqfits > 51){// shouldn't happen
							cerr << "Error in CGGTTS::writeObservationFile() - nqfits too big" << endl;
							exit(EXIT_FAILURE);
						}
						
						if (nqfitpts > 7){ // demand at least half a track - then we are not extrapolating
							double tc=(t-1)-7; // subtract 1 because we've gone one too far
							tutc[nqfits] = tc;
							// Compute and save GPS TOW so that we have it available for computing the pseudorange corrections
							// FIXME This does not handle the week rollover 
							unsigned int gpsDay = (rxm->gpstow / 86400); // use the last receiver measurement for day number
							unsigned int TOD = tc+rx->leapsecs;
							if (TOD >= 86400){
								TOD -= 86400;
								gpsDay++;
								if (gpsDay == 7){
									gpsDay=0;
								}
							}
							// FIXME as a kludge could just drop points at the week rollover
							gpsTOW[nqfits] =  tc + rx->leapsecs + gpsDay*86400;
							Utility::quadFit(qtutc,qprange,nqfitpts,tc,&(uncorrprange[nqfits]) );
							Utility::quadFit(qtutc,qrefpps,nqfitpts,tc,&(refpps[nqfits]) );
							nqfits++;
						}
						nqfitpts=0;
					} // 
					
					if (isv == svtrk[sv].size()) break;  // no more measurements available
				}
				// Now we can compute the pr corrections etc for the fitted prs

				GPS::EphemerisData *ed=NULL;
				npts=0;
				for ( unsigned int q=0;q<nqfits;q++){
					if (ed==NULL) // use only one ephemeris for each track
							ed = rx->gps.nearestEphemeris(sv,gpsTOW[q],maxURA);
					if (NULL == ed) ephemerisMisses++;
					
					double refsyscorr,refsvcorr,iono,tropo,az,el;
					// FIXME MDIO needs to change for L2
					// getPseudorangeCorrections will check for NULL ephemeris
					if (rx->gps.getPseudorangeCorrections(gpsTOW[q],uncorrprange[q],ant,ed,code,
							&refsyscorr,&refsvcorr,&iono,&tropo,&az,&el,&ioe)){
						tutc[npts]=tutc[q]; // ok to overwrite, because npts <= q
						svaz[npts]=az;
						svel[npts]=el;
						mdtr[npts]=tropo;
						mdio[npts]=iono;
						refsv[npts]  = uncorrprange[q]*1.0E9 + refsvcorr  - iono - tropo + refpps[q];
						refsys[npts] = uncorrprange[q]*1.0E9 + refsyscorr - iono - tropo + refpps[q];
						npts++;
					}
					else{
						pseudoRangeFailures++;
					}
				}
			}                                 
			else{ // v2E specifies 30s sampled values 
				int tsearch=trackStart;
				int t=0;
				
				GPS::EphemerisData *ed=NULL;
				while (t< (int) svtrk[sv].size()){
					svtrk[sv].at(t)->dbuf2=0.0;
					ReceiverMeasurement *rxmt = svtrk[sv].at(t)->rm;
					int tmeas=rint(rxmt->tmUTC.tm_sec + rxmt->tmUTC.tm_min*60+ rxmt->tmUTC.tm_hour*3600+rxmt->tmfracs);
					if (tmeas==tsearch){
						if (ed==NULL) // use only one ephemeris for each track
							ed = rx->gps.nearestEphemeris(sv,rxmt->gpstow,maxURA);
						if (NULL == ed) ephemerisMisses++;
						double refsyscorr,refsvcorr,iono,tropo,az,el,refpps;
						// FIXME MDIO needs to change for L2
						// getPseudorangeCorrections will check for NULL ephemeris
						if (rx->gps.getPseudorangeCorrections(rxmt->gpstow,svtrk[sv].at(t)->meas,ant,ed,code,&refsyscorr,&refsvcorr,&iono,&tropo,&az,&el,&ioe)){
							tutc[npts]=tmeas;
							svaz[npts]=az;
							svel[npts]=el;
							mdtr[npts]=tropo;
							mdio[npts]=iono;
							refpps= useTIC*(rxmt->cm->rdg + rxmt->sawtooth)*1.0E9;
							refsv[npts]  = svtrk[sv].at(t)->meas*1.0E9 + refsvcorr  - iono - tropo + refpps;
							refsys[npts] = svtrk[sv].at(t)->meas*1.0E9 + refsyscorr - iono - tropo + refpps;
							svtrk[sv].at(t)->dbuf2 = refsv[npts]/1.0E9; // back to seconds !
							npts++;
						}
						else{
							pseudoRangeFailures++;
						}
						tsearch += 30;
						t++;
					}
					else if (tmeas > tsearch){
						tsearch += 30;
						// don't increment t because this measurement must be re-tested	
					}
					else{
						t++;
					}
				}
			} // else quadfits
			
			if (npts*linFitInterval >= minTrackLength){
				double tc=(trackStart+trackStop)/2.0; // FIXME may need to add MJD to allow rollovers
				
				double aztc,azc,azm,azresid;
				Utility::linearFit(tutc,svaz,npts,tc,&aztc,&azc,&azm,&azresid);
				aztc=rint(aztc*10);
				
				double eltc,elc,elm,elresid;
				Utility::linearFit(tutc,svel,npts,tc,&eltc,&elc,&elm,&elresid);
				eltc=rint(eltc*10);
				
				double mdtrtc,mdtrc,mdtrm,mdtrresid;
				Utility::linearFit(tutc,mdtr,npts,tc,&mdtrtc,&mdtrc,&mdtrm,&mdtrresid);
				mdtrtc=rint(mdtrtc*10);
				mdtrm=rint(mdtrm*10000);
				
				double refsvtc,refsvc,refsvm,refsvresid;
				Utility::linearFit(tutc,refsv,npts,tc,&refsvtc,&refsvc,&refsvm,&refsvresid);
				refsvtc=rint((refsvtc-measDelay)*10); // apply total measurement system delay
				refsvm=rint(refsvm*10000);
				
				double refsystc,refsysc,refsysm,refsysresid;
				Utility::linearFit(tutc,refsys,npts,tc,&refsystc,&refsysc,&refsysm,&refsysresid);
				refsystc=rint((refsystc-measDelay)*10); // apply total measurement system delay
				refsysm=rint(refsysm*10000);
				refsysresid=rint(refsysresid*10);
				
				double mdiotc,mdioc,mdiom,mdioresid;
				Utility::linearFit(tutc,mdio,npts,tc,&mdiotc,&mdtrc,&mdiom,&mdioresid);
				mdiotc=rint(mdiotc*10);
				mdiom=rint(mdiom*10000);
				
				// Some range checks on the data - flag bad measurements
				if (refsvm >  99999) refsvm=99999;
				if (refsvm < -99999) refsvm=-99999;
				
				if (refsysm >  99999) refsysm=99999;
				if (refsysm < -99999) refsysm=-99999;
				
				if (refsysresid > 999.9) refsysresid = 999.9;
				
				if (fabs(refsvm)==99999 || fabs(refsysm) == 99999 || refsysresid == 999.9)
					badMeasurementCnt++;
				
				// Ready to output
				if (eltc >= minElevation*10 && refsysresid <= maxDSG*10){ 
					char sout[155]; // V2E
					goodTrackCnt++;
					switch (ver){
						case V1:
							snprintf(sout,128," %02i %2s %5i %02i%02i00 %4i %3i %4i %11i %6i %11i %6i %4i %3i %4i %4i %4i %4i ",sv,"FF",mjd,hh,mm,
											npts*linFitInterval,(int) eltc,(int) aztc, (int) refsvtc ,(int) refsvm,(int)refsystc,(int) refsysm,(int) refsysresid,
											ioe,(int) mdtrtc, (int) mdtrm, (int) mdiotc, (int) mdiom);
							fprintf(fout,"%s%02X\n",sout,checkSum(sout) % 256);
							break;
						case V2E:
							snprintf(sout,154,"%s%02i %2s %5i %02i%02i00 %4i %3i %4i %11i %6i %11i %6i %4i %3i %4i %4i %4i %4i %2i %2i %3s ",GNSSsys.c_str(),sv,"FF",mjd,hh,mm,
											npts*linFitInterval,(int) eltc,(int) aztc, (int) refsvtc,(int) refsvm,(int)refsystc,(int) refsysm,(int) refsysresid,
											ioe,(int) mdtrtc, (int) mdtrm, (int) mdiotc, (int) mdiom,0,0,GNSScode.c_str());
							fprintf(fout,"%s%02X\n",sout,checkSum(sout) % 256); // FIXME
							break;
					} // switch
				} // if (eltc >= minElevation*10 && refsysresid <= maxDSG*10)
				else{
					if (eltc < minElevation*10) lowElevationCnt++;
					if (refsysresid > maxDSG*10) highDSGCnt++;
				}
			} // if (npts*linFitInterval >= minTrackLength)
			else{
				shortTrackCnt++;
			}
		}
	
		for (unsigned int sv=1;sv<=MAXSV;sv++)
			svtrk[sv].clear();
	} // for (int i=0;i<ntracks;i++){
	
	app->logMessage("Ephemeris search misses: " + boost::lexical_cast<string>(ephemerisMisses));
	app->logMessage("Pseudorange calculation failures: " + boost::lexical_cast<string>(pseudoRangeFailures-ephemerisMisses) );
	app->logMessage("Bad measurements: " + boost::lexical_cast<string>(badMeasurementCnt) );
	
	app->logMessage(boost::lexical_cast<string>(goodTrackCnt) + " good tracks");
	app->logMessage(boost::lexical_cast<string>(lowElevationCnt) + " low elevation tracks");
	app->logMessage(boost::lexical_cast<string>(highDSGCnt) + " high DSG tracks");
	app->logMessage(boost::lexical_cast<string>(shortTrackCnt) + " short tracks");
	
	fclose(fout);
	
	return true;
}
 
//
//	Private members
//		

void CGGTTS::init()
{
	revDateYYYY=2016;
	revDateMM=1;
	revDateDD=1;
	ref="";
	lab="";
	comment="";
	calID="";
	intDly=cabDly=refDly=0.0;
	delayKind=INTDLY;
	quadFits=false;
	minTrackLength=390;
	minElevation=10.0;
	maxDSG=100.0;
	maxURA=3.0; // as reported by receivers, typically 2.0 m, with a few at 2.8 m
}
		
void CGGTTS::writeHeader(FILE *fout)
{
#define MAXCHARS 128
	int cksum=0;
	char buf[MAXCHARS+1];
	
	// NB maximum line length is 128 columns
	switch (ver){
		case V1:
			strncpy(buf,"GGTTS GPS DATA FORMAT VERSION = 01",MAXCHARS);
			break;
		case V2E:
			strncpy(buf,"CGGTTS     GENERIC DATA FORMAT VERSION = 2E",MAXCHARS);
			break;
	}
	
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"REV DATE = %4d-%02d-%02d",revDateYYYY,revDateMM,revDateDD); 
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"RCVR = %s %s %s %4d %s,v%s",rx->manufacturer.c_str(),rx->modelName.c_str(),rx->serialNumber.c_str(),
		rx->commissionYYYY,APP_NAME, APP_VERSION);
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"CH = %02d",rx->channels); 
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"IMS = 99999"); // FIXME dual frequency 
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"LAB = %s",lab.c_str()); 
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"X = %+.3f m",ant->x);
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"Y = %+.3f m",ant->y);
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"Z = %+.3f m",ant->z);
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"FRAME = %s",ant->frame.c_str());
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	if (comment == "") 
		comment="NO COMMENT";
	
	snprintf(buf,MAXCHARS,"COMMENTS = %s",comment.c_str());
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	switch (ver){
		case V1:
		{
			snprintf(buf,MAXCHARS,"INT DLY = %.1f ns",intDly);
			break;
		}
		case V2E:
		{
			string cons;
			switch (constellation){
				case GNSSSystem::BEIDOU:cons="BDS";break;
				case GNSSSystem::GALILEO:cons="GAL";break;
				case GNSSSystem::GLONASS:cons="GLO";break;
				case GNSSSystem::GPS:cons="GPS";break;
			}
			string code1;
			switch (code){
				// FIXME check if BeiDou etc need a different name for code1
				case GNSSSystem::C1:code1="C1";break;
				case GNSSSystem::P1:code1="P1";break;
				case GNSSSystem::P2:code1="P2";break;
			}
			string dly;
			switch (delayKind){
				case INTDLY:dly="INT";break;
				case SYSDLY:dly="SYS";break;
				case TOTDLY:dly="TOT";break;
			}
			snprintf(buf,MAXCHARS,"%s DLY = %.1f ns (%s %s)     CAL_ID = %s",dly.c_str(),intDly,cons.c_str(),code1.c_str(),calID.c_str());
			break;
		}
	}
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	if (delayKind == INTDLY){
		snprintf(buf,MAXCHARS,"CAB DLY = %.1f ns",cabDly);
		cksum += checkSum(buf);
		fprintf(fout,"%s\n",buf);
	}
	
	if (delayKind != TOTDLY){
		snprintf(buf,MAXCHARS,"REF DLY = %.1f ns",refDly);
		cksum += checkSum(buf);
		fprintf(fout,"%s\n",buf);
	}
	
	snprintf(buf,MAXCHARS,"REF = %s",ref.c_str());
	cksum += checkSum(buf);
	fprintf(fout,"%s\n",buf);
	
	snprintf(buf,MAXCHARS,"CKSUM = ");
	cksum += checkSum(buf);
	fprintf(fout,"%s%02X\n",buf,cksum % 256);
	
	fprintf(fout,"\n");
	switch (ver){
		case V1:
			fprintf(fout,"PRN CL  MJD  STTIME TRKL ELV AZTH   REFSV      SRSV     REFGPS    SRGPS  DSG IOE MDTR SMDT MDIO SMDI CK\n");
			fprintf(fout,"             hhmmss  s  .1dg .1dg    .1ns     .1ps/s     .1ns    .1ps/s .1ns     .1ns.1ps/s.1ns.1ps/s  \n");
			break;
		case V2E:
			fprintf(fout,"SAT CL  MJD  STTIME TRKL ELV AZTH   REFSV      SRSV     REFSYS    SRSYS  DSG IOE MDTR SMDT MDIO SMDI FR HC FRC CK\n");
			fprintf(fout,"             hhmmss  s  .1dg .1dg    .1ns     .1ps/s     .1ns    .1ps/s .1ns     .1ns.1ps/s.1ns.1ps/s            \n");
			break;
	}
	
	fflush(fout);
	
#undef MAXCHARS	
}
	

int CGGTTS::checkSum(char *l)
{
	int cksum =0;
	for (unsigned int i=0;i<strlen(l);i++)
		cksum += (int) l[i];
	return cksum;
}

#undef NTRACKS

