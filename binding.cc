/*
 * bindings.cc
 *
 *  Created on: 05/mag/2011
 *      Author: maggiolo00
 */

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <unistd.h>
#include <eio.h>
#include <stdio.h>
#include <string.h>

extern "C" {

#include <libmp3splt/mp3splt.h>

}
using namespace v8;
using namespace node;

class Splitter: public EventEmitter {
private:
	int m_count;

public:
	static void Init(Handle<Object> target) {
		HandleScope scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(New);
		t->Inherit(EventEmitter::constructor_template);
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Splitter"));
		NODE_SET_PROTOTYPE_METHOD(t, "split", Split);
		NODE_SET_PROTOTYPE_METHOD(t, "print", Print);
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
		hw->m_count++;
		hw->Wrap(args.This());
		return args.This();
	}
	struct split_baton_t {
		Splitter *hw;
		char * fname;
		long begin;
		long end;
		Persistent<Function> cb;
	};
	static Handle<Value> Print(const Arguments& args) {

	}
	static Handle<Value> Split(const Arguments& args) {
		HandleScope scope;
		if (args.Length() == 2) {
			if (!args[0]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 0 must be a string")));
			}
			if (!args[1]->IsFunction()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 1 must be a function")));
			}
			Local<String> str = Local<String>::Cast(args[0]);
			Splitter* hw = ObjectWrap::Unwrap<Splitter>(args.This());
			String::Utf8Value unescaped_string(args[0]->ToString());
			split_baton_t *baton = new split_baton_t();
			Local<Function> cb = Local<Function>::Cast(args[1]);
			baton->hw = hw;
//			baton->fname = (char*)malloc((unescaped_string).length());
//			mempcpy(baton->fname,(*unescaped_string), (unescaped_string).length());
//			printf("%s\n",baton->fname);
			baton->cb = Persistent<Function>::New(cb);
			hw->Ref();
			eio_custom(EIO_Split, EIO_PRI_DEFAULT, EIO_AfterSplit, baton);
			ev_ref(EV_DEFAULT_UC);
			return Undefined();
		} else {
			return ThrowException(Exception::TypeError(String::New(
					"At least two arguments required")));
		}
	}
	static int EIO_Split(eio_req *req) {
		split_baton_t *baton = static_cast<split_baton_t *> (req->data);
		int error = 0;
		splt_state * state = mp3splt_new_state(&error);
		mp3splt_find_plugins(state);
		mp3splt_set_filename_to_split(state, "red.mp3");
		int err = mp3splt_append_splitpoint(state, 2, NULL, SPLT_SPLITPOINT);
		mp3splt_set_split_filename_function(state,FileCallback);
		err = mp3splt_append_splitpoint(state, 10, NULL, SPLT_SPLITPOINT);
		err = mp3splt_split(state);
		return 0;
	}
	static void FileCallback(const char * fname,int i){
		printf("%s\n",fname);
	}
	static int EIO_AfterSplit(eio_req *req) {
		HandleScope scope;
		split_baton_t *baton = static_cast<split_baton_t *> (req->data);
		ev_unref(EV_DEFAULT_UC);
		baton->hw->Unref();
		baton->cb->Call(Context::GetCurrent()->Global(), 0, NULL);
		baton->cb.Dispose();
		delete baton;
		return 0;
	}
};

extern "C" void init(Handle<Object> target) {
	Splitter::Init(target);
}

