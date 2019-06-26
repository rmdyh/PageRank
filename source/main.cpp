#include <stdio.h>
#include <map>
#include <set>
#include <string.h>
#include <math.h>
#include <queue>
#include <assert.h>

#include"pagerank.h"

static double now_value[BLOCK_SIZE];
std::map<int,int> id2loc;   /*map web_id to its order in old.bin*/ 
std::set<int> nodeset,allnodeset;/*maintain the web_id that has been processed*/

int main(int argc, char** argv){
    if(split_data(argc,argv)==0)
        return 0;
    if(solve()==0)
        return 0;
    if(output()==0)
        return 0;
    return 0;
}

int split_data(int argc,char ** argv){
    FILE *fp;
    if (argc == 1){
        fp=fopen("../dataset/WikiData.txt","r");
    }
    else{
        fp=fopen(argv[1],"r");
    }
    FILE *matrix=fopen("matrix.bin","wb"),*index=fopen("index.bin","wb"),*old=fopen("old.bin","wb");
    if(fp==NULL||matrix==NULL||index==NULL||old==NULL){
        printf("file cannot open \n");
        return 0;
    }
    int src,dest,t;
    id2loc.clear();
    /*map web_id to its order in old.bin*/
    while(fscanf(fp,"%d%d",&src,&dest)!=EOF){
        if(id2loc.count(src)==0){
            id2loc[src]=id2loc.size();
            fwrite(&src,sizeof(int),1,index);
        }
        if(id2loc.count(dest)==0){
            id2loc[dest]=id2loc.size();
            fwrite(&dest,sizeof(int),1,index);
        }
        ++t;
    }
    printf("Read file successfully. Has %d websites and %d records\n",id2loc.size(),t);
    double init=1.0/id2loc.size();
    nodeset.clear();
    allnodeset.clear();
    fseek(fp,0,SEEK_SET);
    /* break dataset into blocks and init the r_old */
    while(fscanf(fp,"%d%d",&src,&dest)!=EOF){
        if(allnodeset.count(src)==0){
            nodeset.insert(src);
            allnodeset.insert(src);
            fwrite(&init,sizeof(double),1,old);
        }
        if(allnodeset.count(dest)==0){
            nodeset.insert(dest);
            allnodeset.insert(dest);
            fwrite(&init,sizeof(double),1,old);
        }
        /* up to max block-size */
        if(nodeset.size()>=BLOCK_SIZE){
            /* 
              nodeset.size() may be either BLOCK_SIZE or BLOCK_SIZE+1
              Need to cope with the latter case
              by recording 'dest' , erasing it in nodeset
              and adding it latter.
            */
            int backup=0;
            if(nodeset.size()>BLOCK_SIZE){
                backup=dest;
                nodeset.erase(dest);
            }
            // 1. record lseek
            // 2. read the file from head to tail
            //    and write one line of the sparse matrix in matrix.bin
            long pos=ftell(fp);
            fseek(fp,0,SEEK_SET);
            LinkTable lt={0};
            ToNode node;
            while(fscanf(fp,"%d%d",&src,&dest)!=EOF){
                if(lt.pointer==0)
                    lt.pointer=src;
                // write this line to matrix.bin
                if(src!=lt.pointer){
                    if(lt.dest_num!=0){
                        lt.pointer=id2loc[lt.pointer];
                        fwrite(&lt,sizeof(LinkTable),1,matrix);
                        fwrite(node,sizeof(int)*lt.dest_num,1,matrix);
                    }
                    lt={src,0,0};
                }
                // add new dest
                ++lt.degree;
                if(nodeset.count(dest)!=0){
                    node[lt.dest_num]=id2loc[dest];
                    ++lt.dest_num;
                }
            }
            // write this line to matrix.bin
            if(lt.dest_num!=0){
                lt.pointer=id2loc[lt.pointer];
                fwrite(&lt,sizeof(LinkTable),1,matrix);
                fwrite(node,sizeof(int)*lt.dest_num,1,matrix);
            }
            // block end with triple 0
            lt={0,0,0};
            fwrite(&lt,sizeof(LinkTable),1,matrix);
            fseek(fp,pos,SEEK_SET);
            nodeset.clear();
            // add 'dest' to nodeset if erase it before
            if(backup){
                nodeset.insert(backup);
            }
        }
    }
    /* save the last block to matrix.bin */
    if(nodeset.size()>0){
        fseek(fp,0,SEEK_SET);
        LinkTable lt={0};
        ToNode node;
        while(fscanf(fp,"%d%d",&src,&dest)!=EOF){
            if(lt.pointer==0)
                lt.pointer=src;
            if(src!=lt.pointer){
                if(lt.dest_num!=0){
                    lt.pointer=id2loc[lt.pointer];
                    fwrite(&lt,sizeof(LinkTable),1,matrix);
                    fwrite(node,sizeof(int)*lt.dest_num,1,matrix);
                }
                lt={src,0,0};
            }
            ++lt.degree;
            if(nodeset.count(dest)!=0){
                node[lt.dest_num]=id2loc[dest];
                ++lt.dest_num;
            }
        }
        if(lt.dest_num!=0){
            lt.pointer=id2loc[lt.pointer];
            fwrite(&lt,sizeof(LinkTable),1,matrix);
            fwrite(node,sizeof(int)*lt.dest_num,1,matrix);
        }
        lt={0,0,0};
        fwrite(&lt,sizeof(LinkTable),1,matrix);
    }
    id2loc.clear();
    nodeset.clear();
    allnodeset.clear();
    fclose(fp);
    fclose(matrix);
    fclose(index);
    fclose(old);
    return 1;
}

int solve(){
    FILE *matrix=fopen("matrix.bin","rb");
    FILE *old=fopen("old.bin","rb+"),*_new=fopen("new.bin","wb+");
    /* get the number of webs */
    fseek(old,0,SEEK_END);
    int item_size=ftell(old)/sizeof(double);
    int t=0;
    /* iteration start */
    while(true){
        double total=.0; 
        fseek(old,0,SEEK_SET);
        fseek(_new,0,SEEK_SET);
        fseek(matrix,0,SEEK_SET);
        /* calculate r_new block by block */
        for(int begin=0;begin<item_size;begin+=BLOCK_SIZE){
            memset(now_value,0,sizeof(now_value));
            LinkTable lt;
            ToNode node;
            /* read sparse matrix by line */
            while(fread(&lt,sizeof(LinkTable),1,matrix)){
                if(lt.pointer==0){
                /* hit the end of the block */
                    break;
                }
                fread(node,sizeof(int)*lt.dest_num,1,matrix);
                double from_node;
                fseek(old,(lt.pointer-1)*sizeof(double),SEEK_SET);
                fread(&from_node,sizeof(double),1,old);
                for(int i=0;i<lt.dest_num;++i){
                    now_value[node[i]-begin-1]+=BETA*from_node/lt.degree;
                }
            }
            int end=begin+BLOCK_SIZE<item_size?begin+BLOCK_SIZE:item_size;
            for(int i=0;i<end-begin;i++){
                /* calculate the sum of pagerank values*/
                total+=now_value[i];
            }
            fwrite(now_value,sizeof(double),end-begin,_new);
            fflush(_new);
        }
        total=(1.0-total)/item_size;
        double offset=0;
        fseek(old,0,SEEK_SET);
        fseek(_new,0,SEEK_SET);
        /* normalize the r_new and write them into old.bin */
        for(int begin=0;begin<item_size;begin+=BLOCK_SIZE/2){
            int now_size=item_size-begin<BLOCK_SIZE/2?item_size-begin:BLOCK_SIZE/2;
            fread(now_value,sizeof(double),now_size,_new);
            fread(now_value+BLOCK_SIZE/2,sizeof(double),now_size,old);
            for(int i=0;i<now_size;++i){
                now_value[i]+=total; /* normalize */
                /* calculate the difference of r_old and r_new */
                offset+=fabs(now_value[i]-now_value[i+BLOCK_SIZE/2]);
            }
            fseek(old,sizeof(double)*begin,SEEK_SET);
            /* write them into old.bin */
            assert(fwrite(now_value,sizeof(double),now_size,old));
            fflush(old);
        }
        printf("iteration with %f difference\n",offset);
        if(offset<EPSILON){
            /* convergence */
            break;
        }
        t++;
    }
    fclose(matrix);
    fclose(old);
    fclose(_new);
    return 1;
}

struct cmp{
  bool operator()(std::pair<int,double> a, std::pair<int,double> b){
    return a.second < b.second;
  }
}; 
std::priority_queue<std::pair<int,double>,std::vector<std::pair<int,double> >,cmp> p;

int output(){
    FILE *index=fopen("index.bin","rb"),*old=fopen("old.bin","rb");
    FILE *result=fopen("result.txt","w");
    double value;
    int id=0;
    // use priority_queue to fine top x website
    while(fread(&value,sizeof(double),1,old)){
        p.push(std::make_pair(++id,value));
    }
    id=id>TOP?TOP:id;
    while(--id>=0){
        std::pair<int,double> tmp=p.top();
        p.pop();
        int web_id;
        // get web_id from index.bin
        fseek(index,(tmp.first-1)*sizeof(int),SEEK_SET);
        fread(&web_id,sizeof(int),1,index);
        // output the result to result.txt
        fprintf(result,"%d\t%.10f\n",web_id,tmp.second);
    }
    return 1;
}
