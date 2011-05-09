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

extern "C" {

#include <libmp3splt/mp3splt.h>

}
using namespace v8;
using namespace node;
class Splitter;

__thread const char * filename;

class Splitter: public EventEmitter {
private:

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
	int Split(char *fname, char*begin, char*end) {
		int error = 0;
		splt_state * state = mp3splt_new_state(&error);
		mp3splt_find_plugins(state);
		mp3splt_set_filename_to_split(state, fname);
		int err = mp3splt_append_splitpoint(state, c_hundreths(begin),
				NULL, SPLT_SPLITPOINT);
		mp3splt_set_split_filename_function(state, FileCallback);
		err = mp3splt_append_splitpoint(state, c_hundreths(end), NULL,
				SPLT_SPLITPOINT);
		err = mp3splt_split(state);
		//Emit(String::New("split"), 0, NULL);
		return err;
	}
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
	Splitter() : node::EventEmitter() {
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
		char * begin;
		char * end;
		const char * dfname;
		int err;
	};
	static Handle<Value> Print(const Arguments& args) {

	}
	static Handle<Value> Split(const Arguments& args) {
		HandleScope scope;
		if (args.Length() == 4) {
			if (!args[0]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 1 must be a string")));
			}
			if (!args[1]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 2 must be a string")));
			}
			if (!args[2]->IsString()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 3 must be a string")));
			}
			if (!args[3]->IsFunction()) {
				return ThrowException(Exception::TypeError(String::New(
						"Argument 4 must be a function")));
			}
			Local<String> str = Local<String>::Cast(args[0]);
			Splitter* hw = ObjectWrap::Unwrap<Splitter>(args.This());
			String::Utf8Value uns(args[0]->ToString());
			String::Utf8Value begin(args[1]->ToString());
			String::Utf8Value end(args[2]->ToString());
			split_baton_t *baton = new split_baton_t();
			Local<Function> cb = Local<Function>::Cast(args[3]);
			baton->hw = hw;
			baton->fname = (char*) malloc(sizeof(char*));
			baton->begin = (char*) malloc(sizeof(char*));
			baton->end = (char*) malloc(sizeof(char*));
			mempcpy(baton->fname, *uns, strlen(*uns) + 1);
			mempcpy(baton->begin, *begin, strlen(*begin) + 1);
			mempcpy(baton->end, *end, strlen(*end) + 1);
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
		int err = baton->hw->Split(baton->fname,baton->begin,baton->end);
		baton->dfname = filename;
		return 0;

	}
	static void FileCallback(const char * fname, int i) {
		filename = strdup(fname);
	}
	static int EIO_AfterSplit(eio_req *req) {
		HandleScope scope;
		split_baton_t *baton = static_cast<split_baton_t *> (req->data);
		ev_unref(EV_DEFAULT_UC);
		baton->hw->Unref();
	    Local<Value> argv[1];
	    argv[0] = String::New(baton->dfname);
		baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);
		baton->cb.Dispose();
		delete baton;
		return 0;
	}

};

extern "C" void init(Handle<Object> target) {
	Splitter::Init(target);
}

