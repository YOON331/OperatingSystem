#include <stdio.h>
#include <stdint.h>
#include "smalloc.h"

int main ()
{
	void *p1, *p2, *p3, *p4, *p5, *p6 ;

	smdump() ;

	p1 = smalloc(2000) ; 
	printf("smalloc(2000):%p\n", p1) ; 
	smdump() ;

	p2 = smalloc(3500) ; 
	printf("smalloc(3500):%p\n", p2) ; 
	smdump() ;

	p3 = smalloc(300) ; 
	printf("smalloc(300):%p\n", p3) ; 
	smdump() ;

	p4 = smalloc(1800) ; 
	printf("smalloc(1800):%p\n", p4) ; 
	smdump() ;

	p5 = smalloc(3500) ; 
	printf("smalloc(3500):%p\n", p5) ; 
	smdump() ;

	sfree(p3) ; 
	printf("sfree(%p)\n", p3) ;  
	smdump() ;

	sfree(p2) ; 
	printf("sfree(%p)\n", p2) ; 
	smdump() ;

	smcoalesce();
	printf("smcoalesce()\n") ;
	smdump();

	p4 = srealloc(p4, 2500) ;
	printf("srealloc(p4, 2500):%p\n", p4) ;
	smdump() ;

	p5 = srealloc(p5, 2500) ;
	printf("srealloc(p5, 2500):%p\n", p5) ;
	smdump() ;

	p6 = smalloc_mode(500, bestfit) ;
	printf("smalloc_mode(500, bestfit):%p\n", p6) ;
	smdump() ;

	p3 = smalloc_mode(1000, worstfit) ;
	printf("smalloc_mode(1000, worstfit):%p\n", p3) ;
	smdump() ;

	sfree(p1);
	sfree(p3);
	sfree(p4);
	sfree(p5);
	sfree(p6);
	printf("sfree all used memory slot\n") ; 
	smdump() ;

	smcoalesce();
	printf("smcoalesce()\n") ;
	smdump();

}