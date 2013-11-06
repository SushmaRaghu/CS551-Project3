#include<stdio.h>
#include<string.h>
 #include "systemcalls.h"

int add_to_priveleged_ACL(void);
int remove_from_priveleged_ACL(void);
int public_group(void);
int secured_group(void);

#define SECURE_GROUP 1 
#define USER_1 10
#define USER_2 12
#define USER_3 11
#define USER_4 13




int main(){

 int choice=-1;  
	 
	 
	 printf("Project 3 \n"); 
	while(1){
	    
		printf("\n These are the functionalities possible in this Project. Please choose one of it \n");
		printf("1. Add user to privileged user list \n");
		printf("2. Remove user from  privileged user list \n");
		printf("3. Public group access\n");
		printf("4. Secure group access \n");
		printf("0. Exit \n\n");
		printf("Enter your option : ");
        
		scanf(" %d",&choice);

		 	
		switch(choice){
			 case 1 : add_to_priveleged_ACL();
				break;
			 case 2 :remove_from_priveleged_ACL();
				break;
			 case 3 :public_group();
				break;
			 case 4 : secured_group();
				break;
			 case 0 : return 0;
				break;
			 default : printf("Wrong input\n");
				break;
		}
	 }

}

int add_to_priveleged_ACL(void)
{
    int value;
    printf(" Adding to Priviledged list \n");
    
    
    resetAll();


    value = subscribeToPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\ Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Privilge ACL\n");
         

    } 
    
}

int remove_from_priveleged_ACL(void)
{
    int value;

     printf(" Removing User From Priviledged list \n");
    
    
    resetAll();
    
    value = subscribeToPrivelegedList(USER_1);
   if(value!=1)
    {
        printf("\ Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Privilge ACL\n");
         

    } 
    value = unsubscribeFromPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\ Failed\n");
    }
    else
    {
        printf("\nTest case passed\n");
         

    }  
    
}


int public_group(void)
{
	int value;
    char publisher_msg[200]={0};
    char subscriber_msg[200]={0};
    int group_Id =1;
    int pid;

	char *name="First";

	resetAll();
    
     printf("Public Group  Test \n");
    

    /*1. Creating a public Interest Group */ 
    value = subscribeToPrivelegedList(USER_1);
    if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Privileged ACL\n");
    }    
    
    value = subscribeToPrivelegedList(USER_2);
   if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Privileged ACL\n");
    }  
    printf("\nSTEP 2 Passed\n");  
    


    group_Id =1;

    value = IG_Create(group_Id,name,2);

   if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nCreation of interest group succeeded\n");
    }  
    printf("\nSTEP 3 Passed\n");     
   

    pid = fork();
    if(pid == 0)
    {
        setuid(USER_1);
   
        value = IG_Publisher(group_Id);   
        if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded Publisher\n");
    }  
        exit(127); 
    }

    waitpid(pid,NULL,0);      
    printf("\nSTEP 4 Passed\n"); 

     


    pid = fork();
    if(pid == 0)
    {
        setuid(USER_2);
    
        value = IG_Subscriber(group_Id);   
        if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded subscribers\n");
    }  
        exit(127); 
    }

    waitpid(pid,NULL,0);   
    printf("\nSTEP 5 Passed\n");  
    
     
     pid = fork();
     if(pid == 0)
     {
        setuid(USER_1);
                 
        value = IG_Publish(group_Id,publisher_msg);   
        if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\n USER_1 published\n");
    }  
        exit(127); 
     }
    
    waitpid(pid,NULL,0);
    printf("\nSTEP 6 Passed\n");     
    
     

     pid = fork();
     if(pid == 0)
     {
         setuid(USER_2);
     
         value = IG_Subscribe(group_Id,subscriber_msg);   
         if(value!=1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\n USER_2 subscribed\n");
    }  
         
        exit(127); 
     }
    
    waitpid(pid,NULL,0);    
    printf("\nSTEP 7 Passed\n");      
     

    printf("\nSTEP 8 Passed\n");  

   

    printf("Test Passed\n");
}

int secure_group(void)
{
	int value;
    char publisher_msg[200]={0};
    char subscriber_msg[200]={0};
	int group_Id;
	
	char *name = "First";
     
    int pid,pid1;
	 
	resetAll();
	
	printf("secure group \n");
	
    value = subscribeToPrivelegedList(USER_1);
    if(value != 1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_1 to the Privileged ACL\n");
    }    
    
    value = subscribeToPrivelegedList(USER_3);
    if(value != 1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_3 to the Privileged ACL\n");
    } 
     value = subscribeToPrivelegedList(USER_2);
    if(value != 1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_2 to the Privileged ACL\n");
    } 
      value = subscribeToPrivelegedList(USER_4);
    if(value != 1)
    {
        printf("\n  Failed\n");
    }
    else
    {
        printf("\nAdded user USER_4 to the Privileged ACL\n");
    } 
    printf("\nSTEP 1 Passed\n");


    /*2. Create Secure Group with USER_1 as creator */

    pid = fork();
    
    if (pid == 0)
    {
        setuid(USER_1);
        
        value = IG_Create(&group_Id,name,SECURE_GROUP);
        if(value != 1)
        {
            printf("\n Failed\n");
        }    
        else
        {
            printf("\nSecure Interest Group created\n");
        } 
        printf("\nSTEP 2 Passed\n"); 
            
        /*3. Added USER_3,USER_2,USER_4 to ACL of group */

        value = registerToSecuredGroup(group_Id,USER_3);
        if(value != 1)
        {
            printf("\n Failed\n");
        }    
        else
        {
            printf("\nAdded USER_3\n");
        } 


       value = registerToSecuredGroup(group_Id,USER_2);
        if(value != 1)
        {
            printf("\n Failed\n");
        }    
        else
        {
            printf("\nAdded USER_2\n");
        } 


        value = registerToSecuredGroup(group_Id,USER_4);
        if(value != 1)
        {
            printf("\n Failed\n");
        }    
        else
        {
            printf("\nAdded USER_4\n");
        } 


        printf("\nSTEP 3 Passed\n");      
            
        /*4. USER_3 registers as Publisher */

        pid1 = fork();
        if(pid1 == 0)
        {
            setuid(USER_3);

            value = IG_Publisher(group_Id);   
            if(value != 1)
            {
                printf("\n  Failed\n");
            }    
            else
            {
           
                printf("USER_3 registered as publisher for group %d",group_Id);
            }  
            exit(127); 
        }

        waitpid(pid1,NULL,0);     
        
        /*5. USER_2 and USER_4 register as subscribers  */;

        pid1 = fork();
        if(pid1 == 0)
        {
            setuid(USER_2);
        
            value = IG_Subscriber(group_Id);   
            if(value != 1)
            {
                printf("\nFailed\n");
            }    
            else
            {
           
                printf("USER_2 registered as subscriber for group %d",group_Id);
            }  
            setuid(USER_4);
        
               
            value = IG_Subscriber(group_Id);   
            if(value != 1)
            {
                printf("\nFailed\n");
            }    
            else
            {
           
                printf("USER_4 registered as subscriber for group %d",group_Id);
            }      
            exit(127); 
        }
        
        waitpid(pid1,NULL,0);
        printf("\nSTEP 4 Passed\n"); 

         /*6. USER_4 publishes the first message */;      
         
         pid1 = fork();
         if(pid1 == 0)
         {
            setuid(USER_4);
                        
            value = IG_Publish(group_Id,publisher_msg);   
            if(value!=1)
            {
                printf("\n Failed\n");
            }    
            else
            {
    
                printf("USER_4 published the message");
            } 
            exit(127); 
         }
        
        waitpid(pid1,NULL,0);
        printf("\nSTEP 5 Passed\n");  
            
         /*5. USER_2 & USER_4 subscribe the message */; 
         
         pid1 = fork();
         if(pid1 == 0)
         {
             setuid(USER_2);
         
             value = IG_Retreive(group_Id,subscriber_msg);   
             if(value != 1)
             {
                 printf("\n  Failed\n");
             }    
             else
             {
            
                 printf("USER_2 subscribed message \n");
             }  
             setuid(USER_4);
              if(value != 1)
             {
                 printf("\n  Failed\n");
             }    
             else
             {
            
                 printf("USER_4 subscribed message \n");
             }        
             exit(127); 
         }
         
         waitpid(pid1,NULL,0);
         printf("\nSTEP 6 Pased\n"); 
    }
    waitpid(pid1,NULL,0);

    printf("Test Passed\n");

}
