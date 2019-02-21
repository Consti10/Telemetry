//
// Created by Constantin on 09.10.2017.
//

#ifndef OSDTESTER_STRINGHELPER_H
#define OSDTESTER_STRINGHELPER_H

#include <string>
#include <sstream>
#include "android/log.h"


static const int countDigits(long long n)
{
    if(n<0){
        return (int)std::floor(log10(-n))+1;
    }
    return (int)std::floor(log10(n));
}


static const std::wstring intToString(const int value,const int maxStringL){
    std::wstringstream s;
    s << value;
    if(s.str().length()>maxStringL){
        //LOGV3("XXX %d",value);
        return L"E";
    }
    return s.str();
}

static const void doubleToString(std::wstring& sBeforeCome,std::wstring& sAfterCome,double value,int maxLength,int resAfterCome){
    long beforeCome=(long)value;
    double afterCome=value-beforeCome;
    if(afterCome<0)afterCome*=-1;
    //LOGV3("Number:%f, beforeCome:%d afterCome:%f",value,beforeCome,afterCome);

    std::wstringstream ss;
    ss<<beforeCome;

    //If we already have to many chars, we can only return an error
    const auto stringLengthBeforeCome=ss.str().length();
    if(stringLengthBeforeCome>maxLength){
        sBeforeCome=L"E";
        sAfterCome=L"E";
        return;
    }
    sBeforeCome=ss.str();
    //e.g. return 10 instead of 10.
    if(stringLengthBeforeCome+1>=maxLength || resAfterCome==0){
        sAfterCome=L"";
        return;
    }
    //calculate how many chars we can append after the come
    const auto charsLeft=(int)(maxLength-(stringLengthBeforeCome+1));
    if(resAfterCome>charsLeft){
        resAfterCome=charsLeft;
    }
    std::wstringstream ss2;
    ss2<<".";
    long t=(long)(afterCome*(std::pow(10,resAfterCome)));
    ss2<<t;
    //LOGV3("Number:%f, beforeCome:%d afterCome:%f str: %s",value,beforeCome,afterCome,ss.str().c_str());
    if(ss2.str().length()>maxLength){
        sBeforeCome=L"E2";
        sAfterCome=L"E2";
        return;
    }
    sAfterCome=ss2.str();
}

static const std::wstring doubleToString(double value, int maxLength,
                                        int resAfterCome){
    std::wstring beforeCome;
    std::wstring afterCome;
    doubleToString(beforeCome,afterCome,value,maxLength,resAfterCome);
    return beforeCome+afterCome;
}









#endif //OSDTESTER_STRINGHELPER_H
