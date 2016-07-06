#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "lib/xalloc.h"
#include "lib/stack.h"
#include "lib/contracts.h"
#include "lib/c0v_stack.h"
#include "lib/c0vm.h"
#include "lib/c0vm_c0ffi.h"
#include "lib/c0vm_abort.h"

/* call stack frames */
typedef struct frame_info frame;
struct frame_info {
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Function body */
  size_t pc;     /* Program counter */
  c0_value *V;   /* The local variables */
  uint16_t func_length ;
};

void frame_free(stack_elem x) 
{
  REQUIRES(x != NULL);
  frame *f = (frame*)(x) ;
  c0v_stack_free(f->S) ;
  free(f->V) ;
  free(f);
  return ;
}


int execute(struct bc0_file *bc0) {
  REQUIRES(bc0 != NULL);

  /* Variables */
  c0v_stack_t S = c0v_stack_new() ; /* Operand stack of C0 values */
  ubyte *P = ((bc0->function_pool)[0]).code ; //  assigned to main      
   /* The array of bytes that make up the current function */
  uint16_t func_length = ((bc0->function_pool)[0]).code_length ;
  size_t pc = 0 ;  //init at 0 of main 
   /* Your current location within the current byte array P */
  uint16_t vlength = ((bc0->function_pool)[0]).num_args + ((bc0->function_pool)[0]).num_vars ;
  c0_value *V = xmalloc(sizeof(c0_value) * vlength );   
  // REMENMBER TO FREE 
/* The local variables (you won't need this till Task 2) */
  //(void) V;

  /* The call stack, a generic stack that should contain pointers to frames */
  /* You won't need this until you implement functions. */
  gstack_t callStack = stack_new() ;  //REMEMBER TO FREE !!!!

  while (true) {
    
#ifdef DEBUG
    /* You can add extra debugging information here */
    fprintf(stderr, "Opcode %x -- Stack size: %zu -- PC: %zu\n",
            P[pc], c0v_stack_size(S), pc);
#endif
    
    switch (P[pc]) {
      
    /* Additional stack operation: */

    case POP: {
      pc++;
      c0v_pop(S);
      break;
    }

    case DUP: {
      pc++;
      c0_value v = c0v_pop(S);
      c0v_push(S,v);
      c0v_push(S,v);
      break;
    }
      
    case SWAP: {
      ASSERT(c0v_stack_size(S) >= 2) ; // SHOULD ERROR BE RAISED ?
      pc++ ;
      c0_value top = c0v_pop(S) ;
      c0_value bot = c0v_pop(S) ;
      c0v_push(S, top) ;
      c0v_push(S, bot) ;
      break ;
      
    }

    /* Returning from a function.
     * This currently has a memory leak! It will need to be revised
     * when you write INVOKESTATIC. */

    case RETURN: {
    

      // Free everything before returning from the execute function!
      if(stack_empty(callStack))
	{
	  int retval = val2int(c0v_pop(S));
#ifdef DEBUG
      fprintf(stderr, "Returning %d from execute()\n", retval);
#endif
	  assert(c0v_stack_empty(S)) ;
	  c0v_stack_free(S) ;  // main op stack       
	  free(V) ;
	  stack_free(callStack, &frame_free) ; // free callStack
	  return retval;
	}
      else
	{
	  assert( ! stack_empty(callStack)) ;
	  frame *retFn = (frame*)(pop(callStack)) ;
	  c0_value val ;
	  if(c0v_stack_empty(S)) val = int2val(0) ; //if return type is void
	  else val = c0v_pop(S) ;
	  c0v_push(retFn->S , val) ; //push return val onto stack
	  c0v_stack_free(S) ; //free curr stack 
	  free(V) ;
	  pc = retFn->pc ;
	  V = retFn->V ;
	  S = retFn->S ;
	  P = retFn->P ;
	  func_length = retFn->func_length ;
	  free(retFn) ;//free frame struct 
	  break ;	  
	}
    }


    /* Arithmetic and Logical operations */

    case IADD:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ; 
      int32_t numTop = val2int(c0v_pop(S)) ; 
      int32_t numBot = val2int(c0v_pop(S)) ;
      uint32_t sum = (uint32_t)(numTop) + (uint32_t)(numBot) ;
      c0v_push(S, int2val((int32_t)(sum))) ;
      break ; 

    }
      
    case ISUB:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      uint32_t diff = (uint32_t)(numBot) - (uint32_t)(numTop) ;
      c0v_push(S, int2val((int32_t)(diff))) ;
      break ;
    }
      
    case IMUL:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      uint32_t mul = (uint32_t)(numTop) * (uint32_t)(numBot) ;
      c0v_push(S, int2val((int32_t)(mul))) ;
      break ;
    }
      
    case IDIV:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      if(numTop == (int)(0) || (numBot == (int)(0x80000000) && numTop == (int)(-1) ))
        {
          c0_arith_error("Division by 0") ;
          // MUST WE FREE ALL AND RETURN ??? NO NEED TO FREE
        }
      int32_t quo = (numBot) / (numTop) ;
      c0v_push(S, int2val(quo)) ;
      break ;
    }
      
    case IREM:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      if(numTop == (int)(0) || (numBot == (int)(0x80000000) && 
				numTop == (int)(-1)))  
	{
	  c0_arith_error("Modulo by 0") ;
	  // MUST WE FREE ALL AND RETURN ??? NO NEED TO FREE
	}
      uint32_t mod = (numBot) % (numTop) ;
      c0v_push(S, int2val(mod)) ;
      break ;
    }
      
    case IAND:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      int32_t and = numBot & numTop ;
      c0v_push(S, int2val(and)) ;
      break ;
    }
  
    case IOR:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      int32_t or = numBot | numTop ;
      c0v_push(S, int2val(or)) ;
      break ;
    }
      
    case IXOR:{

      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      int32_t xor = numBot ^ numTop ;
      c0v_push(S, int2val(xor)) ;
      break ;
    }
      
    case ISHL:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      if(numTop < 0 || numTop > 31)
	{
	  c0_arith_error("invalid shift value") ;
	}
      int32_t shift = numBot << numTop ;
      c0v_push(S, int2val(shift)) ;
      break ;
    }
      
    case ISHR:{
      ASSERT(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int32_t numTop = val2int(c0v_pop(S)) ;
      int32_t numBot = val2int(c0v_pop(S)) ;
      if(numTop < 0 || numTop > 31)
        {
          c0_arith_error("invalid shift value") ;
        }
      int32_t shift = numBot >> numTop ;
      c0v_push(S, int2val(shift)) ;
      break ;
    }
      
      
    /* Pushing constants */

    case BIPUSH:{
      pc++ ; 
      assert(pc < func_length) ;
      ubyte num = P[pc] ;
      int32_t cons = (int32_t)(int8_t)(num) ; //sign extended
      c0v_push(S, int2val(cons)) ;
      pc++ ; //next instruction 
      assert(pc < func_length) ;
      break ;
      
    }

    case ILDC:{
      assert(pc + 3 < func_length) ;
      pc++ ;
      ubyte msb = P[pc] ;
      pc++ ;
      ubyte lsb = P[pc] ;
      uint16_t ind = ((uint16_t)msb << 8) | ((uint16_t)(lsb)) ;
      ASSERT(ind < bc0->int_count) ;
      c0_value num = int2val((bc0->int_pool)[ind]) ;
      c0v_push(S, num) ;
      pc++ ;
      break ;
    }
      
    case ALDC:{
      assert(pc + 3 < func_length) ;
      pc++ ;
      ubyte msb = P[pc] ;
      pc++ ;
      ubyte lsb = P[pc] ;
      uint16_t ind = ((uint16_t)msb << 8) | ((uint16_t)(lsb)) ;
      ASSERT(ind < bc0->string_count) ;
      c0_value str = ptr2val((void*)((bc0->string_pool)+ ind)) ;
      c0v_push(S, str) ;
      pc++ ;
      break ;
    }

    case ACONST_NULL:{
      pc++ ;
      void *nuPtr = NULL ;
      c0v_push(S, ptr2val(nuPtr)) ;
      break ;

    }



    /* Operations on local variables */

    case VLOAD:{
      assert((pc + 2) < func_length) ; // has at least 2 bytes
      pc++ ;
      ubyte ind = P[pc] ; 
      ASSERT( ind < vlength) ;
      c0v_push(S, V[ind]) ;
      pc++ ;
      break ;
    }
      
    case VSTORE:{
      assert((pc + 2) < func_length) ; // has at least 2 bytes                 
      pc++ ;
      ubyte ind = P[pc] ;
      ASSERT( ind < vlength) ;
      V[ind] = c0v_pop(S) ;
      pc++ ;
      break ;

    }
      
      
    /* Control flow operations */

    case NOP:{
      pc++ ;
      break ;
    }
      
    case IF_CMPEQ:{
      assert(c0v_stack_size(S) >= 2 ) ;
      c0_value topeq = c0v_pop(S) ;
      c0_value boteq = c0v_pop(S) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msbeq = P[pc] ;
      pc++ ;
      ubyte lsbeq = P[pc] ;
      int16_t ofsteq =(int16_t)(((uint16_t)msbeq << 8) | ((uint16_t)(lsbeq))) ;
      pc = pc - 2 ; // reset to start point
      if(val_equal(boteq, topeq))  pc = pc + ofsteq ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
    }
    case IF_CMPNE:{
      assert(c0v_stack_size(S) >= 2 ) ;
      c0_value topne = c0v_pop(S) ;
      c0_value botne = c0v_pop(S) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msbne = P[pc] ;
      pc++ ;
      ubyte lsbne = P[pc] ;
      int16_t ofstne = (int16_t)(((uint16_t)msbne << 8) | ((uint16_t)(lsbne))) ;
      pc = pc - 2 ; // reset to start point
      if(! val_equal(botne, topne))  pc = pc + ofstne ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
    }
    case IF_ICMPLT:{
       assert(c0v_stack_size(S) >= 2 ) ;
       int32_t toplt = val2int(c0v_pop(S)) ;
       int32_t botlt = val2int(c0v_pop(S)) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msblt = P[pc] ;
      pc++ ;
      ubyte lsblt = P[pc] ;
      int16_t ofstlt = (int16_t)(((uint16_t)msblt << 8) | ((uint16_t)(lsblt))) ;
      pc = pc - 2 ; // reset to start point
      if(botlt < toplt)  pc = pc + ofstlt ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
    }
      
    case IF_ICMPGE:
      assert(c0v_stack_size(S) >= 2 ) ;
       int32_t topge = val2int(c0v_pop(S)) ;
       int32_t botge = val2int(c0v_pop(S)) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msbge = P[pc] ;
      pc++ ;
      ubyte lsbge = P[pc] ;
      int16_t ofstge = (int16_t)(((uint16_t)msbge << 8) | ((uint16_t)(lsbge))) ;
      pc = pc - 2 ; // reset to start point
      if(botge >= topge)  pc = pc + ofstge ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
      
    case IF_ICMPGT:{
      assert(c0v_stack_size(S) >= 2 ) ;
       int32_t topgt = val2int(c0v_pop(S)) ;
       int32_t botgt = val2int(c0v_pop(S)) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msbgt = P[pc] ;
      pc++ ;
      ubyte lsbgt = P[pc] ;
      int16_t ofstgt = (int16_t)(((uint16_t)msbgt << 8) | ((uint16_t)(lsbgt))) ;
      pc = pc - 2 ; // reset to start point
      if(botgt > topgt)  pc = pc + ofstgt ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
    }

    case IF_ICMPLE:{
      assert(c0v_stack_size(S) >= 2 ) ;
       int32_t tople = val2int(c0v_pop(S)) ;
       int32_t botle = val2int(c0v_pop(S)) ;
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msble = P[pc] ;
      pc++ ;
      ubyte lsble = P[pc] ;
      int16_t ofstle = (int16_t)(((uint16_t)msble << 8) | ((uint16_t)(lsble))) ;
      pc = pc - 2 ; // reset to start point
      if(botle <= tople)  pc = pc + ofstle ;
      else pc = pc + 3 ; // 3 = <c0,c1> + 1
      assert(pc < func_length) ;
      break ;
    }
      
    case GOTO:{
      assert((pc +3) < func_length) ;
      pc++ ;
      ubyte msbgo = P[pc] ;
      pc++ ;
      ubyte lsbgo = P[pc] ;
      int16_t ofstgo = (int16_t)(((uint16_t)msbgo << 8) | ((uint16_t)(lsbgo))) ;
      pc = pc - 2 + ofstgo ; 
      assert(pc < func_length) ;
      break ;
    }
      
    case ATHROW:{
      assert(c0v_stack_size(S) >= 1) ;
      pc++ ;
      void *thPtr = val2ptr(c0v_pop(S)) ;
      c0_user_error((char*)(thPtr)) ;
      
    }

    case ASSERT:{
      pc++ ;
      assert(c0v_stack_size(S) >= 2) ;
      void *assPtr  = val2ptr(c0v_pop(S)) ;
      int assX = val2int(c0v_pop(S)) ;
      if(assX == 0) c0_assertion_failure((char*)assPtr) ;
      break ;
    }


    /* Function call operations: */

    case INVOKESTATIC:{
      assert((pc + 3) < func_length) ; 
      pc++ ;
      ubyte msbst = P[pc] ;
      pc++ ;
      ubyte lsbst = P[pc] ;
      int16_t indst = (int16_t)(((uint16_t)msbst << 8) | ((uint16_t)(lsbst))) ;
      pc++ ; // at return address 
      size_t retAddrst = pc ; // return address 
      
      assert(indst < bc0->function_count) ;
      uint16_t nArgs = ((bc0->function_pool)[indst]).num_args ;
      uint16_t vlenst = nArgs + ((bc0->function_pool)[indst]).num_vars ;
      c0_value* newVst = xmalloc(sizeof(c0_value) * vlenst) ; //free in return 
      assert(c0v_stack_size(S) >= nArgs) ; // ensure at least n args
      for(int i = 0 ; i < nArgs; i++)
	{
	  newVst[nArgs-1-i] = c0v_pop(S) ;  // palce parameters
	}
      c0v_stack_t newSst = c0v_stack_new() ; //free in return 
      ubyte* newPst = ((bc0->function_pool)[indst]).code ;
      

      frame *newFramest = xmalloc(sizeof(frame)) ;
      newFramest->pc = retAddrst ;
      newFramest->P = P ;
      newFramest->S = S ;
      newFramest->V = V ;
      newFramest->func_length = func_length ;
      push(callStack, (void*)(newFramest)) ;

      S = newSst ;
      V = newVst ;
      pc = (uint16_t)0 ;
      P = newPst ;
      func_length = ((bc0->function_pool)[indst]).code_length ; 
      //set to length of curr function being executed
      break ;
      
    }
      
    case INVOKENATIVE:{
      assert((pc + 3) < func_length) ; 
      pc++ ;
      ubyte msbna = P[pc] ;
      pc++ ;
      ubyte lsbna = P[pc] ;
      int16_t indna = (int16_t)(((uint16_t)msbna << 8) | ((uint16_t)(lsbna))) ;
      pc++ ; // at return address 
       
      assert(indna < bc0->native_count) ;
      uint16_t nArgsna = ((bc0->native_pool)[indna]).num_args ;
      uint16_t tableInd = ((bc0->native_pool)[indna]).function_table_index ;
      c0_value *newVna = xmalloc(sizeof(c0_value) * nArgsna) ; //free now 
      assert(c0v_stack_size(S) >= nArgsna) ; // ensure at least n args
      for(int i = 0 ; i < nArgsna; i++)
	{
	  newVna[nArgsna-1-i] = c0v_pop(S) ;  // palce parameters
	}
      
      c0_value retVal = (*native_function_table[tableInd])(newVna) ;
      c0v_push(S, retVal) ;
      
      free(newVna) ;  // free array
    
      break ;

    }


    /* Memory allocation operations: */

    case NEW:{
      assert(pc + 2 < func_length) ;
      pc++ ;
      size_t sNew = P[pc] ; //size of data
      pc++ ; // next instr
      void *newPtr = xcalloc(1, sNew) ; //an obj of size 1
      c0v_push(S, ptr2val(newPtr)) ;
      break ;
      

    }
      
    case NEWARRAY:{
      assert(pc + 2 <= func_length) ;
      pc++ ;
      ubyte arrElemSize = P[pc] ;
      pc++ ; //next instr
      assert(c0v_stack_size(S) >= 1) ;
      int count = val2int(c0v_pop(S)) ;
      // assert(count >= 0) ; // ASSERT OR ERROR ?
      if(count < 0) c0_memory_error("Invalid Array Size") ;
      void *newArr = xcalloc(count, arrElemSize) ;
      c0_array *arrStr = xmalloc(sizeof(c0_array)) ;
      arrStr->count = count ;
      arrStr->elt_size = arrElemSize ;
      arrStr->elems = newArr ;
      c0v_push(S, ptr2val(arrStr)) ;
      
      break ;
      
    }
    case ARRAYLENGTH:{
      assert(c0v_stack_size(S) >= 1) ;
      pc++ ;
  
      c0_array *lArrStr = (c0_array*)(val2ptr(c0v_pop(S))) ;
     
      if(lArrStr == NULL) c0_memory_error("Dereferencing NULL") ;
      int arrLen = lArrStr->count ;
      c0v_push(S, int2val(arrLen)) ;
      break ;
      
    }


    /* Memory access operations: */

    case AADDF:{
      assert(c0v_stack_size(S) >= 1) ;
      assert(pc + 2 <= func_length) ;
      pc++ ;
      ubyte strOff = P[pc] ;
      pc++ ;  // next instr
      char *vstrPtr = (char*)val2ptr(c0v_pop(S)) ;
      vstrPtr = &vstrPtr[strOff] ;
      c0v_push(S, ptr2val((void*)vstrPtr)) ;
      break ;
    }
      
    case AADDS:{
      pc++ ;
      assert(c0v_stack_size(S) >= 2) ;
      int arrInd = val2int(c0v_pop(S)) ;
      c0_array *aaStr = (c0_array*)(val2ptr(c0v_pop(S))) ;
      if(arrInd < 0 || arrInd >= aaStr->count) 
	{
	  c0_memory_error("Array Out of Bounds") ;
	}
      if(aaStr == NULL) c0_memory_error("Dereferencing NULL") ;
      char *aaArr = (char*)(aaStr->elems) ;
      int aaOfst = arrInd * aaStr->elt_size;
      void *aaAddr = (void*)(&aaArr[aaOfst]) ;
      c0v_push(S, ptr2val(aaAddr)) ;
      break ;
      
    }
      
    case IMLOAD:{
      pc++;
      assert(pc < func_length) ;
      void *imPtr = val2ptr(c0v_pop(S)) ;

      if(imPtr == NULL) c0_memory_error("Dereferencing NULL") ;

      int *intPtr = (int*)(imPtr) ;
      int imRes = *intPtr ;
      c0v_push(S, int2val(imRes)) ;
      break ;
    }
      
    case IMSTORE:{
      assert(pc + 1 < func_length) ;
      assert(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int stVal = val2int(c0v_pop(S)) ;
      void *stPtr = val2ptr(c0v_pop(S)) ;
      
      if(stPtr == NULL) c0_memory_error("Dereferencing NULL") ;

      int *stIntPtr = (int*)(stPtr) ;
      *stIntPtr = stVal ; //store into mem
      break ;
      
      
    }
      
    case AMLOAD:{
      pc++;
      //assert(pc < func_length) ;
      void **amPtr = val2ptr(c0v_pop(S)) ;

      if(amPtr == NULL) c0_memory_error("Dereferencing NULL") ;

      void *BPtr = *amPtr ;
      c0v_push(S, ptr2val(BPtr)) ;

      break ;
    }
      
    case AMSTORE:{
      pc++ ;
      //assert(pc < func_length) 
      assert(c0v_stack_size(S) >= 2 ) ;
      void *stPtr = val2ptr(c0v_pop(S)) ;
      void **amA = val2ptr(c0v_pop(S)) ;
      
      if(amA == NULL) c0_memory_error("Dereferencing NULL ") ;
      
      *amA = stPtr ;
      break ;
      
    }
      
    case CMLOAD:{
      assert(c0v_stack_size(S) >= 1) ;
      pc++ ;
      void *cPtr = val2ptr(c0v_pop(S)) ;
      char *chPtr = (char*)(cPtr) ;

      if(chPtr == NULL) c0_memory_error("Dereferencing NULl") ;

      char cmC = *chPtr ;
      char cmMask = 0x7f ;
      cmC = cmC & cmMask ; // 0 the msb
      assert(0 <= (int)(cmC)) ;
      c0v_push(S, int2val((int)(cmC))) ;
      break ;
    }
      
    case CMSTORE:{
      assert(c0v_stack_size(S) >= 2) ;
      pc++ ;
      int cmVal = val2int(c0v_pop(S)) ;
      char *cmstPtr = (char*)( val2ptr(c0v_pop(S))) ;

      if(cmstPtr == NULL) c0_memory_error("Dereferencing NULL") ;
      
      // assert(0 <= cmVal && cmVal < 128) ;
      int mask = 0x7f ;
      cmVal = cmVal & mask ;
      assert( 0 <= cmVal && cmVal < 128) ;
      *cmstPtr = (char)(cmVal) ;
      break ;

    }
      
    default:
      fprintf(stderr, "invalid opcode: 0x%02x\n", P[pc]);
      abort();
    }
  }
  
  /* cannot get here from infinite loop */
  assert(false);
}

