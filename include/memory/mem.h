#pragma once


 #include "intdefs.h"
struct Memory {
   void* begin;
   void* end;
 };
 typedef struct Memory Memory;


 struct MemoryConst {
   const void* begin;
   const void* end;
 };
 typedef struct MemoryConst MemoryConst;
