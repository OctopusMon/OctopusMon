# include <iostream>
# include <fstream>
# include <string.h>
# include <vector>
// #include "lala.h"
#include "../../../Octopus-Micro/elastic/ElasticSketch.h"
#include "../../../Octopus-Micro/sumax/sumax.h"
#include "../../../Octopus-Micro/dleft/dleft.h"
#include "../../../Octopus-Micro/marble/marble.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
using namespace std;
#define COUNTER_SIZE 32 
#define MAX_HASH_NUM 16
// #define FATK 8
#define FATK 16

typedef ElasticSketch<50,100000> ESketch;
typedef DLeft<3,13600> DL;
typedef DLeft<3,21100> EDL;

typedef SuMaxSketch<4500,3> SM;
typedef SuMaxSketch<7200,3> ESM;

typedef MARBLE<12500> MB;
typedef MARBLE<20000> EMB;

struct lalas{
	int group;
	double ratio;
}la[2000];

bool cmp(lalas x, lalas y){
	return x.ratio>y.ratio;
}

class Sketches{
public:

	int e_num,c_num,e[FATK*FATK/2];

	vector<vector<uint32_t>> ab_fs;
	// vector<TUPLES> ab_fs;
	vector<int> ab_fgs;

	vector<ESketch> insketch;
	vector<ESketch> outsketch;
	vector<ESM> edgesketch;
	vector<SM> coresketch;
	// vector<EMB> edgesketch;
	// vector<MB> coresketch;
	Sketches(){
		e_num=FATK*FATK/2;c_num=FATK*FATK*3/4;
		// insketch.resize(e_num);
		// outsketch.resize(e_num);
		edgesketch.resize(e_num);
		coresketch.resize(c_num);

		int tmp=0;
		for(int i=0;i<FATK;i++){
			for(int j=FATK/2;j<FATK;j++){
				e[tmp++]=FATK*FATK/4+i*FATK+j;
			}
		}
		for(int i=0;i<FATK*FATK/2;i++){
			cout<<e[i]<<" ";
		}
		cout<<endl;

		Clear();
	}
	void Clear(){
		for(int i=0;i<e_num;i++){
			// insketch[i].clear();
			// outsketch[i].clear();
			edgesketch[i].clear();
		}
		for(int i=0;i<c_num;i++) coresketch[i].clear();
		ab_fgs.clear();
		ab_fs.clear();
		memset(la,0,sizeof(lalas)*2000);
	}

	void PPrint(){
		int sz = ab_fs.size();
		cout<<"*******************"<<sz<<endl;
		for(int i=0;i<sz;i++){
			for(int j=0;j<ab_fs[i].size();j++){
				cout<<ab_fs[i][j]<<"*";
			}
			cout<<endl;
		}
	}

	void update_core_key(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
		// TUPLES key(src_ip,dst_ip,src_port,dst_port,_proto);
		vector<uint32_t> key={src_ip,dst_ip,src_port,dst_port,_proto};
		if(find(ab_fs.begin(),ab_fs.end(),key)==ab_fs.end()){
			ab_fs.push_back(key);
			// PPrint();
			// cout<<hex<<src_ip<<" "<<dst_ip<<" "<<src_port<<" "<<dst_port<<" "<<(int)_proto<<endl;
			// cout<<hex<<key.srcIP()<<" "<<key.dstIP()<<" "<<key.srcPort()<<" "<<key.dstPort()<<" "<<(int)key.proto()<<endl;
		}
	}
	double ratio_large_interval(vector<ESketch>& sketch, TUPLES key){
		vector<uint32_t> res(INTERVAL_CNT, 0);
		uint32_t all=0;
		for(int i=0;i<e_num;i++){
			vector<uint32_t> tmp = sketch[i].query_partial_key(key, srcIP, 0xffffff00);
			for(int j=0;j<INTERVAL_CNT;j++){
				res[j]+=tmp[j];
				all+=tmp[j];
			}
		}
		cout<<(key.srcIP()>>8)<<": ";
		for(int j=0;j<INTERVAL_CNT;j++){
			cout<<res[j]<<" ";
		}
		cout<<endl;
		return (double)res[INTERVAL_CNT-1]/(double)all;
	}
	int SSum(vector<ESketch>& sketch, TUPLES key){
		// vector<uint32_t> res(INTERVAL_CNT, 0);
		uint32_t all=0;
		for(int i=0;i<e_num;i++){
			vector<uint32_t> tmp = sketch[i].query_partial_key(key, srcIP, 0xffffff00);
			for(int j=0;j<INTERVAL_CNT;j++){
				// res[j]+=tmp[j];
				all+=tmp[j];
			}
		}
		cout<<(key.srcIP()>>8)<<": ";
		// for(int j=0;j<INTERVAL_CNT;j++){
		cout<<all<<" "<<endl;
		return all;
	}
	vector< vector<uint32_t> > get_ab_fs(){
		int sz = ab_fs.size();
		vector< vector<uint32_t> > res;
		res.resize(sz);
		for(int i=0;i<sz;i++){
			uint32_t src_ip=ab_fs[i][0],dst_ip=ab_fs[i][1];
			uint16_t src_port=ab_fs[i][2],dst_port=ab_fs[i][3];
			uint8_t _proto=ab_fs[i][4];
		
			res[i].push_back(src_ip);
			res[i].push_back(dst_ip);
			res[i].push_back(src_port);
			res[i].push_back(dst_port);
			res[i].push_back(_proto);
		}
		return res;
	}
	
	
	// void ab_fg(){
	// int ab_fg(int top){
	vector<int> ab_fg(int top,int typ){
		int mx=-10000000,flag=0,f_tmp=0;
		uint32_t tmp=e_num+c_num;
		vector<int> res;

		for(int i=0;i<e_num;i++){
			for(int j=0;j<FATK/2;j++){

				uint32_t _srcIP=(e[i]<<20)+(tmp<<8), _dstIP=0;
				uint16_t _srcPort=0, _dstPort=0;
				uint8_t _proto=0;
				TUPLES edge_fg=TUPLES(_srcIP,_dstIP,_srcPort,_dstPort,_proto);

				double ratio = 0;
				if(typ==0||typ==1)
					ratio = SSum(insketch,edge_fg) - SSum(outsketch,edge_fg);
				else
					ratio = ratio_large_interval(outsketch,edge_fg);
				
				la[tmp-e_num-c_num].group = (e[i]<<12)+tmp;
				la[tmp-e_num-c_num].ratio = ratio;
				tmp++;
			}
		}
		sort(la,la+2000,cmp);
		for(int i=0;i<top;i++){
			res.push_back(la[i].group);
		}

		return res;
	}
	~Sketches(){
		
	}
};

PYBIND11_MODULE(Sketches, m) {
	py::class_<Sketches> sketches(m, "Sketches");
	
    
    sketches.def(py::init<>())
        .def("Clear", &Sketches::Clear)
        .def("ab_fg", &Sketches::ab_fg)
        .def("update_core_key", &Sketches::update_core_key)
        .def("get_ab_fs", &Sketches::get_ab_fs)
		.def_readwrite("insketch", &Sketches::insketch)
		.def_readwrite("outsketch", &Sketches::outsketch)
		.def_readwrite("edgesketch", &Sketches::edgesketch)
		.def_readwrite("coresketch", &Sketches::coresketch);
		// .def_readwrite("ab_fs", &Sketches::ab_fs);

	py::class_<TUPLES>(sketches, "TUPLES")
        .def(py::init<>())
        .def(py::init<uint32_t, uint32_t, uint16_t, uint16_t, uint8_t>())
		.def("srcIP", &TUPLES::srcIP)
		.def("dstIP", &TUPLES::dstIP)
		.def("srcPort", &TUPLES::srcPort)
		.def("dstPort", &TUPLES::dstPort)
		.def("proto", &TUPLES::proto)
		.def("srcIP_dstIP", &TUPLES::srcIP_dstIP)
		.def("srcIP_srcPort", &TUPLES::srcIP_srcPort)
		.def("dstIP_dstPort", &TUPLES::dstIP_dstPort);

	py::class_<ESketch>(sketches, "ElasticSketch")
	// py::class_<ESketch>(m, "ElasticSketch")
		.def(py::init<>())
        .def("insert", &ESketch::insert)
        .def("insert_pre", &ESketch::insert_pre)
        .def("query_partial_key", &ESketch::query_partial_key)
        .def("query", &ESketch::query);

	// py::class_<CocoSketch>(sketches, "CoCoSketch")
	// 	.def(py::init<uint32_t, uint32_t>())
    //     .def("insert", &CocoSketch::insert)
    //     .def("query_all", &CocoSketch::query_all)
    //     .def("query", &CocoSketch::query);

	py::enum_<KeyType>(sketches, "KeyType")
    	.value("five_tuples", KeyType::five_tuples)
    	.value("srcIP_dstIP", KeyType::srcIP_dstIP)
    	.value("srcIP_srcPort", KeyType::srcIP_srcPort)
    	.value("dstIP_dstPort", KeyType::dstIP_dstPort)
    	.value("srcIP", KeyType::srcIP)
    	.value("dstIP", KeyType::dstIP)
    	.export_values();

	py::class_<DL>(sketches, "DL")
        .def(py::init<>())
        .def("PPrint", &DL::PPrint)
        .def("insert", &DL::insert)
        .def("insert_pre", &DL::insert_pre)
		.def("query_wait_pre", &DL::query_wait_pre)
        .def("query_wait", &DL::query_wait)
        .def("query_pre", &DL::query_pre)
        .def("query_interval_pre", &DL::query_interval_pre)
        .def("query_interval", &DL::query_interval)
        .def("query", &DL::query);

	py::class_<EDL>(sketches, "EDL")
        .def(py::init<>())
        .def("PPrint", &EDL::PPrint)
        .def("insert", &EDL::insert)
        .def("insert_pre", &EDL::insert_pre)
		.def("query_wait_pre", &EDL::query_wait_pre)
        .def("query_wait", &EDL::query_wait)
		.def("query_pre", &EDL::query_pre)
        .def("query_interval_pre", &EDL::query_interval_pre)
        .def("query", &EDL::query);


	py::class_<ESM>(sketches, "ESM")
        .def(py::init<>())
		.def("insert", &ESM::insert)
		.def("insert_pre", &ESM::insert_pre)
		.def("query_interval_pre", &ESM::query_interval_pre)
		.def("query_wait_pre", &ESM::query_wait_pre)
		.def("query_pre", &ESM::query_pre)
		.def("query", &ESM::query);

	py::class_<SM>(sketches, "SM")
        .def(py::init<>())
		.def("insert", &SM::insert)
		.def("insert_pre", &SM::insert_pre)
		.def("query_interval_pre", &SM::query_interval_pre)
		.def("query_wait_pre", &SM::query_wait_pre)
		.def("query_pre", &SM::query_pre)
		.def("query", &SM::query);

	py::class_<EMB>(sketches, "EMB")
        .def(py::init<>())
		.def("insert", &EMB::insert)
		.def("insert_pre", &EMB::insert_pre)
		.def("query_interval_pre", &EMB::query_interval_pre)
		.def("query_wait_pre", &EMB::query_wait_pre)
		.def("query_pre", &EMB::query_pre)
		.def("query", &EMB::query);

	py::class_<MB>(sketches, "MB")
        .def(py::init<>())
		.def("insert", &MB::insert)
		.def("insert_pre", &MB::insert_pre)
		.def("query_interval_pre", &MB::query_interval_pre)
		.def("query_wait_pre", &MB::query_wait_pre)
		.def("query_pre", &MB::query_pre)
		.def("query", &MB::query);
}

