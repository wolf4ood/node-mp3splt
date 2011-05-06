/*
 * bindings.cc
 *
 *  Created on: 05/mag/2011
 *      Author: maggiolo00
 */

#include <v8.h>
#include <node.h>
#include <node_events.h>
extern "C" {

#include <libmp3splt/mp3splt.h>

}
using namespace v8;
using namespace node;

class Splitter: ObjectWrap {
private:
	int m_count;
public:
	static void Init(Handle<Object> target) {
		HandleScope scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(New);
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Splitter"));
		NODE_SET_PROTOTYPE_METHOD(t, "split", Split);
		target->Set(String::NewSymbol("Splitter"), t->GetFunction());
	}
	Splitter() :
		m_count(0) {
	}
	~Splitter() {
	}
	static Handle<Value> New(const Arguments& args) {
		HandleScope scope;
		Splitter* hw = new Splitter();
		hw->Wrap(args.This());
		return args.This();
	}
	static Handle<Value> Split(const Arguments& args) {
		HandleScope scope;
		int error=0;
		splt_state * state = mp3splt_new_state(&error);
		mp3splt_find_plugins(state);
		mp3splt_set_filename_to_split(state,"red.mp3");
		int err = mp3splt_append_splitpoint(state,2,NULL,SPLT_SPLITPOINT);
		printf("%d\n",err);
		err = mp3splt_append_splitpoint(state,10,NULL,SPLT_SPLITPOINT);
		printf("%d\n",err);
		err = mp3splt_split(state);
		printf("%d\n",err);
		return args.This();
	}
};

extern "C" void init(Handle<Object> target) {
	Splitter::Init(target);
}

