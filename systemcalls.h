#include <lib.h>
#include <unistd.h>
#include <minix/callnr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


 int IG_Lookup(int groups[]){
     message m;
//     m.m7_p1 = groups;
     return(_syscall( PM_PROC_NR, IGLOOKUP, &m ));
 }

  int IG_Create(int * groupId ,char *name,int id){
 
      message m;
	  char *n=name ;
		
	  m.m7_i1= id;
	  m.m7_i2 = getuid();
      m.m7_p1 = n;
	  
    
      return (_syscall( PM_PROC_NR, IGCREATE, &m ));
}

 int IG_Delete(int groupNo){
message m;

	m.m7_i1 = (int)groupNo;
	m.m7_i2 = getuid();
	
	return(_syscall( PM_PROC_NR, IGDELETE, &m ));

}

 int IG_Publisher(int groupNo ){
message m;
   m.m7_i1=(int)groupNo;
   
	 m.m7_i2 = getuid();
   return( _syscall( PM_PROC_NR, IGPUBLISHER, &m ) );

}

 int IG_Subscriber(int groupNo ){
     message m;
	m.m7_i1=groupNo;
     
	 m.m7_i2 = getuid();
   return( _syscall( PM_PROC_NR, IGSUBSCRIBER, &m ) );
}

 int IG_Publish(int groupNo , char *msg){
message m;
   char *p=msg;
   m.m7_i1 = groupNo;
     
	 m.m7_i2 = (pid_t)getuid();
   m.m7_p1 = msg;
   return( _syscall( PM_PROC_NR, IGPUBLISH, &m ) );

}

 int IG_Retreive(int groupNo , char *msg){
 message m;	
   m.m7_i1 = groupNo;
   m.m7_p1 = msg;
    
	 m.m7_i2 = (pid_t)getuid();
   return(_syscall( PM_PROC_NR, IGRETRIEVE, &m ));

}

 int subscribeToPrivelegedList(int process_Id )
{
   message m;
   m.m7_i1=(int)process_Id;
   m.m7_i2 = getuid();
   return( _syscall( PM_PROC_NR, IGADDTOPVL, &m ) );
}

 int unsubscribeFromPrivelegedList(int process_id )
{
   message m;
   m.m7_i1=(int)process_id;
   m.m7_i2 = getuid();
   return( _syscall( PM_PROC_NR, IGREMOVEFROMPVL, &m ) );
}

 int registerToSecuredGroup ( int group_Id, int process_Id)
{
	message m;
	m.m7_i1 = (int)process_Id;     
	m.m7_i2 = getuid();       
    m.m7_i3 = (int)group_Id;     
   	return(_syscall( PM_PROC_NR, IGADDSECUREGROUP, &m ));
}

 int unregisterFromSecuredGroup ( int group_Id,int process_Id)
{
	message m;
	m.m7_i1 = (int)process_Id;
	m.m7_i2 = getuid();
    m.m7_i3 = (int)group_Id;
   	return(_syscall( PM_PROC_NR, IGREMOVEFROMSECUREGROUP, &m ));
}

 int reset_MINIX(void)
{
	message m;
   	return(_syscall( PM_PROC_NR, 81, &m ));
}
 
