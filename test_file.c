#include<stdio.h>
#include<string.h>
 #include "systemcalls.h"

int adding_to_privileged_ACL_Wrong(void);
int removing_from_privileged_ACL_Error(void);
int user_not_in_privileged_ACL_Error(void);
int user_not_the_creator_Error(void);
int user_reg_as_publisher_ACL_Error(void);
int user_reg_as_subscriber_ACL_Error(void);
int user_reg_as_publisher_Sec_group_Error(void);
int user_reg_as_subscriber_Sec_group_Error(void);
  
#define SECURE_GROUP 1 
#define USER_1 10
#define USER_2 12
#define USER_3 11
#define USER_4 13




int main(){

 int option=-1;  
	 
	 
	 printf("Project 3 \n"); 
	while(1){
	    
		 
		printf("\nThese are the test cases written. Please select one  \n");
		printf("1. A user who is not a root tries to add a user to a privileged ACL\n");
		printf("2. A user who is not a root tries to remove a user to a privileged ACL \n");        
		printf("3. User who is not a member of privileged ACL tries to create a group \n");
		printf("4. User who is not the creator of Group tries to add user to ACL\n");
		printf("5. User who is not in privileged ACL tries to register as publisher  \n");
        printf("6. User who is not in privileged ACL tries to register as subscriber  \n");
		printf("7. User who is not in group (secured) tries to register as publisher   \n");
        printf("8. User who is not in group(secured) tries to register as subscriber   \n");
		printf("9. Exit\n");
		printf("Enter your option : ");
		scanf(" %d",&option);
		
		switch(option){
    		 case 1 : adding_to_privileged_ACL_Wrong();
    			break;
    		 case 2 : removing_from_privileged_ACL_Error();
    			break;
    		 case 3 : user_not_in_privileged_ACL_Error();
    			break;
    		 case 4 : user_not_the_creator_Error();
    			break;
    		 case 5 : user_reg_as_publisher_ACL_Error();
    			break;
    		 case 6 : user_reg_as_subscriber_ACL_Error();
    			break;
    		 case 7 : user_reg_as_publisher_Sec_group_Error();
    			break;
    		 case 8 : user_reg_as_subscriber_Sec_group_Error();
    			break;			
    		 case 9 : exit(1);
    			break;
    		 default : printf("Wrong input \n");
    			break;
    	}
     }
}



int adding_to_privileged_ACL_Wrong(void)
{

    int pid;
    int value;
    
     printf(" Adding to privileged list Error\n");
     
    
    resetAll();

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_1);

        value = subscribeToPrivelegedList(USER_2);
        if(value!=1)
        {
            printf("\n Failed\n");
        }
        else
        {
            printf("\n Test case passed:adding to priveleged ACL by USER_1 failed\n");
            
        } 
        exit(127);
    }
    waitpid(pid,NULL,0);    
}


int removing_from_privileged_ACL_Error(void)
{
    int value;

     printf(" Removing User From Privilileged  list Error\n");
      
    
    resetAll();
    

    value = unsubscribeFromPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\n Test case passed: Removing from privileged ACL by USER_1 failed\n");
         

    }     
    
}


int user_not_in_privileged_ACL_Error(void)
{
    int value;
    int group_Id =1;
	char *name = "First";
     printf(" User who is not a member of  privilege ACL trying to create group \n");
    
    
    resetAll();
    

    value = IG_Create(group_Id,name,SECURE_GROUP);
    if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\n Test case passed USER_1 failed to create group\n");
         

    }     
    
}

int user_not_the_creator_Error(void)
{
    int value;
    int group_Id =1;
    int pid;
	char *name = "first";

     printf(" User who is not creator of group tries to add to ACL\n");
     
    
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Priviliged ACL\n");
    }    
    
    value = subscribeToPrivelegedList(USER_3);  
    if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_3 to the Priviliged ACL\n");
    }    
     
    value = subscribeToPrivelegedList(USER_2); 
   if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_2 to the Priviliged ACL\n");
    }    
    

    group_Id =1;

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_1);
       
        value = IG_Create(group_Id,name,SECURE_GROUP);

        if(value != 1)
        {
            printf("\n  Failed\n");
        }    
        else
        {
            printf("\nSecure Interest group createdL\n");
        } 
        printf("\nSTEP 2 Passed\n"); 
            
        exit(127);
    }
    waitpid(pid,NULL,0);  

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_3);
       
       value = registerToSecuredGroup(group_Id,USER_2);
       if(value != 1)
       {
           printf("\n  Failed\n");
       }    
       else
       {
       
           printf("\n USER_3 failed to add USER_2 to group %d",group_Id);
       } 

        printf("\nSTEP 3 Passed\n"); 
            
        exit(127);
    }

    waitpid(pid,NULL,0);     
    
}


int user_reg_as_publisher_ACL_Error(void)
{
    int value;
	char *name = "First";

    char publisher_msg[200]={0};
    int group_Id =1;
    int pid;

     printf(" User who is not a part of ACL tries to register as publisher  \n");
     
    
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the priviliged ACL\n");
    }    
    

    group_Id =1;

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_1);
       
        value = IG_Create(group_Id,name,SECURE_GROUP);

       if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nSecured group created\n");
    }    
        printf("\nSTEP 2 Passed\n"); 
            
        exit(127);
    }
    waitpid(pid,NULL,0);  

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_3);
   
        value = IG_Publisher(group_Id);   
        if(value!=1)
        {
            printf("\n  Failed\n");
        }    
        else
        {
       
            printf("\nTest Passed : USER_3 registered as publisher Failed in group %d",group_Id);
        }  
        exit(127); 
    }

    waitpid(pid,NULL,0);     
    
}

int user_reg_as_subscriber_ACL_Error(void)
{
    int value;
    int group_Id =1;
    int pid;
	
	char *name = "First";

    char publisher_msg[200]={0};

     printf(" User who is not a member of priviliged ACL tries to register as subscriber  \n");
    
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
    if(value != 1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the  ACL\n");
    }    
    

    group_Id =1;

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_1);
       
        value = IG_Create(group_Id,name,SECURE_GROUP);

        if(value != 1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nSecure group created\n");
    }    
    printf("\nSTEP 2 Passed\n"); 
            
        exit(127);
    }
    waitpid(pid,NULL,0);  

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_3);
   
        value = IG_Subscriber(group_Id);   
        if(value != 1)
        {
            printf("\nFailed\n");
        }    
        else
        {
       
            printf("\nTest Passed : Registering as a publisher USER_3 failed in group %d",group_Id);
        }  
        exit(127); 
    }

    waitpid(pid,NULL,0);     
    
}

int user_reg_as_publisher_Sec_group_Error(void)
{
    int value;
    int group_Id =1;
    int pid;
	
	char *name = "First";

    char publisher_msg[200]={0};

     printf(" User who is not in Secured Group ACL tries to register as publisher \n");
    
    
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the privileged ACL\n");
    }    

    value = subscribeToPrivelegedList(USER_3);
    if(value!=1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nAdded user USER_3 to the privileged ACL\n");
    }    


    group_Id =1;

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_1);
       
        value = IG_Create(group_Id,name,SECURE_GROUP);

        if(value != 1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nSecure group created\n");
    }    
	
	printf("\nSTEP 2 Passed\n"); 
            
        exit(127);
    }
    waitpid(pid,NULL,0);  

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_3);
   
        value = IG_Publisher(group_Id);   
         if(value != 1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nRegistering USER_3 as a publisher failed in group %d\n", group_Id);
    }    
        exit(127); 
    }

    waitpid(pid,NULL,0);     
    
}


int user_reg_as_subscriber_Sec_group_Error(void)
{
    int value;
    int group_Id =1;
    int pid;

    char publisher_msg[200]={0};
	char *name = "First";

     printf(" User who is not in group ACL  tries to register as subscriber \n");
     
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
    if(value != 1)
    {
        mprintf("\n  Failed\n");
    }
    else
    {
        mprintf("\nAdded user USER_1 to the priviliged ACL\n");
    }    

    value = subscribeToPrivelegedList(USER_3);
    if(value != 1)
    {
        mprintf("\n  Failed\n");
    }
    else
    {
        mprintf("\nAdded user USER_3 to the priviliged ACL\n");
    }    


    group_Id =1;

    pid = fork();
    
    if (pid == 0)
    {      
        setuid(USER_1);
       
          value = IG_Create(group_Id,name,SECURE_GROUP);

        if(value != 1)
    {
        printf("\n Failed\n");
    }
    else
    {
        printf("\nSecure group created\n");
    }    
	
	printf("\nSTEP 2 Passed\n"); 
            
        exit(127);
    }
    waitpid(pid,NULL,0);  

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_3);
   
        value = IG_Subscriber(group_Id);   
        if(value!=1)
        {
            printf("\n  Failed\n");
        }    
        else
        {
       
            printf("\nTest Passed : Registering USER_3   as subscriber Failed in group %d",group_Id);
        }  
        exit(127); 
    }

    waitpid(pid,NULL,0);     
    
}

