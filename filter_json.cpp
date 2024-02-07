#include <stdio.h>
#include <omp.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"
#include "mct_utils.h"


//g++ -std=c++11 filter_json.cpp -o filter_json -lm -O3 -fopenmp

//./filter_json "/media/sf_VMUbuntu/dblp_v14.json" "/media/sf_VMUbuntu/dblp_v14_filtered.json"

#define BUFF_SIZE 1L*1024*1024*1024 //1GB
#define file_in  "/media/sf_VMUbuntu/dblp_v14.json"
#define file_out   "/media/sf_VMUbuntu/dblp_v14_filtered.json"
#define max_size  6L*1024*1024*1024


using namespace std;
using json = nlohmann::json;


int parse_line(char *buff, const int len, json* out_jline) {
	
	int len2=len;
	while(buff[0]==',' || buff[0]==']' || buff[0]=='\n' || buff[0]=='['){//pomijamy te znaki na poczatku linijki
		len2--;
		buff++;
		if(len2<=0){return 0;}
	}
	while(buff[len2-1]==',' || buff[len2-1]==']' || buff[len2-1]=='\n' || buff[len2-1]=='['){//pomijamy te znaki na koncu linijki
		len2--;
		if(len2<=0){return 0;}
	}
	if(len2<=0){return 0;}
	
	
	json jline;
	if(!json::accept(buff,buff+len2)){
		for(int i=0;i<len2;i++)
		cout<<buff[i];
		cout<<endl<<len2;
		cout<<endl;
	}
	jline = json::parse(buff,buff+len2);

	if(jline.contains("doc_type") && jline["doc_type"] == "Journal" &&
			jline.contains("references") && jline["references"].size()>0 &&
			jline.contains("authors") && jline["authors"].size()>0 &&
			jline.contains("year") && jline.contains("id")){
				
			(*out_jline)["authors"] = json::array();
			bool has_authors=true;
			for (auto& element : jline["authors"]) {//czesc autorow byla pusta
			  if(element["id"]=="")
				  has_authors = false;
			  (*out_jline)["authors"].push_back(element["id"]);
			}
			(*out_jline)["id"]=jline["id"];
			(*out_jline)["references"]=jline["references"];
			(*out_jline)["year"]=jline["year"];
			return 1;
	}
  return 0;  
}

long filter_chunk(char* buffer, size_t size,std::map<std::string,int> &encode_papers,std::map<std::string,int> &encode_authors,long &papers_count,long &authors_count,ofstream &out_stream,long curr_pos) {
    int out_line_count = 0;
    int count = 0;
    vector<int> *posa;
    vector<int> posa_merged;
    int nthreads;
	int i;
	int end_pos=0;
	

	
	
    #pragma omp parallel 
    {
        nthreads = omp_get_num_threads();
        const int ithread = omp_get_thread_num();
        
        #pragma omp single 
        {
            posa = new vector<int>[nthreads];
            posa[0].push_back(0);
        }

		
        //get the number of lines and end of line position
        #pragma omp for reduction(+: count) reduction(max: end_pos)
        for(i=1; i<size; i++) {
            if(buffer[i] == '\n') { //sprawdzamy pozycje enterow
                count++;
                posa[ithread].push_back(i);
                end_pos=max(end_pos,i);
            }
        }
		

        #pragma omp single
        {
			int size=0;
			for(i=0;i<nthreads;i++)
				size+=posa[ithread].size();
			posa_merged.reserve(size);
			for(i=0;i<nthreads;i++)
				posa_merged.insert(posa_merged.end(),posa[i].begin(),posa[i].end());
		}
		
        #pragma omp for  
        for(i=1; i<count ;i++) {   
			json jline;
            const int len = posa_merged[i] - posa_merged[i-1];
            char* buff = &buffer[posa_merged[i-1]];
            const int info = parse_line(buff,len,&jline);
            if (info == 1) {
				for(auto it=jline["authors"].begin();it!=jline["authors"].end();it++){
					if(encode_authors.count(*it)==0){
						#pragma omp critical (authors)
						{
							if(encode_authors.count(*it)==0){//podwojne sprawdzenie zeby nie wchodzic niepotrzebnie w critical
								encode_authors[*it]=i;
								authors_count++;
							}
						}
					}
					*it = encode_authors[*it];
				}
				if(encode_papers.count(jline["id"])==0){
					#pragma omp critical (papers)
					{
						if(encode_papers.count(jline["id"])==0){//podwojne sprawdzenie zeby nie wchodzic niepotrzebnie w critical
							encode_papers[jline["id"]]=i;
							papers_count++;
						}
					}
				}
				jline["id"]=encode_papers[jline["id"]];
				for(auto it=jline["references"].begin();it!=jline["references"].end();it++){
					if(encode_papers.count(*it)==0){
						#pragma omp critical (papers)
						{
							if(encode_papers.count(*it)==0){//podwojne sprawdzenie zeby nie wchodzic niepotrzebnie w critical
								encode_papers[*it]=i;
								papers_count++;
							}
						}
					}
					*it = encode_papers[*it];
				}
				#pragma omp critical (save)
				{
					if(curr_pos!=0 || out_line_count!=0) out_stream<<","<<endl;
					out_line_count++;
					out_stream<<jline.dump();
				}
            }

        }
    }
    

    
    delete[] posa;
    return end_pos;
}


int main (int argc, char *argv[]) {
	FILE * file;
	long full_size;
	char * buffer;
	long read_result_size;
	long filter_result_size;
	long curr_pos=0;

	
	
	b_t(); // start timing

	
	file = fopen (  file_in, "rb" );
	if (file==NULL) {fputs ("File error",stderr); return 1;}

	// obtain file size:
	fseek (file , 0 , SEEK_END);
	full_size = ftell (file);
	rewind (file);
	

	full_size=min(full_size,max_size);
	cout<<full_size<<endl;

	// allocate memory for buffer
	buffer = (char*) malloc (sizeof(char)*BUFF_SIZE);
	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}


	std::map<std::string,int> encode_papers;
	std::map<std::string,int> encode_authors;
	long papers_count=0;
	long authors_count=0;
	
	ofstream out_stream(file_out);
	out_stream<<"["<<endl;
	
	
	
	while(true){
		read_result_size = fread (buffer,1,BUFF_SIZE,file);
		filter_result_size = filter_chunk(buffer, min(read_result_size,full_size-curr_pos),
								encode_papers,encode_authors,papers_count,authors_count,out_stream,curr_pos);
		if(curr_pos+BUFF_SIZE>full_size){break;}
		
		
		
		curr_pos+=filter_result_size;
		
		cout<<curr_pos<<endl;
		fseek (file , curr_pos , SEEK_SET);
	}

	out_stream<<"]"<<endl;
	out_stream.close();



	fclose(file);
	free(buffer);
	
	double tcmp = e_t(); // stop timing
    printf("# COMPUTATION TIME: %f sec\n", tcmp);
    
	
	
	return 0;
}
