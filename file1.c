#include<stdio.h>
main()
{
    FILE *fr,*fw;
    int n,i,j,t;
    fr=fopen("E:\\file\\b.txt","r");
    if(fr==NULL)
    {
        printf("File open error.\n");
        exit(0);
    }
    fw=fopen("E:\\file\\c.txt","w");
    fscanf(fr,"%d",&n);
    int a[n];
    for(i=0;i<n;i++)
    {
        fscanf(fr,"%d",&a[i]);
    }
    for(i=0;i<n;i++)
    {
        for(j=i;j<n;j++)
        {
            if(a[i]>a[j])
            {
                t=a[i];
                a[i]=a[j];
                a[j]=t;
            }
        }
    }
    for(i=0;i<n;i++)
    {
        fprintf(fw,"%6d",a[i]);
        if((i+1)%5==0) fprintf(fw,"\n");
    }
    fclose(fr);
    fclose(fw);
}
