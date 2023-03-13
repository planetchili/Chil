#pragma once 

#define ZZ_CAT_(x,y) x ## y  
#define ZC_CAT(x,y) ZZ_CAT_(x,y)  
#define ZZ_STR_(x) #x  
#define ZC_STR(x) ZZ_STR_(x)  
#define ZC_WSTR(x) ZC_CAT(L,ZC_STR(x))