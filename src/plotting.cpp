/***************************************************************************
                       plotting.cpp  -  GDL routines for plotting
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002-2011 by Marc Schellens et al.
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"
#include "dinterpreter.hpp"
#include "plotting.hpp"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace lib
{

  using namespace std;
//  using std::isinf;
#ifdef _MSC_VER
#define finite _finite
#else
  using std::isnan;
#endif
  
//static values
  static DDouble savedPointX=0.0;
  static DDouble savedPointY=0.0;
  static gdlSavebox saveBox;
  static DFloat sym1x[5]={1, -1, 0, 0, 0}; // +
  static DFloat sym1y[5]={0, 0, 0, -1, 1}; // +
  static DFloat sym2x[11]= {1, -1, 0, 0, 0, 0,1,-1,0,1,-1}; //*
  static DFloat sym2y[11]= {0, 0, 0, -1, 1,0,1,-1,0,-1,1}; // *
  static DFloat sym3x[2]={0,0}; // .
  static DFloat sym3y[2]={0,0}; // .
  static DFloat sym4x[5]={ 0, 1, 0, -1, 0 }; //diamond.
  static DFloat sym4y[5]={ 1, 0, -1, 0, 1 }; //diamond.
  static DFloat sym5x[4]={ -1, 0, 1, -1 }; // triangle up.
  static DFloat sym5y[4]={ -1, 1, -1, -1 }; // triangle up.
  static DFloat sym6x[5]={ -1, 1, 1, -1, -1 }; //square
  static DFloat sym6y[5]={ 1, 1, -1, -1, 1 }; //square
  static DFloat sym7x[5]= {1,-1,0,1,-1}; //x
  static DFloat sym7y[5]= {1,-1,0,-1,1}; //x
  DLong syml[7]={5,11,2,5,4,5,5};

  struct LOCALUSYM {
    DLong nusym;
    DInt fill;
    DFloat usymx[49];
    DFloat usymy[49];
    bool hasColor;
    DLong color;
    bool hasThick;
    DFloat thick;
  };
  static LOCALUSYM localusym;
  
  gdlSavebox* getSaveBox(){return &saveBox;} 

  void gdlDoRangeExtrema(DDoubleGDL *xVal, DDoubleGDL *yVal, DDouble &min, DDouble &max, DDouble xmin, DDouble xmax, bool doMinMax, DDouble minVal, DDouble maxVal)
  {
    DDouble valx, valy;
    SizeT i,k;
    DLong n=xVal->N_Elements();
    if(n!=yVal->N_Elements()) return;
    for (i=0,k=0 ; i<n ; ++i)
    {
       //look only in range x=[xmin,xmax]
       valx=(*xVal)[i];
       if ( (valx<xmin || valx>xmax || isnan(valx))) continue;
       else {
       //min and max of y if not NaN and in range [minVal, maxVal] if doMinMax=yes (min_value, max_value keywords)
       valy=(*yVal)[i];
       if ((doMinMax && (valy<minVal || valy>maxVal )) || isnan(valy)) continue;
       else {if(k==0) {min=valy; max=valy;} else {min=gdlPlot_Min(min,valy); max=gdlPlot_Max(max,valy);}}
       }
       k++;
    }
  }

  void GetMinMaxVal(DDoubleGDL* val, double* minVal, double* maxVal)
  {
#define UNDEF_RANGE_VALUE 1E-12
    DLong minE, maxE;
    const bool omitNaN=true;
    val->MinMax(&minE, &maxE, NULL, NULL, omitNaN);
    if ( minVal!=NULL ) {
       *minVal=(*val)[ minE];
       if (isnan(*minVal)) *minVal = UNDEF_RANGE_VALUE;
    }
    if ( maxVal!=NULL ) {
      *maxVal=(*val)[ maxE];
       if (isnan(*maxVal)) *maxVal = 1.0;
    }
    if ((*maxVal)==(*minVal)) *maxVal=*minVal+1.0;
#undef UNDEF_RANGE_VALUE
  }
  
  void GetMinMaxValuesForSubset(DDoubleGDL* val, DDouble &minVal, DDouble &maxVal, SizeT FinalElement)
  {
#define UNDEF_RANGE_VALUE 1E-12
    DLong minE, maxE;
    const bool omitNaN=true;
    val->MinMax(&minE, &maxE, NULL, NULL, omitNaN, 0, FinalElement);
    minVal=(*val)[ minE];
    if (isnan(minVal)) minVal = UNDEF_RANGE_VALUE;
    maxVal=(*val)[ maxE];
    if (isnan(maxVal)) maxVal = 1.0;
    if (maxVal==minVal) maxVal=minVal+1.0;
#undef UNDEF_RANGE_VALUE
  }

  PLFLT AutoTick(DDouble x)
  {
    if ( x==0.0 ) return 1.0;

    DLong n=static_cast<DLong>(floor(log10(x/3.5)));
    DDouble y=(x/(3.5*pow(10., static_cast<double>(n))));
    DLong m=0;
    if ( y>=1&&y<2 )
      m=1;
    else if ( y>=2&&y<5 )
      m=2;
    else if ( y>=5 )
      m=5;

    PLFLT intv=(PLFLT)(m*pow(10., static_cast<double>(n)));
    return intv;
  }

  PLFLT AutoIntv(DDouble x)
  {
    if ( x==0.0 )
    {
      //      cout << "zero"<<endl;
      return 1.0;
    }

    DLong n=static_cast<DLong>(floor(log10(x/2.82)));
    DDouble y=(x/(2.82*pow(10., static_cast<double>(n))));
    DLong m=0;
    if ( y>=1&&y<2 )
      m=1;
    else if ( y>=2&&y<4.47 )
      m=2;
    else if ( y>=4.47 )
      m=5;

    //    cout << "AutoIntv" << x << " " << y << endl;

    PLFLT intv=(PLFLT)(m*pow(10., static_cast<double>(n)));
    return intv;
  }



 #define EXTENDED_DEFAULT_LOGRANGE 12
  //protect from (inverted, strange) axis log values
  void gdlHandleUnwantedAxisValue(DDouble &min, DDouble &max, bool log)
  {
    bool invert=FALSE;
    DDouble val_min, val_max;
    if (!log) return;

    if(max-min >= 0)
    {
      val_min=min;
      val_max=max;
      invert=FALSE;
    } else {
      val_min=max;
      val_max=min;
      invert=TRUE;
    }

    if ( val_min<=0. )
    {
      if ( val_max<=0. )
      {
        val_min=-EXTENDED_DEFAULT_LOGRANGE;
        val_max=0.0;
      }
      else
      {
        val_min=log10(val_max)-EXTENDED_DEFAULT_LOGRANGE;
        val_max=log10(val_max);
      }
    }
    else
    {
      val_min=log10(val_min);
      val_max=log10(val_max);
    }
    if (invert)
    {
      min=pow(10.,val_max);
      max=pow(10.,val_min);
    } else {
      min=pow(10.,val_min);
      max=pow(10.,val_max);
    }

  }
#undef EXTENDED_DEFAULT_LOGRANGE

   //improved version of "AutoIntv" for:
  // 1/ better managing ranges when all the data have same value
  // 2/ mimic IDL behavior when data are all positive
  // please notice that (val_min, val_max) will be changed
  // and "epsilon" is a coefficient if "extended range" is expected
  // input: linear min and max, output: linear min and max.

  // NOTE GD: this function should be rewritten, documented and tested correctly. Most often, the
  // plots are not exactly what IDL does in the same conditions. The reasons for the choices should be
  // clearly described in the code, to be checked by others.
  
  PLFLT gdlAdjustAxisRange(EnvT* e, int axisId, DDouble &start, DDouble &end, bool log /* = false */, int code /* = 0 */) {
    gdlHandleUnwantedAxisValue(start, end, log);

    DDouble min, max;
    bool invert = FALSE;

    if (end - start >= 0) {
      min = start;
      max = end;
      invert = FALSE;
    } else { //never happens by construction!!!!!!!!!!!!!!!!!!!!!!!
      min = end;
      max = start;
      invert = TRUE;
    }

    PLFLT intv = 1.;
    int cas = 0;
    DDouble x;
    bool debug = false;
    if (debug) {
      cout << "init: " << min << " " << max << endl;
    }
    // case "all below ABS((MACHAR()).xmin)"
    //needs example.
    
//    if (!log && (abs(max) <= -std::numeric_limits<DDouble>::max())) {
//      min = DDouble(0.);
//      max = DDouble(1.);
//      intv = (PLFLT) (2.);
//      cas = 1;
//    }

    if (log) {
      min = log10(min);
      max = log10(max);
    }

    // case "all values are equal"
    if (cas == 0) {
      x = max - min;
      if (abs(x) <= std::numeric_limits<DDouble>::min()) {
        DDouble val_ref;
        val_ref = max;
        if (0.98 * min < val_ref) { // positive case
          max = 1.02 * val_ref;
          min = 0.98 * val_ref;
        } else { // negative case
          max = 0.98 * val_ref;
          min = 1.02 * val_ref;
        }
        if (debug) {
          cout << "Rescale : " << min << " " << max << endl;
        }
        }
      }

    // general case (only negative OR negative and positive)
    if (cas == 0) //rounding is not aka idl due to use of ceil and floor. TBD.
    {
      x = max - min;
      //correct this for calendar values (round to nearest year, month, etc)
      if ( code > 0) {
        if (code ==7 ) {
              if(x>=366)  code=1;
              else if(x>=32)  code=2;
              else if(x>=1.1)  code=3;
              else if(x*24>=1.1)  code=4;
              else if(x*24*60>=1.1)  code=5;
              else code=6;
        }
        static int monthSize[]={31,28,31,30,31,30,31,31,30,31,30,31};
        DLong Day1,Day2 , Year1,Year2 , Hour1,Hour2 , Minute1,Minute2, MonthNum1,MonthNum2;
        DLong idow1,icap1,idow2,icap2;
        DDouble Seconde1,Seconde2;
        j2ymdhms(min, MonthNum1 , Day1 , Year1 , Hour1 , Minute1, Seconde1, idow1, icap1);
        MonthNum1++; //j2ymdhms gives back Month number in the [0-11] range for indexing month name tables. pity.
        j2ymdhms(max, MonthNum2 , Day2 , Year2 , Hour2 , Minute2, Seconde2, idow2, icap2);
        MonthNum2++;
        switch(code){
             case 1:
               //           day mon year h m s.s
               dateToJD(min, 1, 1, Year1, 0, 0, 0.0);
               dateToJD(max, 1, 1, Year2+1, 0, 0, 0.0);
              break;
             case 2:
               dateToJD(min, 1, MonthNum1, Year1, 0, 0, 0.0);
               MonthNum2++;
               if (MonthNum2 > 12) {MonthNum2-=12; Year2+=1;}
               dateToJD(max, 1, MonthNum2, Year2, 0, 0, 0.0);
               break;
             case 3:
               dateToJD(min, Day1, MonthNum1, Year1, 0, 0, 0.0);
               Day2++;
               if (Day2 > monthSize[MonthNum2]) {Day2-=monthSize[MonthNum2]; MonthNum2+=1;}
               if (MonthNum2 > 12) {MonthNum2-=12; Year2+=1;}
               dateToJD(max, Day2, MonthNum2, Year2, 0, 0, 0.0);
               break;
             case 4:
               dateToJD(min, Day1, MonthNum1, Year1, Hour1, 0, 0.0);
               Hour2++;
               if (Hour2 > 23) {Hour2-=24; Day2+=1;}
               if (Day2 > monthSize[MonthNum2]) {Day2-=monthSize[MonthNum2]; MonthNum2+=1;}
               if (MonthNum2 > 12) {MonthNum2-=12; Year2+=1;}
               dateToJD(max, Day2, MonthNum2, Year2, Hour2, 0, 0.0);
               break;
             case 5:
               dateToJD(min, Day1, MonthNum1, Year1, Hour1, Minute1, 0.0);
               Minute2++;
               if (Minute2 > 59) {Minute2-=60; Hour2+=1;}
               if (Hour2 > 23) {Hour2-=24; Day2+=1;}
               if (Day2 > monthSize[MonthNum2]) {Day2-=monthSize[MonthNum2]; MonthNum2+=1;}
               if (MonthNum2 > 12) {MonthNum2-=12; Year2+=1;}
               dateToJD(max, Day2, MonthNum2, Year2, Hour2, Minute2, 0.0);
               break;
             case 6:
               dateToJD(min, Day1, MonthNum1, Year1, Hour1, Minute1, Seconde1);
               Seconde2++;
               if (Seconde2 > 59) {Seconde2-=60; Minute2+=1;}
               if (Minute2 > 59) {Minute2-=60; Hour2+=1;}
               if (Hour2 > 23) {Hour2-=24; Day2+=1;}
               if (Day2 > monthSize[MonthNum2]) {Day2-=monthSize[MonthNum2]; MonthNum2+=1;}
               if (MonthNum2 > 12) {MonthNum2-=12; Year2+=1;}
               dateToJD(max, Day2, MonthNum2, Year2, Hour2, Minute2, Seconde2);
               break;
             default:
              break;
            }
      } 
      else {      
        intv = AutoIntv(x);
        if (log) {
          max = ceil((max / intv) * intv);
          min = floor((min / intv) * intv);
        } else {
          max = ceil(max / intv) * intv;
          min = floor(min / intv) * intv;
        }
      }
      

    }

    if (debug) {
      cout << "cas: " << cas << " new range: " << min << " " << max << endl;
    }
    //give back non-log values
    if (log) {
      min = pow(10, min);
      max = pow(10, max);
    }

    //check if tickinterval would make more than 59 ticks (IDL apparent limit). In which case, IDL plots only the first 59 intervals:
    DDouble TickInterval;
    gdlGetDesiredAxisTickInterval(e, axisId, TickInterval);
    if ( TickInterval > 0.0 ) if ((max-min)/TickInterval > 59) max=min+59.0*TickInterval;

    if (invert) {
      start = max;
      end = min;
    } else {
      start = min;
      end = max;
    }


    return intv;
  }

  void CheckMargin(GDLGStream* actStream,
                   DFloat xMarginL,
                   DFloat xMarginR,
                   DFloat yMarginB,
                   DFloat yMarginT,
                   PLFLT& xMR,
                   PLFLT& xML,
                   PLFLT& yMB,
                   PLFLT& yMT)
  {
    PLFLT sclx=actStream->dCharLength()/actStream->xSubPageSize(); //current char length/subpage size
    xML=xMarginL*sclx; //margin as percentage of subpage
    xMR=xMarginR*sclx;
    PLFLT scly=actStream->dLineSpacing()/actStream->ySubPageSize(); //current char length/subpage size
    yMB=(yMarginB)*scly;
    yMT=(yMarginT)*scly; //to allow subscripts and superscripts (as in IDL)

    if ( xML+xMR>=1.0 )
    {
      Message("XMARGIN to large (adjusted).");
      PLFLT xMMult=xML+xMR;
      xML/=xMMult*1.5;
      xMR/=xMMult*1.5;
    }
    if ( yMB+yMT>=1.0 )
    {
      Message("YMARGIN to large (adjusted).");
      PLFLT yMMult=yMB+yMT;
      yMB/=yMMult*1.5;
      yMT/=yMMult*1.5;
    }
  }

  void setIsoPort(GDLGStream* actStream,
                  PLFLT x1,
                  PLFLT x2,
                  PLFLT y1,
                  PLFLT y2,
                  PLFLT aspect)
  {
    PLFLT X1, X2, Y1, Y2, X1s, X2s, Y1s, Y2s, displacx, displacy, scalex, scaley, offsetx, offsety;
    if ( aspect<=0.0 )
    {
      actStream->vpor(x1, x2, y1, y2);
      return;
    }
    // here we need too compensate for the change of aspect due to eventual !P.MULTI plots
    actStream->vpor(x1, x2, y1, y2); //ask for non-iso window
    actStream->gvpd(X1, X2, Y1, Y2); //get viewport values
    //compute relation desiredViewport-page viewport x=scalex*X+offsetx:
    scalex=(x2-x1)/(X2-X1);
    offsetx=(x1*X2-x2*X1)/(X2-X1);
    scaley=(y2-y1)/(Y2-Y1);
    offsety=(y1*Y2-y2*Y1)/(Y2-Y1);
    //ask for wiewport scaled to isotropic by plplot
    actStream->vpas(x1, x2, y1, y2, aspect);
    //retrieve values
    actStream->gvpd(X1s, X2s, Y1s, Y2s);
    //measure displacement
    displacx=X1s-X1;
    displacy=Y1s-Y1;
    //set wiewport scaled by plplot, displaced, as vpor using above linear transformation
    x1=(X1s-displacx)*scalex+offsetx;
    x2=(X2s-displacx)*scalex+offsetx;
    y1=(Y1s-displacy)*scaley+offsety;
    y2=(Y2s-displacy)*scaley+offsety;
    actStream->vpor(x1, x2, y1, y2);
  }



  void GetSFromPlotStructs(DDouble **sx, DDouble **sy, DDouble **sz)
  {
    DStructGDL* xStruct=SysVar::X();   //MUST NOT BE STATIC, due to .reset 
    DStructGDL* yStruct=SysVar::Y();   //MUST NOT BE STATIC, due to .reset 
    DStructGDL* zStruct=SysVar::Z();   //MUST NOT BE STATIC, due to .reset 
    unsigned sxTag=xStruct->Desc()->TagIndex("S");
    unsigned syTag=yStruct->Desc()->TagIndex("S");
    unsigned szTag=zStruct->Desc()->TagIndex("S");
    if (sx != NULL) *sx= &(*static_cast<DDoubleGDL*>(xStruct->GetTag(sxTag, 0)))[0];
    if (sy != NULL) *sy= &(*static_cast<DDoubleGDL*>(yStruct->GetTag(syTag, 0)))[0];
    if (sz != NULL) *sz= &(*static_cast<DDoubleGDL*>(zStruct->GetTag(szTag, 0)))[0];
  }

  void GetWFromPlotStructs(DFloat **wx, DFloat **wy)
  {
    DStructGDL* xStruct=SysVar::X();   //MUST NOT BE STATIC, due to .reset 
    DStructGDL* yStruct=SysVar::Y();   //MUST NOT BE STATIC, due to .reset 
    unsigned xwindowTag=xStruct->Desc()->TagIndex("WINDOW");
    unsigned ywindowTag=yStruct->Desc()->TagIndex("WINDOW");
    *wx= &(*static_cast<DFloatGDL*>(xStruct->GetTag(xwindowTag, 0)))[0];
    *wy= &(*static_cast<DFloatGDL*>(yStruct->GetTag(ywindowTag, 0)))[0];
  }
  void setPlplotScale(GDLGStream* a)
  {
        DDouble *sx, *sy;
        GetSFromPlotStructs( &sx, &sy );

        DDouble xStart, xEnd, yStart, yEnd;
        xStart=-sx[0]/sx[1];
        xEnd=(1-sx[0])/sx[1];
        yStart=-sy[0]/sy[1];
        yEnd=(1-sy[0])/sy[1];
        a->wind(xStart, xEnd, yStart, yEnd);
  }
  void DataCoordLimits(DDouble *sx, DDouble *sy, DFloat *wx, DFloat *wy,
                       DDouble *xStart, DDouble *xEnd, DDouble *yStart, DDouble *yEnd, bool clip_by_default)
  {
    *xStart=(wx[0]-sx[0])/sx[1];
    *xEnd=(wx[1]-sx[0])/sx[1];
    *yStart=(wy[0]-sy[0])/sy[1];
    *yEnd=(wy[1]-sy[0])/sy[1];

    // patch from Joanna (tracker item no. 3029409, see test_clip.pro)
    if ( !clip_by_default )
    {
      //      cout << "joanna" << endl;
      DFloat wxlen=wx[1]-wx[0];
      DFloat wylen=wy[1]-wy[0];
      DFloat xlen= *xEnd- *xStart;
      DFloat ylen= *yEnd- *yStart;
      *xStart= *xStart-xlen/wxlen*wx[0];
      *xEnd= *xEnd+xlen/wxlen*(1-wx[1]);
      *yStart= *yStart-ylen/wylen*wy[0];
      *yEnd= *yEnd+ylen/wylen*(1-wy[1]);
    }
    //    cout << *xStart <<" "<< *xEnd << " "<< *yStart <<" "<< *yEnd << ""<< endl;
  }

  void GetUsym(DLong **n, DInt **do_fill, DFloat **x, DFloat **y, bool **do_color, DLong **usymColor , bool **do_thick, DFloat **usymThick)
  {
    *n= &(localusym.nusym);
    *do_fill= &(localusym.fill);
    *do_color= &(localusym.hasColor);
    *do_thick= &(localusym.hasThick);
    *usymColor=&(localusym.color);
    *usymThick=&(localusym.thick);
    *x=localusym.usymx;
    *y=localusym.usymy;
  }

  void SetUsym(DLong n, DInt do_fill, DFloat *x, DFloat *y, bool usersymhascolor, DLong usymColor , bool usersymhasthick, DFloat usymThick )
  {
    localusym.nusym=n;
    localusym.fill=do_fill;
    for ( int i=0; i<n; i++ )
    {
      localusym.usymx[i]=x[i];
      localusym.usymy[i]=y[i];
    }
    localusym.hasColor=usersymhascolor;
    localusym.hasThick=usersymhasthick;
    localusym.color=usymColor;
    localusym.thick=usymThick;
  }

  //This is the good way to get world start end end values.
  void GetCurrentUserLimits(GDLGStream *a, DDouble &xStart, DDouble &xEnd, DDouble &yStart, DDouble &yEnd)
  {
    DDouble *sx, *sy;
    GetSFromPlotStructs( &sx, &sy );
    DFloat *wx, *wy;
    GetWFromPlotStructs(&wx, &wy);
    PLFLT x1,x2,y1,y2;
    x1=wx[0];x2=wx[1];y1=wy[0];y2=wy[1];
    xStart=(x1-sx[0])/sx[1];
    xEnd=(x2-sx[0])/sx[1];
    yStart=(y1-sy[0])/sy[1];
    yEnd=(y2-sy[0])/sy[1];
  //probably overkill now...
    if ((yStart == yEnd) || (xStart == xEnd))
      {
        if (yStart != 0.0 && yStart == yEnd){
          Message("PLOTS: !Y.CRANGE ERROR, setting to [0,1]");
        yStart = 0;
        yEnd = 1;
        }
        if (xStart != 0.0 && xStart == xEnd){
          Message("PLOTS: !X.CRANGE ERROR, setting to [0,1]");
        xStart = 0;
        xEnd = 1;
        }
      }
  }
  
  void ac_histo(GDLGStream *a, int i_buff, PLFLT *x_buff, PLFLT *y_buff, bool xLog)
  {
    PLFLT x, x1, y, y1, val;
    for ( int jj=1; jj<i_buff; ++jj )
    {
      x1=x_buff[jj-1];
      x=x_buff[jj];
      y1=y_buff[jj-1];
      y=y_buff[jj];
      // cf patch 3567803
      if ( xLog )
      {
        //  val=log10((pow(10.0,x1)+pow(10.0,x))/2.0);
        val=x1+log10(0.5+0.5*(pow(10.0, x-x1)));
      }
      else
      {
        val=(x1+x)/2.0;
      }
      a->join(x1, y1, val, y1);
      a->join(val, y1, val, y);
      a->join(val, y, x, y);
    }
  }


  void stopClipping(GDLGStream *a)
  {
    if (saveBox.initialized) {
    a->vpor(saveBox.nx1, saveBox.nx2, saveBox.ny1, saveBox.ny2); //restore norm of current box
    a->wind(saveBox.wx1, saveBox.wx2, saveBox.wy1, saveBox.wy2); //give back world of current box
    } else cerr<<"plot \"savebox\" not initialized, please report" <<endl;
  }

  void saveLastPoint(GDLGStream *a, DDouble wx, DDouble wy)
  {
    a->WorldToNormedDevice(wx, wy, savedPointX, savedPointY);
    if (GDL_DEBUG_PLSTREAM) fprintf(stderr,"saveLastPoint as %lf %lf\n", savedPointX, savedPointY);
  }

  void getLastPoint(GDLGStream *a, DDouble& wx, DDouble& wy)
  {
    a->NormedDeviceToWorld(savedPointX, savedPointY, wx, wy);
    if (GDL_DEBUG_PLSTREAM) fprintf(stderr,"getLastPoint: Got dev: %lf %lf giving %lf %lf world\n", savedPointX, savedPointY, wx, wy);
  }

///
/// Draws a line along xVal, yVal
/// @param general environnement pointer 
/// @param graphic stream 
/// @param xVal pointer on DDoubleGDL x values
/// @param yVal pointer on DDoubleGDL y values
/// @param minVal DDouble min value to plot.
/// @param maxVal DDouble max value to plot.
/// @param doMinMax bool do we use minval & maxval above?
/// @param xLog bool scale is log in x
/// @param yLog bool scale is log in y
/// @param psym DLong plotting symbol code
/// @param append bool values must be drawn starting from last plotted value 
/// @param color DLongGDL* pointer to color list (NULL if no use)
///

  void draw_polyline(GDLGStream *a,
    DDoubleGDL *xVal, DDoubleGDL *yVal,
    DDouble minVal, DDouble maxVal, bool doMinMax,
    bool xLog, bool yLog,  //end non-implicit parameters
    DLong psym, bool useProjInfo, bool append, DLongGDL *color)
  {
    bool docolor=(color != NULL);
 
    // Get decomposed value for colors
    DLong decomposed=GraphicsDevice::GetDevice()->GetDecomposed();
    //if docolor, do we really have more than one color?
    if (docolor) if (color->N_Elements() == 1) { //do the job once and forget it after.
      a->Color ( ( *color )[0], decomposed);
      docolor=false;
      
    }    
    if (GDL_DEBUG_PLSTREAM) fprintf(stderr,"draw_polyline()\n");
    SizeT plotIndex=0;
    bool line=false;
    DLong psym_=0;

    if ( psym<0 )
    {
      line=true;
      psym_= -psym;
    }
    else if ( psym==0 )
    {
      line=true;
      psym_=psym;
    }
    else
    {
      psym_=psym;
    }

    //usersym and other syms as well!
    DFloat *userSymX, *userSymY;
    DLong *userSymArrayDim;
    //initialize symbol vertex list
    static PLFLT xSym[49];
    static PLFLT ySym[49];
    DInt *do_fill;
    bool *usersymhascolor;
    bool *usersymhasthick;
    DLong *usymColor;
    DFloat *usymThick;
    static DInt nofill=0;
    if ( psym_==8 )
    {
      GetUsym(&userSymArrayDim, &do_fill, &userSymX, &userSymY, &usersymhascolor, &usymColor , &usersymhasthick, &usymThick );
      if ( *userSymArrayDim==0 )
      {
        ThrowGDLException("No user symbol defined.");
      }
    }
    else if ( (psym_>0&&psym_<8))
    {
      do_fill=&nofill;
      userSymArrayDim=&(syml[psym_-1]);
      switch(psym_)
      {
        case 1:
          userSymX=sym1x;
          userSymY=sym1y;
          break;
        case 2:
          userSymX=sym2x;
          userSymY=sym2y;
          break;
        case 3:
          userSymX=sym3x;
          userSymY=sym3y;
          break;
        case 4:
          userSymX=sym4x;
          userSymY=sym4y;
          break;
        case 5:
          userSymX=sym5x;
          userSymY=sym5y;
          break;
        case 6:
          userSymX=sym6x;
          userSymY=sym6y;
          break;
        case 7:
          userSymX=sym7x;
          userSymY=sym7y;
          break;
     }
    }

    DLong minEl=(xVal->N_Elements()<yVal->N_Elements())?
    xVal->N_Elements():yVal->N_Elements();
    // if scalar x
    if ( xVal->N_Elements()==1&&xVal->Rank()==0 )
      minEl=yVal->N_Elements();
    // if scalar y
    if ( yVal->N_Elements()==1&&yVal->Rank()==0 )
      minEl=xVal->N_Elements();

    // is one of the 2 "arrays" a singleton or not ?

    PLFLT y, yMapBefore, y_ref;
    int flag_y_const=0;
    y_ref=static_cast<PLFLT>((*yVal)[0]);
    if ( yVal->N_Elements()==1&&yVal->Rank()==0 ) flag_y_const=1;

    PLFLT x, x1, xMapBefore, x_ref;
    int flag_x_const=0;
    x_ref=static_cast<PLFLT>((*xVal)[0]);
    if ( xVal->N_Elements()==1&&xVal->Rank()==0 ) flag_x_const=1;

    // AC 070601 we use a buffer to use the fast ->line method
    // instead of the slow ->join one.
    // 2 tricks:
    // trick 1/ size of buffer is limited to 1e4 (compromize syze/speed) in order to be able to manage very
    //    large amount of data whitout duplicating all the arrays
    // trick 2/ when we have a NaN or and Inf, we realize the plot, then reset.

    int GDL_POLYLINE_BUFFSIZE=500000; // idl default seems to be more than 2e6 !!

    if ( minEl<GDL_POLYLINE_BUFFSIZE ) GDL_POLYLINE_BUFFSIZE=append?minEl+1:minEl;
    int i_buff=0;
    PLFLT *x_buff=new PLFLT[GDL_POLYLINE_BUFFSIZE];
    PLFLT *y_buff=new PLFLT[GDL_POLYLINE_BUFFSIZE];

    bool isBad=FALSE;

    for ( SizeT i=0; i<minEl; ++i ) {
      isBad=FALSE;
      if ( append ) //start with the old point
      {
        getLastPoint(a, x, y);
        i--; //to get good counter afterwards
        append=FALSE; //and stop appending after!
        if ( xLog ) x=pow(10, x);
        if ( yLog ) y=pow(10, y);
      }
      else
      {
        if ( !flag_x_const ) x=static_cast<PLFLT>((*xVal)[i]);
        else x=x_ref;
        if ( !flag_y_const ) y=static_cast<PLFLT>((*yVal)[i]);
        else y=y_ref;
      }
      
      //note: here y is in minVal maxVal
      if ( doMinMax ) isBad=((y<minVal)||(y>maxVal));
      if ( xLog ) x=log10(x);
      if ( yLog ) y=log10(y);
      isBad=(isBad||!isfinite(x)|| !isfinite(y)||isnan(x)||isnan(y));
      if ( isBad )
      {
        if ( i_buff>0 )
        {
          if ( line )
          {
            if (docolor) for (SizeT jj=0; jj< i_buff-1 ; ++jj)
            {
              a->Color ( ( *color )[plotIndex%color->N_Elements ( )], decomposed);
              a->line(2, &(x_buff[jj]), &(y_buff[jj]));
              plotIndex++;
            }
            else a->line(i_buff, x_buff, y_buff);
          }
          if (psym_>0&&psym_<8)
          {
            a->styl(0, NULL, NULL); //symbols drawn in continuous lines
            for ( int j=0; j<i_buff; ++j )
            {
              for ( int kk=0; kk < *userSymArrayDim; kk++ )
              {
                xSym[kk]=x_buff[j]+userSymX[kk]*a->getPsymConvX();
                ySym[kk]=y_buff[j]+userSymY[kk]*a->getPsymConvY();
              }
              if (docolor)
              {
                a->Color ( ( *color )[plotIndex%color->N_Elements ( )], decomposed);
                plotIndex++;
              }
              if ( *do_fill==1 )
              {
                a->fill(*userSymArrayDim, xSym, ySym);
              }
              else
              {
                a->line(*userSymArrayDim, xSym, ySym);
              }
            }
          }
        else if ( psym_==8 )
        {
          a->styl(0, NULL, NULL); //symbols drawn in continuous lines
          if (*usersymhascolor)
          {
            a->Color(*usymColor, decomposed);
          }
          if (*usersymhasthick)
          {
            a->Thick(*usymThick);
          }
          for (int j = 0; j < i_buff; ++j)
          {
            for (int kk = 0; kk < *userSymArrayDim; kk++)
            {
              xSym[kk] = x_buff[j] + userSymX[kk] * a->getPsymConvX();
              ySym[kk] = y_buff[j] + userSymY[kk] * a->getPsymConvY();
            }
            if (*do_fill == 1)
            {
              a->fill(*userSymArrayDim, xSym, ySym);
            } else
            {
              a->line(*userSymArrayDim, xSym, ySym);
            }
          }
        }
        else if ( psym_==10 )
          {
            ac_histo(a, i_buff, x_buff, y_buff, xLog);
          }
          i_buff=0;
        }
        continue;
      }

      x_buff[i_buff]=x;
      y_buff[i_buff]=y;
      i_buff=i_buff+1;

      //	cout << "nbuf: " << i << " " << i_buff << " "<< n_buff_max-1 << " " << minEl-1 << endl;

      if ( (i_buff==GDL_POLYLINE_BUFFSIZE)||((i==minEl-1)&& !append)||((i==minEl)&&append) )
      {
        if ( line )
        {
          if (docolor) for (SizeT jj=0; jj< i_buff-1 ; ++jj)
            {
              a->Color ( ( *color )[plotIndex%color->N_Elements ( )], decomposed);
              a->line(2, &(x_buff[jj]), &(y_buff[jj]));
              plotIndex++;
            }
            else a->line(i_buff, x_buff, y_buff);
        }
        if ( psym_>0&&psym_<8 )
        {
          a->styl(0, NULL, NULL); //symbols drawn in continuous lines
          for ( int j=0; j<i_buff; ++j )
          {
            for ( int kk=0; kk < *userSymArrayDim; kk++ )
            {
              xSym[kk]=x_buff[j]+userSymX[kk]*a->getPsymConvX();
              ySym[kk]=y_buff[j]+userSymY[kk]*a->getPsymConvY();
            }
            if (docolor)
            {
              a->Color ( ( *color )[plotIndex%color->N_Elements ( )], decomposed);
              plotIndex++;
            }
            if ( *do_fill==1 )
            {
              a->fill(*userSymArrayDim, xSym, ySym);
            }
            else
            {
              a->line(*userSymArrayDim, xSym, ySym);
            }
          }

        }
        else if ( psym_==8 )
        {
          a->styl(0, NULL, NULL); //symbols drawn in continuous lines
          if (*usersymhascolor)
          {
            a->Color(*usymColor, decomposed);
          }
          if (*usersymhasthick)
          {
            a->Thick(*usymThick);
          }
          for (int j = 0; j < i_buff; ++j)
          {
            for (int kk = 0; kk < *userSymArrayDim; kk++)
            {
              xSym[kk] = x_buff[j] + userSymX[kk] * a->getPsymConvX();
              ySym[kk] = y_buff[j] + userSymY[kk] * a->getPsymConvY();
            }
            if (*do_fill == 1)
            {
              a->fill(*userSymArrayDim, xSym, ySym);
            } else
            {
              a->line(*userSymArrayDim, xSym, ySym);
            }
          }
        }
        else if ( psym_==10 )
        {
          ac_histo(a, i_buff, x_buff, y_buff, xLog);
        }

        // we must recopy the last point since the line must continue (tested via small buffer ...)
        x_buff[0]=x_buff[i_buff-1];
        y_buff[0]=y_buff[i_buff-1];
        i_buff=1;
      }
    }

    delete[] x_buff;
    delete[] y_buff;
    //save last point
    saveLastPoint(a, x, y);
  }

 
  //BACKGROUND COLOR


  
  //Very special usage only in plotting surface
  void gdlSetGraphicsPenColorToBackground(GDLGStream *a)
  {
    a->plstream::col0( 0);
  }

  //COLOR


  // helper for NOERASE


  //PSYM



  //SYMSIZE



  //CHARSIZE








  //THICK

  //LINESTYLE
  void gdlLineStyle(GDLGStream *a, DLong style)
  {
      static PLINT mark1[]={75};
      static PLINT space1[]={1500};
      static PLINT mark2[]={1500};
      static PLINT space2[]={1500};
      static PLINT mark3[]={1500, 100};
      static PLINT space3[]={1000, 1000};
      static PLINT mark4[]={1500, 100, 100, 100};
      static PLINT space4[]={1000, 1000, 1000, 1000};
      static PLINT mark5[]={3000};
      static PLINT space5[]={1500};          // see plplot-5.5.3/examples/c++/x09.cc
      switch(style)
      {
        case 0:
          a->styl(0, mark1, space1);
          return;
         case 1:
          a->styl(1, mark1, space1);
          return;
        case 2:
          a->styl(1, mark2, space2);
          return;
        case 3:
          a->styl(2, mark3, space3);
          return;
        case 4:
          a->styl(4, mark4, space4);
          return;
        case 5:
          a->styl(1, mark5, space5);
          return;
        default:
          a->styl(0, NULL, NULL);
          return;
      }
  }


  
  //crange to struct

  void gdlStoreAxisCRANGE(int axisId, DDouble Start, DDouble End, bool log)
  {
    DStructGDL* Struct=NULL;
    if ( axisId==XAXIS ) Struct=SysVar::X();
    if ( axisId==YAXIS ) Struct=SysVar::Y();
    if ( axisId==ZAXIS ) Struct=SysVar::Z();
    if ( Struct!=NULL )
    {
      int debug=0;
      if ( debug ) cout<<"Set     :"<<Start<<" "<<End<<endl;

      unsigned crangeTag=Struct->Desc()->TagIndex("CRANGE");
      if ( log )
      {
        (*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[0]=log10(Start);
        (*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[1]=log10(End);
        if ( debug ) cout<<"set log"<<Start<<" "<<End<<endl;
      }
      else
      {
        (*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[0]=Start;
        (*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[1]=End;
      }
    }
  }

  //CRANGE from struct

  void gdlGetCurrentAxisRange(int axisId, DDouble &Start, DDouble &End, bool checkMapset)
  {
    DStructGDL* Struct=NULL;
    if ( axisId==XAXIS ) Struct=SysVar::X();
    if ( axisId==YAXIS ) Struct=SysVar::Y();
    if ( axisId==ZAXIS ) Struct=SysVar::Z();
    Start=0;
    End=0;
    if ( Struct!=NULL )
    {
      int debug=0;
      if ( debug ) cout<<"Get     :"<<Start<<" "<<End<<endl;
      bool isProj;
      get_mapset(isProj);
      if (checkMapset && isProj && axisId!=ZAXIS) {
        DStructGDL* mapStruct=SysVar::Map();   //MUST NOT BE STATIC, due to .reset 
        static unsigned uvboxTag=mapStruct->Desc()->TagIndex("UV_BOX");
        static DDoubleGDL *uvbox;
        uvbox=static_cast<DDoubleGDL*>(mapStruct->GetTag(uvboxTag, 0));
        if (axisId==XAXIS) {
          Start=(*uvbox)[0];
          End=(*uvbox)[2];
        } else {
          Start=(*uvbox)[1];
          End=(*uvbox)[3];
        }
      } else {
        static unsigned crangeTag=Struct->Desc()->TagIndex("CRANGE");
        Start=(*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[0];
        End=(*static_cast<DDoubleGDL*>(Struct->GetTag(crangeTag, 0)))[1];

        static unsigned typeTag=Struct->Desc()->TagIndex("TYPE");
        if ( (*static_cast<DLongGDL*>(Struct->GetTag(typeTag, 0)))[0]==1 )
        {
          Start=pow(10., Start);
          End=pow(10., End);
          if ( debug ) cout<<"Get log :"<<Start<<" "<<End<<endl;
        }
      }
    }
  }

  void gdlGetCurrentAxisWindow(int axisId, DDouble &wStart, DDouble &wEnd)
  {
    DStructGDL* Struct=NULL;
    if ( axisId==XAXIS ) Struct=SysVar::X();
    if ( axisId==YAXIS ) Struct=SysVar::Y();
    if ( axisId==ZAXIS ) Struct=SysVar::Z();
    wStart=0;
    wEnd=0;
    if ( Struct!=NULL )
    {
      static unsigned windowTag=Struct->Desc()->TagIndex("WINDOW");
      wStart=(*static_cast<DFloatGDL*>(Struct->GetTag(windowTag, 0)))[0];
      wEnd=(*static_cast<DFloatGDL*>(Struct->GetTag(windowTag, 0)))[1];
    }
  }

  //Stores [XYZ].WINDOW, .REGION and .S
  void gdlStoreAxisSandWINDOW(GDLGStream* actStream, int axisId, DDouble Start, DDouble End, bool log)
  {
    PLFLT p_xmin, p_xmax, p_ymin, p_ymax, norm_min, norm_max, charDim;
    actStream->gvpd(p_xmin, p_xmax, p_ymin, p_ymax); //viewport normalized coords
    DStructGDL* Struct=NULL;
    if ( axisId==XAXIS ) {Struct=SysVar::X(); norm_min=p_xmin; norm_max=p_xmax; charDim=actStream->nCharLength();}
    if ( axisId==YAXIS ) {Struct=SysVar::Y(); norm_min=p_ymin; norm_max=p_ymax; charDim=actStream->nCharHeight();}
    if ( axisId==ZAXIS ) {Struct=SysVar::Z(); norm_min=0; norm_max=1; charDim=actStream->nCharLength();}
    if ( Struct!=NULL )
    {
      unsigned marginTag=Struct->Desc()->TagIndex("MARGIN");
      DFloat m1=(*static_cast<DFloatGDL*>(Struct->GetTag(marginTag, 0)))[0];
      DFloat m2=(*static_cast<DFloatGDL*>(Struct->GetTag(marginTag, 0)))[1];
      static unsigned regionTag=Struct->Desc()->TagIndex("REGION");
      (*static_cast<DFloatGDL*>(Struct->GetTag(regionTag, 0)))[0]=max(0.0,norm_min-m1*charDim);
      (*static_cast<DFloatGDL*>(Struct->GetTag(regionTag, 0)))[1]=min(1.0,norm_max+m2*charDim);

      //      if ( log ) {Start=log10(Start); End=log10(End);}
      static unsigned windowTag=Struct->Desc()->TagIndex("WINDOW");
      (*static_cast<DFloatGDL*>(Struct->GetTag(windowTag, 0)))[0]=norm_min;
      (*static_cast<DFloatGDL*>(Struct->GetTag(windowTag, 0)))[1]=norm_max;

      static unsigned sTag=Struct->Desc()->TagIndex("S");
      (*static_cast<DDoubleGDL*>(Struct->GetTag(sTag, 0)))[0]=
      (norm_min*End-norm_max*Start)/(End-Start);
      (*static_cast<DDoubleGDL*>(Struct->GetTag(sTag, 0)))[1]=
      (norm_max-norm_min)/(End-Start);
    }
  }

  void gdlStoreCLIP(DLongGDL* clipBox)
  {
    DStructGDL* pStruct=SysVar::P();   //MUST NOT BE STATIC, due to .reset 
    int i;
    static unsigned clipTag=pStruct->Desc()->TagIndex("CLIP");
    for ( i=0; i<clipBox->N_Elements(); ++i ) (*static_cast<DLongGDL*>(pStruct->GetTag(clipTag, 0)))[i]=(*clipBox)[i];
  }

  void gdlGetAxisType(int axisId, bool &log)
  {
    DStructGDL* Struct;
    if ( axisId==XAXIS ) Struct=SysVar::X();
    if ( axisId==YAXIS ) Struct=SysVar::Y();
    if ( axisId==ZAXIS ) Struct=SysVar::Z();
    if ( Struct!=NULL )
    {
      static unsigned typeTag=Struct->Desc()->TagIndex("TYPE");
      if ( (*static_cast<DLongGDL*>(Struct->GetTag(typeTag, 0)))[0]==1 )
        log=true;
      else
        log=false;
    }
  }

  void get_mapset(bool &mapset)
  {
    DStructGDL* Struct=SysVar::X();   //MUST NOT BE STATIC, due to .reset 
    if ( Struct!=NULL )
    {
      static unsigned typeTag=Struct->Desc()->TagIndex("TYPE");

      if ( (*static_cast<DLongGDL*>(Struct->GetTag(typeTag, 0)))[0]==3 )
        mapset=true;
      else
        mapset=false;
    }
  }

  void set_mapset(bool mapset)
  {
    DStructGDL* Struct=SysVar::X();   //MUST NOT BE STATIC, due to .reset 
    if ( Struct!=NULL )
    {
      static unsigned typeTag=Struct->Desc()->TagIndex("TYPE");
      (*static_cast<DLongGDL*>(Struct->GetTag(typeTag, 0)))[0]=(mapset)?3:0;
    }
  }


  //axis type (log..)

  void gdlStoreAxisType(int axisId, bool Type)
  {
    DStructGDL* Struct=NULL;
    if ( axisId==XAXIS ) Struct=SysVar::X();
    if ( axisId==YAXIS ) Struct=SysVar::Y();
    if ( axisId==ZAXIS ) Struct=SysVar::Z();
    if ( Struct!=NULL )
    {
      static unsigned typeTag=Struct->Desc()->TagIndex("TYPE");
      (*static_cast<DLongGDL*>(Struct->GetTag(typeTag, 0)))[0]=Type;
    } 
  }

  void doOurOwnFormat(PLINT axisNotUsed, PLFLT value, char *label, PLINT length, PLPointer data)
  {
    struct GDL_TICKDATA *ptr = (GDL_TICKDATA* )data;
    //was:
//    static string normalfmt[7]={"%1.0fx10#u%d#d","%2.1fx10#u%d#d","%3.2fx10#u%d#d","%4.2fx10#u%d#d","%5.4fx10#u%d#d","%6.5fx10#u%d#d","%7.6fx10#u%d#d"};
//    static string specialfmt="10#u%d#d";
//    static string specialfmtlog="10#u%s#d";
    
    //we need !3x!X to insure the x sign is always written in single roman.
    static string normalfmt[7]={"%1.0f!3x!X10!E%d!N","%2.1f!3x!X10!E%d!N","%3.2f!3x!X10!E%d!N","%4.2f!3x!X10!E%d!N","%5.4f!3x!X10!E%d!N","%6.5f!3x!X10!E%d!N","%7.6f!3x!X10!E%d!N"};
    static string specialfmt="10!E%d!N";
    static string specialfmtlog="10!E%s!N";
    PLFLT z;
    int ns;
    char *i;
    int sgn=(value<0)?-1:1;
    //special cases, since plplot gives approximate zero values, not strict zeros.
    if (!(ptr->isLog) && (sgn*value<ptr->axisrange*1e-6)) 
    {
      snprintf(label, length, "0");
      return;
    }
    //in log, plplot gives correctly rounded "integer" values but 10^0 needs a bit of help.
    if ((ptr->isLog) && (sgn*value<1e-6)) //i.e. 0 
    {
      snprintf(label, length, "1"); 
      return;
    }
    
    int e=floor(log10(value*sgn));
    char *test=(char*)calloc(2*length, sizeof(char)); //be safe
    if (!isfinite(log10(value*sgn))||(e<4 && e>-4)) 
    {
      snprintf(test, length, "%f",value);
      ns=strlen(test);
      i=strrchr (test,'0');
      while (i==(test+ns-1)) //remove trailing zeros...
      {
          *i='\0';
        i=strrchr(test,'0');
        ns--;
      }
      i=strrchr(test,'.'); //remove trailing '.'
      if (i==(test+ns-1)) {*i='\0'; ns--;}
      if (ptr->isLog) {
        snprintf( label, length, specialfmtlog.c_str(),test);
      }
      else
      {
        strcpy(label, test);
      }
    }
    else
    {
      z=value*sgn/pow(10.,e);
      snprintf(test,20,"%7.6f",z);
      ns=strlen(test);
      i=strrchr(test,'0');
      while (i==(test+ns-1))
      {
          *i='\0';
        i=strrchr(test,'0');
        ns--;
      }
      ns-=2;ns=(ns>6)?6:ns;
      if (floor(sgn*z)==1 && ns==0) {
        snprintf( label, length, specialfmt.c_str(),e);
      } else {
        snprintf( label, length, normalfmt[ns].c_str(),sgn*z,e);
      }
    }
    free(test);
  }
  void doFormatAxisValue(DDouble value, string &label)
  {
    static string normalfmt[7]={"%1.0fx10^%d","%2.1fx10^%d","%3.2fx10^%d","%4.2fx10^%d","%5.4fx10^%d","%6.5fx10^%d","%7.6fx10^%d"};
    static string specialfmt="10^%d";
    static const int length=20;
    PLFLT z;
    int ns;
    char *i;
    int sgn=(value<0)?-1:1;
    //special cases, since plplot gives approximate zero values, not strict zeros.
    if (sgn*value< std::numeric_limits<DDouble>::min()) 
    {
      label="0"; 
      return;
    }
    
    int e=floor(log10(value*sgn));
    char *test=(char*)calloc(2*length, sizeof(char)); //be safe
    if (!isfinite(e)||(e<4 && e>-4)) 
    {
      snprintf(test, length, "%f",value);
      ns=strlen(test);
      i=strrchr (test,'0');
      while (i==(test+ns-1)) //remove trailing zeros...
      {
          *i='\0';
        i=strrchr(test,'0');
        ns--;
      }
      i=strrchr(test,'.'); //remove trailing '.'
      if (i==(test+ns-1)) {*i='\0'; ns--;}
    }
    else
    {
      z=value*sgn/pow(10.,e);
      snprintf(test,20,"%7.6f",z);
      ns=strlen(test);
      i=strrchr(test,'0');
      while (i==(test+ns-1))
      {
          *i='\0';
        i=strrchr(test,'0');
        ns--;
      }
      ns-=2;ns=(ns>6)?6:ns;
	if (floor(sgn*z)==1 && ns==0)
	  snprintf( test, length, specialfmt.c_str(),e);
	else
	  snprintf( test, length, normalfmt[ns].c_str(),sgn*z,e);
    }
    label=test;
    free(test);
  }
  
  BaseGDL* format_axis_values(EnvT *e){
    DDoubleGDL* p0D = e->GetParAs<DDoubleGDL>( 0);
    DStringGDL* res = new DStringGDL( p0D->Dim(), BaseGDL::NOZERO);
    SizeT nEl = p0D->N_Elements();
    for( SizeT i=0; i<nEl; ++i)
	  {
	    doFormatAxisValue((*p0D)[i], (*res)[i]);
	  }
  return res;
  }
  //just a wrapper for doOurOwnFormat() adding general font code translation.
  void gdlSimpleAxisTickFunc(PLINT axis, PLFLT value, char *label, PLINT length, PLPointer data)
  {
    struct GDL_TICKDATA *ptr = (GDL_TICKDATA* )data;
    doOurOwnFormat(axis, value, label, length, data);
    //translate format codes (as in mtex).
    double nchars;
    std::string out = ptr->a->TranslateFormatCodes(label, &nchars);
    ptr->nchars=max(ptr->nchars,nchars);
    strcpy(label,out.c_str());
  }
  
  void gdlMultiAxisTickFunc(PLINT axis, PLFLT value, char *label, PLINT length, PLPointer multiaxisdata)
  {
    static GDL_TICKDATA tdata;
    static SizeT internalIndex=0;
    static DLong lastUnits=0;
    static string theMonth[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    PLINT Month, Day , Year , Hour , Minute, dow, cap;
    PLFLT Second;
    struct GDL_MULTIAXISTICKDATA *ptr = (GDL_MULTIAXISTICKDATA* )multiaxisdata;
    tdata.a=ptr->a;
    tdata.isLog=ptr->isLog;
    if (ptr->counter != lastUnits)
    {
      lastUnits=ptr->counter;
      internalIndex=0;
    }
    if (ptr->what==GDL_TICKFORMAT || (ptr->what==GDL_TICKFORMAT_AND_UNITS && ptr->counter < ptr->nTickFormat) )
    {
      if (ptr->counter > ptr->nTickFormat-1)
      {
        doOurOwnFormat(axis, value, label, length, &tdata);
      }
      else
      {
        if (((*ptr->TickFormat)[ptr->counter]).substr(0,1) == "(")
        { //internal format, call internal func "STRING"
          EnvT *e=ptr->e;
          static int stringIx = LibFunIx("STRING");
          assert( stringIx >= 0);
          EnvT* newEnv= new EnvT(e, libFunList[stringIx], NULL);
          Guard<EnvT> guard( newEnv);
          // add parameters
          newEnv->SetNextPar( new DDoubleGDL(value));
          newEnv->SetNextPar( new DStringGDL(((*ptr->TickFormat)[ptr->counter]).c_str()));
          // make the call
          BaseGDL* res = static_cast<DLibFun*>(newEnv->GetPro())->Fun()(newEnv);
          strncpy(label,(*static_cast<DStringGDL*>(res))[0].c_str(),1000);
        }
        else // external function: if tickunits not specified, pass Axis (int), Index(int),Value(Double)
          //    else pass also Level(int)
          // Thanks to Marc for code snippet!
          // NOTE: this encompasses the 'LABEL_DATE' format, an existing procedure in the IDL library.
        {
          EnvT *e=ptr->e;
          DString callF=(*ptr->TickFormat)[ptr->counter];
          // this is a function name -> convert to UPPERCASE
          callF = StrUpCase( callF);
          	//  Search in user proc and function
          SizeT funIx = GDLInterpreter::GetFunIx( callF);

          EnvUDT* newEnv = new EnvUDT( e->CallingNode(), funList[ funIx], (DObjGDL**)NULL);
          Guard< EnvUDT> guard( newEnv);
          // add parameters
          newEnv->SetNextPar( new DLongGDL(axis));
          newEnv->SetNextPar( new DLongGDL(internalIndex));
          newEnv->SetNextPar( new DDoubleGDL(value));
          if (ptr->what==GDL_TICKFORMAT_AND_UNITS) newEnv->SetNextPar( new DLongGDL(ptr->counter));
          // guard *before* pushing new env
          StackGuard<EnvStackT> guard1 ( e->Interpreter()->CallStack());
          e->Interpreter()->CallStack().push_back(newEnv);
          guard.release();

          BaseGDL* retValGDL = e->Interpreter()->call_fun(static_cast<DSubUD*>(newEnv->GetPro())->GetTree()); 
          // we are the owner of the returned value
          Guard<BaseGDL> retGuard( retValGDL);
          strncpy(label,(*static_cast<DStringGDL*>(retValGDL))[0].c_str(),1000);
        }
      }
    }
    else if (ptr->what==GDL_TICKUNITS || (ptr->what==GDL_TICKFORMAT_AND_UNITS && ptr->counter >= ptr->nTickFormat))
    {
      if (ptr->counter > ptr->nTickUnits-1 )
      {
        doOurOwnFormat(axis, value, label, length, &tdata);
      }
      else
      {
        DString what=StrUpCase((*ptr->TickUnits)[ptr->counter]);
        if (what.length()<1) {
          doOurOwnFormat(axis, value, label, length, &tdata);
        }
        else if (what.substr(0,7)=="NUMERIC") {
          doOurOwnFormat(axis, value, label, length, &tdata);
     } else {
          j2ymdhms(value, Month , Day , Year , Hour , Minute, Second, dow, cap);
          int convcode=0;
          if (what.length()<1) convcode=7;
          else if (what.substr(0,4)=="YEAR") convcode=1;
          else if (what.substr(0,5)=="MONTH") convcode=2;
          else if (what.substr(0,3)=="DAY") convcode=3;
          else if (what.substr(0,4)=="HOUR") convcode=4;
          else if (what.substr(0,6)=="MINUTE") convcode=5;
          else if (what.substr(0,6)=="SECOND") convcode=6;
          else if (what.substr(0,4)=="TIME")
          {
            if(ptr->axisrange>=366)  convcode=1;
            else if(ptr->axisrange>=32)  convcode=2;
            else if(ptr->axisrange>=1.1)  convcode=3;
            else if(ptr->axisrange*24>=1.1)  convcode=4;
            else if(ptr->axisrange*24*60>=1.1)  convcode=5;
            else convcode=6;
          } else convcode=7;
          switch(convcode){
            case 1:
              snprintf( label, length, "%d", Year);
            break;
            case 2:
              snprintf( label, length, "%s", theMonth[Month].c_str());
              break;
            case 3:
              snprintf( label, length, "%d", Day);
              break;
            case 4:
              snprintf( label, length, "%02d", Hour);
              break;
            case 5:
              snprintf( label, length, "%02d", Minute);
              break;
            case 6:
              snprintf( label, length, "%05.2f", Second);
              break;
            case 7:
              doOurOwnFormat(axis, value, label, length, &tdata);
              break;
          }
          
        }
      }
    }
    //translate format codes (as in mtex).
    double nchars;
    std::string out = ptr->a->TranslateFormatCodes(label, &nchars);
    ptr->nchars=max(ptr->nchars,nchars);
    strcpy(label,out.c_str());
    internalIndex++;
  }

  void gdlSingleAxisTickNamedFunc( PLINT axis, PLFLT value, char *label, PLINT length, PLPointer data)
  {
    static GDL_TICKDATA tdata;
    struct GDL_TICKNAMEDATA *ptr = (GDL_TICKNAMEDATA* )data;
    tdata.isLog=ptr->isLog;
    tdata.axisrange=ptr->axisrange;
    if (ptr->counter > ptr->nTickName-1)
    {
      doOurOwnFormat(axis, value, label, length, &tdata);
    }
    else
    {
      snprintf( label, length, "%s", ((*ptr->TickName)[ptr->counter]).c_str() );
    }
    //translate format codes (as in mtex).
    double nchars;
    std::string out = ptr->a->TranslateFormatCodes(label, &nchars);
    ptr->nchars=max(ptr->nchars,nchars);
    strcpy(label,out.c_str());
    ptr->counter++;
  }

  bool T3Denabled()
  {
    DStructGDL* pStruct=SysVar::P();   //MUST NOT BE STATIC, due to .reset 
    DLong ok4t3d=(*static_cast<DLongGDL*>(pStruct->GetTag(pStruct->Desc()->TagIndex("T3D"), 0)))[0];
    if (ok4t3d==0) return false; else return true;
  }
  
  void usersym(EnvT *e)
  {
    DFloatGDL *xyVal, *xVal, *yVal;
    Guard<BaseGDL> p0_guard;
    DLong n;
    DInt do_fill;
    bool do_color;
    bool do_thick;
    DFloat thethick;
    DLong thecolor;
    DFloat *x, *y;
    SizeT nParam=e->NParam();
    if (nParam==0) e->Throw("Incorrect number of arguments.");
    if ( nParam==1 )
    {
      BaseGDL* p0=e->GetNumericArrayParDefined(0)->Transpose(NULL); //hence [49,2]

      xyVal=static_cast<DFloatGDL*>
      (p0->Convert2(GDL_FLOAT, BaseGDL::COPY));
      p0_guard.Reset(p0); // delete upon exit

      if ( xyVal->Rank()!=2||xyVal->Dim(1)!=2 )
        e->Throw(e->GetParString(0)+" must be a 2-dim array of type [2,N] in this context.");

      if ( xyVal->Dim(0)>49 )
      {
        e->Throw("Max array size for USERSYM is 49");
      }
      n=xyVal->Dim(0);
      // array is in the good order for direct C assignement
      x=&(*xyVal)[0];
      y=&(*xyVal)[n];
    }
    else
    {
      xVal=e->GetParAs< DFloatGDL>(0);
      if ( xVal->Rank()!=1 )
        e->Throw(e->GetParString(0)+" must be a 1D array in this context: ");

      yVal=e->GetParAs< DFloatGDL>(1);
      if ( yVal->Rank()!=1 )
        e->Throw("Expression must be a 1D array in this context: "+e->GetParString(1));

      if ( xVal->Dim(0)!=yVal->Dim(0) )
      {
        e->Throw("Arrays must have same size ");
      }

      if ( xVal->Dim(0)>49 )
      {
        e->Throw("Max array size for USERSYM is 49");
      }
      n=xVal->Dim(0);
      x=&(*xVal)[0];
      y=&(*yVal)[0];
    }
    do_fill=0;
    static int FILLIx = e->KeywordIx("FILL");
    if ( e->KeywordSet(FILLIx) )
    {
      do_fill=1;
    }
    //IDL does not complain if color is undefined.
    do_color=false;
    thecolor=0;
    static int COLORIx = e->KeywordIx("COLOR");
    if ( e->KeywordPresent(COLORIx))
    {
      if (e->IfDefGetKWAs<DLongGDL>( COLORIx )) {
        e->AssureLongScalarKW(COLORIx, thecolor);
        do_color=true;
      }
    }
    //IDL does not complain if thick is undefined.
    do_thick=false;
    thethick=0;
    static int THICKIx = e->KeywordIx("THICK");
    if ( e->KeywordPresent(THICKIx))
    {
      if (e->IfDefGetKWAs<DFloatGDL>( THICKIx )) {
        e->AssureFloatScalarKW(THICKIx, thethick);
        do_thick=true;
      }
    }    
    SetUsym(n, do_fill, x, y, do_color, thecolor, do_thick, thethick);
  }
#ifdef USE_LIBPROJ4
void GDLgrProjectedPolygonPlot( GDLGStream * a, PROJTYPE ref, DStructGDL* map,
  DDoubleGDL *lons_donottouch, DDoubleGDL *lats_donottouch, bool isRadians, bool const doFill, DLongGDL *conn ) {
    DDoubleGDL *lons,*lats;
    lons=lons_donottouch->Dup(); Guard<BaseGDL> lonsGuard( lons);
    lats=lats_donottouch->Dup(); Guard<BaseGDL> latsGuard( lats);

    DStructGDL* localMap = map;
    if (localMap==NULL) localMap=SysVar::Map( );
    bool mapSet; 
    get_mapset(mapSet); //if mapSet, output will be converted to normalized coordinates as this seems to be the way to do it.
    bool doConn = (conn != NULL);
    DLongGDL *gons, *lines;
    if (!isRadians) {
    SizeT nin = lons->N_Elements( );
#pragma omp parallel if (nin >= CpuTPOOL_MIN_ELTS && (CpuTPOOL_MAX_ELTS == 0 || CpuTPOOL_MAX_ELTS <= nin))
      {
#pragma omp for
        for ( OMPInt in = 0; in < nin; in++ ) { //pass in radians for gdlProjForward
          (*lons)[in] *= DEG_TO_RAD;
          (*lats)[in] *= DEG_TO_RAD;
        }
      }
    }
    DDoubleGDL *res = gdlProjForward( ref, localMap, lons, lats, conn, doConn, gons, doFill, lines, !doFill, false );
    SizeT nout = res->N_Elements( ) / 2;
    if (nout < 1) {GDLDelete(res); return;} //projection clipped totally these values.
    DDoubleGDL *res2 = static_cast<DDoubleGDL*> (static_cast<BaseGDL*> (res)->Transpose( NULL )); GDLDelete(res);
    int minpoly;
    if ( doFill ) {
      conn = gons;
      minpoly = 3;
    } else {
      conn = lines;
      minpoly = 2;
    }
    SizeT index = 0;
    SizeT size;
    SizeT start;
    while ( index < conn->N_Elements( ) ) {
      size = (*conn)[index];
      if ( size == 0 ) break; //cannot be negative!
      start = (*conn)[index + 1];
      if ( size >= minpoly ) {
        if ( doFill ) {
          a->fill( size, (PLFLT*) &((*res2)[start]), (PLFLT*) &((*res2)[start + nout]) );
        } else {
          a->line( size, (PLFLT*) &((*res2)[start]), (PLFLT*) &((*res2)[start + nout]) );
        }
      }
      index += (size + 1);
    }
    GDLDelete( res2 );
    if ( doFill ) GDLDelete( gons );
    else GDLDelete( lines );
  }
#endif
} // namespace
