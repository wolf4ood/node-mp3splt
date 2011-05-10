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
#include <limits.h>
#include <sstream>

extern "C" {

#include <libmp3splt/mp3splt.h>

}
using namespace v8;
using namespace node;
class Splitter;

__thread char ** filename;
__thread int dim = 0;

class Splitter: public EventEmitter {
private:

	splt_state * state;

	long c_hundreths(const char *s) {
		long minutes = 0, seconds = 0, hundredths = 0, i;
		long hun = -1;

		if (strcmp(s, "") == 0) {
			return LONG_MAX;
		}

		for (i = 0; i < strlen(s); i++) {
			if ((s[i] < 0x30 || s[i] > 0x39) && (s[i] != '.')) {
				return -1;
			}
		}

		if (sscanf(s, "%ld.%ld.%ld", &minutes, &seconds, &hundredths) < 2) {
			return -1;
		}

		if ((minutes < 0) || (seconds < 0) || (hundredths < 0)) {
			return -1;
		}

		if ((seconds > 59) || (hundredths > 99)) {
			return -1;
		}

		if (s[strlen(s) - 2] == '.') {
			hundredths *= 10;
		}

		hun = hundredths;
		hun += (minutes * 60 + seconds) * 100;

		return hun;
	}
	char * Split(char *fname) {
		int err = 0;
		mp3splt_find_plugins(state);
		mp3splt_set_split_filename_function(state, FileCallback);
		mp3splt_set_filename_to_split(state, fname);
		err = mp3splt_split(state);
		char * errstr = mp3splt_get_strerror(state, err);
		return errstr;
	}
	void AppendSplitPoint(char*point) {
		int err = mp3splt_append_splitpoint(state, c_hundreths(point), NULL,
				SPLT_SPLITPOINT);
	}
public:
	static void Init(Handle<Object> target) {
		HandleScope scope;
		Local<FunctionTemplate> t = FunctionTemplate::New(New);
		t->Inherit(EventEmitter::constructor_template);
		t->InstanceTemplate()->SetInternalFieldCount(1);
		t->SetClassName(String::NewSymbol("Splitter"));
		NODE_SET_PROTOTYPE_METHOD(t, "appendSplitPoint", AppendSplitPoint);
		NODE_SET_PROTOTYPE_METHOD(t, "split", Split);
		target->Set(String::NewSymbol("Splitter"), t->GetFunction());
	}
	Splitter() :
		node::EventEmitter() {
		int error = 0;
		state = mp3splt_new_state(&error);
	}
	~Splitter() {
	}
	static Handle<Value> New(const Arguments& args) {
		HandleScope scope;
		Splitter* hw = new Splitter();
		hw->Wrap(args.This());
		return args.This();
	}
	struct split_baton_t {
		Splitter *hw;
		Persistent<Function> cb;
		char * fname;
		int dim;
		char ** dfname;
		char * err;
	};
	static Handle<Value> AppendSplitPoint(const Arguments& args) {
		HandleScope scope;
		if (args.Length() == 1) {
			if (!args[0]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument must be a string")));
			}
			Splitter* hw = ObjectWrap::Unwrap<Splitter>(args.This());
			String::Utf8Value uns(args[0]->ToString());
			hw->AppendSplitPoint(strdup((*uns)));
		} else {

			return ThrowException(Exception::TypeError(String::New(
					"One argument required")));

		}
		return args.This();
	}
	static Handle<Value> Split(const Arguments& args) {
		HandleScope scope;
		if (args.Length() == 2) {
			if (!args[0]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 1 must be a string")));
			}
			if (!args[1]->IsFunction()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 4 must be a function")));
			}
			Local<String> str = Local<String>::Cast(args[0]);
			Splitter* hw = ObjectWrap::Unwrap<Splitter>(args.This());
			String::Utf8Value uns(args[0]->ToString());
			split_baton_t *baton = new split_baton_t();
			Local<Function> cb = Local<Function>::Cast(args[1]);
			baton->hw = hw;
			baton->fname = (char*) malloc(sizeof(char*));
			mempcpy(baton->fname, *uns, strlen(*uns) + 1);
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
		baton->err = baton->hw->Split(baton->fname);
		baton->dfname = filename;
		baton->dim = dim;
		return 0;

	}
	static void FileCallback(const char * fname, int i) {
		if (!filename) {
			filename = (char**) malloc(sizeof(char*));
		}
		filename[dim] = strdup(fname);
		dim++;
	}
	static int EIO_AfterSplit(eio_req *req) {
		HandleScope scope;
		split_baton_t *baton = static_cast<split_baton_t *> (req->data);
		ev_unref(EV_DEFAULT_UC);
		baton->hw->Unref();
		Local<Value> argv[2];
		if (baton->dfname) {
		    Local<Array> attr = Array::New(baton->dim);
		    for(int i=0;i<baton->dim;i++){
		    	attr->Set(i,String::New(baton->dfname[i]));
		    }
			argv[0] = attr;
			argv[1] = Integer::New(baton->dim);
		} else {
			argv[0] = Local<Value>::New(Null());
			argv[1] = String::New(baton->err);
		}
		baton->cb->Call(Context::GetCurrent()->Global(), 2, argv);
		baton->cb.Dispose();
		delete baton;
		return 0;
	}

};

extern "C" void init(Handle<Object> target) {
	Splitter::Init(target);
}

